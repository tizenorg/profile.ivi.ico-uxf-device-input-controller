/*
 * Copyright (c) 2013, TOYOTA MOTOR CORPORATION.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
/**
 * @brief   Current date & time string for log output
 *
 * @date    Feb-08-2013
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

const char *dbg_curtime(void);

/*--------------------------------------------------------------------------*/
/**
 * @brief   dbg_curtime: Current time for Debug Write
 *
 * @param   None
 * @return  String pointer for current time
 */
/*--------------------------------------------------------------------------*/
const char *
dbg_curtime(void)
{
    struct timeval  NowTime;            /* Current date & time              */
    static int      NowZone = (99*60*60);/* Local time                      */
    extern long     timezone;           /* System time zone                 */
    static char     sBuf[28];

    if (NowZone > (24*60*60))  {
        tzset();
        NowZone = timezone;
    }
    gettimeofday(&NowTime, (struct timezone *)0);
    NowTime.tv_sec -= NowZone;

    sprintf(sBuf, "[%02d:%02d:%02d.%03d]",
            (int)((NowTime.tv_sec/3600) % 24),
            (int)((NowTime.tv_sec/60) % 60),
            (int)(NowTime.tv_sec % 60),
            (int)NowTime.tv_usec/1000);

    return(sBuf);
}

