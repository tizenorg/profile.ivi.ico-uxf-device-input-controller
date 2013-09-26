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
 * @date    Sep-08-2013
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <strings.h>
#include    <errno.h>
#include    <pthread.h>
#include    "ico_dic-gtforce.h"

/* prototype of static function             */
/* callback function from wayland global    */
static void ico_dic_wayland_globalcb(void *data, struct wl_registry *registry,
                                     uint32_t wldispid, const char *event,
                                     uint32_t version);

/* table/variable                           */
extern Ico_Dic_Mng      gIco_Dic_Mng;

static const struct wl_registry_listener registry_listener = {
    ico_dic_wayland_globalcb
};

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_dic_wayland_init
 *          connect to wayland of specified Display. specified NULL to
 *          connected Display, connect to the default Display.
 *
 * @param[in]   display             display to connect
 * @param[in]   callback            callback function
 * @return      result
 * @retval      ICO_DIC_EOK         Success
 * @retval      ICO_DIC_ERR         Failed
 */
/*--------------------------------------------------------------------------*/
int
ico_dic_wayland_init(const char *display, Ico_Dic_Wayland_Cb callback)
{
    ICO_TRA("ico_dic_wayland_init: Enter");

    int ret;

    /* regist callback funtion  */
    gIco_Dic_Mng.Dic_CallBack = callback;

    /* connect to wayland(retry max 5 sec)  */
    for (ret = 0; ret < (5000/20); ret++) {
        gIco_Dic_Mng.Wayland_Display = wl_display_connect(display);
        if (gIco_Dic_Mng.Wayland_Display) {
            break;
        }
        usleep(20*1000);
    }
    if (! gIco_Dic_Mng.Wayland_Display) {
        ICO_ERR("ico_dic_wayland_init: Leave(ERR), Wayland Connect Error");
        return ICO_DIC_ERR;
    }

    /* add listener of wayland registry */
    gIco_Dic_Mng.Wayland_Registry =
        wl_display_get_registry(gIco_Dic_Mng.Wayland_Display);
    wl_registry_add_listener(gIco_Dic_Mng.Wayland_Registry,
                             &registry_listener, &gIco_Dic_Mng);

    /* display dispatch to wait     */
    do  {
        usleep(20*1000);
        wl_display_dispatch(gIco_Dic_Mng.Wayland_Display);
    } while ((gIco_Dic_Mng.Wayland_WindowMgr == NULL) ||
             (gIco_Dic_Mng.Wayland_InputCtl == NULL) ||
             (gIco_Dic_Mng.Wayland_InputMgr == NULL));

    /* get the Wayland descriptor   */
    gIco_Dic_Mng.WaylandFd = wl_display_get_fd(gIco_Dic_Mng.Wayland_Display);
    ICO_DBG("ico_dic_wayland_init: Wayland FD = %d", gIco_Dic_Mng.WaylandFd);

    /* create epoll file descriptor */
    gIco_Dic_Mng.Dic_efd = epoll_create1(EPOLL_CLOEXEC);
    if (gIco_Dic_Mng.Dic_efd < 0) {
        ICO_ERR("ico_dic_wayland_init: Leave(ERR), Epoll Create Error");
        return ICO_DIC_ERR;
    }
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = gIco_Dic_Mng.WaylandFd;
    if (epoll_ctl(gIco_Dic_Mng.Dic_efd, EPOLL_CTL_ADD,
                  gIco_Dic_Mng.WaylandFd, &ev) != 0) {
        ICO_ERR("ico_dic_wayland_init: Leave(ERR), Epoll ctl Error");
        return ICO_DIC_ERR;
    }
    ICO_TRA("ico_dic_wayland_init: Leave(EOK)");
    return ICO_DIC_OK;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_dic_wayland_globalcb
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
ico_dic_wayland_globalcb(void *data, struct wl_registry *registry, uint32_t wldispid,
                          const char *event, uint32_t version)
{
    ICO_DBG("ico_dic_wayland_globalcb: Event=%s DispId=%08x", event, wldispid);

    if (strcmp(event, "ico_window_mgr") == 0)  {
        gIco_Dic_Mng.Wayland_WindowMgr = wl_registry_bind(gIco_Dic_Mng.Wayland_Registry,
                                                          wldispid,
                                                          &ico_window_mgr_interface, 1);
        ICO_DBG("ico_dic_wayland_globalcb: wl_registry_bind(ico_window_mgr)");
    }
    else if (strcmp(event, "ico_input_mgr_control") == 0)   {
        /* conect to multi input manager control interface  */
        gIco_Dic_Mng.Wayland_InputCtl = wl_registry_bind(gIco_Dic_Mng.Wayland_Registry,
                                                         wldispid,
                                                         &ico_input_mgr_control_interface, 1);
        ICO_DBG("ico_dic_wayland_globalcb: wl_registry_bind(ico_input_mgr_control)");
    }
    else if (strcmp(event, "ico_input_mgr_device") == 0) {
        /* conect to multi input manager device interface   */
        gIco_Dic_Mng.Wayland_InputMgr = wl_registry_bind(gIco_Dic_Mng.Wayland_Registry,
                                                         wldispid,
                                                         &ico_input_mgr_device_interface, 1);
        ICO_DBG("ico_dic_wayland_globalcb: wl_registry_bind(ico_input_mgr_device)");
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_dic_wayland_iterate
 *          iterate processing of wayland
 *
 * @param[in]   timeout     wait time miri-sec
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
int
ico_dic_wayland_iterate(struct epoll_event *ev_ret, int timeout)
{
    int nfds;
    int ii;
    static int  errcount = 0;

    memset(ev_ret, 0, sizeof(struct epoll_event) * ICO_DIC_EVENT_NUM);
    wl_display_flush(gIco_Dic_Mng.Wayland_Display);

    while (1) {
        if ((nfds = epoll_wait(gIco_Dic_Mng.Dic_efd, ev_ret,
                               ICO_DIC_EVENT_NUM, timeout)) > 0) {
            for (ii = 0; ii < nfds; ii++) {
                if (ev_ret[ii].data.fd == gIco_Dic_Mng.WaylandFd) {
                    wl_display_dispatch(gIco_Dic_Mng.Wayland_Display);
                    ICO_DBG("ico_dic_wayland_iterate: Event wayland fd");
                    errcount ++;
                    if (errcount > 50)  {
                        ICO_ERR("ico_dic_wayland_iterate: Error wayland disconnect");
                        exit(9);
                    }
                    if ((errcount % 5) == 0)    {
                        usleep(50*1000);
                    }
                }
                else    {
                    errcount = 0;
                }
            }
            return nfds;
        }
        else if (nfds == 0) {
            return 0;
        }
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_dic_add_fd
 *          Add file descriptor to watch in pool.
 *
 * @param[in]   fd                  File descriptor
 * @return      result
 * @retval      ICO_DIC_EOK        Success
 * @retval      ICO_DIC_ERR        Failed
 */
/*--------------------------------------------------------------------------*/
int
ico_dic_add_fd(int fd)
{
    ICO_DBG("ico_dic_add_fd: Enter(fd=%d)", fd);

    struct epoll_event      ev;

    if (gIco_Dic_Mng.Dic_efd <= 0) {
        ICO_ERR("ico_dic_add_fd: Leave(ERR), Epoll Never Created");
        return ICO_DIC_ERR;
    }

    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    if (epoll_ctl(gIco_Dic_Mng.Dic_efd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        ICO_ERR("ico_dic_add_fd: Leave(ERR), Epoll ctl Error");
        return ICO_DIC_ERR;
    }
    ICO_DBG("ico_dic_add_fd: Leave(EOK)");

    return ICO_DIC_OK;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_dic_wayland_finish
 *          Finish wayland connection
 *
 * @param       nothing
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
void
ico_dic_wayland_finish(void)
{
    ICO_DBG("ico_dic_wayland_finish: Enter");

    wl_display_flush(gIco_Dic_Mng.Wayland_Display);
    wl_display_disconnect(gIco_Dic_Mng.Wayland_Display);

    ICO_DBG("ico_dic_wayland_finish: Leave");
}

