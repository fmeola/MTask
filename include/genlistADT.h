/*
 * genListADT.h
 * Contrato del TAD Lista Lineal generica, cada elemento es void *
 * y se inserta una copia del mismo.
 * Simplemente encadenada ordenada, sin repeticiones
 * Se proveen funciones de iteracion para recorrer la lista
 */

#ifndef GENLISTADT_H_
#define GENLISTADT_H_

typedef void * listElementT;

/* El siguiente tipo define el TAD Lista.
 * Se lo define como puntero a "cierta estructura"
 * para ocultar la real representacion de la misma
 */
typedef struct listCDT *listADT;



/*
 * Descripcion:	Inicializa la lista con la funcion de comparacion
 *              que no puede ser NULL
 * -------------------------------------------------------------------
 * Precondicion:		-
 * Postcondicion: Lista creada
 */
listADT NewList(unsigned int bytes, int (*compare) (void*, void*));


/* Funcion:  		Insert
 * Uso:  		Insert( list, myElement);
 * -------------------------------------------------------------------
 * Descripci�n: 	Inserta una copia del elemento en la lista 
 *  			ordenada, usando la funci�n de comparaci�n.
 * 			Si el elemento ya exist�a, lo ignora.
 * 			Si hay problemas en la alocaci�n de memoria
 * 			finaliza la aplicaci�n
 * -------------------------------------------------------------------
 * Precondicion:	Lista que sea valida, previamente creada
 *			myElement != NULL
 * Postcondicion: 	Lista con el nuevo elemento,si no estaba repetido
 *			ni era NULL
 */
void Insert(listADT list, listElementT element);


/* Funcion:  		Delete
 * Uso:  		Delete(list, myElement );
 * -------------------------------------------------------------------
 * Descripci�n: 	Elimina el elemento de la lista ordenada,
 *  			usando la funci�n de comparacion. Si el elemento
 * 			no existia devuelve 0, caso contrario 1
 * -------------------------------------------------------------------
 * Precondicion:	Lista que sea valida, previamente creada
 * Postcondicion: 	Lista sin el elemento
 */
int Delete(listADT list, listElementT element);


/* Funcion:  		ListIsEmpty
 * Uso:  		if ( ListIsEmpty( list ) ...
 * -------------------------------------------------------------------
 * Descripci�n: 	Devuelve 1 si la lista est� vac�a o
 * 			0 en caso contrario
 * -------------------------------------------------------------------
 * Precondicion:	Lista que sea valida, previamente creada
 * Postcondicion: 	- (lista invariante).
 */
int ListIsEmpty(listADT list);




/* Funcion:  		ElementBelongs
 * Uso:  		if ( ElementBelongs( list, 12.3 ))...
 * -------------------------------------------------------------------
 * Descripci�n: 	Devuelve 1 si el elemento pertenece a la lista
 * 			o 0 en caso contrario
 * -------------------------------------------------------------------
 * Precondicion:	Lista previamente creada
 * Postcondicion: 	- (lista invariante).
 */
int ElementBelongs(listADT list, listElementT element);



/* Funcion:  		ToBegin
 * Uso:  		ToBegin(list)
 * -------------------------------------------------------------------
 * Descripci�n: 	Coloca el iterador en el primer elemento de la lista
 * -------------------------------------------------------------------
 * Precondicion:	Lista no vac�a
 * Postcondicion: 	- (lista invariante).
 */
void ToBegin(listADT list);


/* Funcion:  		NextElement
 * Uso:  		listADT list;
 *                      listElementT elem;
 * 			ToBegin(list)
 * 			while ( (elem = NextElement(list))  != NULL)
 * 			{
 * 				.....
 * 				.....
 * 			}
 * -------------------------------------------------------------------
 * Descripci�n: 	Retorna una copia del siguiente elemento del iterador
 * 			Si ya no hay m�s elementos, retorna NULL
 *			Si se la invoca despu�s de Insert o Delete puede retornar
 *                      direcciones inv�lidas (referencias a elementos eliminados, etc.)
 * -------------------------------------------------------------------
 * Precondicion:	Lista creada
 * Postcondicion: 	Iterador apunta al siguiente elemento
 */
listElementT NextElement(listADT list);


/* Funcion: 		Size
 * Uso: 	
 * 		listADT list;
 *		listElementT elem;
 *		n = Size( list );
 * 		ToBegin(list)
 *		for(i=1; i <= n; i++)
 * 		{
 * 			elem = NextElement(list);
 * 			.....
 * 		}
 * -------------------------------------------------------------------
 * Descripci�n: 	Retorna la cantidad de nodos que hay en la lista
 *                      Puede usarse para iterar sobre la misma
 * -------------------------------------------------------------------
 * Precondicion:	Lista que sea valida.
 * Postcondicion: 	-
 */
int Size(listADT list);



/* Funcion: 		FreeList
 * Uso: 			FreeList( list );
 * -------------------------------------------------------------------
 * Descripci�n: 	Destruye la lista, liberando recursos reservados.
 * -------------------------------------------------------------------
 * Precondicion:	Lista que sea valida.
 * Postcondicion: 	-
 */
void FreeList(listADT list);

#endif /* GENLISTADT_H_ */
