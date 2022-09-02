
#ifndef _darray_h
#define _darray_h

#include "cmod.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif
#include "stdtypes.h"
#include "stdfunc.h"

#include <stdint.h>

#define DARRAY_DEFAULTSIZE 16

typedef struct {
	unsigned int nof_elems;
	unsigned int max_elems;
    void **elem;
} DArray;

void DArray_Init(DArray *darray, unsigned int size);
DArray *DArray_Create(unsigned int size); /* Creates DArray, size = suggested size */
DArray *DArray_Clone(const DArray *darray, CloneFunc *clonefunc); /* Clones darray, elems cloned w clonefunc */
Bool    DArray_IsEqual(DArray *darray1, DArray *darray2, EqualFunc *elemeq);
void    DArray_Destroy(DArray *darray, DestFunc *destfunc);
void    DArray_Grow(DArray *darray, unsigned int factor); /* Used internally */
void    DArray_DelQuick(DArray *darray, unsigned int elem_ix); /* Swaps last elem with elem_ix */
#define DArray_QuickDel DArray_DelQuick
void    DArray_Del(DArray *darray, unsigned int elem_ix); /* Removes elm from darray, O(n). Preserves order */
void    DArray_DelRange(DArray *darray, unsigned int first_ix, unsigned int last_ix); /* Not including last is removed */
void   *DArray_Pop(DArray *darray);
void   *DArray_Last(DArray *darray);
#define DArray_IsEmpty(d) ((d)->nof_elems == 0)
//#define DArray_Last(darray) ((darray)->elem[(darray)->nof_elems - 1])
DArray *DArray_Add(DArray *darray, void *elem); /* Appends element to darray, O(1) */
DArray *DArray_SetElem(DArray *darray, unsigned int elem_ix, void *elem); /* Sets elem, overwriting */
void    DArray_Insert(DArray *darray, unsigned int elem_ix, void *elem); /* Inserts elm, moves elms fwd */
Bool    DArray_IsIn(DArray *darray, void *elem, EqualFunc *elemeq);
#define DArray_Has DArray_IsIn
void DArray_Swap(DArray *darray, int ix1, int ix2);
void    DArray_Reverse(DArray *darray, int start, int stop);
void    DArray_ReverseAll(DArray *darray);
#define DArray_Rev DArray_ReverseAll
void    DArray_QuickSort(DArray *darray, CompFunc *compare);
//void    DArray_InsertionSort(DArray *darray, SortFunc *sortkey);
//void    DArray_Sort(DArray *darray, SortFunc *sortkey);
int     DArray_BinSearchIx(DArray *d, void *elem, CompFunc *compare); /* -1 if not in */
void    DArray_InsertSorted(DArray *darray, void *elem, CompFunc *compare);
void    DArray_SortInt64(DArray *darray); /* Assumes every *elem points to a struct w an int64 as first component */
int     DArray_LookUpInt(DArray *darray, int key);
int     DArray_LookUpInt64(DArray *darray, long long key); /* Returns largest smaller than key */
int     DArray_IsInt64In(DArray *darray, long long key); /* Return -1 if not in, smallest index otherwise */
void    DArray_SortInt(DArray *darray); /* Assumes every *elem points to a struct w an int as first component */
void    DArray_SortDouble(DArray *darray); /* Assumes every *elem points to a struct w a double as first component */
void   *DArray_Max(DArray *darray, CompFunc *compare);
unsigned int DArray_ArgMax(DArray *darray, CompFunc *compare);
void    DArray_Merge(DArray *d1, DArray *d2);
#define DArray_SetNofElems(d, v) ((d)->nof_elems = (v))
#define DArray_IsValidIx(d, k) ((unsigned int)(k) < DArray_NofElems(d))
//Bool    DArray_IsValidIx(DArray *d, int ix); /* Boundary check */
#ifdef CMOD_SERIALIZED
void    DArray_Serialize(DArray *darray, Serialized *ser, SerializeFunc *elemserialize);
DArray *DArray_DeSerialize(Serialized *ser, Func elemdeserialize);
#endif
void    DArray_Display(DArray *darray, unsigned int n, DispFunc *elemdisplay);

/*
#ifndef CMOD_OPTIMIZE_SPEED
void   *DArray_Elem(DArray *darray, unsigned int elem_ix); 
unsigned int DArray_NofElems(DArray *darray);
*/
#define DArray_Elem(d,i) ((d)->elem[(i)])
#define DArray_NofElems(d) ((d)->nof_elems)
#define DArray_MaxElems(d) ((d)->max_elems)
#define DArray_Push DArray_Add

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
/* Macros that 'fake' integer DArray - by using void* as ints */
#define IDArray DArray

#define IDArray_Create DArray_Create
IDArray *IDArray_CreateRange(int start, int stop);
IDArray *IDArray_Clone(IDArray *id);
#define IDArray_Clear(d) ((d)->nof_elems = 0)
#define IDArray_IsEmpty(d) ((d)->nof_elems == 0)
#define IDArray_IsEqual(d1, d2) DArray_IsEqual((d1), (d2), NULL)
#define IDArray_Del DArray_Del
#define IDArray_Delete DArray_Del
#define IDArray_Add(d,n) DArray_Add((d), ((void *)(n)))
#define IDArray_Push IDArray_Add
#define IDArray_Pop (int)DArray_Pop
#define IDArray_Last(darray) (int)DArray_Last(darray)
#define IDArray_SetElem(d,i,n) DArray_SetElem((d),(i),(void *)(n))
#define IDArray_Insert(d,i,n) DArray_Insert((d),(i),(void *)(n))
#define IDArray_Swap DArray_Swap
#define IDArray_Elem *(int *)&DArray_Elem
#define IDArray_NofElems DArray_NofElems
#define IDArray_MaxElems DArray_MaxElems
#define IDArray_Merge DArray_Merge
#define IDArray_Reverse DArray_Reverse
#define IDArray_ReverseAll DArray_ReverseAll
Bool    IDArray_RemoveAll(IDArray *idarray, int elem); /* All occurences of elem are deleted */
Bool    IDArray_RemoveOne(IDArray *idarray, int elem); /* ONE occurence of elem is deleted */
void    IDArray_QuickSortIx(IDArray *idarray, int start, int end);
#define IDArray_QuickSort(idarray) IDArray_QuickSortIx((idarray), 0, IDArray_NofElems(idarray)-1)
#define IDArray_SortIncreasing(idarray) IDArray_QuickSortIx((idarray), 0, IDArray_NofElems(idarray)-1)
#define IDArray_Sort IDArray_QuickSort
Bool    IDArray_IsSorted(IDArray *idarray);
unsigned int IDArray_SmallestLargerIndex(IDArray *idarray, int value); /* Assumes array sorted increasingly */
Bool    IDArray_IsInSorted(IDArray *idarray, int elem);
Bool    IDArray_IsIn(IDArray *idarray, int elem);
void    IDArray_RemoveDuplicates(IDArray *a);
#define IDArray_Has IDArray_IsIn
void    IDArray_Union(IDArray *a, IDArray *b); /* a := a U b, reordered and duplicates removed*/
#define IDArray_Intersection IDArray_Intersect
void    IDArray_Intersect(IDArray *a, IDArray *b); /* a := a \cap b, reordered */
void    IDArray_Subtract(IDArray *a, IDArray *b); /* a := a - b, removes all elements of b from a */
int     IDArray_Max(IDArray *a);
int     IDArray_Min(IDArray *a);
unsigned int IDArray_ArgMax(IDArray *a); /* index to largest */
unsigned int IDArray_ArgMin(IDArray *a);
void    IDArray_Display(IDArray *idarray, unsigned int n);
void    IDArray_Destroy(IDArray *idarray);

#ifdef CMOD_SERIALIZED
IDArray *IDArray_DeSerialize(Serialized *ser);
void     IDArray_Serialize(IDArray *idarray, Serialized *ser);
#endif

#define FDArray DArray

#define FDArray_Create DArray_Create
#define FDArray_Clone(d) DArray_Clone((d), NULL)
#define FDArray_IsEqual(d1, d2) DArray_IsEqual((d1), (d2), NULL)
#define FDArray_Destroy(d) DArray_Destroy((d), NULL)
#define FDArray_Del DArray_Del
void    FDArray_Add(FDArray *fd, float c);
#define FDArray_Push FDArray_Add
#define FDArray_Pop (*(float *)&DArray_Pop
//#define FDArray_Last(darray) (*(float *)&DArray_Last(darray)))
#define FDArray_SetElem(d,i,v) DArray_SetElem((d),(i),(void *)(v))
#define FDArray_Insert(d,i,v) DArray_Insert((d),(i),(void *)(v))
#define FDArray_Swap DArray_Swap
#define FDArray_Elem(d,i) (*(float *)&DArray_Elem(d, i))
#define FDArray_Merge DArray_Merge
#define FDArray_NofElems DArray_NofElems
#define FDArray_MaxElems DArray_MaxElems
float   FDArray_Sum(FDArray *fdarray);
float   FDArray_Average(FDArray *fdarray);
void    FDArray_QuickSortIx(FDArray *fdarray, unsigned int start, unsigned int end);
#define FDArray_Sort(fd) FDArray_QuickSortIx((fd), 0, FDArray_NofElems(fd)-1)
#define FDArray_QuickSort(fd) FDArray_Sort(fd)
void    FDArray_Display(FDArray *fdarray, unsigned int n);
float   FDArray_Min(FDArray *fdarray);
float   FDArray_Max(FDArray *fdarray);
void    FDArray_Div(FDArray *fdarray, float val);
void    FDArray_Mul(FDArray *fdarray, float val);

#ifdef CMOD_NOMAKEFILE
#include "darray.c"
#endif

#endif

