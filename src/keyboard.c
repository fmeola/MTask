// Copiado en buena parte del driver de teclado de Minix, especialmente
// todo lo relativo al mapeado de teclas. Usamos los mapas de Minix.

// Nota del original de Minix (keyboard.c):

/* Keyboard driver for PC's and AT's.
 *
 * Changes: 
 *   Jul 13, 2004   processes can observe function keys  (Jorrit N. Herder)
 *   Jun 15, 2004   removed wreboot(), except panic dumps (Jorrit N. Herder)
 *   Feb 04, 1994   loadable keymaps  (Marcus Hampel)
 */

#include "kernel.h"

// Definiciones para el mapa de teclas (de Minix: keymap.h).

#define C(c)    ((c) & 0x1F)    /* Map to control code          */
#define A(c)    ((c) | 0x80)    /* Set eight bit (ALT)          */
#define CA(c)   A(C(c))         /* Control-Alt                  */
#define L(c)    ((c) | HASCAPS) /* Add "Caps Lock has effect" attribute */

#define EXT     0x0100          /* Normal function keys         */
#define CTRL    0x0200          /* Control key                  */
#define SHIFT   0x0400          /* Shift key                    */
#define ALT     0x0800          /* Alternate key                */
#define EXTKEY  0x1000          /* extended keycode             */
#define HASCAPS 0x8000          /* Caps Lock has effect         */

/* Scan code conversion. */
#define KEY_RELEASE     0200
#define ASCII_MASK      0177

/* Numeric keypad */
#define HOME    (0x01 + EXT)
#define END     (0x02 + EXT)
#define UP      (0x03 + EXT)
#define DOWN    (0x04 + EXT)
#define LEFT    (0x05 + EXT)
#define RIGHT   (0x06 + EXT)
#define PGUP    (0x07 + EXT)
#define PGDN    (0x08 + EXT)
#define MID     (0x09 + EXT)
#define NMIN    (0x0A + EXT)
#define PLUS    (0x0B + EXT)
#define INSRT   (0x0C + EXT)

/* Alt + Numeric keypad */
#define AHOME   (0x01 + ALT)
#define AEND    (0x02 + ALT)
#define AUP     (0x03 + ALT)
#define ADOWN   (0x04 + ALT)
#define ALEFT   (0x05 + ALT)
#define ARIGHT  (0x06 + ALT)
#define APGUP   (0x07 + ALT)
#define APGDN   (0x08 + ALT)
#define AMID    (0x09 + ALT)
#define ANMIN   (0x0A + ALT)
#define APLUS   (0x0B + ALT)
#define AINSRT  (0x0C + ALT)

/* Ctrl + Numeric keypad */
#define CHOME   (0x01 + CTRL)
#define CEND    (0x02 + CTRL)
#define CUP     (0x03 + CTRL)
#define CDOWN   (0x04 + CTRL)
#define CLEFT   (0x05 + CTRL)
#define CRIGHT  (0x06 + CTRL)
#define CPGUP   (0x07 + CTRL)
#define CPGDN   (0x08 + CTRL)
#define CMID    (0x09 + CTRL)
#define CNMIN   (0x0A + CTRL)
#define CPLUS   (0x0B + CTRL)
#define CINSRT  (0x0C + CTRL)

/* Lock keys */
#define CALOCK  (0x0D + EXT)    /* caps lock    */
#define NLOCK   (0x0E + EXT)    /* number lock  */
#define SLOCK   (0x0F + EXT)    /* scroll lock  */

/* Function keys */
#define F1      (0x10 + EXT)
#define F2      (0x11 + EXT)
#define F3      (0x12 + EXT)
#define F4      (0x13 + EXT)
#define F5      (0x14 + EXT)
#define F6      (0x15 + EXT)
#define F7      (0x16 + EXT)
#define F8      (0x17 + EXT)
#define F9      (0x18 + EXT)
#define F10     (0x19 + EXT)
#define F11     (0x1A + EXT)
#define F12     (0x1B + EXT)

/* Alt+Fn */
#define AF1     (0x10 + ALT)
#define AF2     (0x11 + ALT)
#define AF3     (0x12 + ALT)
#define AF4     (0x13 + ALT)
#define AF5     (0x14 + ALT)
#define AF6     (0x15 + ALT)
#define AF7     (0x16 + ALT)
#define AF8     (0x17 + ALT)
#define AF9     (0x18 + ALT)
#define AF10    (0x19 + ALT)
#define AF11    (0x1A + ALT)
#define AF12    (0x1B + ALT)

/* Ctrl+Fn */
#define CF1     (0x10 + CTRL)
#define CF2     (0x11 + CTRL)
#define CF3     (0x12 + CTRL)
#define CF4     (0x13 + CTRL)
#define CF5     (0x14 + CTRL)
#define CF6     (0x15 + CTRL)
#define CF7     (0x16 + CTRL)
#define CF8     (0x17 + CTRL)
#define CF9     (0x18 + CTRL)
#define CF10    (0x19 + CTRL)
#define CF11    (0x1A + CTRL)
#define CF12    (0x1B + CTRL)

/* Shift+Fn */
#define SF1     (0x10 + SHIFT)
#define SF2     (0x11 + SHIFT)
#define SF3     (0x12 + SHIFT)
#define SF4     (0x13 + SHIFT)
#define SF5     (0x14 + SHIFT)
#define SF6     (0x15 + SHIFT)
#define SF7     (0x16 + SHIFT)
#define SF8     (0x17 + SHIFT)
#define SF9     (0x18 + SHIFT)
#define SF10    (0x19 + SHIFT)
#define SF11    (0x1A + SHIFT)
#define SF12    (0x1B + SHIFT)

/* Alt+Shift+Fn */
#define ASF1    (0x10 + ALT + SHIFT)
#define ASF2    (0x11 + ALT + SHIFT)
#define ASF3    (0x12 + ALT + SHIFT)
#define ASF4    (0x13 + ALT + SHIFT)
#define ASF5    (0x14 + ALT + SHIFT)
#define ASF6    (0x15 + ALT + SHIFT)
#define ASF7    (0x16 + ALT + SHIFT)
#define ASF8    (0x17 + ALT + SHIFT)
#define ASF9    (0x18 + ALT + SHIFT)
#define ASF10   (0x19 + ALT + SHIFT)
#define ASF11   (0x1A + ALT + SHIFT)
#define ASF12   (0x1B + ALT + SHIFT)

#define MAP_COLS        6       /* Number of columns in keymap */
#define NR_SCAN_CODES   0x80    /* Number of scan codes (rows in keymap) */

// Mapas de teclas

#define keymap keymap_us_std
#include "keymaps/us-std.src"
#undef keymap

#define keymap keymap_spanish
#include "keymaps/spanish.src"
#undef keymap

static const char *names[] =
{
	"us-std",
	"spanish",
	NULL
};

static unsigned short *keymaps[] =
{
	keymap_us_std,
	keymap_spanish
};

static const char *kbd_name;
static unsigned short *keymap;

/* Miscellaneous. */
#define ESC_SCAN        0x01	/* ESC */
#define SLASH_SCAN      0x35	/* numeric / */
#define RSHIFT_SCAN     0x36	/* right SHIFT */
#define HOME_SCAN       0x47	/* HOME */
#define INS_SCAN        0x52	/* INS */
#define DEL_SCAN        0x53	/* DEL */

#define NONE			-1U		/* no key */
#define ESC				0x1B	/* ESC */

static bool esc;				/* escape scan code detected? */
static bool alt_l;				/* left alt key state */
static bool alt_r;				/* right alt key state */
static bool alt;				/* either alt key */
static bool ctrl_l;				/* left control key state */
static bool ctrl_r;				/* right control key state */
static bool ctrl;				/* either control key */
static bool shift_l;			/* left shift key state */
static bool shift_r;			/* right shift key state */
static bool shift;				/* either shift key */
static bool num_down;			/* num lock key depressed */
static bool caps_down;			/* caps lock key depressed */
static bool scroll_down;		/* scroll lock key depressed */
static unsigned locks;			/* per console lock keys state */

/* Lock key active bits.  Chosen to be equal to the keyboard LED bits. */
#define SCROLL_LOCK     0x01
#define NUM_LOCK        0x02
#define CAPS_LOCK       0x04

static char numpad_map[] = { 'H', 'Y', 'A', 'B', 'D', 'C', 'V', 'U', 'G', 'S', 'T', '@' };

// Definiciones del controlador de teclado
#define KBD			0x60
#define KBDCTL		0x64
#define KBDOBF		2
#define KBDINT		1
#define KBDBUFSIZE	32

// Proceso de entrada de teclas
#define INPUTPRIO	10000		// Alta prioridad, para que funcione como "bottom half"

static MsgQueue_t *scan_mq, *key_mq;

static void 
kbdint(unsigned irq)
{
	// Interrupción de teclado. Por ahora tomamos todo lo que viene como si fueran
	// makes y breaks de teclas, cuando se envíen comandos al teclado (por ejemplo
	// para prender y apagar los LEDs), habrá que impedir que entren aquí las respuestas
	// o procesarlas por separado.

	unsigned c = inb(KBD);
	PutMsgQueueCond(scan_mq, &c);
}

#if 0
static void 
kbd_send(unsigned data)
{
    while ( inb(KBDCTL) & KBDOBF )
		Yield();
    outb(KBD, data);
}

static void 
kbd_send_ctl(unsigned data)
{
    while ( inb(KBDCTL) & KBDOBF )
		Yield();
    outb(KBDCTL, data);
}
#endif

static void
set_leds()
{
	// TODO. De todos modos no funciona en VirtualBox.
}

static unsigned
map_key(unsigned scode)
{
	/* Map a scan code to an ASCII code. */

	bool caps;
	unsigned column;
	unsigned short *keyrow;

	if (scode == SLASH_SCAN && esc)
		return '/';				/* don't map numeric slash */

	keyrow = &keymap[scode * MAP_COLS];

	caps = shift;
	if ((locks & NUM_LOCK) && HOME_SCAN <= scode && scode <= DEL_SCAN)
		caps = !caps;
	if ((locks & CAPS_LOCK) && (keyrow[0] & HASCAPS))
		caps = !caps;

	if (alt)
	{
		column = 2;
		if (ctrl || alt_r)
			column = 3;			/* Ctrl + Alt == AltGr */
		if (caps)
			column = 4;
	}
	else
	{
		column = 0;
		if (caps)
			column = 1;
		if (ctrl)
			column = 5;
	}
	return keyrow[column] & ~HASCAPS;
}

static unsigned
make_break(unsigned scode)
{
	/* This routine can handle keyboards that interrupt only on key depression,
	 * as well as keyboards that interrupt on key depression and key release.
	 * For efficiency, the interrupt routine filters out most key releases.
	 */

	unsigned ch;
	bool make, escape;

	/* High-order bit set on key release. */
	make = (scode & KEY_RELEASE) == 0;	/* true if pressed */

	ch = map_key(scode &= ASCII_MASK);	/* map to ASCII */

	escape = esc;				/* Key is escaped?  (true if added since the XT) */
	esc = false;

	switch (ch)
	{
		case CTRL:				/* Left or right control key */
			*(escape ? &ctrl_r : &ctrl_l) = make;
			ctrl = ctrl_l | ctrl_r;
			break;
		case SHIFT:			/* Left or right shift key */
			*(scode == RSHIFT_SCAN ? &shift_r : &shift_l) = make;
			shift = shift_l | shift_r;
			break;
		case ALT:				/* Left or right alt key */
			*(escape ? &alt_r : &alt_l) = make;
			alt = alt_l | alt_r;
			break;
		case CALOCK:			/* Caps lock - toggle on 0 -> 1 transition */
			if (!caps_down && make)
			{
				locks ^= CAPS_LOCK;
				set_leds();
			}
			caps_down = make;
			break;
		case NLOCK:			/* Num lock */
			if (!num_down && make)
			{
				locks ^= NUM_LOCK;
				set_leds();
			}
			num_down = make;
			break;
		case SLOCK:			/* Scroll lock */
			if (!scroll_down && make)
			{
				locks ^= SCROLL_LOCK;
				set_leds();
			}
			scroll_down = make;
			break;
		case EXTKEY:			/* Escape keycode */
			esc = true;			/* Next key is escaped */
			return NONE;
		default:				/* A normal key */
			return make ? ch : NONE;
	}

	return NONE;
}

static void
input_task(void *arg)
{
	unsigned char scode;
	unsigned ch;

	while (true)
	{
		if ( !GetMsgQueue(scan_mq, &scode) )
			continue;

		/* Perform make/break processing. */
		if ( (ch = make_break(scode)) == NONE )
			continue;

		if (1 <= ch && ch <= 0xFF)
			/* A normal character. */
			PutMsgQueue(key_mq, &ch);
		else if (HOME <= ch && ch <= INSRT)
		{
			/* An ASCII escape sequence generated by the numeric pad. */
			unsigned c = ESC;
			PutMsgQueue(key_mq, &c);
			c = '[';
			PutMsgQueue(key_mq, &c);
			c = numpad_map[ch - HOME];
			PutMsgQueue(key_mq, &c);
		}
		else
		{
			// Aquí deberían procesarse teclas especiales de procesamiento
			// inmediato como Fn, Alt-Fn, etc.
		}
	}
}

// Interfaz

bool 
mt_kbd_getch_timed(unsigned *c, unsigned timeout)
{
	*c = 0;
	return GetMsgQueueTimed(key_mq, c, timeout);
}

bool 
mt_kbd_getch(unsigned *c)
{
	*c = 0;
	return GetMsgQueue(key_mq, c);
}

void
mt_kbd_init(void)
{
	keymap = keymaps[0];
	kbd_name = names[0];
	scan_mq = CreateMsgQueue("Scan code", KBDBUFSIZE, 1, false, false);
	key_mq = CreateMsgQueue("Input key", KBDBUFSIZE, 1, true, false);
	Ready(CreateTask(input_task, 0, NULL, "Input task", INPUTPRIO));
	mt_set_int_handler(KBDINT, kbdint);
	mt_enable_irq(KBDINT);
}

const char *
mt_kbd_getlayout(void)
{
	return kbd_name;
}

bool
mt_kbd_setlayout(const char *name)
{
	unsigned n;
	const char *p;

	for ( n = 0 ; (p = names[n]) ; n++ )
		if ( strcmp(name, p) == 0 )
		{
			kbd_name = p;
			keymap = keymaps[n];
			return true;
		}
	return false;
}

const char **
mt_kbd_layouts(void)
{
	return names;
}
