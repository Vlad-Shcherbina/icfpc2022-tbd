
#include "cmod.h"
#include "darray.h"
#include "hashtable.h"
#include "list.h"
#include "memory.h"
#include "stdtypes.h"
#include <stdint.h>

/*===========================================================================
   HashElem methods
  ---------------------------------------------------------------------------*/
  
typedef struct {
   void *key;
   void *elem;
} HashElem;

/*---------------------------------------------------------------------------*/

HashElem *HashElem_Create(void *key, void *elem) {
   HashElem *he;
   he = Memory_Reserve(1, HashElem);
   he->key = key;
   he->elem = elem;
   return he;
}

/*---------------------------------------------------------------------------*/

HashElem *HashElem_Clone(HashElem *he, CloneFunc *KeyClone, CloneFunc *ElemClone) {
   HashElem *he2;
   
   he2 = Memory_Reserve(1, HashElem);
   if (KeyClone != NULL)
      he2->key = KeyClone(he->key);
   else
      he2->key = he->key;
      
   if (ElemClone != NULL)
      he2->elem = KeyClone(he->elem);
   else
      he2->elem = he->elem;
   
   return he2;
}

/*---------------------------------------------------------------------------*/

void HashElem_Destroy(HashElem *he, DestFunc *KeyDestroy, DestFunc *ElemDestroy) {

   if (KeyDestroy != NULL) 
      KeyDestroy(he->key);

   if (ElemDestroy != NULL) 
      ElemDestroy(he->elem);

   Memory_Free(he, HashElem);
}

/*===========================================================================
   HashTable methods
  ---------------------------------------------------------------------------*/

unsigned int HashTable_DefaultHash(void *elem) {
   intptr_t ptr;
   
   ptr = (intptr_t)elem;
   ptr = ptr ^ (ptr >> 5) ^ (ptr >> 9);
   return (unsigned int)ptr;
}

Bool HashTable_DefaultEqual(void *e1, void *e2) {
   return (e1 == e2);
}

/*---------------------------------------------------------------------------*/

HashTable *HashTable_Create(unsigned int initial_size, 
                            HashFunc *hashfunc, EqualFunc *equalfunc, 
                            DestFunc *keydestroy, DestFunc *elemdestroy) 
{
	HashTable *hashtable;
   unsigned int i;

	hashtable = Memory_Reserve(1, HashTable);

   if (initial_size <= 1) initial_size = 2;
   i=1;
   while (i<initial_size) i<<=1;
   
   hashtable->size = i;
   hashtable->nof_elems = 0;
   hashtable->table = Memory_Reserve(hashtable->size, List *);
   hashtable->rehashallowed = TRUE;
   hashtable->duplicatekeysallowed = FALSE;
   hashtable->maxload = 0.5;

   hashtable->keydestroy = (DestFunc *)keydestroy;
   hashtable->elemdestroy = (DestFunc *)elemdestroy;
   
   for (i=0; i<hashtable->size; i++) 
      hashtable->table[i] = List_Create();

   if (hashfunc == NULL) 
      hashtable->hashfunc = (HashFunc *)HashTable_DefaultHash;
   else
      hashtable->hashfunc = (HashFunc *)hashfunc;

   if (equalfunc == NULL) 
      hashtable->equalfunc = (EqualFunc *)HashTable_DefaultEqual;
   else
      hashtable->equalfunc = (EqualFunc *)equalfunc;

	return hashtable;
}

void HashTable_AllowDuplicateKeys(HashTable *hashtable, Bool val) {
   hashtable->duplicatekeysallowed = val;
}
/*---------------------------------------------------------------------------*/
#if 0 /* this was buggy! */
/* TODO: Not MT-safe! */
static CloneFunc *keyclone, *elemclone;

void *HashElem_Clon(void *he) {
   return HashElem_Clone(he, keyclone, elemclone);
}
#endif

List *HashTable_CloneList(List *list, CloneFunc *keyclone, CloneFunc *elemclone) {
   List *l;
   if (List_IsEmpty(list)) 
      return List_Create();
   else {
      l = HashTable_CloneList(List_Rest(list), keyclone, elemclone);
      return List_Insert(l, HashElem_Clone((HashElem *)List_First(list), keyclone, elemclone));
   }
}

HashTable *HashTable_Clone(HashTable *hashtable, CloneFunc *keyclone, CloneFunc *elemclone) {
	HashTable *hashtable2;
   unsigned int i;

   hashtable2 = Memory_Reserve(1, HashTable);

   hashtable2->size = hashtable->size;
   hashtable2->nof_elems = hashtable->nof_elems;
   hashtable2->maxload = hashtable->maxload;
   
   hashtable2->hashfunc = hashtable->hashfunc;
   hashtable2->equalfunc = hashtable->equalfunc;
   hashtable2->keydestroy = hashtable->keydestroy;
   hashtable2->elemdestroy = hashtable->elemdestroy;
   
   hashtable2->table = Memory_Reserve(hashtable2->size, List *);
   for (i=0; i<hashtable2->size; i++) {
      hashtable2->table[i] = HashTable_CloneList(hashtable->table[i], keyclone, elemclone);
   }
      
	return hashtable2;
}

/*---------------------------------------------------------------------------*/

DArray *HashTable_LookUpAll(HashTable *hashtable, void *key) {
   DArray *d;
   unsigned int ix;
   HashElem *e;
   List *l;
   
   d = DArray_Create(4);   
   ix = ((unsigned int)hashtable->hashfunc(key)) & (hashtable->size - 1);
   for (l = hashtable->table[ix]; !List_IsEmpty(l); l = List_Rest(l)) {
      e = (HashElem *)List_First(l);
      if (hashtable->equalfunc(key, e->key)) {
         DArray_Add(d, e->elem);
      }
   } 

   return d;
}

/*---------------------------------------------------------------------------*/

void      *HashTable_LookUp(HashTable *hashtable, void *key) {
   unsigned int ix;
   HashElem *e;
   List *l;
   
   ix = ((unsigned int)hashtable->hashfunc(key)) & (hashtable->size - 1);

   for (l = hashtable->table[ix]; !List_IsEmpty(l); l = List_Rest(l)) {
      e = (HashElem *)List_First(l);
      if (hashtable->equalfunc(key, e->key)) return e->elem;
   } 

   return NULL;
}

/*---------------------------------------------------------------------------*/

void      *HashTable_Remove(HashTable *hashtable, void *key) {
   int ix;
   HashElem *e;
   void *elem;
   List *l;
   
   ix = ((unsigned int)hashtable->hashfunc(key)) & (hashtable->size - 1);
   l = hashtable->table[ix];
   while (!List_IsEmpty(l)) {
      e = (HashElem *)List_First(l);
      if (hashtable->equalfunc(key, e->key)) {
         /* TODO? Use l instead?*/
         hashtable->table[ix] = List_Remove(hashtable->table[ix], e);
         hashtable->nof_elems--;
         elem = e->elem;
         HashElem_Destroy(e, hashtable->keydestroy, NULL);
         return elem;
      }
      l = List_Rest(l);
   } 
   return NULL;
}

/*---------------------------------------------------------------------------*/

void       HashTable_Rehash(HashTable *hashtable) {
   HashTable *ht2;
   unsigned int i;
   List *l;
   HashElem *he;
   List **oldtable;
   unsigned int oldsize;
   
   ht2 = HashTable_Create(hashtable->size * 2, 
      hashtable->hashfunc, hashtable->equalfunc, 
      hashtable->keydestroy, hashtable->elemdestroy);
   
   for (i=0; i<hashtable->size; i++) {
      for (l=hashtable->table[i]; !List_IsEmpty(l); l=List_Rest(l)) {
         he = (HashElem *)List_First(l);
         if (he != NULL) 
            HashTable_Insert(ht2, he->key, he->elem);
      }
   }

   oldsize = hashtable->size;
   oldtable = hashtable->table;
   
   /* TODO: potentially not MT-safe */
   hashtable->size = ht2->size;
   hashtable->table = ht2->table;
   hashtable->nof_elems = ht2->nof_elems;

   for (i=0; i<oldsize; i++) {
      for (l=oldtable[i]; !List_IsEmpty(l); l=List_Rest(l)) {
         he = (HashElem *)List_First(l);
         HashElem_Destroy(he, NULL, NULL);
      }
      List_Destroy(oldtable[i], NULL);
   }
   Memory_Free(oldtable, List *);
   Memory_Free(ht2, HashTable);
}

/*---------------------------------------------------------------------------*/

Bool HashTable_IsRehashNeeded(HashTable *hashtable) {
   return (hashtable->nof_elems >= hashtable->maxload * hashtable->size);
}

/*---------------------------------------------------------------------------*/

void       HashTable_Insert(HashTable *hashtable, void *key, void *elem) {
   int ix;
   List *l;
   HashElem *he;

   if (HashTable_IsRehashNeeded(hashtable) && hashtable->rehashallowed) {
      HashTable_Rehash(hashtable);
   }

   ix = ((unsigned int)hashtable->hashfunc(key)) & (hashtable->size - 1);
   if (!hashtable->duplicatekeysallowed) {
      for (l=hashtable->table[ix]; !List_IsEmpty(l); l = List_Rest(l)) {
         he = (HashElem *)List_First(l);
         if (he != NULL && hashtable->equalfunc(he->key, key)) {
            HashElem_Destroy(he, hashtable->keydestroy, hashtable->elemdestroy);
            List_SetFirst(l, HashElem_Create(key, elem));
            return;
         }
      }
   }
   hashtable->table[ix] = List_Insert(hashtable->table[ix], HashElem_Create(key, elem));
   hashtable->nof_elems++;
}

/*---------------------------------------------------------------------------*/

void       HashTable_SetMaxLoad(HashTable *hashtable, double maxload) {
   hashtable->maxload = maxload;
}

/*---------------------------------------------------------------------------*/

Bool       HashTable_IsRehashAllowed(HashTable *hashtable) {
   return hashtable->rehashallowed;
}

/*---------------------------------------------------------------------------*/

void       HashTable_SetRehashAllowed(HashTable *hashtable, Bool val) {
   hashtable->rehashallowed = val;
}

/*---------------------------------------------------------------------------*/

DArray    *HashTable_InsertedKeys(HashTable *hashtable) {
   DArray *da;
   List *l;
   HashElem *he;
   unsigned int i;
   
   da = DArray_Create(hashtable->size / 2);
   for (i=0; i<hashtable->size; i++) {
      for (l=hashtable->table[i]; !List_IsEmpty(l); l = List_Rest(l)) {
         he = (HashElem *)List_First(l);
         DArray_Add(da, he->key);
      }
   }
   return da;
}

/*---------------------------------------------------------------------------*/

void HashTable_Destroy(HashTable *hashtable) {
   unsigned int i;
   List *l;
   
   for (i=0; i<hashtable->size; i++) {
   	   for (l=hashtable->table[i]; !List_IsEmpty(l); l=List_Rest(l)) {
           HashElem_Destroy((HashElem *)List_First(l), hashtable->keydestroy, hashtable->elemdestroy);
       }
       List_Destroy(hashtable->table[i],  NULL);
   }

   Memory_Free(hashtable->table, List *);
   
	Memory_Free(hashtable, HashTable);
}

/*---------------------------------------------------------------------------*/

/* This uses Robert Jenkins hasvalue generator from www.burtleburtle.net/bob */

#define HASH_MIX(a,b,c) { \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

typedef unsigned long int ub4;

unsigned int HashTable_BlockHashValue(unsigned char *k, unsigned int len) {
   register unsigned int a, b, c, length;

   /* Set up the internal state */
   length = len;
   a = b = c = 0x9e3779b9;  /* the golden ratio; an arbitrary value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12) {
      a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
      b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
      c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
      HASH_MIX(a,b,c);
      k += 12; 
      len -= 12;
   }

   /* handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((ub4)k[10]<<24);
   case 10: c+=((ub4)k[9]<<16);
   case 9 : c+=((ub4)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((ub4)k[7]<<24);
   case 7 : b+=((ub4)k[6]<<16);
   case 6 : b+=((ub4)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((ub4)k[3]<<24);
   case 3 : a+=((ub4)k[2]<<16);
   case 2 : a+=((ub4)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   HASH_MIX(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}

/*---------------------------------------------------------------------------*/

#ifdef CMOD_DEBUG_OUTPUT
static DispFunc *keydisp, *elemdisp;

#include <stdio.h>
void HashElem_Display(HashElem *he, int n, DispFunc *keydisp, DispFunc *elemdisp) {
   int i;
   
   for (i=0; i<n; i++) printf(" ");
   printf("[ Key = "); 
   keydisp(he->key, 0);
   printf(", Elem = ");
   elemdisp(he->elem, 0);
   printf("]");
}

/*---------------------------------------------------------------------------*/

void HashElem_Disp(HashElem *he, int n) { HashElem_Display(he, 0, keydisp, elemdisp); }

/*---------------------------------------------------------------------------*/

void HashTable_Display(HashTable *hashtable, int n, DispFunc *keydisp, DispFunc *elemdisp) {
   int i, j;
   List *l;
  
   for (i=0; i<n; i++) printf(" "); 
   printf("HashTable, size = %i, nof_elems = %i\n", hashtable->size, hashtable->nof_elems);
   for (i=0; i<n; i++) printf(" ");
   printf("HashCode     Element list\n");

   for (i=0; i<hashtable->size; i++) {
      if (!List_IsEmpty(hashtable->table[i])) {
         for (j=0; j<n; j++) printf(" ");
         printf("%6i       ",i);
         for (l=hashtable->table[i]; !List_IsEmpty(l); l=List_Rest(l)) {
            HashElem_Display((HashElem *)List_First(l), n, keydisp, elemdisp);
         }
//         List_Display(hashtable->table[i], HashElem_Disp);
         
         printf("\n");
      }
   }

   printf("\n");
}
#endif

