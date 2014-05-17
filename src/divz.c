#include "kernel.h"

static void
divz_handler(unsigned except_num, unsigned error, mt_regs_t *regs)
{
	printk("Excepcion 0: intenta dividir por cero.\n");

	/* Regs apunta a los registros de divide() en el momento de producirse 
	 * la excepción. La división a / b se hace cargando a en el par edx:eax y
	 * ejecutando el equivalente de:
			idiv dword [ebp + 12]
	 * Para comprobar esto ejecutar "make s/divz.s" y ver el código en 
	 * assembler. Hay que poner un valor distinto de 0 en [ebp + 12].
	 */

	printk("Cambiamos el 0 por 1.\n");
	*(int *)(regs->ebp + 12) = 1;
}

static int
divide(int a, int b)
{
	return a / b;
}

int
divz_main(int argc, char **argv)
{
	if (argc != 3 )
	{
		cprintk(LIGHTRED, BLACK, "Divz requiere dos argumentos\n");
		return 1;
	}
	
	int a = atoi(argv[1]);
	int b = atoi(argv[2]);

	// Instalamos la excepción de división por cero
	mt_set_exception_handler(0, divz_handler);

	// Dividimos
	printk("%d / %d = %d\n", a, b, divide(a, b));

	// Desinstalamos la excepción
	mt_set_exception_handler(0, NULL);

	return 0;
}
