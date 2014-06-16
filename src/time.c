#include "kernel.h"
#include "../include/time.h"
#include "../include/genlistADT.h"
#include "apps.h"

#define DEFAULT_HUSO 1

/* Fuente consultada: http://wiki.osdev.org/RTC */

static struct cmdentry
{
    char *name;
    int (*func)(int argc, char **argv);
}
cmdtab[] =
{
    {   "setkb",        setkb_main },
    {   "shell",        shell_main },
    {   "sfilo",        simple_phil_main },
    {   "filo",         phil_main },
    {   "xfilo",        extra_phil_main },
    {   "afilo",        atomic_phil_main },
    {   "camino",       camino_main },
    {   "camino_ns",    camino_ns_main },
    {   "prodcons",     prodcons_main },
    {   "divz",         divz_main },
    {   "date",         time_main },
    { }
};

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

const char * map[] = {
    ".. . . . . . . . . . . . . . . . . . . . . . . . . . . . .\n",
    ".. . . . . . . .#######. . . . . . . . . . . . . . . . . .\n",
    ".. . . . . . .#. .#### . . . ####. . .###############. . .\n",
    ".. . ########. ##. ##. . . #####9################### . . .\n",
    ".. . . ##########. . . . 6#####################. . . . . .\n",
    ".. . . .4######5 . . . .   ################### . . . . . .\n",
    ".. . . . ### .   . . . .#####. ##############. 8 . . . . .\n",
    ".. . . . . 3#### . . . .#######. ##########. . . . . . . .\n",
    ".. . . . . .###### . . . .#### . . . . .## . . . . . . . .\n",
    ".. . . . . . ####2 . . . .#### # . . . . . ##### . . . . .\n",
    ".. . . . . . ##1 . . . . . ##. . . . . . . . ##7 .#. . . .\n",
    ".. . . . . . ##. . . . . . . . . . . . . . . . . . . . . .\n",
    ".. . . . . . . . . . . . . . . . . . . . . . . . . . . . .\n"
};

typedef struct
{
    int id;
    char * city;
    char * code;
    int diff;
} huso;

const huso husos[] =
{
    { 1, "Buenos Aires", "ART",  -3 },
    { 2, "Rio de Janeiro", "BRT", -3 },
    { 3, "Mexico DF", "CDT",-6 },
    { 4, "San Francisco", "PDT",-8 },
    { 5, "Washington DC", "EDT",-5 },
    { 6, "Londres", "BST",0 },
    { 7, "Sidney", "MDT",+10 },
    { 8, "Tokio", "JST",+9 },
    { 9, "Moscu", "MSK",+4 }
};

static bool fmt12 = false;
static int currentHuso = 1;
static listADT alarms = NULL;
static int alarmCount = 0;

void printHusos(void) {
    int i;
    for(i = 0; i < sizeof(map)/sizeof(map[0]); i++)
        printk("%s", map[i]);
    for(i = 0; i < sizeof(husos)/sizeof(huso); i++)
        printk("%d: %s\n", husos[i].id, husos[i].city);
}

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

char * asctime(char * str_time, const time_t * tp, int fmt) {
    // Thu May 29 11:35:33 ART 2014
    char time[9];
    int wday, mon;
    char day[3];
    char year[3];
    if(fmt) {
        wday = ((tp->tm_wday & 0xF0) >> 4) * 10 + (tp->tm_wday & 0x0F);
        strcpy(str_time, _days_abbrev[wday]);
        strcat(str_time, " ");
        mon = ((tp->tm_mon & 0xF0) >> 4) * 10 + (tp->tm_mon & 0x0F);
        strcat(str_time, _months_abbrev[mon]);
        strcat(str_time, " ");
    }
    day[0] = ((tp->tm_mday & 0xF0) >> 4) + '0';
    day[1] = ((tp->tm_mday & 0x0F)) + '0';
    day[2] = 0;
    strcat(str_time, day);
    strcat(str_time, " ");   
    int d = ((tp->tm_hour & 0xF0) >> 4);
    int u = ((tp->tm_hour & 0x0F));
    int h = d * 10 + u;
    h = h - husos[DEFAULT_HUSO - 1].diff + husos[currentHuso - 1].diff ;
    if(fmt12 && h >= 12)
        h = h - 12;
    time[0] = h/10 + '0';
    time[1] = h%10 + '0';
    time[2] = ':';
    time[3] = ((tp->tm_min & 0xF0) >> 4) + '0';
    time[4] = ((tp->tm_min & 0x0F)) + '0';
    time[5] = ':';
    time[6] = ((tp->tm_sec & 0xF0) >> 4) + '0';
    time[7] = ((tp->tm_sec & 0x0F)) + '0';
    time[8] = 0;
    strcat(str_time, time);
    strcat(str_time, " ");
    strcat(str_time, husos[currentHuso - 1].code);
    strcat(str_time, " ");
    if(fmt) {
        year[0] = ((tp->tm_year & 0xF0) >> 4) + '0';
        year[1] = ((tp->tm_year & 0x0F)) + '0';
        year[2] = 0;
        strcat(str_time, "20");
        strcat(str_time, year);
    }
    return str_time;
}

int time_main(int argc, char * argv[]) {
    char timeString[24] = {0};
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
            fmt12 = true;
        }
        if(!strcmp(argv[1], "-f24")) {
            fmt12 = false;
        }
        if(!strcmp(argv[1], "-alarm")) {
            if(argc == 7) {
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
                a.name = argv[6];
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
                cprintk(LIGHTRED, BLACK, "No hay alarmas.\n");
                return 0;
            }
            ToBegin(alarms);
            while((a = NextElement(alarms)) != NULL) {
				char auxString[24] = {0};
                printk("%s @ %s\n", a->name, asctime(auxString, &(a->date), 0));
			}
            return 0;
        }
        if(!strcmp(argv[1], "-gmt")) {
            char c;
            printHusos();
            printk("\nIngrese el codigo: ");
            mt_kbd_getch(&c);
            printk("%c\n", c);
            if(c - '0' >= 1 && c - '0' <= 9)
                currentHuso = c - '0';
            else
                printk("Codigo Invalido.\n");
        }
		if(!strcmp(argv[1], "-help")) {
			printk("Ayuda del comando date\n");
			printk("-gmt Cambia la hora a un huso horario del mapa.\n");
			printk("-f12 Cambia el formato a 12 horas.\n");
			printk("-f24 Cambia el formato a 24 horas.\n");
			printk("-l Lista las proximas alarmas agendadas.\n");
			printk("-alarm [hh] [mm] [ss] [dd] [cmd]\n\t Agenda una alarma para lanzar el comando [cmd] a las hh:mm:ss del dia [dd] del mes actual.\n");
			printk("-set [hh] [mm] [ss] [dd] [mm] [yy]\n\t Cambia la fecha y hora actual con yy desde el 2000.\n");
			printk("-reset Setea la fecha y hora actual a las 00:00:00 1/1/2000.\n");
			return 0;
		}
    }
    read_time(&t);
    printk("%s\n", asctime(timeString, &t, 1));
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
    t.tm_wday = 0; // TODO Dia de la semana.
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
    struct cmdentry *cp;
    alarm * first;
    outb(0x70, 0x0C);
    printk("RING!\n");
    ToBegin(alarms);
    first = NextElement(alarms);
    printk("%d: %s\n", first->id, first->name);
    /* aplicaciones */
    bool found = false;
    for ( cp = cmdtab ; cp->name ; cp++ )
        if ( strcmp(first->name, cp->name) == 0 )
        {
            found = true;
            int n = cp->func(1, NULL);
            if ( n != 0 )
                cprintk(LIGHTRED, BLACK, "Status: %d\n", n);
            break;
        }
    if ( !found )
        cprintk(LIGHTRED, BLACK, "Comando %s desconocido\n", first->name);
    Delete(alarms, first);
    ToBegin(alarms);
    first = NextElement(alarms);
    if(first != NULL) {
        set_register_bit(0x0B, 5, 1); /* Habilito interrupcion de alarma. */
        set_alarm(&(first -> date));
    }
}
