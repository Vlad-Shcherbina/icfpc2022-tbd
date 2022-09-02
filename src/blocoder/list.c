/*****************************************************************************
 * Fil            : list.c
 * Författare     : Johan Köpman
 * Skapad         : 1996-10-01
 * Senast ändrad  : 1996-10-01
 * Beskrivning    : ADT för enkellänkade listor.
 *---------------------------------------------------------------------------*
 * ÄNDRINGAR:
 *****************************************************************************/

#include "list.h"
#include "memory.h"
#include "stdtypes.h"
#include <stdio.h>

/*---------------------------------------------------------------------------*
 * Funktion     : List_CreateNode(datum, list) -> List*
 * Indata       : datum       Nya noden
 *                list        Listan som skall följa den nya noden.
 * Returnerar   : Den nya listan med den nyinsatta noden först.
 *---------------------------------------------------------------------------*/
List * List_CreateNode(Ref datum, List * list) {
	List * node;
/*
	#ifdef MEMORY_DEBUG
	node = Memory_DebugAlloc(sizeof(List), "List");
	#else
	node = Memory_Alloc(sizeof(List));
	#endif
*/
   node = Memory_Reserve(1, List);
	node->first = datum;
	node->rest  = list;

	return node;
}


/*---------------------------------------------------------------------------*
 * Funktion     : List_Create(void) -> List
 * Indata       : void
 * Returnerar   : En tom lista.
 *---------------------------------------------------------------------------*/
List * List_Create(void) {
	return NULL;
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_Clone(List, Func) -> List
 * Indata       : En lista
 * Returnerar   : En klon av originallistan. Om CloneFunc == NULL klonas inte
 *                elementen utan bara pekarna till dem.
 *---------------------------------------------------------------------------*/

List *List_CreateCopy(List *src) {
	List* dst;

	if (List_IsEmpty(src))
		return List_Create();

   dst = List_CreateCopy(List_Rest(src));
   return List_Insert(dst, List_First(src));     
}

/*---------------------------------------------------------------------------*/

List *List_CreateClone(List *src, CloneFunc *clonefunc) {
	List* dst;

	if (List_IsEmpty(src))
		return List_Create();

   dst = List_CreateClone(List_Rest(src), clonefunc);
   return List_Insert(dst, clonefunc(List_First(src)));
}

/*---------------------------------------------------------------------------*/

List *List_Clone(List* src, CloneFunc *clonefunc) {   
   if (clonefunc == NULL) 
      return List_CreateCopy(src);
   return List_CreateClone(src, clonefunc);
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_Destroy(list, func) -> void
 * Indata       : list        En lista
 *                func        Funktion som förstör ett element i listan.
 * Returnerar   : void
 *---------------------------------------------------------------------------*/
void  List_Destroy(List * list, DestFunc *Destroy) {
	List* temp = list;

	while (!List_IsEmpty(temp)){
		if (Destroy !=NULL)
			Destroy(List_First(list));
		temp = List_Rest(list);
/*
		#ifdef MEMORY_DEBUG
		Memory_DebugDealloc(list, "List");
		#else
		Memory_Dealloc(list);
		#endif
*/
   	Memory_Free(list, List);
		list = temp;
	}
}


// a little useful cutie, inserted by nadir / RAGE
void  List_Traverse(List *list, Proc Func) {
	List* temp = list;

	if (Func == NULL) return;

	while (!List_IsEmpty(temp)) {
		Func(List_First(temp));
		temp = List_Rest(temp);
	}
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_First(list) -> Ref
 * Indata       : list        En lista
 * Returnerar   : Det första elementet i listan.
 *---------------------------------------------------------------------------*/
#ifndef CMOD_OPTIMIZE_SPEED
Ref List_First(List * list) {
	return list->first;
}

/*---------------------------------------------------------------------------*/

void List_SetFirst(List *list, Ref datum) {
   list->first = datum;
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_Rest(list) -> List
 * Indata       : list        En lista
 * Returnerar   : Resten av listan. 2 elementet och framåt.
 *---------------------------------------------------------------------------*/
List * List_Rest(List * list) {
	return list->rest;
}
#endif

/*---------------------------------------------------------------------------*
 * Funktion     : List_Insert(list, datum) -> List
 * Indata       : list        En lista
 *                datum       Ett element
 * Returnerar   : Listan med det insatta elementet först.
 *---------------------------------------------------------------------------*/
List * List_Insert(List * list, Ref datum) {
	return List_CreateNode(datum, list);
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_Remove(list, datum) -> List
 * Indata       : list        En lista
 *                datum       Ett element
 * Returnerar   : Listan med elementet borttaget..
 *---------------------------------------------------------------------------*/
void List_Remove0(List * list, List* prev, Ref datum) {
	List* rest;

	if (List_IsEmpty(list))
		return;
	else if (List_First(list) == datum){
		rest = list->rest;
      Memory_Free(list, List);
/*
		#ifdef MEMORY_DEBUG
		Memory_DebugDealloc(list, "List");
		#else
		Memory_Dealloc(list);
		#endif
*/
		prev->rest = rest;
	}
	else
		List_Remove0(list->rest, list, datum);
}

List* List_Remove(List * list, Ref datum) {
	List* next;

	if (List_IsEmpty(list))
		return list;
	else if (List_First(list) == datum){
		next = List_Rest(list);
      Memory_Free(list, List);
/*
		#ifdef MEMORY_DEBUG
		Memory_DebugDealloc(list, "List");
		#else
		Memory_Dealloc(list);
		#endif
*/
		return next;
	}
	else {
		List_Remove0(list->rest, list, datum);
		return list;
	}
}

List* List_RemoveFirst(List* list) {
	List* next;

	if (List_IsEmpty(list))
		return list;
	else {
		next = List_Rest(list);
      Memory_Free(list, List);

/*	
   	#ifdef MEMORY_DEBUG
		Memory_DebugDealloc(list, "List");
		#else
		Memory_Dealloc(list);
		#endif
*/
		return next;
	}
}

/*---------------------------------------------------------------------------*
 * Funktion     : List_IsEmpty(list) -> Bool
 * Indata       : list        En lista.
 * Returnerar   : Sant om listan är tom.
 *---------------------------------------------------------------------------*/
#ifndef CMOD_OPTIMIZE_SPEED
Bool  List_IsEmpty(List * list) {
  return (list == NULL);
}
#endif

/*---------------------------------------------------------------------------*
 * Funktion     : List_Display(list, func) -> void
 * Indata       : list        En lista.
 *                func        Funktion som skriver ut ett element.
 * Returnerar   :
 *---------------------------------------------------------------------------*/
#ifdef CMOD_DEBUG_OUTPUT
void  List_Display(List * list, int n, DispFunc *NodeDisplay) {
  int i;

  for (i=0; i<n; i++) printf(" ");
  printf("[ ");

  for (; !List_IsEmpty(list); list = List_Rest(list)){
    NodeDisplay(List_First(list), 0);
    printf(" ");
  }

  printf("]");
}
#endif
