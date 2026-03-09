/* fake time.h for myos TCC port */
#ifndef _FAKE_TIME_H
#define _FAKE_TIME_H

typedef long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

static inline time_t time(time_t *t) {
    if (t) *t = 0;
    return 0;
}

static inline struct tm *localtime(const time_t *t) {
    (void)t;
    static struct tm _tm = {0};
    return &_tm;
}

#endif