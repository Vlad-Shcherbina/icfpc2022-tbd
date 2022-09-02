#ifndef _list_h
#define _list_h

/*****************************************************************************
 * Fil            : list.h
 * F�rfattare     : Johan K�pman
 * Skapad         : 1996-09-20
 * Senast �ndrad  : 1996-09-23
 * Beskrivning    : Abstrakt datatyp f�r hantering av enkell�nkade listor.
 *---------------------------------------------------------------------------*
 * OPERATIONER:
 *   List List_Create(void);
 *     Skapar en tom lista.
 *   void  List_Destroy(List list, ListDestroyFunc func);
 *     F�rst�r och frig�r listan inklusive alla element som ing�r.
 *
 *   Ref List_First(List list);
 *     Returnerar det f�rsta elementet.
 *   int   List_Rest(List list);
 *     Returnerar resten av listan..
 *   void  List_Insert(List list, Ref datum);
 *     S�tter in ett element f�rst i listan.
 *   void  List_Remove(List list, Ref datum);
 *     Tar bort 'datum' ur 'list'.
 *   Bool  List_IsEmpty(List list);
 *     Ser efter om listan �r tom.
 *   void  List_Display(List list, ListDispFunc func);
 *     Skriver ut alla element i listan. Varje element skrivs ut med func.
 *****************************************************************************/

#include "cmod.h"
#include "stdtypes.h"

typedef struct List List;
struct List {
	Ref first;
	List* rest;
};


List* List_Create(void);
/* if NULL not cloning the elements. */
List* List_Clone(List *list, CloneFunc *clonefunc);

void  List_Destroy(List* list, DestFunc *func);

List* List_Insert(List* list, Ref datum);
#define List_Add(l,r) List_Insert((l),(r))
List* List_Remove(List* list, Ref datum);
List* List_RemoveFirst(List* list);
void  List_Traverse(List *list, Proc Func);
#define List_Next(l) List_Rest(l)
#define List_Peek(l) List_First(l)

#ifdef CMOD_DEBUG_OUTPUT
void  List_Display(List* list, int n, DispFunc *disp_func);
#endif

#ifndef CMOD_OPTIMIZE_SPEED
void  List_SetFirst(List *list, Ref datum);
Ref    List_First(List* list);
List*  List_Rest(List* list);
Bool   List_IsEmpty(List* list);
#else
#define List_SetFirst(l,d) (l)->first = (d)
#define List_First(l) (l)->first
#define List_Rest(l) (l)->rest
#define List_IsEmpty(l) ((l) == NULL)
#endif

#define ListFor(l,i) for((i)=List_First(l); !List_IsEmpty(i); (i)=List_Rest(l))
#define LIST_LOOP(l,i) ListFor(l,i)

#ifdef CMOD_NOMAKEFILE
#include "list.c"
#endif
#endif

