#include "kernel.h"

int
setkb_main(int argc, char **argv)
{
	if ( argc > 2 )
	{
		cprintk(LIGHTRED, BLACK, "Cantidad de argumentos incorrecta\n");
		return 1;
	}
	if ( argc == 1 )
	{
		printk("Teclado actual: %s\n", mt_kbd_getlayout());
		printk( "Disponibles:\n");
		const char **p = mt_kbd_layouts();
		while ( *p )
			printk("\t%s\n", *p++);
		return 0;
	}
	if ( mt_kbd_setlayout(argv[1]) )
	{
		printk("Teclado actual: %s\n", argv[1]);
		return 0;
	}
	cprintk(LIGHTRED, BLACK, "Teclado %s desconocido\n", argv[1]);
	return 2;
}
