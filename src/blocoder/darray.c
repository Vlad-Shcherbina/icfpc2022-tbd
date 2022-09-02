
#include "darray.h"
#include "memory.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif
#include "stdfunc.h"
#include "stdtypes.h"

#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/

void DArray_Init(DArray *darray, unsigned int size) {
   if (size <= 0) size = 4;
	darray->nof_elems = 0;
	darray->elem = Memory_Reserve(size, void *);
	darray->max_elems = size;
}

/*---------------------------------------------------------------------------*/

DArray *DArray_Create(unsigned int size) {
	DArray *darray;
		
	darray = Memory_Reserve(1, DArray);
   DArray_Init(darray, size);
   
	return darray;
}

/*---------------------------------------------------------------------------*/

void DArray_InitClone(const DArray *darray, DArray *darray2, CloneFunc *clonefunc) {
   unsigned int i;
   
   if (clonefunc == NULL) {
      for (i=0; i<DArray_NofElems(darray); i++) 
         DArray_Add(darray2, DArray_Elem(darray, i));
      } else {
      for (i=0; i<DArray_NofElems(darray); i++) 
         DArray_Add(darray2, clonefunc(DArray_Elem(darray, i)));
   }
}

DArray *DArray_Clone(const DArray *darray, CloneFunc *clonefunc) {
   DArray *darray2;
   
   darray2 = DArray_Create(darray->max_elems);
   DArray_InitClone(darray, darray2, clonefunc);
   return darray2;
}

/*---------------------------------------------------------------------------*/

Bool    DArray_IsEqual(DArray *darray1, DArray *darray2, EqualFunc *elemeq) {
   unsigned int i;
   
   if (darray1 == darray2) return TRUE;
   if (darray1 == NULL || darray2 == NULL) return FALSE;
   
   if (DArray_NofElems(darray1) != DArray_NofElems(darray2)) return FALSE;
   
   if (elemeq == NULL) { 
      for (i=0; i<DArray_NofElems(darray1); i++) {
         if (DArray_Elem(darray1,i) != DArray_Elem(darray2,i)) return FALSE;
      }
   } else {
      for (i=0; i<DArray_NofElems(darray1); i++) {
         if (!elemeq(DArray_Elem(darray1,i), DArray_Elem(darray2,i))) return FALSE;
      }      
   }
   return TRUE;
}

/*---------------------------------------------------------------------------*/

void DArray_Destroy(DArray *darray, DestFunc *destfunc) {
   unsigned int i;
   if (darray == NULL) return;
   if (destfunc != NULL) {
      for (i=0; i<DArray_NofElems(darray); i++) {
         destfunc(DArray_Elem(darray, i));
      }
   } 
	Memory_Free(darray->elem, void *);
	Memory_Free(darray, DArray);
}

/*---------------------------------------------------------------------------*/

void DArray_GrowToSize(DArray *darray, unsigned int new_size) {
   unsigned int i;
   void **elm2;

   elm2 = Memory_Realloc(darray->elem, new_size*sizeof(void *), void *);
   
   if (elm2 != NULL) darray->elem = elm2;
   
   for (i=darray->nof_elems; i<new_size; i++) darray->elem[i] = NULL;
   darray->max_elems = new_size;
/*	
	for (i=0; i<darray->nof_elems; i++) {
		d2 = DArray_Add(d2, DArray_Elem(darray, i));
	}
*/
}

/*---------------------------------------------------------------------------*/

void DArray_Grow(DArray *darray, unsigned int factor) { 
   DArray_GrowToSize(darray, darray->max_elems*factor);
}

/*---------------------------------------------------------------------------*/

DArray *DArray_Add(DArray *darray, void *elem) {
	if (darray->nof_elems == darray->max_elems) {
      void **elm2;
      elm2 = Memory_Realloc(darray->elem, darray->max_elems*2*sizeof(void *), void *);
      if (elm2 != NULL) {
/*         Memory_Dealloc(darray->elem);*/
         darray->elem = elm2;         
      }   
/*		darray = DArray_Grow(darray, 2); */
   	darray->max_elems *= 2;
   }
   if (darray->nof_elems >= darray->max_elems) {
      fprintf(stderr, "DArray index error! Accessing ix %i of %i\n", darray->nof_elems, darray->max_elems);
   }
	darray->elem[darray->nof_elems++] = elem;
	return darray;
}

/*---------------------------------------------------------------------------*/

void DArray_DelQuick(DArray *darray, unsigned int elem_ix) {
   /* Stored array indices may no longer be valid after this! */
   darray->nof_elems--;
   DArray_Swap(darray, elem_ix, darray->nof_elems);
}

/*---------------------------------------------------------------------------*/

void DArray_Del(DArray *darray, unsigned int elem_ix) {
    if (elem_ix < darray->nof_elems-1)
	   Memory_Move(&darray->elem[elem_ix], &darray->elem[elem_ix+1], (darray->nof_elems - elem_ix - 1)*sizeof(void *));
    darray->nof_elems--;
}

/*---------------------------------------------------------------------------*/

void DArray_DelRange(DArray *darray, unsigned int first_ix, unsigned int last_ix) {
   if (last_ix <= first_ix) return;
	Memory_Move(&darray->elem[first_ix], &darray->elem[last_ix], (darray->nof_elems - last_ix)*sizeof(void *));
	darray->nof_elems-=last_ix - first_ix;
}

/*---------------------------------------------------------------------------*/


void   *DArray_Pop(DArray *darray) {
   void *elm;
   if (darray->nof_elems == 0) return NULL;
   elm = darray->elem[darray->nof_elems - 1];
   darray->nof_elems--;
   return elm;
}

/*---------------------------------------------------------------------------*/

void *DArray_Last(DArray *darray) { 
   if (darray->nof_elems>0) return darray->elem[darray->nof_elems - 1]; else return NULL;
}

/*---------------------------------------------------------------------------*/

int DArray_BinSearchIx(DArray *d, void *elem, CompFunc *compare) {
   int ix, lower, upper, n, c;

   n = DArray_NofElems(d);
   
   if (n == 0) return -1;
   lower = 0;
   upper = n;
   
   do {
      ix = (lower+upper)/2;
      c = compare(elem, DArray_Elem(d, ix));
      if (c > 0) lower = ix + 1;
      else
      if (c < 0) upper = ix;
      else return ix;
   } while (upper > lower);
   return -1;
}

/*---------------------------------------------------------------------------*/

Bool DArray_IsIn(DArray *darray, void *elem, EqualFunc *elemeq) {
   unsigned int i;
   for (i=0; i<DArray_NofElems(darray); i++) {
      if (elemeq == NULL && DArray_Elem(darray, i) == elem) return TRUE;
      else
      if (elemeq(elem, DArray_Elem(darray, i))) return TRUE;
   }
   return FALSE;
}

/*---------------------------------------------------------------------------*/

DArray *DArray_SetElem(DArray *darray, unsigned int elem_ix, void *elem) {
   if (elem_ix >= darray->max_elems) {
      if ((128*elem_ix) / darray->max_elems > 192)
         DArray_GrowToSize(darray, 1 + elem_ix);
      else
         DArray_Grow(darray, 2);
   }
   darray->elem[elem_ix] = elem;
   if (elem_ix >= darray->nof_elems) {
      for (;darray->nof_elems < elem_ix ; darray->nof_elems++) 
         darray->elem[darray->nof_elems] = NULL;
      darray->nof_elems = elem_ix+1;
   }
   return darray;
}

/*---------------------------------------------------------------------------*/

/* untested */
void    DArray_Insert(DArray *darray, unsigned int elem_ix, void *elem) {
   unsigned int i;
   if (elem_ix >= DArray_NofElems(darray)) return;
   DArray_Add(darray, NULL);
   for (i=DArray_NofElems(darray)-1; i>elem_ix; i--) {
      darray->elem[i] = darray->elem[i-1];
   }
   darray->elem[elem_ix] = elem;
}

/*---------------------------------------------------------------------------*/

/* Assumes array sorted increasingly */
void DArray_InsertSorted(DArray *darray, void *elem, CompFunc *compare) {
   int ix;
   DArray_Add(darray, NULL);
   ix=DArray_NofElems(darray)-2;
   while(ix>=0 && compare(elem, darray->elem[ix]) < 0) {
      darray->elem[ix+1] = darray->elem[ix];
      ix--;
   }       
   darray->elem[ix+1] = elem;
}

/*---------------------------------------------------------------------------*/

unsigned int DArray_ArgMax(DArray *darray, CompFunc *compare) {
   unsigned int i, bestix;

   bestix = 0;
   
   for (i=1; i<darray->nof_elems; i++) {
      if (compare(DArray_Elem(darray, i), DArray_Elem(darray, bestix)) > 0)
         bestix = i;         
   }
   
   return bestix;
}

/*---------------------------------------------------------------------------*/

void *DArray_Max(DArray *darray, CompFunc *compare) {
   unsigned int i;
   void *best;

   best = DArray_Elem(darray, 0);
   
   for (i=1; i<darray->nof_elems; i++) {
      if (compare(DArray_Elem(darray, i), best) > 0) 
         best = DArray_Elem(darray, i);
   }
   
   return best;
}

/*---------------------------------------------------------------------------*/

inline void DArray_Swap(DArray *darray, int ix1, int ix2) {
   void *tmp;
   
   if (ix1 == ix2) return;
//   *(int *)&darray->elem[ix1] ^= (int)darray->elem[ix2];
//   *(int *)&darray->elem[ix2] ^= (int)darray->elem[ix1];
//   *(int *)&darray->elem[ix1] ^= (int)darray->elem[ix2];
//   void *tmp = DArray_Elem(darray,ix2);
   tmp = darray->elem[ix2];
   darray->elem[ix2] = darray->elem[ix1];
   darray->elem[ix1] = tmp;
//   darray = DArray_SetElem(darray,ix2,DArray_Elem(darray,ix1));
//   return DArray_SetElem(darray,ix1,tmp);
}

/*---------------------------------------------------------------------------*/

void DArray_Reverse(DArray *darray, int start, int stop) {
   int ix1, ix2;
   void *temp;
/*
   for (ix1=start,ix2=stop-1; ix1<ix2; ix1++,ix2--) {
      *(int *)&darray->elem[ix1] ^= (int)darray->elem[ix2];
      *(int *)&darray->elem[ix2] ^= (int)darray->elem[ix1];
      *(int *)&darray->elem[ix1] ^= (int)darray->elem[ix2];      
   }
*/
   ix1=start;
   ix2=stop-1;
   do {
      temp = darray->elem[ix1];
      darray->elem[ix1] = darray->elem[ix2];
      darray->elem[ix2] = temp;
      ix1++;
      ix2--;
   } while(ix1<ix2);
}

/*---------------------------------------------------------------------------*/

void DArray_ReverseAll(DArray *darray) { DArray_Reverse(darray, 0, DArray_NofElems(darray)); }

/*---------------------------------------------------------------------------*/
/*
void DArray_QuickSortInterval(DArray *darray, int first, int last, CompFunc *compare) {
   int i,pivot;
   pivot = (first+last+1)/2;
}
*/
/*---------------------------------------------------------------------------*/

void DArray_QuickSort(DArray *darray, CompFunc *compare) {
   qsort(darray->elem, darray->nof_elems, sizeof(void *), compare);
}

/*---------------------------------------------------------------------------*/

//void    DArray_SortIx(DArray *darray, SortFunc *sortkey, int start, int stop);

/*---------------------------------------------------------------------------*/

#if 0
void    DArray_Sort(DArray *darray, SortFunc *sortkey) {
   Void_RadixSort(darray->elem, sortkey, darray->nof_elems);
}

/*---------------------------------------------------------------------------*/

void    DArray_InsertionSort(DArray *darray, SortFunc *sortkey) {
   void **newelm;
   void **backup;
   
   newelm = Memory_Reserve(darray->max_elems, void *);
   Void_InsertionSortIx(darray->elem, newelm, sortkey, 0, darray->nof_elems);
   backup = darray->elem;
   darray->elem = newelm;
   Memory_Free(backup, void *);
}
#endif

/*---------------------------------------------------------------------------*/

void DArray_SortIntervalInt(DArray *darray, int first, int one_after_last) {
   int i = first, j = one_after_last-1;
   int **arr = (int **)darray->elem;
   int z = *arr[(first + one_after_last) >> 1];

    do {
       while(*arr[i] < z) i++;
       while(*arr[j] > z) j--;

       if(i <= j) {
          int *y = arr[i];
          arr[i] = arr[j]; 
          arr[j] = y;
          i++; 
          j--;
       }
    } while(i <= j);

    if(first < j) DArray_SortIntervalInt(darray, first, j+1);
    if(i < one_after_last) DArray_SortIntervalInt(darray, i, one_after_last); 

}

/*---------------------------------------------------------------------------*/

void DArray_SortIntervalInt64(DArray *darray, int first, int one_after_last) {
   int i = first, j = one_after_last-1;
   long long **arr = (long long **)darray->elem;
   long long z = *arr[(first + one_after_last) >> 1];

   while(i <= j) {
      while(*arr[i] < z) i++;
      while(*arr[j] > z) j--;

      if(i <= j) {
         long long *y = arr[i];
         arr[i] = arr[j]; 
         arr[j] = y;
         i++; 
         j--;
      }
   }
   
   if (first < j) DArray_SortIntervalInt64(darray, first, j+1);
   if (i < one_after_last) DArray_SortIntervalInt64(darray, i, one_after_last); 
}

/*---------------------------------------------------------------------------*/

void DArray_SortIntervalDouble(DArray *darray, int first, int one_after_last) {
   int i = first, j = one_after_last-1;
   double **arr = (double **)darray->elem;
   double z = *arr[(first + one_after_last) >> 1];

    do {
       while(*arr[i] < z) i++;
       while(*arr[j] > z) j--;

       if(i <= j) {
          double *y = arr[i];
          arr[i] = arr[j]; 
          arr[j] = y;
          i++; 
          j--;
       }
    } while(i <= j);

    if(first < j) DArray_SortIntervalDouble(darray, first, j+1);
    if(i < one_after_last) DArray_SortIntervalDouble(darray, i, one_after_last); 
}

/*---------------------------------------------------------------------------*/

void DArray_SortInt(DArray *d) { 
   DArray_SortIntervalInt(d, 0, d->nof_elems); 
}

/*---------------------------------------------------------------------------*/

void DArray_SortInt64(DArray *d) { 
   DArray_SortIntervalInt64(d, 0, d->nof_elems); 
}

/*---------------------------------------------------------------------------*/

void DArray_SortIntAscending(DArray *d) { 
   DArray_SortInt(d);
   DArray_Rev(d);
}

/*---------------------------------------------------------------------------*/

void DArray_SortDouble(DArray *d) { 
   DArray_SortIntervalDouble(d, 0, d->nof_elems); 
}

/*---------------------------------------------------------------------------*/

int DArray_LookUpInt(DArray *darray, int key) {
   int first=0, middle, one_past_last=darray->nof_elems;
   int **arr = (int **)darray->elem;
   
   while (one_past_last > first) {
      middle = (one_past_last + first)>>1;
      if (*arr[middle] > key) one_past_last = middle;
      if (*arr[middle] < key) first = middle + 1;
      else {
         while (middle>0 && *arr[middle-1] == key) middle--;
         return middle;
      }
   }
   return -1;
}

/*---------------------------------------------------------------------------*/

int DArray_IsInt64In(DArray *darray, long long key) {
   int first=0, middle, one_after_last=darray->nof_elems;
   long long **arr = (long long **)darray->elem;
   
   while (one_after_last > first) {
      middle = (one_after_last + first) >> 1;
      if (*arr[middle] > key) one_after_last = middle;
      else if (*arr[middle] < key) first = middle + 1;
      else {
         while (middle>0 && *arr[middle-1] == key) middle--;
         return middle;
      }
   }
   return -1;
}

/*---------------------------------------------------------------------------*/

int DArray_LookUpInt64(DArray *darray, long long key) {
   int first=0, middle, one_after_last=darray->nof_elems;
   long long **arr = (long long **)darray->elem;
   
   while (one_after_last > first) {
      middle = (one_after_last + first) >> 1;
      if (*arr[middle] > key) one_after_last = middle;
      else if (*arr[middle] < key) first = middle + 1;
      else {
         while (middle>0 && *arr[middle-1] == key) middle--;
         return middle;
      }
   }
   return first;
}

/*---------------------------------------------------------------------------*/

void    DArray_Merge(DArray *d1, DArray *d2) {
   unsigned int i;
   for (i=0; i<DArray_NofElems(d2); i++) {
      DArray_Add(d1, DArray_Elem(d2, i));
   }
}

/*---------------------------------------------------------------------------*/

#ifdef CMOD_SERIALIZED
void DArray_Serialize(DArray *darray, Serialized *ser, SerializeFunc *elemserialize) {
   int i;
   /* TODO: Take care of potential 64-bit ints? */
   Serialized_AppendWrapped(ser, &darray->max_elems, sizeof(darray->max_elems));
   Serialized_AppendWrapped(ser, &darray->nof_elems, sizeof(darray->nof_elems));
   for (i=0; i<DArray_NofElems(darray); i++) {
      elemserialize(DArray_Elem(darray, i), ser); 
   }   
}

/*---------------------------------------------------------------------------*/

DArray *DArray_DeSerialize(Serialized *ser, Func elemdeserialize) {
   DArray *d;
   int max, nof;
   int i;
   
   Serialized_ReadWrapped(ser, &max, sizeof(d->max_elems));
   Serialized_ReadWrapped(ser, &nof, sizeof(d->nof_elems));
   d = DArray_Create(max);
   for (i=0; i<nof; i++) {
      DArray_SetElem(d, i, elemdeserialize(ser));
   }
   d->nof_elems = nof;
   return d;
}

/*---------------------------------------------------------------------------*/

void IDArray_Serialize(IDArray *idarray, Serialized *ser) {
   int i;
   /* TODO: Take care of potential 64-bit ints? */
   Serialized_AppendWrapped(ser, &idarray->max_elems, sizeof(idarray->max_elems));
   Serialized_AppendWrapped(ser, &idarray->nof_elems, sizeof(idarray->nof_elems));
   for (i=0; i<DArray_NofElems(idarray); i++) {
      Serialized_AppendWrapped(ser, &idarray->elem[i], sizeof(int)); 
   }   
}

/*---------------------------------------------------------------------------*/

IDArray *IDArray_DeSerialize(Serialized *ser) {
   IDArray *d;
   int max, nof;
   int i;
   
   Serialized_ReadWrapped(ser, &max, sizeof(d->max_elems));
   Serialized_ReadWrapped(ser, &nof, sizeof(d->nof_elems));
   d = IDArray_Create(max);
   for (i=0; i<nof; i++) {
      Serialized_ReadWrapped(ser, &d->elem[i], sizeof(int));
   }
   d->nof_elems = nof;
   return d;
}
#endif

/*---------------------------------------------------------------------------*/

void DArray_Display(DArray *darray, unsigned int n, DispFunc *elemdisplay) {
   unsigned int i;

   for (i=0; i<n; i++) printf(" ");
   printf("Max elems: %i, Nof elems: %i\n", darray->max_elems, darray->nof_elems);
//   for (i=0; i<n; i++) printf(" ");
   for (i=0; i<DArray_NofElems(darray); i++) {
      elemdisplay(DArray_Elem(darray, i), n+3);
      if (i<DArray_NofElems(darray)-1) printf(", ");
   }
   printf("\n");
}

/*---------------------------------------------------------------------------*/

#ifndef CMOD_OPTIMIZE_SPEED

void   *DArray_Elem(DArray *darray, unsigned int elem_ix) {
	return darray->elem[elem_ix];
}

/*---------------------------------------------------------------------------*/

unsigned int DArray_NofElems(DArray *darray) {
	return darray->nof_elems;
}

#endif

/*---------------------------------------------------------------------------*/

IDArray *IDArray_CreateRange(int start, int stop) {
   IDArray *a;
   int i;
   if (stop > start) {
      a = IDArray_Create(stop - start + 1);
      for (i=start; i<stop; i++) {
         IDArray_Add(a,i);
      }
   } else {
      a = IDArray_Create(start - stop + 1);
      for (i=start; i<stop; i--) {
         IDArray_Add(a,i);
      }      
   }  
   return a; 
}

/*---------------------------------------------------------------------------*/

IDArray *IDArray_Clone(IDArray *id) { return DArray_Clone((id), NULL); }

/*---------------------------------------------------------------------------*/

Bool IDArray_RemoveAll(IDArray *idarray, int elem) {
   unsigned int i;
   Bool del = FALSE;
   for (i=0; i<IDArray_NofElems(idarray); i++) {
      if (IDArray_Elem(idarray, i) == elem) {
         IDArray_Del(idarray, i);
         i--;
         del = TRUE;
      }
   }
   return del;
}

/*---------------------------------------------------------------------------*/

Bool IDArray_RemoveOne(IDArray *idarray, int elem) {
   unsigned int i;

   for (i=0; i<IDArray_NofElems(idarray); i++) {
      if (IDArray_Elem(idarray, i) == elem) {
         IDArray_Del(idarray, i);
         return TRUE;
      }
   }
   return FALSE;
}

/*---------------------------------------------------------------------------*/

void    IDArray_QuickSortIx(IDArray *idarray, int start, int end) {
   int i, j;
   int median;
   int t;
   
   if (end <= start) return;
   
   median = IDArray_Elem(idarray, end);
   
   j = end;
   i = start-1;
   while (TRUE) {
      while(IDArray_Elem(idarray, ++i) < median);
      while(IDArray_Elem(idarray, --j) > median && j>start);

      if (i>=j) break;
      t = IDArray_Elem(idarray, j);
      IDArray_SetElem(idarray, j, IDArray_Elem(idarray, i));
      IDArray_SetElem(idarray, i, t);
   }

   t = IDArray_Elem(idarray, i);
   IDArray_SetElem(idarray, i, IDArray_Elem(idarray, end));
   IDArray_SetElem(idarray, end, t);

   IDArray_QuickSortIx(idarray, start, i-1);
   IDArray_QuickSortIx(idarray, i+1, end);
}

/*---------------------------------------------------------------------------*/

Bool    IDArray_IsSorted(IDArray *idarray) {
   unsigned int i;
   for (i=0; i<IDArray_NofElems(idarray)-1; i++) {
      if (IDArray_Elem(idarray, i) > IDArray_Elem(idarray, i+1)) {
//         printf("ix %i, val = %i\n", i, IDArray_Elem(idarray, i));
         return FALSE;
      }
   }
   return TRUE;
}

/*---------------------------------------------------------------------------*/

Bool IDArray_IsInSorted(IDArray *idarray, int elem) {
   unsigned int cur,low=0, high=idarray->nof_elems;
   while(high > low + 1) {
      cur = (high+low) >> 1;
      if (IDArray_Elem(idarray, cur) < elem) low = cur+1;
      else
      if (IDArray_Elem(idarray, cur) > elem) high = cur;
      else
      return TRUE;
   }
   return FALSE;
}

/*---------------------------------------------------------------------------*/

Bool IDArray_IsIn(IDArray *idarray, int elem) {
   unsigned int i;
   for (i=0; i<IDArray_NofElems(idarray); i++) {
      if (IDArray_Elem(idarray, i) == elem) return TRUE;
   }
   return FALSE;
}

/*---------------------------------------------------------------------------*/

void IDArray_RemoveDuplicates(IDArray *a) {
   unsigned int i, c = 0;
   
   if (a == NULL || IDArray_NofElems(a) == 0) return;
   if (!IDArray_IsSorted(a)) IDArray_QuickSort(a);
   for (i=1; i<IDArray_NofElems(a); i++) {
      if (IDArray_Elem(a,i-1) != IDArray_Elem(a,i)) {
         c++;
         IDArray_SetElem(a,c, IDArray_Elem(a,i));
      }
   }
   a->nof_elems = c+1;
}

/*---------------------------------------------------------------------------*/

void IDArray_Union(IDArray *a, IDArray *b) { /* a := a U b, reordered and duplicates removed*/
   unsigned int i;
   
   if (a->max_elems < IDArray_NofElems(a) + IDArray_NofElems(b)) {
      DArray_GrowToSize(a, IDArray_NofElems(a) + IDArray_NofElems(b) + 1);
   }
   
   for (i=0; i<IDArray_NofElems(b); i++) {
      IDArray_Add(a, IDArray_Elem(b, i));
   }
   
   IDArray_RemoveDuplicates(a);
}

/*---------------------------------------------------------------------------*/

void IDArray_Intersect(IDArray *a, IDArray *b) { /* a := a \cap b, reordered */
   unsigned int i,j,t;
   
   if (!IDArray_IsSorted(a)) IDArray_QuickSort(a);
   if (!IDArray_IsSorted(b)) IDArray_QuickSort(b);
   i=0; 
   j=0;
   t=0;
   while(i<IDArray_NofElems(a) && j<IDArray_NofElems(b)) {
      if (IDArray_Elem(a,i) == IDArray_Elem(b,j)) {
         IDArray_SetElem(a,t,IDArray_Elem(a,i));
         t++;
         i++;
         j++;
      } else
      if (IDArray_Elem(a,i) < IDArray_Elem(b,j)) {
         i++;
      } else {
         j++;
      }
   }
   a->nof_elems = t;
}

/*---------------------------------------------------------------------------*/

void IDArray_Subtract(IDArray *a, IDArray *b) { /* a := a - b, removes all elements of b from a */
   unsigned int i,j,t;
   
   if (!IDArray_IsSorted(a)) IDArray_QuickSort(a);
   if (!IDArray_IsSorted(b)) IDArray_QuickSort(b);

   i=0; 
   j=0;
   t=0;
   while(i<IDArray_NofElems(a) && j<IDArray_NofElems(b)) {
      if (IDArray_Elem(a,i) == IDArray_Elem(b,j)) {         
         i++;
      } else
      if (IDArray_Elem(a,i) < IDArray_Elem(b,j)) {
         if (i>t) IDArray_SetElem(a,t,IDArray_Elem(a,i));
         i++;
         t++;
      } else {
         j++;
      }      
   }
   
   while (i<IDArray_NofElems(a)) {
      if (i>t) IDArray_SetElem(a,t,IDArray_Elem(a,i));
      i++;
      t++;   
   }
   a->nof_elems = t;   
}

/*---------------------------------------------------------------------------*/

int IDArray_Max(IDArray *a) {
   unsigned int i;
   int largest = IDArray_Elem(a, 0);
   for (i=1; i<IDArray_NofElems(a); i++) {
      if (IDArray_Elem(a, i) > largest) largest = IDArray_Elem(a, i);
   }
   return largest;
}

/*---------------------------------------------------------------------------*/

int IDArray_Min(IDArray *a) {
   unsigned int i;
   int smallest = IDArray_Elem(a, 0);
   for (i=1; i<IDArray_NofElems(a); i++) {
      if (IDArray_Elem(a, i) < smallest) smallest = IDArray_Elem(a, i);
   }
   return smallest;
}

/*---------------------------------------------------------------------------*/

unsigned int IDArray_ArgMax(IDArray *a) { /* index to largest */
   unsigned int largest = 0, i;
   for (i=1; i<IDArray_NofElems(a); i++) {
      if (IDArray_Elem(a, i) > IDArray_Elem(a, largest)) largest = i;
   }
   return largest;
}
/*---------------------------------------------------------------------------*/

unsigned int IDArray_ArgMin(IDArray *a) {
   unsigned int smallest = 0, i;
   for (i=1; i<IDArray_NofElems(a); i++) {
      if (IDArray_Elem(a, i) < IDArray_Elem(a, smallest)) smallest = i;
   }
   return smallest;
}
/*---------------------------------------------------------------------------*/

void IDArray_Destroy(IDArray *idarray) { DArray_Destroy(idarray, NULL); }

/*---------------------------------------------------------------------------*/

void    IDArray_Display(IDArray *idarray, unsigned int n) {
   unsigned int i;
   for (i=0; i<n; i++) printf(" ");
   if (idarray == NULL) printf("NULL"); else
   for (i=0; i<IDArray_NofElems(idarray); i++) {
      printf("%i ", IDArray_Elem(idarray, i));
   }
   printf("\n");
}

/*---------------------------------------------------------------------------*/

void FDArray_Add(FDArray *fd, float c) {
   union {
      float g;
      void *v;
   } b;
   b.g = c;
   
   DArray_Add(fd, b.v);
}

/*---------------------------------------------------------------------------*/

void    FDArray_QuickSortIx(FDArray *fdarray, unsigned int start, unsigned int end) {
   unsigned int i, j;
   float median,t;
   float *elm = (float *)fdarray->elem;
   
   if (end <= start) return;
   
   median = FDArray_Elem(fdarray, end);
   
   j = end;
   i = start-1;
   while (TRUE) {
      while(FDArray_Elem(fdarray, ++i) < median);
      while(FDArray_Elem(fdarray, --j) > median && j>start);

      if (i>=j) break;
      t = FDArray_Elem(fdarray, j);
      elm[j] = FDArray_Elem(fdarray, i);
      elm[i] = t;
   }

   t = FDArray_Elem(fdarray, i);
   elm[i] = FDArray_Elem(fdarray, end);
   elm[end] = t;

   FDArray_QuickSortIx(fdarray, start, i-1);
   FDArray_QuickSortIx(fdarray, i+1, end);
}

/*---------------------------------------------------------------------------*/

unsigned int IDArray_SmallestLargerIndex(IDArray *idarray, int value) {
   unsigned int i = 0;
   while (i<IDArray_NofElems(idarray) && IDArray_Elem(idarray, i) < value) i++;
   return i;
}

/*---------------------------------------------------------------------------*/

float FDArray_Sum(FDArray *fdarray) {
   unsigned int i;
   float sum = 0.0;
   
   for (i=0; i<FDArray_NofElems(fdarray); i++) {
      sum+=FDArray_Elem(fdarray, i);
   }
   return sum;
}

/*---------------------------------------------------------------------------*/

float FDArray_Average(FDArray *fdarray) {
   if (FDArray_NofElems(fdarray)>0)
      return FDArray_Sum(fdarray)/(float)FDArray_NofElems(fdarray);
   else
      return 0.0;
}

/*---------------------------------------------------------------------------*/

float FDArray_Min(FDArray *fdarray) {
   float min;
   unsigned int i;
   
   if (FDArray_NofElems(fdarray)==0) return 0.0;
   min = FDArray_Elem(fdarray,0);
   for (i=0; i<FDArray_NofElems(fdarray); i++) {
      if (FDArray_Elem(fdarray,i)<min) min=FDArray_Elem(fdarray,i);
   }
   return min;
}

/*---------------------------------------------------------------------------*/

float   FDArray_Max(FDArray *fdarray) {
   float max;
   unsigned int i;
   
   if (FDArray_NofElems(fdarray)==0) return 0.0;
   max = FDArray_Elem(fdarray,0);
   for (i=0; i<FDArray_NofElems(fdarray); i++) {
      if (FDArray_Elem(fdarray,i)>max) max=FDArray_Elem(fdarray,i);
   }
   return max;
}

/*---------------------------------------------------------------------------*/

void FDArray_Mul(FDArray *fdarray, float val) {
   unsigned int i;
   float *arr = (float *)fdarray->elem;
      
   for (i=0; i<FDArray_NofElems(fdarray); i++) arr[i] *= val;
}

/*---------------------------------------------------------------------------*/

void    FDArray_Div(FDArray *fdarray, float val) {
   if (val == 0.0) val=1e-16;
   else if (val == -0.0) val=-1e-16;
   
   FDArray_Mul(fdarray, 1.0/val);
}

/*---------------------------------------------------------------------------*/

void    FDArray_Display(FDArray *fdarray, unsigned int n) {
   unsigned int i;
   for (i=0; i<n; i++) printf(" ");
   for (i=0; i<FDArray_NofElems(fdarray)-1; i++) {
      printf("%.2f ", FDArray_Elem(fdarray, i));
   }
   printf("%.2f\n", *(float *)DArray_Last(fdarray));
}

/*---------------------------------------------------------------------------*/

