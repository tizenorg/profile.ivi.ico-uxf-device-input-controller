/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Device Input Controller(GtForce joystick)
 * @brief   joystick input event to Input Manager
 *
 * @date    Feb-08-2013
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <strings.h>
#include    <errno.h>
#include    <pthread.h>
#include    <sys/ioctl.h>
#include    <linux/joystick.h>
#include    <glib.h>

#include    "ico_ictl-local.h"

/* type definition                                                                  */
typedef struct  _Ico_ICtl_JS    {
    int                     fd;                 /* device file fd                   */
    char                    device[32];         /* device name                      */
    char                    ictl[32];           /* input controller name            */
    int                     type;               /* device type                      */
    int                     hostid;             /* host Id(currently unused)        */
}   Ico_ICtl_JS;

typedef struct  _Ico_Ictl_Code  {
    unsigned short          code;               /* code value                       */
    char                    name[20];           /* code name                        */
}   Ico_Ictl_Code;

typedef struct  _Ico_ICtl_JS_Input  {
    char                    name[20];           /* input switch name                */
    int                     input;              /* input number                     */
    int                     type;               /* input event type                 */
    int                     number;             /* input event number               */
    Ico_Ictl_Code           code[20];           /* key code                         */
    int                     last;               /* last input code                  */
}   Ico_ICtl_JS_Input;

/* prototype of static function                                                     */
static void PrintUsage(const char *pName);

/* table/variable                                                                   */
int                 mPseudo = 0;                /* pseudo input device for test     */
int                 mDebug = 0;                 /* debug mode                       */
int                 mEventLog = 0;              /* event input log                  */
struct timeval      lastEvent = { 0, 0 };       /* last input event time            */
int                 gRunning = 1;               /* run state(1:run, 0:finish)       */

/* Input Contorller Table           */
Ico_ICtl_Mng        gIco_ICtrl_Mng = { 0 };
Ico_ICtl_JS         gIco_ICtrl_JS = { 0 };
int                 nIco_ICtrl_JS_Input = 0;
Ico_ICtl_JS_Input   *gIco_ICtrl_JS_Input = NULL;

/* static functions                 */
/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_find_input_by_name: find Input Table by input switch name
 *
 * @param[in]   name        input switch name
 * @return  result
 * @retval  !=NULL      success(Input Table address)
 * @retval  ==NULL      failed
 */
/*--------------------------------------------------------------------------*/
static Ico_ICtl_JS_Input *
ico_ictl_find_input_by_name(const char *name)
{
    Ico_ICtl_JS_Input   *iMng = NULL;
    int                  ii;

    for (ii = 0; ii < nIco_ICtrl_JS_Input; ii++)    {
        if (strncasecmp(name, gIco_ICtrl_JS_Input[ii].name, 16) == 0) {
            iMng = &gIco_ICtrl_JS_Input[ii];
            break;
        }
    }
    return iMng;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_find_input_by_param: find Input Table by input switch type and number
 *
 * @param[in]   type        input event type (of Linux Input subsystem)
 * @param[in]   number      input event number (of Linux Input subsystem)
 * @return  result
 * @retval  !=NULL      success(Input Table address)
 * @retval  ==NULL      failed
 */
/*--------------------------------------------------------------------------*/
static Ico_ICtl_JS_Input *
ico_ictl_find_input_by_param(int type, int number)
{
    Ico_ICtl_JS_Input   *iMng = NULL;
    int                  ii;

    for (ii = 0; ii < nIco_ICtrl_JS_Input; ii++)    {
        if ((gIco_ICtrl_JS_Input[ii].type == type)
            && (gIco_ICtrl_JS_Input[ii].number == number))  {
            iMng = &gIco_ICtrl_JS_Input[ii];
            break;
        }
    }
    return iMng;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   conf_getUint: convert integer string to value
 *
 * @param[in]   str         integer string
 * @return  result
 * @retval  >=0         success(converted vaule)
 * @retval  -1          failed
 */
/*--------------------------------------------------------------------------*/
static int
conf_getUint(const char *str)
{
    int     key = -1;
    char    *errpt;

    if (str != NULL)    {
        errpt = NULL;
        key = strtol(str, &errpt, 0);
        if ((errpt) && (*errpt != 0))  {
            key = -1;
        }
    }
    return key;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   conf_countNumericalKey: get configuration list
 *
 * @param[in]   keyfile     configuration file
 * @param[in]   group       configuration key group name
 * @return  result
 * @retval  !=NULL          success(configuration list)
 * @retval  ==NULL          failed
 */
/*--------------------------------------------------------------------------*/
static GList *
conf_countNumericalKey(GKeyFile *keyfile, const char *group)
{
    GList* list=NULL;
    char **result;
    gsize length;
    int i;

    result = g_key_file_get_keys(keyfile, group, &length, NULL);

    for (i = 0; i < (int)length; i++) {
        int id = conf_getUint(result[i]);
        if (id >= 0) {
            list=g_list_append(list, g_strdup(result[i]));
        }
    }
    g_strfreev(result);
    return list;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   conf_appendStr: connect strings
 *
 * @param[in]   str1        string 1
 * @param[in]   str2        string 2
 * @return  connected string
 */
/*--------------------------------------------------------------------------*/
static char *
conf_appendStr(const char *str1, const char *str2)
{
    static char buf[128];
    snprintf(buf, sizeof(buf)-1, "%s%s", str1, str2);
    return buf;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_read_conf: read configuration file
 *
 * @param[in]   file        configuration file path name
 * @return  result
 * @retval  ICO_ICTL_OK     sccess
 * @retval  ICO_ICTL_ERR    failed
 */
/*--------------------------------------------------------------------------*/
static int
ico_ictl_read_conf(const char *file)
{
    DEBUG_PRINT("ico_ictl_read_conf: Enter(file=%s)", file);

    GKeyFile            *keyfile;
    GKeyFileFlags       flags;
    GString             *filepath;
    GList               *idlist;
    GError              *error = NULL;
    Ico_ICtl_JS_Input   *iMng;
    char                *name;
    gsize               length;
    int                 ii, jj;

    keyfile = g_key_file_new();
    flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;

    filepath = g_string_new(file);

    if (! g_key_file_load_from_file(keyfile, filepath->str, flags, &error)) {
        ERROR_PRINT("ico_ictl_read_conf: Leave(can not open conf file)");
        g_string_free(filepath, TRUE);
        return ICO_ICTL_ERR;
    }
    g_string_free(filepath, TRUE);

    /* count number of key in [device] section   */
    memset((char *)&gIco_ICtrl_JS, 0, sizeof(gIco_ICtrl_JS));
    name = g_key_file_get_string(keyfile, "device", "name", &error);
    if (name)   {
        strncpy(gIco_ICtrl_JS.device, name, sizeof(gIco_ICtrl_JS.device)-1);
    }
    name = g_key_file_get_string(keyfile, "device", "ictl", &error);
    if (name)   {
        strncpy(gIco_ICtrl_JS.ictl, name, sizeof(gIco_ICtrl_JS.ictl)-1);
    }
    gIco_ICtrl_JS.type = g_key_file_get_integer(keyfile, "device", "type", &error);
    gIco_ICtrl_JS.hostid = g_key_file_get_integer(keyfile, "device", "ecu", &error);

    /* count number of key in [input] section   */
    idlist = conf_countNumericalKey(keyfile, "input");
    length = g_list_length(idlist);
    if (length <= 0)    {
        length = 1;
    }
    nIco_ICtrl_JS_Input = 0;
    gIco_ICtrl_JS_Input = (Ico_ICtl_JS_Input *)malloc(sizeof(Ico_ICtl_JS_Input) * length);
    if (! gIco_ICtrl_JS_Input)  {
        ERROR_PRINT("joystick_gtforce: No Memory");
        exit(1);
    }
    memset((char *)gIco_ICtrl_JS_Input, 0, sizeof(Ico_ICtl_JS_Input) * length);

    for (ii = 0; ii < (int)length; ii++) {
        const char  *g = "input";
        char        *key = (char *)g_list_nth_data(idlist, ii);
        gsize       listsize;
        gint        *attr;
        gchar       **code;

        name = g_key_file_get_string(keyfile, g, key, &error);
        if (name == NULL)   continue;

        iMng = ico_ictl_find_input_by_name(name);
        if (iMng != NULL)   {
            /* multiple define  */
            ERROR_PRINT("ico_ictl_read_conf: switch name(%s) re-define", name);
            continue;
        }
        iMng = &gIco_ICtrl_JS_Input[nIco_ICtrl_JS_Input];

        iMng->input = conf_getUint(key);
        strncpy(iMng->name, name, sizeof(iMng->name)-1);

        /* event            */
        attr = g_key_file_get_integer_list(keyfile, g, conf_appendStr(key, ".event"),
                                           &listsize, &error);
        if (listsize < 2)   continue;
        iMng->type = attr[0];
        iMng->number = attr[1];

        /* code             */
        code = g_key_file_get_string_list(keyfile, g, conf_appendStr(key, ".code"),
                                          &listsize, &error);
        if ((code == NULL) || (listsize <= 0))  {
            strcpy(iMng->code[0].name, iMng->name);
        }
        else    {
            if ((int)listsize > 20) listsize = 20;
            for (jj = 0; jj < (int)listsize; jj++) {
                char    *p = (char *)code[jj];
                while ((*p != 0) && (*p != ':'))    {
                    iMng->code[jj].code = iMng->code[jj].code * 10 + *p - '0';
                    p ++;
                }
                if (*p) {
                    p ++;
                    strncpy(iMng->code[jj].name, p, sizeof(iMng->code[jj].name)-1);
                }
                if (iMng->code[jj].name[0] == 0)    {
                    strcpy(iMng->code[jj].name, iMng->name);
                }
            }
        }
        if (code)   g_strfreev(code);

        nIco_ICtrl_JS_Input ++;

        DEBUG_PRINT("%s input:%d(type=%d,number=%d,code=%d[%s],%d[%s])",
                    iMng->name, iMng->input, iMng->type, iMng->number,
                    iMng->code[0].code, iMng->code[0].name,
                    iMng->code[1].code, iMng->code[1].name);
    }
    DEBUG_PRINT("ico_ictl_read_conf: Leave");

    return ICO_ICTL_OK;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_js_open: open input jyostick input device
 *
 * @param[in]   ictlDevName     device name
 * @return  result
 * @retval  >= 0            sccess(device file descriptor)
 * @retval  ICO_ICTL_ERR    failed
 */
/*--------------------------------------------------------------------------*/
static int
ico_ictl_js_open(const char *ictlDevName)
{
    DEBUG_PRINT("ico_ictl_js_open: Enter(device=%s)", ictlDevName)

    int                 fd = -1;
    char                devFile[64];
    char                devName[64];
    int                 ii, jj, kk;

    if (ictlDevName == NULL) {
        ERROR_PRINT("ico_ictl_js_open: Leave(failed devname NULL)");
        return ICO_ICTL_ERR;
    }

    char *pdev = getenv(ICO_ICTL_INPUT_DEV);
    if ((pdev != NULL) && (*pdev != 0)) {
        DEBUG_PRINT("ico_ictl_js_open: Pseudo input device(%s)", pdev);
        mPseudo = 1;
    }
    else    {
        pdev = (char *)ictlDevName;
    }
    for (ii = 0; ii < 16; ii++) {
        if (mPseudo)    {
            snprintf(devFile, 64, "/dev/input/event%d", ii);
        }
        else    {
            snprintf(devFile, 64, "/dev/input/js%d", ii);
        }
        fd = open(devFile, O_RDONLY | O_NONBLOCK);
        if (fd < 0)     continue;

        memset(devName, 0, sizeof(devName));
        if (mPseudo)    {
            ioctl(fd, EVIOCGNAME(sizeof(devName)), devName);
        }
        else    {
            ioctl(fd, JSIOCGNAME(sizeof(devName)), devName);
        }
        kk = 0;
        for (jj = 0; devName[jj]; jj++) {
            if (devName[jj] != ' ') {
                devName[kk++] = devName[jj];
            }
        }
        devName[kk] = 0;
        DEBUG_PRINT("ico_ictl_js_open: %d.%s", ii+1, devName);

        if (strncasecmp(devName, pdev, sizeof(devName)) == 0)   break;
        /* not match, close */
        close(fd);
        fd = -1;
    }

    if (fd < 0) {
        ERROR_PRINT("ico_ictl_js_open: Leave(not find device file)");
        return ICO_ICTL_ERR;
    }
    DEBUG_PRINT("ico_ictl_js_open: Leave(found %s[%s] as %s)", pdev, devFile, ictlDevName);
    return fd;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   ico_ictl_js_read: read input jyostick input device
 *
 * @param[in]   fd          file descriptor
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
ico_ictl_js_read(int fd)
{
    DEBUG_PRINT("ico_ictl_js_read: Enter(fd=%d)", fd)

    struct js_event     events[8];
    struct input_event  pevents[8];
    int                 rSize;
    int                 ii, jj;
    int                 number, value, type, code, state;

    if (mPseudo)    {
        /* Pseudo event input for Debug */
        rSize = read(fd, pevents, sizeof(pevents));
        if (rSize > 0)  {
            for (ii = 0; ii < rSize/((int)sizeof(struct input_event)); ii++)    {
                events[ii].time = (pevents[ii].time.tv_sec % 1000) * 1000 +
                                  pevents[ii].time.tv_usec / 1000;
                events[ii].type = pevents[ii].type;
                events[ii].number = pevents[ii].code;
                events[ii].value = pevents[ii].value;
                if ((events[ii].type == 2) && (events[ii].value == 9))  {
                    events[ii].value = 0;
                }
                else if ((events[ii].type == 1) && (events[ii].number == 9))    {
                    events[ii].number = 0;
                }
                DEBUG_PRINT("ico_ictl_js_read: pseude event.%d %d.%d.%d",
                            ii, events[ii].type, events[ii].number, events[ii].value);
            }
            rSize = ii * sizeof(struct js_event);
        }
    }
    else    {
        rSize = read(fd, events, sizeof(events));
    }
    if (rSize < 0)  {
        ii = errno;
        if ((ii == EINTR) || (ii == EAGAIN))    {
            return;
        }
        DEBUG_PRINT("ico_ictl_js_read: Leave(read error[%d])", ii)
        exit(9);
    }
    for (ii = 0; ii < (rSize / (int)sizeof(struct js_event)); ii++) {
        Ico_ICtl_JS_Input   *iMng = NULL;

        type = events[ii].type;
        number = events[ii].number;
        value = events[ii].value;
        DEBUG_PRINT("ico_ictl_js_read: Read(type=%d, number=%d, value=%d",
                    type, number, value);

        iMng = ico_ictl_find_input_by_param(type, number);
        if (iMng == NULL) {
            continue;
        }

        if (mEventLog)  {
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
                    lastEvent.tv_sec = curtime.tv_sec;
                    lastEvent.tv_usec = curtime.tv_usec;
                }
            }
            else    {
                lastEvent.tv_sec = curtime.tv_sec;
                lastEvent.tv_usec = curtime.tv_usec;
            }
            for (jj = 0;
                 jj < (int)(sizeof(gIco_ICtrl_JS_Input)/sizeof(Ico_ICtl_JS_Input)); jj++) {
                if ((type == gIco_ICtrl_JS_Input[jj].type) &&
                    (number == gIco_ICtrl_JS_Input[jj].number))  {
                    break;
                }
            }
        }

        if (iMng->code[1].code != 0)    {
            if (value < 0) {
                code = iMng->code[0].code;
                state = WL_KEYBOARD_KEY_STATE_PRESSED;
                iMng->last = code;
            }
            else if (value > 0) {
                code = iMng->code[1].code;
                state = WL_KEYBOARD_KEY_STATE_PRESSED;
                iMng->last = code;
            }
            else {
                if (iMng->last != iMng->code[0].code && iMng->last != iMng->code[1].code) {
                    continue;
                }
                code = iMng->last;
                state = WL_KEYBOARD_KEY_STATE_RELEASED;
                iMng->last = -1;
            }
        }
        else    {
            if (value == 0) {
                code = iMng->code[0].code;
                state = WL_KEYBOARD_KEY_STATE_RELEASED;
            }
            else if (value == 1) {
                code = iMng->code[0].code;
                state = WL_KEYBOARD_KEY_STATE_PRESSED;
            }
            else {
                continue;
            }
        }
        ico_input_mgr_device_input_event(gIco_ICtrl_Mng.Wayland_InputMgr, events[ii].time,
                                         gIco_ICtrl_JS.device, iMng->input, code, state);
    }
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   signal_int: signal handler
 *
 * @param[in]   signum      signal number(unused)
 * @return      nothing
 */
/*--------------------------------------------------------------------------*/
static void
signal_int(int signum)
{
    gRunning = 0;
}

/*--------------------------------------------------------------------------*/
/**
 * @brief   Device Input Controllers: For Joy Stick
 *          main routine
 *
 * @param   main() finction's standard parameter (argc,argv)
 * @return  result
 * @retval  0       success
 * @retval  1       failed
 */
/*--------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    struct epoll_event  ev_ret[16];
    char                *ictlDevName = "DrivingForceGT";
    int                 JSfd;
    int                 ii, jj;
    int                 ret;
    struct sigaction    sigint;

    /* get device name from parameter   */
    for (ii = 1; ii < argc; ii++) {
        if (strcasecmp( argv[ii], "-h") == 0) {
            PrintUsage(argv[0]);
            exit( 0 );
        }
        else if (strcasecmp( argv[ii], "-d") == 0) {
            /* debug    */
            mDebug = 1;
        }
        else if (strcasecmp( argv[ii], "-l") == 0) {
            /* event input log  */
            mEventLog = 1;
        }
        else {
            ictlDevName = argv[ii];
        }
    }

    /* change to daemon */
    if (! mDebug) {
        if (daemon(0, 1) < 0) {
            fprintf(stderr, "%s: Can not Create Daemon\n", argv[0]);
            exit(1);
        }
    }

    /* open joystick    */
    JSfd = ico_ictl_js_open(ictlDevName);
    if (JSfd < 0) {
        ERROR_PRINT("main: Leave(Error device open)");
        exit(1);
    }
    gIco_ICtrl_JS.fd = JSfd;

    /* read conf file   */
    char *confpath = getenv(ICO_ICTL_CONF_ENV);
    if (!confpath)  {
        confpath = ICO_ICTL_CONF_FILE;
    }
    ico_ictl_read_conf(confpath);

    /* initialize wayland   */
    ico_ictl_wayland_init(NULL, NULL);
    ico_ictl_add_fd(JSfd);

    /* send configuration informations to Multi Input Manager   */
    for (ii = 0; ii < nIco_ICtrl_JS_Input; ii++)    {
        ico_input_mgr_device_configure_input(
                gIco_ICtrl_Mng.Wayland_InputMgr, gIco_ICtrl_JS.device, gIco_ICtrl_JS.type,
                gIco_ICtrl_JS_Input[ii].name, gIco_ICtrl_JS_Input[ii].input,
                gIco_ICtrl_JS_Input[ii].code[0].name, gIco_ICtrl_JS_Input[ii].code[0].code);
        for (jj = 1; jj < 20; jj++)     {
            if (gIco_ICtrl_JS_Input[ii].code[jj].code == 0) break;
            ico_input_mgr_device_configure_code(
                    gIco_ICtrl_Mng.Wayland_InputMgr, gIco_ICtrl_JS.device,
                    gIco_ICtrl_JS_Input[ii].input, gIco_ICtrl_JS_Input[ii].code[jj].name,
                    gIco_ICtrl_JS_Input[ii].code[jj].code);
        }
    }

    /* signal init  */
    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);

    /* main loop    */
    while (gRunning) {
        ret = ico_ictl_wayland_iterate(ev_ret, 200);
        for (ii = 0; ii < ret; ii++) {
            if (ev_ret[ii].data.fd == JSfd) {
                ico_ictl_js_read(JSfd);
            }
        }
    }
    ico_ictl_wayland_finish();

    exit(0);
}

static void PrintUsage(const char *pName)
{
    fprintf( stderr, "Usage: %s [-h] [-d] DeviceName\n", pName );
    fprintf( stderr, "       ex)\n");
    fprintf( stderr, "          %s \"Driving Force GT\"\n", pName);
}

