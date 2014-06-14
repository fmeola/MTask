#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "genlistADT.h"


/* Programa para probar las funciones de la lista genérica ordenada 
** sin repeticiones
**/

/* Funcion de comparacion de enteros para ordenar en forma descendente */
int
compInts(int *n, int *m)
{
	if ( *n == *m )
		return 0;

	if ( *n < *m )
		return 1;

	return -1;
}

/* Funcion de comparacion para strings */
int
compStrings(char *s, char *t)
{
	int res;
	res = strcmp(s, t);
	if (res > 0)
		res = 1;
	else if ( res < 0 )
		res = -1;
	return res;
}

int
main(void)
{
	listADT listInt;
	int n, m, *p;

	listInt    = NewList(sizeof(int), ( int (*) (void*, void*)) compInts);


	/* La lista creada debe estar vacía */
	assert(ListIsEmpty(listInt));
	assert(Size(listInt) == 0);

	n = 5;	
	assert(ElementBelongs(listInt, &n) == 0);

	/* Retorna 0 si no pudo borrar */
	assert(Delete(listInt,&n) == 0);

	/* Insertamos un elemento */
	Insert(listInt, &n);
	assert(ElementBelongs(listInt, &n) == 1);
	/* Para asegurarnos que inserte copias y no punteros, cambiamos el valor derecho de n,
        obviamente su valor izquierdo es el mismo */
	n = 8, m = 10;
	assert(ElementBelongs(listInt, &n) == 0);
	assert(Size(listInt) == 1);	
	assert(ElementBelongs(listInt, &m) == 0);
	/* Verificamos que la comparacion sea de valores y no de punteros */
	m = 5;
	assert(ElementBelongs(listInt, &m) == 1);


	/* No debe insertar repetidos */
	m = 5;
	Insert(listInt, &m);
	assert(ElementBelongs(listInt, &m));
	assert(ListIsEmpty(listInt) == 0);
	assert(Size(listInt) == 1);


	/* Borrar un único elemento deja la lista vacía */
	n = 5;
	assert(Delete(listInt, &n));
	assert(ElementBelongs(listInt, &n) == 0);
	assert(ListIsEmpty(listInt));
	assert(Size(listInt)==0);


	/* Debe insertar en forma ordenada */
	n=1;
	Insert(listInt, &n);
	n=10;
	Insert(listInt, &n);
	n=20;
	Insert(listInt, &n);
	n=15;
	Insert(listInt, &n);
	assert(Size(listInt) == 4);
	ToBegin(listInt);

	p = NextElement(listInt);
	assert(*p == 20);
	assert(*(int*)NextElement(listInt) == 15);
	assert(*(int*)NextElement(listInt) == 10);
	assert(*(int*)NextElement(listInt) == 1);
	assert(NextElement(listInt) == NULL);
	assert(Size(listInt) == 4);


	FreeList(listInt);

	printf("Verificado exitosamente\n");

	return 0;
	
}
