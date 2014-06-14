#include "kernel.h"
#include "../include/time.h"
#include "../include/genlistADT.h"

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

static listADT alarms = NULL;
static int alarmCount = 0;

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

void set_alarm(time_t * tp) {
    Atomic();
    outb(0x70, 0x05);
    outb(0x71, tp->tm_hour);
    outb(0x70, 0x03);
    outb(0x71, tp->tm_min);
    outb(0x70, 0x01);
    outb(0x71, tp->tm_sec);
    outb(0x70, 0x0D);
    outb(0x71, tp->tm_mday);
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
        if(!strcmp(argv[1], "-reset")) {
            reset_time();
            return 0;
        }
        if(!strcmp(argv[1], "-set")) {
            if(argc == 8) {
                set_time_wrapper(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                    atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
                return 0;
            }
        }
        if(!strcmp(argv[1], "-f12")) {
            set_register_bit(0x0B, 1, 0);
            return 0;
        }
        if(!strcmp(argv[1], "-f24")) {
            set_register_bit(0x0B, 1, 1);
            return 0;
        }
        if(!strcmp(argv[1], "-alarm")) {
            if(argc == 6) {
                //void (*f) (unsiged);
                //f = alarm_handler;
                if(alarms == NULL)
                    alarms = NewList(sizeof(alarm), (int(*)(void *, void *)) compAlarm);
                time_t newAlarm;
                newAlarm.tm_hour = toBCD(atoi(argv[2]));
                newAlarm.tm_min = toBCD(atoi(argv[3]));
                newAlarm.tm_sec = toBCD(atoi(argv[4]));
                newAlarm.tm_mday = toBCD(atoi(argv[5]));
                alarm a;
                alarm * first;
                a.id = alarmCount++;
                a.name = "hola";
                a.date = newAlarm;
                Insert(alarms, &a);
                ToBegin(alarms);
                first = NextElement(alarms);
                set_register_bit(0x0B, 5, 1);
                set_alarm(&(first -> date));
                mt_enable_irq(8);
                mt_set_int_handler(8, alarm_handler);
                return 0;
            }
        }
        if(!strcmp(argv[1], "-l")) {
            alarm * a;
            if(alarms == NULL || ListIsEmpty(alarms)) {
                printk("No hay alarmas.\n");
                return 0;
            }
            ToBegin(alarms);
            while((a = NextElement(alarms)) != NULL)
                printk("%d - %s - %s\n", a->id, asctime(timeString, &(a->date)), a->name);
            return 0;
        }
        
    }
    read_time(&t);
    printk("%s\n", asctime(timeString, &t));
    return 0;
}

/* Funcion de comparacion de enteros para ordenar en forma descendente */
int compAlarm(alarm *a1, alarm *a2) {
    int diff;
    long a1secs = fromBCD((a1->date).tm_mday) * 86400 + 
        fromBCD((a1->date).tm_hour) * 3600 + 
        fromBCD((a1->date).tm_min) * 60 + 
        fromBCD((a1->date).tm_sec);
    long a2secs = fromBCD((a2->date).tm_mday) * 86400 +
        fromBCD((a2->date).tm_hour) * 3600 +
        fromBCD((a2->date).tm_min) * 60 +
        fromBCD((a2->date).tm_sec);
    diff = a1secs - a2secs;
    if(diff > 0)
        return 1;
    if(diff < 0)
        return -1;
    return 0;
}


int toBCD(int n) {
    return ((n / 10) << 4)|(n% 10);
}

int fromBCD(int n) {
    return ((n & 0xF0) >> 4) * 10 + (n & 0x0F);
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

void set_alarm_wrapper(int hour, int min, int sec, int day) {
    time_t t;
    t.tm_hour = toBCD(hour);
    t.tm_min = toBCD(min);
    t.tm_sec = toBCD(sec);
    t.tm_mday = toBCD(day);
    set_alarm(&t);
}

void alarm_handler(unsigned irq_number) {
    alarm * first;
    int register_c;
    outb(0x70, 0x0C);
    register_c = inb(0x71);
    printk("RING!\n");
    ToBegin(alarms);
    first = NextElement(alarms);
    printk("%d: %s\n", first->id, first->name);
    Delete(alarms, first);
    ToBegin(alarms);
    first = NextElement(alarms);
    if(first != NULL) {
        printk("Seteando RTC...\n");
        set_register_bit(0x0B, 5, 1);
        set_alarm(&(first -> date));
    }
    return;
}




