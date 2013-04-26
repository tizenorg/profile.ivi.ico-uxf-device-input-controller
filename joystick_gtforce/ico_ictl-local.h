/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   header file of Input Controllers
 *
 * @date    Feb-20-2013
 */

#ifndef _ICO_ICTL_LOCAL_H_
#define _ICO_ICTL_LOCAL_H_

#include    <wayland-client-protocol.h>
#include    <wayland-client.h>
#include    <wayland-egl.h>
#include    <wayland-util.h>
#include    <sys/ioctl.h>
#include    <sys/epoll.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <fcntl.h>

#include    <ico_input_mgr-client-protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

/* call back function */
typedef void (*Ico_ICtl_Wayland_Cb)( void );

/* Deafult config file                      */
#define ICO_ICTL_CONF_FILE  \
                        "/opt/etc/ico-uxf-device-input-controller/joystick_gtforce.conf"
/* Environment valible name for config file */
#define ICO_ICTL_CONF_ENV   "ICTL_GTFORCE_CONF"
/* Environment valible name for input device*/
#define ICO_ICTL_INPUT_DEV  "ICTL_GTFORCE_DEV"

#define ICO_ICTL_TOUCH_NONE     0           /* not exist touch event                */
#define ICO_ICTL_TOUCH_KEYIN    1           /* touch event is reserved              */
#define ICO_ICTL_TOUCH_PASSED   2           /* touch event is passed                */

#define ICO_ICTL_EVENT_NUM      (16)
/* return val */
#define ICO_ICTL_OK             0           /* success                              */
#define ICO_ICTL_ERR            (-1)        /* failedi                              */

/* management table */
typedef struct  _Ico_ICtl_Mng   {
    /* Multi Input Controller   */
    int                         ICTL_ID;            /* multi input controller ID    */
    Ico_ICtl_Wayland_Cb         ICtl_CallBack;      /* call back function           */

    /* wayland interfaces       */
    struct wl_display           *Wayland_Display;   /* Wayland Display              */
    struct wl_registry          *Wayland_Registry;  /* Wayland Registory            */
    struct ico_input_mgr_device *Wayland_InputMgr;  /* Wayland multi input manager  */

    int                         WaylandFd;          /* file descriptor of Wayland   */
    int                         ICTL_EFD;           /* descriptor of epoll          */

}   Ico_ICtl_Mng;

/* function prototype           */
                                                /* initialization to wayland        */
int ico_ictl_wayland_init(const char *display ,Ico_ICtl_Wayland_Cb callback);
void ico_ictl_wayland_finish(void);             /* finish wayland connection        */
                                                /* iterate wayland connection       */
int ico_ictl_wayland_iterate(struct epoll_event *ev_ret, int timeout);
int ico_ictl_add_fd(int fd);                    /* add file descriptor              */

/* macro for debug              */
extern const char *dbg_curtime(void);
extern int  mDebug;
#define DEBUG_PRINT(fmt, ...)   \
    {if (mDebug) {fprintf(stderr, "%sDBG> "fmt" (%s:%d)\n",dbg_curtime(),##__VA_ARGS__,__FILE__,__LINE__); fflush(stderr);}}
#define ERROR_PRINT(fmt, ...)   \
    {fprintf(stderr, "%sERR> "fmt" (%s:%d)\n",dbg_curtime(),##__VA_ARGS__,__FILE__,__LINE__); fflush(stderr);}

#ifdef __cplusplus
}
#endif
#endif  /* _ICO_ICTL_LOCAL_H_ */

