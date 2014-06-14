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

typedef struct {
	int id;
	char * name;
	time_t date;
} alarm;

/* Guarda el tiempo en la estructura */
void time(time_t * tp);

/* Convierte a una cadena de texto el tiempo */
char * asctime(char * str_time, const time_t * tp, int fmt);

/* Alarm Handler */
void alarm_handler(unsigned irq_number);

void set_alarm_wrapper(int hour, int min, int sec, int day);
void set_time_wrapper(int hour, int min, int sec, int day, int mon, int year);
int toBCD(int n);
int time_main(int argc, char * argv[]);
int pow(int b, int e);
int clear_bit(int byte, int bit);
int set_bit(int byte, int bit);
void set_time_format(int fmt);
void reset_time();
void set_alarm(time_t * tp);
void set_time(time_t * tp);
void set_register_bit(int register_address, int bit, int set);
void read_time(time_t * tp);

int compAlarm(alarm *n, alarm *m);
int fromBCD(int n) ;



#endif
