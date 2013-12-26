/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   header file of Device Input Controllers for GtForce
 *
 * @date    Sep-08-2013
 */

#ifndef _ICO_DIC_GTFORCE_H_
#define _ICO_DIC_GTFORCE_H_

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

#include    <ico_window_mgr-client-protocol.h>
#include    <ico_input_mgr-client-protocol.h>
#include    <ico_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/* call back function */
typedef void (*Ico_Dic_Wayland_Cb)( void );

/* Deafult config file                      */
#define ICO_DIC_CONF_FILE   \
                        "/opt/etc/ico/device-input-controller/%s.conf"
/* Environment valible name for config file */
#define ICO_DIC_CONF_ENV    "DIC_GTFORCE_CONF"
/* Environment valible name for input device*/
#define ICO_DIC_INPUT_DEV   "DIC_GTFORCE_DEV"

/* maximum value                            */
#define ICO_DIC_MAX_APPID_LEN   80          /* application id name length           */
#define ICO_DIC_MAX_CODES       2           /* maximum codes in same switch         */

/* event code                               */
#define ICO_DIC_TOUCH_NONE      0           /* not exist touch event                */
#define ICO_DIC_TOUCH_KEYIN     1           /* touch event is reserved              */
#define ICO_DIC_TOUCH_PASSED    2           /* touch event is passed                */
#define ICO_DIC_EVENT_NUM       (16)

/* return val */
#define ICO_DIC_OK              0           /* success                              */
#define ICO_DIC_ERR             (-1)        /* failedi                              */

/* management table */
typedef struct  _Ico_Dic_Mng    {
    /* Multi Input Controller   */
    int                         Dic_id;         /* multi input controller ID        */
    Ico_Dic_Wayland_Cb          Dic_CallBack;   /* call back function               */

    /* wayland interfaces       */
    struct wl_display           *Wayland_Display;   /* Wayland Display              */
    struct wl_registry          *Wayland_Registry;  /* Wayland Registory            */
    struct ico_window_mgr       *Wayland_WindowMgr; /* Wayland multi window manager */
    struct ico_input_mgr_control *Wayland_InputCtl; /* Wayland multi input manager  */
    struct ico_input_mgr_device *Wayland_InputMgr;  /* Wayland multi input manager  */

    int                         WaylandFd;          /* file descriptor of Wayland   */
    int                         Dic_efd;            /* descriptor of epoll          */

}   Ico_Dic_Mng;

/* function prototype           */
                                                /* initialization to wayland        */
int ico_dic_wayland_init(const char *display ,Ico_Dic_Wayland_Cb callback);
void ico_dic_wayland_finish(void);              /* finish wayland connection        */
                                                /* iterate wayland connection       */
int ico_dic_wayland_iterate(struct epoll_event *ev_ret, int timeout);
int ico_dic_add_fd(int fd);                     /* add file descriptor              */

#ifdef __cplusplus
}
#endif
#endif  /* _ICO_DIC_GTFORCE_H_ */
