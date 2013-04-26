/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Header file for uint test common routines
 *
 * @date    Feb-08-2013
 */

#ifndef _TEST_COMMON_H_
#define _TEST_COMMON_H_

#include    <GLES2/gl2.h>               /* OpenGL ES 2.x                    */
#include    <EGL/egl.h>                 /* EGL                              */
#include    <wayland-client.h>          /* Wayland client library           */
#include    <wayland-egl.h>             /* Wayland EGL library              */
#include    <wayland-util.h>            /* Wayland Misc library             */

/* Function prototype           */
int getdata(void *window_mgr, const char *prompt, int fd, char *buf, const int size);
void print_log(const char *fmt, ...);
void wayland_dispatch_nonblock(struct wl_display *display);
void sleep_with_wayland(struct wl_display *display, int msec);
void wait_with_wayland(struct wl_display *display, int msec, int *endflag);
int sec_str_2_value(const char *ssec);
EGLDisplay opengl_init(struct wl_display *display, EGLConfig *rconf, EGLContext *rctx);
EGLSurface opengl_create_window(struct wl_display *display, struct wl_surface *surface,
                                EGLDisplay dpy, EGLConfig conf, EGLContext ctx,
                                const int width, const int height, const int color);
void opengl_clear_window(const unsigned int color);
void opengl_swap_buffer(struct wl_display *display, EGLDisplay dpy, EGLSurface egl_surface);

#endif /*_TEST_COMMON_H_*/
