/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Device Input Controller(eGalax Touchpanel)
 *          Send touchpanel input enevt to Weston
 *
 * @date    Feb-20-2013
 */

#include "ico_ictl-touch_egalax.h"

/* Change touch event       */
#define REPLACE_TOUCH_EVENT 1       /* Change touch event to mouse left button event*/

/* Program name             */
#define     CALIBDAE_DEV_NAME       "ico_ictl-touch_egalax"

static void print_usage(const char *pName);
static char *find_event_device(void);
static int setup_uinput(char *uinputDeviceName);
static void set_eventbit(int uifd);
static int open_uinput(char *uinputDeviceName);
static void close_uinput(int uifd);
static void event_iterate(int uifd, int evfd);
static void setup_sighandler(void);
static void terminate_program(const int signal);
static int setup_program(void);
static int calibration_event(struct input_event *in, struct input_event *out);
static void push_event(struct input_event *ev);
static void push_eventlog(const char *cmd, const int value, struct timeval *tp);

int             mRunning = -1;          /* Running flag             */
int             mDebug = 0;             /* Debug flag               */
int             mEventLog = 0;          /* event input log          */
struct timeval  lastEvent = { 0, 0 };   /* last input event time    */

/* Configurations               */
int             mDispWidth = CALIBRATION_DISP_WIDTH;
int             mDispHeight = CALIBRATION_DISP_HEIGHT;
int             mPosX[4] = { 0, 0, 0, 0 };
int             mPosY[4] = { 0, 0, 0, 0 };
int             mXlow = 0;
int             mXheigh = 0;
int             mYlow = 0;
int             mYheigh = 0;

/* Optiones                     */
int             mTrans = 0;             /* Rotate(0,90,180 or 270)  */
int             mRevX = 0;              /* Reverse X coordinate     */
int             mRevY = 0;              /* Reverse Y coordinate     */

/*--------------------------------------------------------------------------*/
/**
 * @brief   Device Input Controller: For eGalax TouchPanel
 *          main routine
 *
 * @param   main() finction's standard parameter (argc,argv)
 * @return  result
 * @retval  0       success
 * @retval  8       failed(configuration error)
 * @retval  9       failed(event device error)
 */
/*--------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    int     ii;
    int     err;
    int     uifd;                               /* uinput fd */
    int     evfd;                               /* event device fd */
    char    *eventDeviceName = NULL;            /* event device name to hook */
    char    *uinputDeviceName = "/dev/uinput";  /* User Input module */

    for (ii = 1; ii < argc; ii++) {
        if (strcmp(argv[ii], "-h") == 0)    {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[ii], "-d") == 0) {
            mDebug = 1;
        }
        else if (strcmp(argv[ii], "-t") == 0) {
            if ((ii < (argc-1)) && (argv[ii+1][0] != '-'))  {
                ii++;
                mTrans = strtol(argv[ii], (char **)0, 0);
            }
            else    {
                mTrans = 90;
            }
        }
        else if (strcmp(argv[ii], "-l") == 0) {
            mEventLog = 1;                  /* Input Log for System Test    */
        }
        else if (strcmp(argv[ii], "-L") == 0) {
            mEventLog = 2;                  /* Output Log for Unit Test     */
        }
        else {
            eventDeviceName = argv[ii];
        }
    }

    if (eventDeviceName == NULL) {
        /* If event device not present, get default device  */
        eventDeviceName = find_event_device();
        if (eventDeviceName == NULL) {
            /* System has no touchpanel, Error  */
            exit(9);
        }
    }

    err = setup_program();
    if (err < 0) {
        if (err == -2)  {
            fprintf(stderr, "%s: Illegal config value\n", argv[0]);
        }
        else    {
            fprintf(stderr, "%s: Can not read config file\n", argv[0]);
        }
        exit(8);
    }

    if (! mDebug) {
        if (daemon(0, 1) < 0) {
            fprintf(stderr, "%s: Can not Create Daemon\n", argv[0]);
            exit(1);
        }
    }

    /* setup uinput device      */
    uifd = setup_uinput(uinputDeviceName);
    if (uifd < 0) {
        fprintf(stderr, "uinput(%s) initialize failed. Continue anyway\n", uinputDeviceName);
        exit(9);
    }

    evfd = open(eventDeviceName, O_RDONLY);
    if (evfd < 0) {
        fprintf(stderr, "event device(%s) Open Error[%d]\n", eventDeviceName, errno);
        close_uinput(uifd);
        exit(9);
    }
    CALIBRATION_DEBUG("main: input device(%s) = %d\n", eventDeviceName, evfd);

    setup_sighandler();

    /* event read               */
    mRunning = 1;
    event_iterate(uifd, evfd);

    if (evfd >= 0)  {
        close(evfd);
    }

    close_uinput(uifd);

    exit(0);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       initialize with get configurations
 *
 * @param       nothing
 * @return      result
 * @retval      0           sucess
 * @retval      -1          config file read error
 * @retval      -2          illegal config value
 */
/*--------------------------------------------------------------------------*/
static int
setup_program(void)
{
    char    *confp;
    char    buff[128];
    FILE    *fp;
    int     work;

    /* Get configuration file path  */
    confp = getenv(CALIBRATOIN_CONF_ENV);
    if (! confp)  {
        confp = CALIBRATOIN_CONF_FILE;
    }

    /* Open configuration file      */
    fp = fopen(confp, "r");
    if (fp == NULL) {
        perror(confp);
        return -1;
    }

    while (fgets(buff, sizeof(buff), fp)) {
        if (buff[0] == '#') {
            /* comment line, skip       */
            continue;
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_DISP_W,
                         sizeof(CALIBRATOIN_STR_DISP_W) - 1) == 0) {
            /* screen width             */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mDispWidth = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("mDispWidth = %d\n", mDispWidth);
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_DISP_H,
                         sizeof(CALIBRATOIN_STR_DISP_H) - 1) == 0) {
            /* screen height            */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mDispHeight = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("mDispHeight = %d\n", mDispHeight);
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_POS1,
                         sizeof(CALIBRATOIN_STR_POS1) - 1) == 0) {
            /* position1 : Top-Left     */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mPosX[0] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            mPosY[0] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("POS1 = %dx%d\n", mPosX[0], mPosY[0]);
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_POS2,
                         sizeof(CALIBRATOIN_STR_POS2) - 1) == 0) {
            /* position2 : Top-Right    */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mPosX[1] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            mPosY[1] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("POS2 = %dx%d\n", mPosX[1], mPosY[1]);
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_POS3,
                         sizeof(CALIBRATOIN_STR_POS3) - 1) == 0) {
            /* position3 : Bottom-Left  */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mPosX[2] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            mPosY[2] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("POS3 = %dx%d\n", mPosX[2], mPosY[2]);
        }
        else if (strncmp(buff,
                         CALIBRATOIN_STR_POS4,
                         sizeof(CALIBRATOIN_STR_POS4) - 1) == 0) {
            /* position4 : Bottom-Right */
            strtok(buff, CALIBRATOIN_STR_SEPAR);
            mPosX[3] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            mPosY[3] = atoi(strtok(NULL, CALIBRATOIN_STR_SEPAR));
            CALIBRATION_INFO("POS4 = %dx%d\n", mPosX[3], mPosY[3]);
        }
    }
    fclose(fp);

    /* Reverse X coordinate, if need    */
    if (mPosX[0] > mPosX[1])    {
        int     work;
        work = mPosX[0];
        mPosX[0] = mPosX[1];
        mPosX[1] = work;
        work = mPosX[2];
        mPosX[2] = mPosX[3];
        mPosX[3] = work;
        mRevX = 1;
        CALIBRATION_INFO("Reverse X\n");
    }

    /* Reverse Y coordinate, if need    */
    if (mPosY[0] > mPosY[2])    {
        work = mPosY[0];
        mPosY[0] = mPosY[2];
        mPosY[2] = work;
        work = mPosY[1];
        mPosY[1] = mPosY[3];
        mPosY[3] = work;
        mRevY = 1;
        CALIBRATION_INFO("Reverse Y\n");
    }

    if ((mRevX != 0) || (mRevY != 0))   {
        CALIBRATION_INFO("Changed POS1 = %dx%d\n", mPosX[0], mPosY[0]);
        CALIBRATION_INFO("Changed POS2 = %dx%d\n", mPosX[1], mPosY[1]);
        CALIBRATION_INFO("Changed POS3 = %dx%d\n", mPosX[2], mPosY[2]);
        CALIBRATION_INFO("Changed POS4 = %dx%d\n", mPosX[3], mPosY[3]);
    }

    mXlow = (mPosX[0] + mPosX[2]) / 2;
    mXheigh = (mPosX[1] + mPosX[3]) / 2;
    if (mXheigh <= mXlow)   {
        return -2;
    }

    mYlow = (mPosY[0] + mPosY[1]) / 2;
    mYheigh = (mPosY[2] + mPosY[3]) / 2;
    if (mYheigh <= mYlow)   {
        return -2;
    }
    return 1;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       signal handler
 *
 *
 * @param[in]   signal  signal numnber
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
terminate_program(const int signal)
{
    mRunning = -signal;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       setup signal handler
 *
 * @param       nothing
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
setup_sighandler(void)
{
    sigset_t    mask;

    sigemptyset(&mask);

    signal(SIGTERM, terminate_program);

    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   find_event_device: Find eGalax touchpanel device
 *
 *
 * @param       nothing
 * @return      device name
 * @retval      != NULL     device name string
 * @retvel      == NULL     system has no eGalax touchpanel
 */
/*--------------------------------------------------------------------------*/
static char *
find_event_device(void)
{
    FILE    *fp;
    int     eGalax = 0;
    int     i, j, k;
    char    buf[240];
    static char edevice[64];

    char *pdev = getenv(CALIBRATOIN_INPUT_DEV);

    if ((pdev != NULL) && (*pdev != 0)) {
        /* Search pseudo input device for Debug */
        int     fd;
        for (i = 0; i < 16; i++) {
            snprintf(edevice, 64, "/dev/input/event%d", i);
            fd = open(edevice, O_RDONLY | O_NONBLOCK);
            if (fd < 0)     continue;

            memset(buf, 0, sizeof(buf));
            ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
            close(fd);
            k = 0;
            for (j = 0; buf[j]; j++) {
                if (buf[j] != ' ') {
                    buf[k++] = buf[j];
                }
            }
            buf[k] = 0;

            if (strncasecmp(buf, pdev, sizeof(buf)) == 0)   {
                CALIBRATION_INFO("Event device of eGalax=<%s>\n", edevice);
                return edevice;
            }
        }
    }
    else    {
        /* Search input device      */
        fp = fopen("/proc/bus/input/devices", "r");
        if (!fp)    {
            CALIBRATION_PRINT("find_event_device: /proc/bus/input/devices Open Error\n");
            return NULL;
        }

        while (fgets(buf, sizeof(buf), fp)) {
            if (eGalax == 0)    {
                for (i = 0; buf[i]; i++)    {
                    if (strncmp(&buf[i], "eGalax", 6) == 0) {
                        /* Find eGalax touchpanel device    */
                        eGalax = 1;
                        break;
                    }
                }
            }
            else    {
                for (i = 0; buf[i]; i++)    {
                    if (strncmp(&buf[i], "Handlers", 8) == 0)   {
                        /* Find eGalax device and Handlers  */
                        for (j = i+8; buf[j]; j++)  {
                            if (buf[j] != ' ')  continue;
                            strcpy(edevice, "/dev/input/");
                            for (k = strlen(edevice); k < (int)sizeof(edevice); k++)    {
                                j++;
                                if ((buf[j] == 0) || (buf[j] == ' ') ||
                                    (buf[j] == '\t') || (buf[j] == '\n') ||
                                    (buf[j] == '\r'))   break;
                                edevice[k] = buf[j];
                            }
                            edevice[k] = 0;
                            CALIBRATION_INFO("Event device of eGalax=<%s>\n", edevice);
                            fclose(fp);
                            return edevice;
                        }
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    CALIBRATION_PRINT("%s: System has no eGalax Touchpanel\n", CALIBDAE_DEV_NAME);
    return NULL;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       event input and convert (main loop)
 *
 *
 * @param[in]   uifd        event output file descriptor
 * @param[in]   evfd        event input file descriptor
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
event_iterate(int uifd, int evfd)
{
    fd_set      fds;
    int         ret;
    int         rsize;
    int         ii;
    int         retry;
    struct timeval  delay;
    struct input_event events[128];
    struct input_event event;

    ioctl(evfd, EVIOCGRAB, 1);

    retry = 0;

    while (mRunning > 0) {
        delay.tv_sec = 1;
        delay.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(evfd, &fds);

        ret = select(evfd + 1, &fds, NULL, NULL, &delay);
        if (ret <= 0) {
            continue;
        }
        if (FD_ISSET(evfd, &fds)) {
            rsize = read(evfd, events, sizeof(events));
            if (rsize <= 0) {
                if (rsize < 0)  {
                    CALIBRATION_PRINT("event_iterate: input device(%d) end<%d>\n", evfd, errno);
                    retry ++;
                    if (retry > CALIBRATOIN_RETRY_COUNT)    {
                        return;
                    }
                }
                usleep(CALIBRATOIN_RETRY_WAIT * 1000);
                continue;
            }
            retry = 0;
            for (ii = 0; ii < (int)(rsize/sizeof(struct input_event)); ii++) {
                ret = calibration_event(&events[ii], &event);
#ifdef  REPLACE_TOUCH_EVENT
                if (ret >= 0)   {
                    if (write(uifd, &event, sizeof(struct input_event)) < 0)    {
                        CALIBRATION_PRINT("%s: Event write error %d[%d]\n",
                                          CALIBDAE_DEV_NAME, uifd, errno);
                    }
                    if (mEventLog == 2) {
                        push_event(&event);
                    }
                    if (ret > 0)   {
                        event.type = EV_KEY;
                        event.code = BTN_LEFT;
                        event.value = 1;
                        if (write(uifd, &event, sizeof(struct input_event)) < 0)    {
                            CALIBRATION_PRINT("%s: Event write error %d[%d]\n",
                                              CALIBDAE_DEV_NAME, uifd, errno);
                        }
                        else    {
                            CALIBRATION_DEBUG("EV_KEY=BTN_LEFT\n");
                        }
                        if (mEventLog == 2) {
                            push_event(&event);
                        }
                    }
                }
#else  /*REPLACE_TOUCH_EVENT*/
                ret = write(uifd, &event, sizeof(struct input_event));
                if (mEventLog == 2) {
                    push_event(&event);
                }
#endif /*REPLACE_TOUCH_EVENT*/
            }
        }
    }
    ioctl(evfd, EVIOCGRAB, 0);
}

static void
push_eventlog(const char *event, const int value, struct timeval *tp)
{
    struct timeval  curtime;

    gettimeofday(&curtime, NULL);

    if (lastEvent.tv_sec)   {
        int     sec, usec;

        sec = curtime.tv_sec - lastEvent.tv_sec;
        usec = curtime.tv_usec - lastEvent.tv_usec;

        if (usec < 0)   {
            sec -= 1;
            usec += 1000000;
        }
        usec += 500;
        if( usec >= 1000000 )   {
            usec -= 1000000;
            sec += 1;
        }
        if ((sec > 0) || ((sec == 0) && (usec >= 10000)))   {
            printf("sleep %d.%03d\n", sec, usec / 1000);
            lastEvent.tv_sec = curtime.tv_sec;
            lastEvent.tv_usec = curtime.tv_usec;
        }
    }
    else    {
        lastEvent.tv_sec = curtime.tv_sec;
        lastEvent.tv_usec = curtime.tv_usec;
    }
    printf("%s=%d\t# %d.%03d\n", event, value, (int)tp->tv_sec, (int)(tp->tv_usec/1000));
    fflush(stdout);
}

static void
push_event(struct input_event *ev)
{
    switch(ev->type)    {
    case EV_ABS:
        switch(ev->code)    {
        case ABS_X:
            push_eventlog("X", ev->value, &(ev->time));
            break;
        case ABS_Y:
            push_eventlog("Y", ev->value, &(ev->time));
            break;
        default:
            break;
        }
        break;
    case EV_KEY:
        switch(ev->code)    {
        case BTN_TOUCH:
            push_eventlog("Touch", ev->value, &(ev->time));
            break;
        case BTN_LEFT:
            push_eventlog("Button", ev->value, &(ev->time));
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       convert x/y coordinates
 *
 * @param[in]   in          input event
 * @param[in]   out         output converted event
 * @return      result
 * @retval      -1          input event queued(no output)
 * @retval      0           no input event
 * @retval      1           event converted
 */
/*--------------------------------------------------------------------------*/
static int
calibration_event(struct input_event *in, struct input_event *out)
{
    int     ret = 0;
#ifdef  REPLACE_TOUCH_EVENT
    static int queue_touch = 0;
#endif /*REPLACE_TOUCH_EVENT*/

    memcpy(out, in, sizeof(struct input_event) );

    switch (in->type) {
    case EV_ABS:
        /* absolute coordinate      */
        switch (in->code) {
            /* X/Y coordinate       */
        case ABS_X:
            out->value = mDispWidth * (in->value - mXlow) / (mXheigh - mXlow);
            if (mRevX)  {
                out->value = mDispWidth - out->value;
            }
            if (out->value < 0)             out->value = 0;
            if (out->value >= mDispWidth)   out->value = mDispWidth - 1;

            if (mTrans == 90)   {
                out->code = ABS_Y;
                out->value = mDispWidth - out->value - 1;
            }
            else if (mTrans == 180) {
                out->value = mDispWidth - out->value - 1;
            }
            else if (mTrans == 270) {
                out->code = ABS_Y;
            }
            if (mEventLog == 1) {
                push_eventlog("X", in->value, &(in->time));
            }
#ifdef  REPLACE_TOUCH_EVENT
            if (queue_touch & 1)    {
                queue_touch &= ~1;
                if (queue_touch == 0)   {
                    ret = 1;
                }
            }
#endif /*REPLACE_TOUCH_EVENT*/
            CALIBRATION_DEBUG("ABS_X=%s %d=>%d\n",
                              (out->code == ABS_X) ? "X" : "Y",
                              in->value, out->value);
            break;

        case ABS_Y:
            out->value = mDispHeight * (in->value - mYlow) / (mYheigh - mYlow);
            if (mRevY)  {
                out->value = mDispHeight - out->value;
            }
            if (out->value < 0)             out->value = 0;
            if (out->value >= mDispHeight)  out->value = mDispHeight - 1;

            if (mTrans == 90)   {
                out->code = ABS_X;
            }
            else if (mTrans == 180) {
                out->value = mDispHeight - out->value - 1;
            }
            else if (mTrans == 270) {
                out->code = ABS_X;
                out->value = mDispHeight - out->value - 1;
            }
            if (mEventLog == 1) {
                push_eventlog("Y", in->value, &(in->time));
            }
#ifdef  REPLACE_TOUCH_EVENT
            if (queue_touch & 2)    {
                queue_touch &= ~2;
                if (queue_touch == 0)   {
                    ret = 1;
                }
            }
#endif /*REPLACE_TOUCH_EVENT*/
            CALIBRATION_DEBUG("ABS_Y=%s %d=>%d\n",
                              (out->code == ABS_X) ? "X" : "Y",
                              in->value, out->value);
            break;

        default:
            CALIBRATION_DEBUG("calibration_event: Unknown code(0x%x)\n", (int)in->code);
            break;
        }
        break;

#ifdef  REPLACE_TOUCH_EVENT
    case EV_KEY:
        if (in->code == BTN_TOUCH)  {
            if (mEventLog == 1) {
                push_eventlog("Touch", in->value, &(in->time));
            }
            /* Touch event change to mouse left button event    */
            out->code = BTN_LEFT;
            if (out->value != 0)    {
                queue_touch = 3;
                ret = -1;
                CALIBRATION_DEBUG("BTN_TOUCH=LEFT, queue(%d)\n", out->value);
            }
            else    {
                ret = 0;
                CALIBRATION_DEBUG("BTN_TOUCH=LEFT(%d)\n", out->value);
            }
        }
        break;
#endif /*REPLACE_TOUCH_EVENT*/

    case EV_SYN:
        CALIBRATION_DEBUG("calibration_event: SYN\n");
        break;

    default:
        CALIBRATION_DEBUG("calibration_event: Unknown type(0x%x)\n", (int)in->type);
        break;
    }
    if (out->value < 0) {
        out->value = 0;
    }

    return ret;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       setup uinput device for event output
 *
 * @param[in]   uinputDeviceName        uinput device node name
 * @return      uinput file descriptor
 * @retval      >= 0    file descriptor
 * @retval      < 0     error
 */
/*--------------------------------------------------------------------------*/
static int
setup_uinput(char *uinputDeviceName)
{
    int     uifd;
    struct uinput_user_dev  uinputDevice;

    uifd = open_uinput(uinputDeviceName);
    if (uifd < 0)   {
        perror("Open uinput");
        return -1;
    }

    memset(&uinputDevice, 0, sizeof(uinputDevice));
    strcpy(uinputDevice.name, CALIBDAE_DEV_NAME);
    uinputDevice.absmax[ABS_X] = mDispWidth;
    uinputDevice.absmax[ABS_Y] = mDispHeight;

    /* uinput device configuration  */
    if (write(uifd, &uinputDevice, sizeof(uinputDevice)) < (int)sizeof(uinputDevice)) {
        perror("Regist uinput");
        CALIBRATION_PRINT("setup_uinput: write(%d) Error[%d]\n", uifd, errno);
        close(uifd);
        return -1;
    }

    /* uinput set event bits        */
    set_eventbit(uifd);

    if (ioctl(uifd, UI_DEV_CREATE, NULL) < 0)   {
        CALIBRATION_PRINT("setup_uinput: ioclt(%d,UI_DEV_CREATE,) Error[%d]\n", uifd, errno);
    }
    return uifd;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       event bit set
 *
 * @param[in]   uifd        uinput file descriptor
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
set_eventbit(int uifd)
{
    ioctl(uifd, UI_SET_EVBIT, EV_SYN);

    ioctl(uifd, UI_SET_EVBIT, EV_ABS);
    ioctl(uifd, UI_SET_ABSBIT, ABS_X);
    ioctl(uifd, UI_SET_ABSBIT, ABS_Y);

    ioctl(uifd, UI_SET_EVBIT, EV_KEY);
#ifdef  REPLACE_TOUCH_EVENT
    ioctl(uifd, UI_SET_KEYBIT, BTN_LEFT);
#else  /*REPLACE_TOUCH_EVENT*/
    ioctl(uifd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(uifd, UI_SET_KEYBIT, BTN_TOOL_PEN);
#endif /*REPLACE_TOUCH_EVENT*/

    ioctl(uifd, UI_SET_EVBIT, EV_MSC);
    ioctl(uifd, UI_SET_MSCBIT, MSC_SCAN);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       open uinput device
 *
 * @param[in]   uinputDeviceName    uinput device name
 * @return      uinput file descriptor
 * @retval      >= 0        file descriptor
 * @retval      < 0         error
 */
/*--------------------------------------------------------------------------*/
static int
open_uinput(char *uinputDeviceName)
{
    return open(uinputDeviceName, O_RDWR);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       close uinput device
 *
 * @param[in]   uifd        uinput file descriptor
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
close_uinput(int uifd)
{
    if (uifd >= 0) {
        ioctl(uifd, UI_DEV_DESTROY, NULL);
        close(uifd);
        uifd = -1;
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       print help message
 *
 * @param[in]   pName       program name
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
print_usage(const char *pName)
{
    fprintf(stderr, "Usage: %s [-h][-d][-t [rotate]] [device]\n", pName );
}

