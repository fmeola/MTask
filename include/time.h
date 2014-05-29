#ifndef _time_h
#define _time_h

typedef struct {
    int tm_sec;         /* seconds */
    int tm_min;         /* minutes */
    int tm_hour;        /* hours */
    int tm_mday;        /* day of the month */
    int tm_mon;         /* month */
    int tm_year;        /* year */
    int tm_wday;        /* day of the week */
    int tm_yday;        /* day in the year */
    int tm_isdst;       /* daylight saving time */
} time_t;

/* Guarda el tiempo en la estructura */
void time(time_t * tp);

/* Convierte a una cadena de texto el tiempo */
char * asctime(char * str_time, const time_t * tp);

#endif
