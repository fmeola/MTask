#include "kernel.h"

#define PASS_MAX 10

#define LEFTCAR			'<'
#define RIGHTCAR		'>'
#define EMPTY			' '
#define BARRIER			'\xBA'
#define SENSOR			'*'
#define NOTHING			' '

#define RIGHTCOL		80
#define SENSOR1_POS		9
#define BARRIER_POS		19
#define ONEWAY_START	20
#define ONEWAY_END		39
#define SENSOR2_POS		40
#define ROAD_END		59
#define ROAD_LEN		(ROAD_END+1)

#define ROAD_FG			BROWN
#define ROAD_BG			BROWN
#define BORDER_FG		BLACK
#define BORDER_BG		BLACK
#define CAR_FG			BLACK
#define CAR_BG			YELLOW
#define BARRIER_FG		LIGHTCYAN
#define SENSOR_FG		LIGHTGREEN

#define TDELAY			100

#define	BASELINE		11
#define MSGLINE			16
#define	OFFSET			((RIGHTCOL-ROAD_LEN)/2)

typedef enum { LEFTBOUND, RIGHTBOUND } DIRECTION;
typedef enum { IDLE, LEFTBOUND_BUSY, LEFTBOUND_DRAINING,
				RIGHTBOUND_BUSY, RIGHTBOUND_DRAINING } ROADSTATE;
typedef enum { LEFTBOUND_IN, LEFTBOUND_OUT, LEFTBOUND_BARRIER,
				RIGHTBOUND_IN, RIGHTBOUND_OUT, RIGHTBOUND_BARRIER } MESSAGE;

static unsigned char road[2][ROAD_LEN];
static unsigned ncars;
static unsigned pass_max;
static Task_t *ctl;

/* funciones de entrada-salida */

static void 
mputch(int x, int y, int c, unsigned fg, unsigned bg)
{
	Atomic();
	mt_cons_gotoxy(x, y);
	cprintk(fg, bg, "%c", c);
	Unatomic();
}

static unsigned
mgetch(void)
{
	unsigned c;

	while ( !mt_kbd_getch(&c) )
		;
	return c;
}

static int 
mprint(int x, int y, char *format, ...)
{
	int n;
	va_list args;

	Atomic();
	mt_cons_gotoxy(x, y);
	mt_cons_setattr(LIGHTCYAN, BLACK);
	va_start(args, format);
	n = vprintk(format, args);
	va_end(args);
	mt_cons_clreol();
	Unatomic();
	return n;
}

/* Funciones de manejo del camino */

static int
get_road(DIRECTION dir, int pos)
{
	return road[dir][pos];
}

static void
draw(DIRECTION dir, int pos, int value, int delta_y, unsigned fg, unsigned bg)
{
	int x, y;

	if ( dir == LEFTBOUND )
	{
		x = OFFSET + ROAD_END - pos;
		y = pos < ONEWAY_START || pos > ONEWAY_END ?
			BASELINE-delta_y : BASELINE;
	}
	else
	{
		x = OFFSET + pos;
		y = pos < ONEWAY_START || pos > ONEWAY_END ?
			BASELINE+delta_y : BASELINE;
	}
	mputch(x, y, value, fg, bg);
}

static void
set_road(DIRECTION dir, int pos, int value, unsigned fg, unsigned bg)
{
	road[dir][pos] = value;
	draw(dir, pos, value, 1, fg, bg);
}


static void
set_border(DIRECTION dir, int pos, unsigned value, unsigned fg, unsigned bg)
{
	draw(dir, pos, value, 2, fg, bg);
}

static void
open_barrier(DIRECTION dir)
{
	set_road(dir, BARRIER_POS, EMPTY, ROAD_FG, ROAD_BG);
	set_border(dir, BARRIER_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(dir, BARRIER_POS-1, BARRIER, BARRIER_FG, BORDER_BG);
}

static void
close_barrier(DIRECTION dir)
{
	Atomic();
	while ( get_road(dir, BARRIER_POS) != EMPTY )
		Yield();
	set_road(dir, BARRIER_POS, BARRIER, BARRIER_FG, ROAD_BG);
	Unatomic();
	set_border(dir, BARRIER_POS-1, NOTHING, BORDER_FG, BORDER_BG);
}

static void
init_road(void)
{
	int i;

	for ( i = 0 ; i < ROAD_LEN ; i++ )
	{
		set_road(LEFTBOUND, i, EMPTY, ROAD_FG, ROAD_BG);
		set_road(RIGHTBOUND, i, EMPTY, ROAD_FG, ROAD_BG);
	}
	set_border(LEFTBOUND,  SENSOR1_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(LEFTBOUND,  BARRIER_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(LEFTBOUND,  SENSOR2_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(RIGHTBOUND, SENSOR1_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(RIGHTBOUND, BARRIER_POS, SENSOR, SENSOR_FG, BORDER_BG);
	set_border(RIGHTBOUND, SENSOR2_POS, SENSOR, SENSOR_FG, BORDER_BG);
}

/* Procesos */

static void
control(void *arg)
{
	int leftbound_cars = 0, rightbound_cars = 0, in_road = 0, passed = 0;
	ROADSTATE state = IDLE;
	MESSAGE msg;
	unsigned size;

	close_barrier(LEFTBOUND);
	close_barrier(RIGHTBOUND);

	while ( true )
	{
		size = sizeof msg;
		Receive(NULL, &msg, &size);
		switch ( msg )
		{
			case LEFTBOUND_IN:
				++leftbound_cars;
				break;
			case RIGHTBOUND_IN:
				++rightbound_cars;
				break;
			case LEFTBOUND_BARRIER:
				--leftbound_cars;
				++in_road;
				if ( passed < pass_max )
					++passed;
				break;
			case RIGHTBOUND_BARRIER:
				--rightbound_cars;
				++in_road;
				if ( passed < pass_max )
					++passed;
				break;
			case LEFTBOUND_OUT:
			case RIGHTBOUND_OUT:
				--in_road;
				break;
		}
		switch ( state )
		{
			case IDLE:
				if ( leftbound_cars )
				{
					open_barrier(LEFTBOUND);
					passed = 0;
					state = LEFTBOUND_BUSY;
				}
				else if ( rightbound_cars )
				{
					open_barrier(RIGHTBOUND);
					passed = 0;
					state = RIGHTBOUND_BUSY;
				}
				break;
			case LEFTBOUND_BUSY:
				if ( !leftbound_cars && !rightbound_cars && !in_road )
				{
					close_barrier(LEFTBOUND);
					state = IDLE;
				}
				else if ( rightbound_cars && (!leftbound_cars ||
							passed == pass_max) )
				{
					close_barrier(LEFTBOUND);
					if ( in_road )
						state = LEFTBOUND_DRAINING;
					else
					{
						open_barrier(RIGHTBOUND);
						passed = 0;
						state = RIGHTBOUND_BUSY;
					}
				}
				break;
			case LEFTBOUND_DRAINING:
				if ( !in_road )
				{
					open_barrier(RIGHTBOUND);
					passed = 0;
					state = RIGHTBOUND_BUSY;
				}
				break;
			case RIGHTBOUND_BUSY:
				if ( !leftbound_cars && !rightbound_cars && !in_road )
				{
					close_barrier(RIGHTBOUND);
					state = IDLE;
				}
				else if ( leftbound_cars && (!rightbound_cars ||
							passed == pass_max) )
				{
					close_barrier(RIGHTBOUND);
					if ( in_road )
						state = RIGHTBOUND_DRAINING;
					else
					{
						open_barrier(LEFTBOUND);
						passed = 0;
						state = LEFTBOUND_BUSY;
					}
				}
				break;
			case RIGHTBOUND_DRAINING:
				if ( !in_road )
				{
					open_barrier(LEFTBOUND);
					passed = 0;
					state = LEFTBOUND_BUSY;
				}
				break;
		}
	}
}

static void
car(void *arg)
{
	DIRECTION *dir = arg;
	int pos;
	int carsymbol = *dir == LEFTBOUND ? LEFTCAR : RIGHTCAR;
	MESSAGE car_in = *dir == LEFTBOUND ? LEFTBOUND_IN : RIGHTBOUND_IN; 
	MESSAGE car_barrier = *dir == LEFTBOUND ?
							LEFTBOUND_BARRIER : RIGHTBOUND_BARRIER; 
	MESSAGE car_out = *dir == LEFTBOUND ? LEFTBOUND_OUT : RIGHTBOUND_OUT; 

	++ncars; // suponemos que es atómico
	for ( pos = 0 ; pos < ROAD_LEN ; pos++ )
	{
		Atomic();
		while ( get_road(*dir, pos) != EMPTY )
			Yield();
		if ( pos < ROAD_END && get_road(*dir, pos+1) != EMPTY )
			Delay(TDELAY);
		while ( get_road(*dir, pos) != EMPTY )
			Yield();
		set_road(*dir, pos, carsymbol, CAR_FG, CAR_BG);
		if ( pos == SENSOR1_POS )
			Send(ctl, &car_in, sizeof car_in);
		else if ( pos == BARRIER_POS )
			Send(ctl, &car_barrier, sizeof car_barrier);
		else if ( pos == SENSOR2_POS )
			Send(ctl, &car_out, sizeof car_out);
		if ( pos )
			set_road(*dir, pos-1, EMPTY, ROAD_FG, ROAD_BG);
		Unatomic();
		Delay(TDELAY);
	}
	set_road(*dir, ROAD_END, EMPTY, ROAD_FG, ROAD_BG);
	--ncars;
}

static void
send_car(DIRECTION dir)
{
	static DIRECTION left = LEFTBOUND, right = RIGHTBOUND;

	Ready(CreateTask(car, 0, dir == LEFTBOUND ? &left : &right, 
			"car", DEFAULT_PRIO));
}

int
camino_ns_main(int argc, char **argv)
{
	bool cursor;
	unsigned c;

	ncars = 0;
	pass_max = argc > 1 ? atoi(argv[1]): PASS_MAX;

	mt_cons_clear();
	cursor = mt_cons_cursor(false);

	init_road();

	Ready(ctl = CreateTask(control, 0, NULL, "control", DEFAULT_PRIO));

	mprint(OFFSET, MSGLINE,		"I: auto hacia la izquierda");
	mprint(OFFSET, MSGLINE+1,	"D: auto hacia la derecha");
	mprint(OFFSET, MSGLINE+2,	"S: salir");

	do
		switch ( c = mgetch() )
		{
			case 'I':
			case 'i':
				send_car(LEFTBOUND);
				break;
			case 'D':
			case 'd':
				send_car(RIGHTBOUND);
				break;
			default:
				break;
		}
	while ( c != 's' && c != 'S' ); 

	mprint(OFFSET, MSGLINE + 3, "Esperando que terminen los autos...");
	while ( ncars )
		Yield();

	DeleteTask(ctl);

	mt_cons_clear();
	mt_cons_cursor(cursor);

	return 0;
}
