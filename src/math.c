#include "kernel.h"

#define CP_SIZE 108

// Manejador de la excepción 7
static void
math_handler(unsigned except_num, unsigned error, mt_regs_t *regs)
{
	// La tarea actual intenta ejecutar una instrucción de coprocesador
	// sin ser la dueña del mismo. Esto se produce porque está levantado
	// el bit TS en CR0.

	// Bajar el bit
	mt_clts();

	// Si alguien usó el coprocesador antes, guardar el estado. 
	// Si no, resetearlo.
	if ( mt_fpu_task )
	{
		if ( !mt_fpu_task->math_data )
			mt_fpu_task->math_data = Malloc(CP_SIZE);
		mt_fsave(mt_fpu_task->math_data);
	}
	else
		mt_finit();
	
	// Si tenemos un estado guardado, reponerlo
	if ( mt_curr_task->math_data )
		mt_frstor(mt_curr_task->math_data);

	// Ahora esta tarea es la dueña del coprocesador
	mt_fpu_task = mt_curr_task;
}

void
mt_setup_math(void)
{
	// Capturar la excepción 7, que se dispara cuando se ejecuta una
	// instrucción de coprocesador con el bit TS de CR0 levantado.
	mt_set_exception_handler(7, math_handler);
}

