#include "kernel.h"
#include "../include/time.h"

/* Fuente consultada: http://wiki.osdev.org/RTC */

const char *_days[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

const char *_days_abbrev[] = {
  "Sun", "Mon", "Tue", "Wed", 
  "Thu", "Fri", "Sat"
};

const char *_months[] = {
  "January", "February", "March",
  "April", "May", "June",
  "July", "August", "September",
  "October", "November", "December"
};

const char *_months_abbrev[] = {
  "Jan", "Feb", "Mar",
  "Apr", "May", "Jun",
  "Jul", "Aug", "Sep",
  "Oct", "Nov", "Dec"
};


void read_time(time_t * tp) {
    Atomic();
    outb(0x70, 4);
    tp->tm_hour = inb(0x71);
    outb(0x70, 2);
    tp->tm_min = inb(0x71);
    outb(0x70, 0);
    tp->tm_sec = inb(0x71);
    outb(0x70, 6);
    tp->tm_wday = inb(0x71);
    outb(0x70, 7);
    tp->tm_mday = inb(0x71);
    outb(0x70, 8);
    tp->tm_mon = inb(0x71);
    outb(0x70, 9);
    tp->tm_year = inb(0x71);
    Unatomic();
    return;
}

void set_register_bit(int register_address, int bit, int set) {
    int r;
    Atomic();
    outb(0x70, register_address);
    r = inb(0x71);
    outb(0x70, register_address);
    if(set)
        outb(0x71, set_bit(r, bit));
    else
        outb(0x71, clear_bit(r, bit));
    Unatomic();
}

void set_time(time_t * tp) {
    Atomic();
    outb(0x70, 4);
    outb(0x71, tp->tm_hour);
    outb(0x70, 2);
    outb(0x71, tp->tm_min);
    outb(0x70, 0);
    outb(0x71, tp->tm_sec);
    outb(0x70, 6);
    outb(0x71, tp->tm_wday);
    outb(0x70, 7);
    outb(0x71, tp->tm_mday);
    outb(0x70, 8);
    outb(0x71, tp->tm_mon);
    outb(0x70, 9);
    outb(0x71, tp->tm_year);
    Unatomic();
}

void reset_time() {
    int i;
    Atomic();
    for(i = 0; i < 10; i++) {
        outb(0x70, i);
        outb(0x71, 0);
    }
    Unatomic();
}

void set_time_format(int fmt) {
    int register_b;
    Atomic();
    outb(0x70, 0x0B);
    register_b = inb(0x71);
    outb(0x70, 0x0B);
    if(fmt)
        outb(0x71, set_bit(register_b, 1));
    else
        outb(0x71, clear_bit(register_b, 1));
    Unatomic();
}

int set_bit(int byte, int bit) {
    return byte | pow(2, bit);
}

int clear_bit(int byte, int bit) {
    return byte & !pow(2, bit);
}

int pow(int b, int e) {
    int i;
    int res = 1;
    for(i = 0; i < e; i++)
        res *= b;
    return res;
}

char * asctime(char * str_time, const time_t * tp) {
    // Thu May 29 11:35:33 2014
    char time[9];
    int wday, mon;
    char day[3];
    char year[3];
    wday = ((tp->tm_wday & 0xF0) >> 4) * 10 + (tp->tm_wday & 0x0F);
    strcpy(str_time, _days_abbrev[wday]);
    strcat(str_time, " ");
    mon = ((tp->tm_mon & 0xF0) >> 4) * 10 + (tp->tm_mon & 0x0F);
    strcat(str_time, _months_abbrev[mon]);
    strcat(str_time, " ");
    day[0] = ((tp->tm_mday & 0xF0) >> 4) + '0';
    day[1] = ((tp->tm_mday & 0x0F)) + '0';
    day[2] = 0;
    strcat(str_time, day);
    strcat(str_time, " ");
    time[0] = ((tp->tm_hour & 0xF0) >> 4) + '0';
    time[1] = ((tp->tm_hour & 0x0F)) + '0';
    time[2] = ':';
    time[3] = ((tp->tm_min & 0xF0) >> 4) + '0';
    time[4] = ((tp->tm_min & 0x0F)) + '0';
    time[5] = ':';
    time[6] = ((tp->tm_sec & 0xF0) >> 4) + '0';
    time[7] = ((tp->tm_sec & 0x0F)) + '0';
    time[8] = 0;
    strcat(str_time, time);
    strcat(str_time, " ");
    year[0] = ((tp->tm_year & 0xF0) >> 4) + '0';
    year[1] = ((tp->tm_year & 0x0F)) + '0';
    year[2] = 0;
    strcat(str_time, "20");
    strcat(str_time, year);
    return str_time;
}

int time_main(int argc, char * argv[]) {
    char timeString[24];
    time_t t;
    if(argc > 1) {
        if(!strcmp(argv[1], "-reset"))
            reset_time();
        if(!strcmp(argv[1], "-set")) {
            if(argc == 8) {
                set_time_wrapper(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                    atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
                return;
            }
        }
    }
    read_time(&t);
    printk("%s\n", asctime(timeString, &t));
    return 0;
}

int toBCD(int n) {
    return ((n / 10) << 4)|(n% 10);
}

void set_time_wrapper(int hour, int min, int sec, int day, int mon, int year) {
    time_t t;
    t.tm_hour = toBCD(hour);
    t.tm_min = toBCD(min);
    t.tm_sec = toBCD(sec);
    t.tm_mday = toBCD(day);
    t.tm_mon = toBCD(mon - 1);
    t.tm_year = toBCD(year);
    t.tm_wday = 0; // TODO Arreglar dia de la semana.
    set_time(&t);
}




