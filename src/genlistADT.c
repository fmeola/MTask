/*
 * genlistADT.c
 *
 */

/* Version Iterativa de listas con header */

#include "kernel.h"
#include "genlistADT.h"


struct listCDT
{
	int (*fComp) (void *, void *);	/* funcion de comparacion */
	struct listNode * first;	/* puntero al primer nodo de la lista */
	struct listNode * actual;	/* puntero al actual, para iterar */
        unsigned int bytes;             /* cantidad de bytes que ocupa cada elemento */
	unsigned int size;		/* cantidad de nodos */
};

typedef struct listNode
{
	listElementT data;
	struct listNode * next;
}listNode;


// static void
// Error(const char* s)
// {
// 	printk("%s", s);
// }


listADT
NewList( unsigned int bytes, int (*fComp) (void *, void *) )
{
	listADT l = Malloc(sizeof( struct listCDT));
	if ( l == NULL)
		return NULL;
	l->fComp = fComp;
	l->first = NULL;
	l->actual = NULL;
        l->bytes  = bytes;
	l->size = 0;
}


int
ListIsEmpty( listADT list)
{
	return list->size == 0;
	/* 
	return list->first == NULL
	*/
}


void
ToBegin( listADT list )
{
	list->actual = list->first;
}

listElementT
NextElement( listADT list)
{
	listElementT data;

	if ( list->actual == NULL )
		return NULL;

	data = Malloc(list->bytes);
	if (data == NULL)
		return NULL;

	memcpy(data, list->actual->data, list->bytes);

	list->actual = list->actual->next;
	
	return data;
}




int
ElementBelongs( listADT list, listElementT element)
{
	listNode * node;
	int cmp;
	node = list->first;

	/* Como no sabemos la complejidad de la funcion de comparacion, conservamos 
	** el resultado en una variable.
	*/
	while( node != NULL &&   (cmp = (*list->fComp) (node->data, element)) == -1)
		node = node->next;

	return node != NULL && cmp == 0;
}


/* Funcion auxiliar para insertar en la lista de nodos, en forma analoga
** a insertar en una lista lineal sin header
*/
static int
InsertNode( listNode ** list, listElementT element, int (*f)(void *, void*), unsigned int bytes)
{

	/* Inserto al final o delante del actual porque es mayor */
	if( *list == NULL || (*f)((*list)->data, element) == 1 )
	{
		listNode * auxi = Malloc(sizeof( listNode ));
		if (auxi == NULL) {
			Panic("No hay lugar para otro nodo\n");
		}
		auxi->next = *list;
		auxi->data = Malloc(bytes);
         	if (auxi->data == NULL) {
			free(auxi);
			return 0;
		}
		memcpy(auxi->data, element, bytes);
		*list = auxi;
		return 1;
	}


	/* Si no es vacia ni mayor verificar si es igual (no se insertan repetidos) */
	if( (*f)((*list)->data, element) == 0 )
		return 0;

	/* El elemento actual es menor */
	return InsertNode( &((*list)->next), element, f, bytes);


}


void
Insert( listADT list, listElementT element)
{
	if (element == NULL)
		return;

	list->size += InsertNode(&(list->first), element, list->fComp, list->bytes);

}


static int
DeleteNode( listNode **list, listElementT element, int (*f)(void *, void*))
{
	if( *list == NULL || (*f)((*list)->data, element) == 1 )
		return 0;

	if( (*f)((*list)->data, element) == 0 )
	{
		listNode * aux = *list;
		*list = (*list)->next;
		free(aux->data);
		free(aux);
		return 1;
	}

	/* El elemento actual es menor */
	return DeleteNode( &((*list)->next), element, f);

}

int
Delete( listADT list, listElementT element)
{
	int res;
	res = DeleteNode(&(list->first), element, list->fComp);
	list->size -= res;
	return res;
}


int
Size( listADT list )
{
	return list->size;
}

void
FreeList( listADT list)
{
	listNode * auxi, * node = list->first;

	while ( node != NULL )
	{
		auxi = node->next;
		free(node->data);
		free(node);
		node = auxi;
	}

	/* Liberamos el header */
	free(list);
}
