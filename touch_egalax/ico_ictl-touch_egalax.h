/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Device Input Controller(cartouch Touchpanel)
 *          Program common include file
 *
 * @date    Feb-20-2013
 */

#ifndef _ICO_ICTL_TOUCH_EGALAX_H_
#define _ICO_ICTL_TOUCH_EGALAX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>

/* Default screen size      */
#define CALIBRATION_DISP_WIDTH     1920
#define CALIBRATION_DISP_HEIGHT    1080

/* Configuration file       */
#define CALIBRATOIN_CONF_LEN_MAX    128
#define CALIBRATOIN_CONF_ENV        "CALIBRATOIN_CONF"
#define CALIBRATOIN_INPUT_DEV       "ICTL_TOUCH_DEV"
#define CALIBRATOIN_CONF_FILE   \
                        "/opt/etc/ico-uxf-device-input-controller/egalax_calibration.conf"

/* Configuration items      */
#define CALIBRATOIN_STR_SEPAR       "=*"            /* Delimitor                */
#define CALIBRATOIN_STR_DISP_W      "DWIDTH"        /* Screen width             */
#define CALIBRATOIN_STR_DISP_H      "DHEIGHT"       /* Scneen height            */
#define CALIBRATOIN_STR_POS1        "POSITION1"     /* Top-Left position        */
#define CALIBRATOIN_STR_POS2        "POSITION2"     /* Top-Right position       */
#define CALIBRATOIN_STR_POS3        "POSITION3"     /* Bottom-Left position     */
#define CALIBRATOIN_STR_POS4        "POSITION4"     /* Bottom-Right position    */

/* Error retry              */
#define CALIBRATOIN_RETRY_COUNT     10              /* number of error retry    */
#define CALIBRATOIN_RETRY_WAIT      10              /* wait time(ms) for retry  */

/* Debug macros             */
#define CALIBRATION_DEBUG(fmt, ...) {if (mDebug) {fprintf(stdout, "%s:%d "fmt, __func__, __LINE__, ##__VA_ARGS__); fflush(stdout);}}
#define CALIBRATION_INFO(fmt, ...)  {if (mDebug) {fprintf(stdout, "%s:%d "fmt, __func__, __LINE__, ##__VA_ARGS__); fflush(stdout);}}
#define CALIBRATION_PRINT(...)       {fprintf(stdout, ##__VA_ARGS__); fflush(stdout);}

#endif /*_ICO_ICTL_TOUCH_EGALAX_H_*/

