
#ifndef _hashtable_h
#define _hashtable_h

#include "cmod.h"
#include "darray.h"
#include "list.h"
#include "stdtypes.h"

typedef struct {
   unsigned int size;
   unsigned int nof_elems;
   double       maxload;
   Bool         duplicatekeysallowed;
   Bool         rehashallowed;
   HashFunc    *hashfunc;
   EqualFunc   *equalfunc;
   DestFunc    *keydestroy;
   DestFunc    *elemdestroy;
   List       **table;
} HashTable;

HashTable *HashTable_Create(unsigned int initial_size, 
                            HashFunc *hashfunc, EqualFunc *equalfunc, 
                            DestFunc *keydestroy, DestFunc *elemdestroy);
HashTable *HashTable_Clone(HashTable *hashtable, CloneFunc *keyclone, CloneFunc *elemclone);

void       HashTable_AllowDuplicateKeys(HashTable *hashtable, Bool val); // use before inserting if allowing duplicate keys
DArray    *HashTable_LookUpAll(HashTable *hashtable, void *key); // return all elems that uses key
void      *HashTable_LookUp(HashTable *hashtable, void *key); // NULL if not present
void      *HashTable_Remove(HashTable *hashtable, void *key);
void       HashTable_Insert(HashTable *hashtable, void *key, void *elem);
void       HashTable_Rehash(HashTable *hashtable); // forces rehash with twice the capacity
Bool       HashTable_IsRehashNeeded(HashTable *hashtable);

void       HashTable_SetMaxLoad(HashTable *hashtable, double maxload);
Bool       HashTable_IsRehashAllowed(HashTable *hashtable);
void       HashTable_SetRehashAllowed(HashTable *hashtable, Bool val);
DArray    *HashTable_InsertedKeys(HashTable *hashtable);

void       HashTable_Destroy(HashTable *hashtable);

unsigned int HashTable_BlockHashValue(unsigned char *k, unsigned int len);

#ifdef CMOD_DEBUG_OUTPUT
void       HashTable_Display(HashTable *ht, int nof_spaces, DispFunc *keydisp, DispFunc *elemdisp);
#endif

#ifdef CMOD_NOMAKEFILE
#include "hashtable.c"
#endif
#endif

