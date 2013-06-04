/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Touchpanel(eGalax) Calibration Tool
 *
 * @date    Feb-20-2013
 */

#include "ico_ictl-touch_egalax.h"

/* Number of times to repeat a touch about each point   */
#define CALIBCONF_READ_NUM  (5)
#define XY_COORDNATE_DELTA  (50)

/* Macro for adjust coordinate      */
#define delta_add(x)    \
    (x) > 256 ? (x) + XY_COORDNATE_DELTA : ((x) > XY_COORDNATE_DELTA ? (x) - XY_COORDNATE_DELTA : 0);

static void print_usage(const char *pName);
static char *find_event_device(void);
static FILE *open_conffile(void);
static void get_coordinates(int evfd);
static void read_event(int evfd, int *x, int *y);
static void sort_data(int buff[], int left, int right);

int             mDispWidth = CALIBRATION_DISP_WIDTH;
int             mDispHeight = CALIBRATION_DISP_HEIGHT;
int             mPosX[4];
int             mPosY[4];

int             mDebug = 0;

/*--------------------------------------------------------------------------*/
/**
 * @brief   Touchpanel(eGalax) Calibration Tool main routine
 *
 * @param   arguments of standard main routeins(argc, argv)
 * @return  result
 * @retval  0       sucess
 * @retval  8       configuration file Error
 * @retval  9       can not open event device(touchpanel)
 */
/*--------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    int     ii, cnt;
    int     evfd;                               /* event device fd */
    char    *eventDeviceName = NULL;            /* event device name to hook */
    FILE    *fp;
    char    buff[128];

    /* Get options                      */
    for (ii = 1; ii < argc; ii++) {
        if (strcasecmp(argv[ii], "-h") == 0) {
            /* Help                     */
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcasecmp(argv[ii], "-width") == 0)   {
            /* Screen width             */
            ii++;
            if (ii >= argc) {
                print_usage(argv[0]);
                exit(0);
            }
            mDispWidth = strtol(argv[ii], (char **)0, 0);
            if ((mDispWidth <= 0) || (mDispWidth > 8192))   {
                print_usage(argv[0]);
                exit(0);
            }
        }
        else if (strcasecmp(argv[ii], "-height") == 0)  {
            /* Screen height            */
            ii++;
            if (ii >= argc) {
                print_usage(argv[0]);
                exit(0);
            }
            mDispHeight = strtol(argv[ii], (char **)0, 0);
            if ((mDispHeight <= 0) || (mDispHeight > 8192)) {
                print_usage(argv[0]);
                exit(0);
            }
        }
        else {
            /* Input event device name  */
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

    evfd = open(eventDeviceName, O_RDONLY);
    if (evfd < 0) {
        perror("Open event device");
        exit(9);
    }

    /* Open configuration file for output   */
    fp = open_conffile();
    if (fp == NULL) {
        fprintf(stderr, "%s: Can not open config file\n", argv[0]);
        close(evfd);
        exit(8);
    }

    CALIBRATION_PRINT("\n");
    CALIBRATION_PRINT("+================================================+\n");
    CALIBRATION_PRINT("| Configration Tool for Calibration Touch ver0.1 |\n");
    CALIBRATION_PRINT("+------------------------------------------------+\n");
    CALIBRATION_PRINT("| use display width = %d\n", CALIBRATION_DISP_WIDTH);
    CALIBRATION_PRINT("| use display height = %d\n", CALIBRATION_DISP_HEIGHT);
    CALIBRATION_PRINT("+------------------------------------------------+\n");

    get_coordinates(evfd);

    CALIBRATION_PRINT("+------------------------------------------------+\n");
    CALIBRATION_PRINT("| save config                                    |\n");
    CALIBRATION_PRINT("+------------------------------------------------+\n");

    cnt = sprintf(buff, "%s=%d\n", CALIBRATOIN_STR_DISP_W, mDispWidth);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    cnt = sprintf(buff, "%s=%d\n", CALIBRATOIN_STR_DISP_H, mDispHeight);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    cnt = sprintf(buff, "%s=%d*%d\n", CALIBRATOIN_STR_POS1, mPosX[0], mPosY[0]);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    cnt = sprintf(buff, "%s=%d*%d\n", CALIBRATOIN_STR_POS2, mPosX[1], mPosY[1]);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    cnt = sprintf(buff, "%s=%d*%d\n", CALIBRATOIN_STR_POS3, mPosX[2], mPosY[2]);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    cnt = sprintf(buff, "%s=%d*%d\n", CALIBRATOIN_STR_POS4, mPosX[3], mPosY[3]);
    fwrite(buff, sizeof(char), cnt, fp);
    CALIBRATION_PRINT("| %s", buff);

    if (evfd >= 0) {
        close(evfd);
    }

    /* Close outputed configuration file    */
    fclose(fp);

    CALIBRATION_PRINT("|                                                |\n");
    CALIBRATION_PRINT("| finish Tools                                   |\n");

    exit(0);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   find_event_device: Find eGalax touchpanel device
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

    /* Search input device      */
    fp = fopen("/proc/bus/input/devices", "r");
    if (!fp)    {
        CALIBRATION_INFO("find_event_device: /proc/bus/input/devices Open Error\n");
        return(NULL);
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
                        return(edevice);
                    }
                    break;
                }
            }
        }
    }
    fclose(fp);

    CALIBRATION_PRINT("System has no eGalax Touchpanel\n");
    return(NULL);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       read coordinates form touchpanel
 *
 * @param[in]   evfd        touchpanel event device
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
get_coordinates(int evfd)
{
    int     bufX[CALIBCONF_READ_NUM];
    int     bufY[CALIBCONF_READ_NUM];
    int     x, y;
    int     ii;

    CALIBRATION_PRINT("| Touch the Top-Left corner of the screen %d times\n",
                      CALIBCONF_READ_NUM);
    for (ii = 0; ii < CALIBCONF_READ_NUM; ii++) {
        read_event(evfd, &x, &y);
        bufX[ii] = x;
        bufY[ii] = y;
        CALIBRATION_PRINT("| # %d: %dx%d \n", ii, x, y);
    }
    sort_data(bufX, 0, CALIBCONF_READ_NUM - 1);
    sort_data(bufY, 0, CALIBCONF_READ_NUM - 1);
    mPosX[0] = delta_add(bufX[CALIBCONF_READ_NUM / 2]);
    mPosY[0] = delta_add(bufY[CALIBCONF_READ_NUM / 2]);

    CALIBRATION_PRINT("+------------------------------------------------+\n");
    CALIBRATION_PRINT("| Touch the Top-Right corner of the screen %d times\n",
                      CALIBCONF_READ_NUM);
    for (ii = 0; ii < CALIBCONF_READ_NUM; ii++) {
        read_event(evfd, &x, &y);
        bufX[ii] = x;
        bufY[ii] = y;
        CALIBRATION_PRINT("| # %d: %dx%d \n", ii, x, y);
    }
    sort_data(bufX, 0, CALIBCONF_READ_NUM - 1);
    sort_data(bufY, 0, CALIBCONF_READ_NUM - 1);
    mPosX[1] = delta_add(bufX[CALIBCONF_READ_NUM / 2]);
    mPosY[1] = delta_add(bufY[CALIBCONF_READ_NUM / 2]);

    CALIBRATION_PRINT("+------------------------------------------------+\n");
    CALIBRATION_PRINT("| Touch the Bottom-Left corner of the screen %d times\n",
                      CALIBCONF_READ_NUM);
    for (ii = 0; ii < CALIBCONF_READ_NUM; ii++) {
        read_event(evfd, &x, &y);
        bufX[ii] = x;
        bufY[ii] = y;
        CALIBRATION_PRINT("| # %d: %dx%d \n", ii, x, y);
    }
    sort_data(bufX, 0, CALIBCONF_READ_NUM - 1);
    sort_data(bufY, 0, CALIBCONF_READ_NUM - 1);
    mPosX[2] = delta_add(bufX[CALIBCONF_READ_NUM / 2]);
    mPosY[2] = delta_add(bufY[CALIBCONF_READ_NUM / 2]);

    CALIBRATION_PRINT("+------------------------------------------------+\n");
    CALIBRATION_PRINT("| Touch the Bottom-Right corner of the screen %d times\n",
                      CALIBCONF_READ_NUM);
    for (ii = 0; ii < CALIBCONF_READ_NUM; ii++) {
        read_event(evfd, &x, &y);
        bufX[ii] = x;
        bufY[ii] = y;
        CALIBRATION_PRINT("| # %d: %dx%d \n", ii, x, y);
    }
    sort_data(bufX, 0, CALIBCONF_READ_NUM - 1);
    sort_data(bufY, 0, CALIBCONF_READ_NUM - 1);
    mPosX[3] = delta_add(bufX[CALIBCONF_READ_NUM / 2]);
    mPosY[3] = delta_add(bufY[CALIBCONF_READ_NUM / 2]);
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       sort integer dates
 *
 * @param[in]   buff    array of integer datas
 * @param[in]   left    array start index
 * @param[in]   right   array end index
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
sort_data(int buff[], int left, int right)
{
    int     ii, jj, pivot;
    int     tmp;

    ii = left;
    jj = right;

    pivot = buff[(left + right) / 2];
    while (1) {
        while (buff[ii] < pivot) {
            ii++;
        }
        while (pivot < buff[jj]) {
            jj--;
        }
        if (ii >= jj) {
            break;
        }
        tmp = buff[ii];
        buff[ii] = buff[jj];
        buff[jj] = tmp;
        ii++;
        jj--;
    }

    if (left < (ii - 1)) {
        sort_data(buff, left, ii - 1);
    }
    if ((jj + 1) < right) {
        sort_data(buff, jj + 1, right);
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       event read from touchpanel
 *
 * @param[in]   evfd        touchpanel event device
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
read_event(int evfd, int *x, int *y)
{
    fd_set      fds;
    int         ret;
    int         rsize;
    int         ii;
    int         flagX = 0;
    int         flagY = 0;
    struct timeval  delay;
    struct input_event events[64];

    while (1) {
        /* select timeoutt value    */
        delay.tv_sec = 0;
        delay.tv_usec = 100*1000;
        /* fs_set                   */
        FD_ZERO(&fds);
        FD_SET(evfd, &fds);

        /* wait for event read or timedout  */
        ret = select(evfd + 1, &fds, NULL, NULL, &delay);
        if (ret <= 0) {
            continue;
        }
        if (FD_ISSET(evfd, &fds)) {
            /* event read           */
            rsize = read(evfd, events, sizeof(events) );
            for (ii = 0; ii < (int)(rsize/sizeof(struct input_event)); ii++) {
                if ((events[ii].type == EV_ABS) && (events[ii].code == ABS_X)) {
                    /* X       */
                    flagX++;
                    *x = events[ii].value;
                }
                else if ((events[ii].type == EV_ABS) && (events[ii].code == ABS_Y)) {
                    /* Y       */
                    flagY++;
                    *y = events[ii].value;
                }
            }
        }
        /* Input 2 times (Touch On and Off) */
        if ((flagX >= 2) && (flagY >= 2)) {
            break;
        }
    }

    while (1) {
        delay.tv_sec = 0;
        delay.tv_usec = 200*1000;
        FD_ZERO(&fds);
        FD_SET(evfd, &fds);

        /* wait for input (or timedout) */
        ret = select(evfd + 1, &fds, NULL, NULL, &delay);
        if (ret == 0) {
            break;
        }
        else if (ret < 0) {
            continue;
        }
        rsize = read(evfd, events, sizeof(events));
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       open configuration file
 *
 * @param       nothing
 * @return      file descriptor
 * @retval      != NULL     file descriptor
 * @retval      NULL        open error
 */
/*--------------------------------------------------------------------------*/
static FILE *
open_conffile(void)
{
    char    *confp;
    FILE    *fp;

    /* Get configuration file path  */
    confp = getenv(CALIBRATOIN_CONF_ENV);
    if (! confp)  {
        confp = CALIBRATOIN_CONF_FILE;
    }

    /* Open configuration file      */
    fp = fopen(confp, "w");
    if (fp == NULL) {
        perror(confp);
        return fp;
    }
    return fp;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief       print help
 *
 * @param[in]   pName       program name
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
print_usage(const char *pName)
{
    fprintf(stderr, "Usage: %s [-h][-width width][-height height] [device]\n", pName);
}

