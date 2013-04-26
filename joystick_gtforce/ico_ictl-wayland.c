/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Device Input Controllers(wayland processing)
 *          processing related wayland
 *
 * @date    Feb-08-2013
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <strings.h>
#include    <errno.h>
#include    <pthread.h>

#include    "ico_ictl-local.h"

/* prototype of static function             */
/* callback function from wayland global    */
static void ico_ictl_wayland_globalcb(void *data, struct wl_registry *registry,
                                      uint32_t wldispid, const char *event,
                                      uint32_t version);

/* table/variable                           */
extern Ico_ICtl_Mng         gIco_ICtrl_Mng;

static const struct wl_registry_listener registry_listener = {
    ico_ictl_wayland_globalcb
};

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_wayland_init
 *          connect to wayland of specified Display. specified NULL to
 *          connected Display, connect to the default Display.
 *
 * @param[in]   display             display to connect
 * @param[in]   callback            callback function
 * @return      result
 * @retval      ICO_ICTL_EOK        Success
 * @retval      ICO_ICTL_ERR        Failed
 */
/*--------------------------------------------------------------------------*/
int
ico_ictl_wayland_init(const char *display, Ico_ICtl_Wayland_Cb callback)
{
    DEBUG_PRINT("ico_ictl_wayland_init: Enter");

    int ret;

    /* regist callback funtion  */
    gIco_ICtrl_Mng.ICtl_CallBack = callback;

    /* connect to wayland(retry max 3 sec)  */
    for (ret = 0; ret < (3000/20); ret++) {
        gIco_ICtrl_Mng.Wayland_Display = wl_display_connect(display);
        if (gIco_ICtrl_Mng.Wayland_Display) {
            break;
        }
        usleep(20*1000);
    }
    if (! gIco_ICtrl_Mng.Wayland_Display) {
        ERROR_PRINT("ico_ictl_wayland_init: Leave(ERR), Wayland Connect Error");
        return ICO_ICTL_ERR;
    }

    /* add listener of wayland registry */
    gIco_ICtrl_Mng.Wayland_Registry =
        wl_display_get_registry(gIco_ICtrl_Mng.Wayland_Display);
    wl_registry_add_listener(gIco_ICtrl_Mng.Wayland_Registry,
                             &registry_listener, &gIco_ICtrl_Mng);

    /* display dispatch to wait     */
    do  {
        wl_display_dispatch(gIco_ICtrl_Mng.Wayland_Display);
    } while (gIco_ICtrl_Mng.Wayland_InputMgr == NULL);

    /* get the Wayland descriptor   */
    gIco_ICtrl_Mng.WaylandFd =
        wl_display_get_fd(gIco_ICtrl_Mng.Wayland_Display);
    DEBUG_PRINT("ico_ictl_wayland_init: WFD = %d", gIco_ICtrl_Mng.WaylandFd);

    /* create epoll file descriptor */
    gIco_ICtrl_Mng.ICTL_EFD = epoll_create1(EPOLL_CLOEXEC);
    if (gIco_ICtrl_Mng.ICTL_EFD < 0) {
        ERROR_PRINT("ico_ictl_wayland_init: Leave(ERR), Epoll Create Error");
        return ICO_ICTL_ERR;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = gIco_ICtrl_Mng.WaylandFd;
    if (epoll_ctl(gIco_ICtrl_Mng.ICTL_EFD, EPOLL_CTL_ADD,
                  gIco_ICtrl_Mng.WaylandFd, &ev) != 0) {
        ERROR_PRINT("ico_ictl_wayland_init: Leave(ERR), Epoll ctl Error");
        return ICO_ICTL_ERR;
    }
    DEBUG_PRINT("ico_ictl_wayland_init: Leave(EOK)");
    return ICO_ICTL_OK;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_wayland_globalcb
 *
 * @param[in]   data        the information that appointed at the time of
 *                          callback registration
 * @param[in]   registry    wayland registry
 * @param[in]   wldispid    wayland displya id
 * @param[in]   event       event name
 * @param[in]   version     version of Wayland
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
ico_ictl_wayland_globalcb(void *data, struct wl_registry *registry, uint32_t wldispid,
                          const char *event, uint32_t version)
{
    DEBUG_PRINT("ico_ictl_wayland_globalcb: Event=%s DispId=%08x", event, wldispid);

    if (strcmp(event, "ico_input_mgr_device") == 0) {
        /* connect ictl_master in multi input manager   */
        gIco_ICtrl_Mng.Wayland_InputMgr = (struct ico_input_mgr_device *)
            wl_registry_bind(gIco_ICtrl_Mng.Wayland_Registry,
                             wldispid, &ico_input_mgr_device_interface, 1);

        DEBUG_PRINT("ico_ictl_wayland_globalcb: wl_registry_bind(ico_input_mgr_device)");
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_wayland_iterate
 *          iterate processing of wayland
 *
 * @param[in]   timeout     wait time miri-sec
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
int
ico_ictl_wayland_iterate(struct epoll_event *ev_ret, int timeout)
{
    int nfds;
    int ii = 0;

    memset(ev_ret, 0, sizeof(struct epoll_event) * ICO_ICTL_EVENT_NUM);
    wl_display_flush(gIco_ICtrl_Mng.Wayland_Display);

    while (1) {
        if ((nfds = epoll_wait( gIco_ICtrl_Mng.ICTL_EFD, ev_ret,
                                       ICO_ICTL_EVENT_NUM, timeout)) > 0) {
            for (ii = 0; ii < nfds; ii++) {
                if (ev_ret[ii].data.fd == gIco_ICtrl_Mng.WaylandFd) {
                    wl_display_dispatch(gIco_ICtrl_Mng.Wayland_Display);
                    DEBUG_PRINT( "ico_ictl_wayland_iterate: Exit wayland fd");
                }
            }
            return nfds;
        }
        else if (nfds == 0) {
            return nfds;
        }
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_add_fd
 *          Add file descriptor to watch in pool.
 *
 * @param[in]   fd                  File descriptor
 * @return      result
 * @retval      ICO_ICTL_EOK        Success
 * @retval      ICO_ICTL_ERR        Failed
 */
/*--------------------------------------------------------------------------*/
int
ico_ictl_add_fd(int fd)
{
    DEBUG_PRINT("ico_ictl_add_fd: Enter(fd=%d)", fd);

    struct epoll_event      ev;

    if (gIco_ICtrl_Mng.ICTL_EFD <= 0) {
        ERROR_PRINT("ico_ictl_add_fd: Leave(ERR), Epoll Never Created");
        return ICO_ICTL_ERR;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(gIco_ICtrl_Mng.ICTL_EFD, EPOLL_CTL_ADD, fd, &ev) != 0) {
        ERROR_PRINT("ico_ictl_add_fd: Leave(ERR), Epoll ctl Error");
        return ICO_ICTL_ERR;
    }
    DEBUG_PRINT("ico_ictl_add_fd: Leave(EOK)");

    return ICO_ICTL_OK;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_wayland_finish
 *          Finish wayland connection
 *
 * @param       nothing
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
void
ico_ictl_wayland_finish(void)
{
    DEBUG_PRINT("ico_ictl_wayland_finish: Enter");

    wl_display_flush(gIco_ICtrl_Mng.Wayland_Display);
    wl_display_disconnect(gIco_ICtrl_Mng.Wayland_Display);

    DEBUG_PRINT("ico_ictl_wayland_finish: Leave");
}

