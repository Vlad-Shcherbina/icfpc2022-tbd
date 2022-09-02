/*****************************************************************************
 * Fil            : memory.h
 * Författare     : Johan Köpman & Tapani
 * Skapad         : 1996-10-01
 * Senast ändrad  : 2003-01-21
 * Beskrivning    : Abstraktion för minnesallokering..
 *****************************************************************************/

#include "cmod.h"
#include "memory.h"
#include "stdtypes.h"
#include "stdfunc.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

extern Bool std_htmlmode;

#ifdef MSDOS
#include <mem.h>
#endif

#ifdef CMOD_OS_LINUX
#include <signal.h>
#endif

#ifdef CMOD_DEBUG_MEMORY
/* -------------------------------------------------------------------------- */
#if 0
void Error(char* format, ...) {
  va_list   params;

  va_start(params, format);
  vprintf(format, params);
  va_end(params);

}

/* -------------------------------------------------------------------------- */

void Error_Fatal(char* format, ...) {
  va_list   params;

  va_start(params, format);
  vprintf(format, params);
  va_end(params);

  exit(-1);
}
#endif
/* -------------------------------------------------------------------------- */

__attribute__((malloc))
void *Memory_NormalAlloc(int size) { return malloc(size); }

/* -------------------------------------------------------------------------- */

void  Memory_NormalDealloc(void *ptr) { free(ptr); }

/* -------------------------------------------------------------------------- */

__attribute__((malloc))
void *Memory_Alloc(int size) {
  void *ptr;
  
  ptr = malloc(size);
  
  if (ptr == NULL) Error_Fatal("Can't allocate %d bytes", size);
  
  return ptr;
}

/* -------------------------------------------------------------------------- */

void Memory_Dealloc(void *ptr) { if (ptr != NULL) free(ptr); }
//void Memory_Dealloc(void *ptr) { ; }

void *Memory_Copy(void *dest, const void *src, int n) { return memcpy(dest, src, n); }

void *Memory_Move(void *dest, const void *src, int n) { return memmove(dest, src, n); }

void *Memory_NormalRealloc(void *ptr, int size) { return realloc(ptr, size); }

void  Memory_Set(void *dest, int n, char data) { memset(dest, data, n); }

void  Memory_Clear(void *dest, int n) { memset(dest, 0, n); }

__attribute__((malloc))
void *Memory_AllocAligned(int aligned, int size) { return memalign(aligned, size); }

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

			/*  Memory Debug Functions */

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
#endif

#ifdef CMOD_DEBUG_MEMORY

#include "darray.h"
#include "hashtable.h"
#include "string.h"
#include "stdtypes.h"

#include <string.h>

#define MEMORY_MAX_TYPES 2000

typedef struct {
   unsigned short   allocation_ix;
   int              size;
} MemoryDebugAllocation;

/* -------------------------------------------------------------------------- */

MemoryDebugAllocation *MemoryDebugAllocation_Create(int alloc_ix, int size) {
   MemoryDebugAllocation *m;
   m = (MemoryDebugAllocation *)malloc(sizeof(MemoryDebugAllocation));
   m->allocation_ix = alloc_ix;
   m->size = size;
   return m;
}

/* -------------------------------------------------------------------------- */

void MemoryDebugAllocation_Destroy(MemoryDebugAllocation *m) { free(m); }

/* -------------------------------------------------------------------------- */

void MemoryDebugAllocation_Display(MemoryDebugAllocation *m, int n) {
   int i;
   for (i=0; i<n; i++) printf(" ");
   printf("aix = %i, size = %i\n", m->allocation_ix, m->size);
}

/* ========================================================================== */

typedef struct {
   HashTable *ht;
   int nof_allocs;
   int nof_deallocs;
   int peak_usage;
} MemoryDebugObjectInfo;

/* -------------------------------------------------------------------------- */

unsigned int ptrhashval(const void *ptr) {
   return HashTable_BlockHashValue((unsigned char *)&ptr, sizeof(void *));
}

Bool ptreq(const void *a, const void *b) { return (a == b); }
void ptrdisplay(void *a, int n) { 
   int i;
   for (i=0; i<n; i++) printf(" ");
   printf("%p", a); 
}

/* -------------------------------------------------------------------------- */

MemoryDebugObjectInfo *MemoryDebugObjectInfo_Create(void) {
   MemoryDebugObjectInfo *m;
   m = (MemoryDebugObjectInfo *)malloc(sizeof(MemoryDebugObjectInfo));
   m->ht = HashTable_Create(16, ptrhashval, ptreq, (DestFunc *)NULL, (DestFunc *)MemoryDebugAllocation_Destroy);
   m->nof_allocs = 0;
   m->nof_deallocs = 0;
   m->peak_usage = 0;
   return m;
}

/* -------------------------------------------------------------------------- */

void MemoryDebugObjectInfo_Destroy(MemoryDebugObjectInfo *m) {
   HashTable_Destroy(m->ht);
   free(m);
}

/* -------------------------------------------------------------------------- */
#ifdef CMOD_DEBUG_OUTPUT
void MemoryDebugObjectInfo_Display(MemoryDebugObjectInfo *m, int n) {
   int i;

   for (i=0; i<n; i++) printf(" ");
   printf("(");
   HashTable_Display(m->ht, 0, ptrdisplay, (DispFunc *)MemoryDebugAllocation_Display);
   printf(") %i %i %i ------------------------------\n", m->nof_allocs, m->nof_deallocs, m->peak_usage);
}
#endif

/* ========================================================================== */

typedef struct {
   int  mem_current_usage;
   int  mem_peak_usage;
   long long mem_total_allocated;
   long long mem_total_deallocated;

   int  nof_allocs;
   int  nof_deallocs;
   
   HashTable *allocated_objects; /* HashTable: String -> (HashTable: void * -> MemAlloc) */
   
   /* row:col stuff */
   DArray    *allocating_function;
   HashTable *allocating_function_ix;
   
   FILE      *output;
} MemoryDebugInfo;

MemoryDebugInfo memory_debug_info;
Bool            memory_debug_initialized = FALSE;

/* -------------------------------------------------------------------------- */

void Memory_SigSegvHandler(int sig) {
   if (!std_htmlmode)
      Std_StackDumpFull(memory_debug_info.output, "Segmentation fault", ANSI_RED ANSI_BRIGHT, 0, 9999, TRUE);
   else
      Std_StackDumpFull(memory_debug_info.output, "Segmentation fault", "<font color=\"red\"><pre>", 0, 9999, TRUE);
   exit(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------- */

#ifdef CMOD_OS_LINUX
void Memory_SetSigSegvHandler() { signal(SIGSEGV, Memory_SigSegvHandler); }
#else
void Memory_SetSigSegvHandler() { }
#endif

/* -------------------------------------------------------------------------- */

void Memory_DebugInit(FILE *output) {
   memory_debug_initialized = FALSE;
   memory_debug_info.output = (output==NULL) ? stderr : output;
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Memory debugging initialing...");
   fflush(memory_debug_info.output);
#endif
   memory_debug_info.mem_current_usage = 0;
   memory_debug_info.mem_peak_usage = 0;
   memory_debug_info.mem_total_allocated = 0;
   memory_debug_info.mem_total_deallocated = 0;
   memory_debug_info.nof_allocs = 0;
   memory_debug_info.nof_deallocs = 0;

   Memory_SetSigSegvHandler();

   memory_debug_info.allocated_objects = 
      HashTable_Create(32, (HashFunc *)String_HashVal, (EqualFunc *)String_IsEqual, (DestFunc *)String_Destroy,(DestFunc *)MemoryDebugObjectInfo_Destroy);
//   memory_debug_info.object_names = DArray_Create(16);

   memory_debug_info.allocating_function = DArray_Create(16);
   memory_debug_info.allocating_function_ix = 
      HashTable_Create(32, (HashFunc *)String_HashVal, (EqualFunc *)String_IsEqual, (DestFunc *)String_Destroy, NULL);
   
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Memory debugging initialized\n");
   fflush(memory_debug_info.output);
#endif
   
   memory_debug_initialized = TRUE;
}

/* -------------------------------------------------------------------------- */

void *Memory_DebugRealloc(void *ptr, int size, char *name, char *fname, int row) {
   void *newptr;
   MemoryDebugObjectInfo *mdoi;
   MemoryDebugAllocation *m;

#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Realloc(%x, %i, %s, %s, %i)\n", ptr, size, name, fname, row);
   fflush(memory_debug_info.output);
#endif

   if (!memory_debug_initialized) return Memory_NormalRealloc(ptr, size);
      
   newptr = Memory_DebugAlloc2(size, name, __FILE__, __LINE__);

   if (ptr == NULL) return newptr;
   
   mdoi = (MemoryDebugObjectInfo *)HashTable_LookUp(memory_debug_info.allocated_objects, name);
   if (mdoi == NULL) {
#ifndef CMOD_ALLOW_EXIT
   	Std_StackDump("Memory error?");
      fprintf(memory_debug_info.output, "Wrong type or non-allocated block passed to realloc from %s:%i\n",fname,row);      
      return newptr;
#else
   	Std_StackDump("Memory error?");
      Error_Fatal("Wrong type or non-allocated block passed to realloc from %s:%i\n",fname,row);
#endif
   }
      
   m = (MemoryDebugAllocation *)HashTable_LookUp(mdoi->ht, ptr);
   if (m == NULL) {
#ifdef CMOD_DEBUG_OUTPUT
      HashTable_Display(memory_debug_info.allocated_objects, 0, (DispFunc *)String_Display, (DispFunc *)MemoryDebugObjectInfo_Display);
#endif
#ifndef CMOD_ALLOW_EXIT
   	Std_StackDump("Memory error?");
   	fprintf(memory_debug_info.output, "Non-allocated block passed to realloc from %s:%i\n",fname,row);
      return newptr;
#else
   	Std_StackDump("Memory error?");
      Error_Fatal("Non-allocated block passed to realloc from %s:%i\n",fname,row);
#endif
   }
   
   Memory_Copy(newptr, ptr, MIN(m->size, size));
   Memory_DebugDealloc2(ptr, name, __FILE__, __LINE__);
   return newptr;
}

/* -------------------------------------------------------------------------- */

void *Memory_DebugCalloc(int size, char* name, char *fname, int row) {
   void *ret = Memory_DebugAlloc2(size, name, fname, row);
   Memory_Clear(ret, size);
   return ret;
}

/* -------------------------------------------------------------------------- */

__attribute__((malloc))
void *Memory_DebugAlloc2(int size, char* name, char *fname, int row) {
   void *ptr;
   MemoryDebugObjectInfo *mdoi;
   MemoryDebugAllocation *m;
   int ix;
   String *allocation;

#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Alloc(%i, %s, %s, %i) total = %i\n", size, name, fname, row
   , memory_debug_info.mem_total_allocated);
   fflush(memory_debug_info.output);
#endif
         
   ptr = malloc(size);

#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Allocation at %x\n", ptr);
   fflush(memory_debug_info.output);
#endif

   if (ptr == NULL) {
#ifndef CMOD_ALLOW_EXIT
   	fprintf(memory_debug_info.output, "Can't allocate %d bytes of type '%s' at %s:%i!", size, name, fname, row);
      fflush(memory_debug_info.output);
      return ptr;
#else
      Error_Fatal("Can't allocate %d bytes of type '%s' at %s:%i!", size, name, fname, row);
#endif
   }
      
   if (!memory_debug_initialized) return ptr;

   memory_debug_initialized = FALSE;
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Internal mode ON\n");
#endif
/*
   if (memory_debug_info.allocated_object->table[24] != NULL)
      mdoi = (MemoryDebugObjectInfo *)memory_debug_info.allocated_objects->table[24]->first;
      if (mdoi != NULL)
         fprintf(stderr, "%x %i %i\n", mdoi->ht, mdoi->nof_allocs, mdoi->nof_deallocs);

   mdoi = NULL;
*/
//   HashTable_Display(memory_debug_info.allocated_objects, String_Display, MemoryDebugObjectInfo_Display);
   /* update general vars */
   memory_debug_info.mem_total_allocated += size;   
   memory_debug_info.mem_current_usage += size;
   if (memory_debug_info.mem_current_usage > memory_debug_info.mem_peak_usage)
      memory_debug_info.mem_peak_usage = memory_debug_info.mem_current_usage;
      
   memory_debug_info.nof_allocs++;
   
   /* TODO: NOT MT-safe!!! <*/
   /* Check if we have allocated mem at this file:row before */
   allocation = (char *)malloc((21+strlen(fname))*sizeof(char));
   sprintf(allocation, "%s:%i", fname, row);   
   /* "allocation" holds a string containing file:row */

   void *aix = HashTable_LookUp(memory_debug_info.allocating_function_ix, allocation);
   if (aix == NULL) {
      ix = DArray_NofElems(memory_debug_info.allocating_function);
      DArray_Add(memory_debug_info.allocating_function, allocation);
      HashTable_Insert(memory_debug_info.allocating_function_ix, allocation, (void *)ix);
   } else {
      ix = (long)aix;
      free(allocation);
   }
         
   m = MemoryDebugAllocation_Create(ix, size);

   /* Check if we have got allocations of this object type before */
   mdoi = (MemoryDebugObjectInfo *)HashTable_LookUp(memory_debug_info.allocated_objects, name);
   if (mdoi == NULL) {
   	mdoi = MemoryDebugObjectInfo_Create();      
      HashTable_Insert(memory_debug_info.allocated_objects, String_Clone(name), mdoi);
/*
      if (mdoi->nof_allocs > 100000) fprintf(stderr, "Bad mdoi!\n");
   } else 
      if (mdoi->nof_allocs > 100000) {
         fprintf(stderr, "HERE!\n");
         fprintf(stderr, "name = %s\n", name);
         
         HashTable_Display(memory_debug_info.allocated_objects, String_Display, MemoryDebugObjectInfo_Display);

         MemoryDebugObjectInfo_Display(mdoi);
*/
      }

   HashTable_Insert(mdoi->ht, ptr, m);
   mdoi->nof_allocs++;
   if (mdoi->nof_allocs - mdoi->nof_deallocs > mdoi->peak_usage)
      mdoi->peak_usage = mdoi->nof_allocs - mdoi->nof_deallocs;
   memory_debug_initialized = TRUE; // hashtable_insert creates inf of elems otherwise
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Internal mode OFF\n");
#endif
   return ptr;
}

/* -------------------------------------------------------------------------- */

void  Memory_DebugDealloc2(void *ptr, char* name, char *fname, int row) {
   int size;
   MemoryDebugAllocation *m;  
   MemoryDebugObjectInfo *mdoi;
   char message[256];

#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Dealloc(%x, %s, %s, %i)\n", ptr, name, fname, row);
   fflush(memory_debug_info.output);
#endif
   
   if (!memory_debug_initialized) {
      free(ptr);
      return;
   }

   memory_debug_initialized = FALSE;
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Internal mode ON\n");
#endif
   mdoi = (MemoryDebugObjectInfo *)HashTable_LookUp(memory_debug_info.allocated_objects, name);
   if (mdoi == NULL) {
#ifndef CMOD_ALLOW_EXIT
//   	fprintf(memory_debug_info.output, "Deallocation of a non allocated \"%s\" at %s:%i!", name, fname, row);
   	sprintf(message, "Deallocation of non allocated \"%s *\" %p", name, ptr);
      if (!std_htmlmode)
         Std_StackDumpFull(memory_debug_info.output, message, ANSI_BLUE ANSI_BRIGHT, 1, 99, TRUE);
      else
         Std_StackDumpFull(memory_debug_info.output, message, "<font color=\"blue\"><pre>", 1, 99, TRUE);
      fflush(memory_debug_info.output);
      Std_StackDump("Double free");
      return;
#else
      Error_Fatal("Deallocation of a non allocated \"%s\" at %s:%i!", name, fname, row);
#endif
      
      return;
   }
   mdoi->nof_deallocs++;
   
   m = (MemoryDebugAllocation *)HashTable_Remove(mdoi->ht, ptr);
#ifdef CMOD_DEBUG_OUTPUT
   if (m == NULL) {
      HashTable_Display(memory_debug_info.allocated_objects, 0, (DispFunc *)String_Display, (DispFunc *)MemoryDebugObjectInfo_Display);
   }
#endif
   if (m == NULL) {
#ifndef CMOD_ALLOW_EXIT
   	sprintf(message, "Deallocation of non allocated \"%s *\" %p", name, ptr);
      if (!std_htmlmode)
      Std_StackDumpFull(memory_debug_info.output, message, ANSI_BRIGHT ANSI_BLUE, 1, 99, TRUE);
      else
   	Std_StackDumpFull(memory_debug_info.output, message, "<font color=\"blue\"><pre>", 1, 99, TRUE);

//   	fprintf(memory_debug_info.output, "Deallocation of a non allocated \"%s\" *%p at %s:%i!", name, ptr, fname, row);
//      fflush(memory_debug_info.output);
      return;
#else
      Error_Fatal("Deallocation of a non allocated \"%s\" at %s:%i!", name, fname, row);
#endif      
   }
   size = m->size;

#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Deallocation of %i bytes\n", size);
#endif
   
   memory_debug_info.mem_total_deallocated += size;   
   memory_debug_info.mem_current_usage -= size;
   memory_debug_info.nof_deallocs++;   
   memory_debug_initialized = TRUE;
#ifdef CMOD_DEBUG_MEMORY_ALL
   fprintf(memory_debug_info.output, "Internal mode OFF\n");
#endif

   if (ptr != NULL)
      free(ptr);
   MemoryDebugAllocation_Destroy(m);
}

/* -------------------------------------------------------------------------- */

#ifndef CMOD_NOMAKEFILE
typedef struct {
   void *key;
   void *elem;
} HashElem;
#endif

void Memory_DumpUnallocatedBlocks(void) {
   DArray *da;
   int i,j, k;
   MemoryDebugObjectInfo *mdoi;
   void *key;
   HashTable *ht;
   List *l;
   MemoryDebugAllocation *mda;
   HashElem *he;
   
   fprintf(memory_debug_info.output, "Unallocated memory blocks:\n----------------------------\n");
   da = HashTable_InsertedKeys(memory_debug_info.allocated_objects);
   for (i=0; i<DArray_NofElems(da); i++) {
      key = DArray_Elem(da, i);
      mdoi = (MemoryDebugObjectInfo *)HashTable_LookUp(memory_debug_info.allocated_objects, key);
      ht = mdoi->ht;

      for (j=0; j<ht->size; j++) {
         if (!List_IsEmpty(ht->table[j])) {
            for (l=ht->table[j]; !List_IsEmpty(l); l=List_Rest(l)) {
               he = (HashElem *)List_Peek(l);
               fprintf(memory_debug_info.output, "%s %p:", (String *)key, he->key);
               if (String_IsEqual((String *)key, "String")) {
                  fprintf(memory_debug_info.output, "%s\n", (String *)he->key);
               } else {
                  mda = (MemoryDebugAllocation *)he->elem;
                  for (k=0; k<mda->size; k++) {
                     fprintf(memory_debug_info.output, "%02X ", *(((unsigned char *)he->key) + k));
                     if ((k&15) == 15) fprintf(memory_debug_info.output, "\n");
                  }
                  if ((k&15) != 0) fprintf(memory_debug_info.output, "\n");
               }
            }
         }
      }
   }

   fprintf(memory_debug_info.output, "\n");
}
/* -------------------------------------------------------------------------- */


void Memory_DebugRestore(void) {
   int i;
   DArray *da;
   void *key;
   MemoryDebugObjectInfo *mdoi;
   FILE *out;
   
   memory_debug_initialized = FALSE;
   out = memory_debug_info.output;
#ifdef CMOD_MEMORY_DEBUG_DUMPUNALLOCATED
   Memory_DumpUnallocatedBlocks();
#endif
   if (std_htmlmode) fprintf(out, "<p><pre>\n");
   fprintf(out, "\nMemory allocated (total)  : %lli\n", memory_debug_info.mem_total_allocated);
   fprintf(out, "Memory deallocated (total): %lli\n", memory_debug_info.mem_total_deallocated);
   fprintf(out, "Memory peak usage         : %i\n", memory_debug_info.mem_peak_usage);
   fprintf(out, "+----------------+-------------+-------------+-------------+-------------+\n");
   fprintf(out, "| Object         | Peak        | Allocs      | Deallocs    | Difference  |\n");
   fprintf(out, "+----------------+-------------+-------------+-------------+-------------+\n");

   da = HashTable_InsertedKeys(memory_debug_info.allocated_objects);
   for (i=0; i<DArray_NofElems(da); i++) {
      key = DArray_Elem(da, i);
      mdoi = (MemoryDebugObjectInfo *)HashTable_LookUp(memory_debug_info.allocated_objects, key);
      fprintf(out, "| %-15s| %11d | %11d | %11d | %11d |\n", 
         (String *)key,
         mdoi->peak_usage,
         mdoi->nof_allocs,
         mdoi->nof_deallocs,
         mdoi->nof_allocs - mdoi->nof_deallocs);
   }
   fprintf(out, "+----------------+-------------+-------------+-------------+-------------+\n");
   if (std_htmlmode) fprintf(out, "</pre>\n");

   fflush(memory_debug_info.output);

/*
 
   for (i = 0; i < memory_debug_info.nof_funcs; i++) {
      printf("| %-15s| %11d | %11d | %11d | %11d |\n", 
         memory_debug_info.funcs[i], 
         memory_debug_info.peak_allocs[i], 
         memory_debug_info.called[i], 
         memory_debug_info.freed[i], 
         memory_debug_info.called[i] - memory_debug_info.freed[i]);
   }
   
   printf("+----------------+-------------+-------------+-------------+-------------+\n");
   printf("| %-15s|             | %11d | %11d | %11d |\n", "Total sum",
	   memory_debug_info.memory_usage[0], 
      memory_debug_info.memory_usage[1], 
      memory_debug_info.memory_usage[0] - memory_debug_info.memory_usage[1]);

   printf("+----------------+-------------+-------------+-------------+-------------+\n");

   printf("| %-15s | %11d | %11d | %11d | %11d |\n", "In bytes",
       memory_debug_info.peak_allocated_bytes,
       memory_debug_info.current_allocated_bytes,
       memory_debug_info.total_allocated_bytes,
       memory_debug_info.total_freed_bytes);

   printf("+----------------+-------------+-------------+-------------+-------------+\n");
*/
   DArray_Destroy(da, NULL);
   HashTable_Destroy(memory_debug_info.allocated_objects);
   DArray_Destroy(memory_debug_info.allocating_function, NULL);
   HashTable_Destroy(memory_debug_info.allocating_function_ix);
}

/* -------------------------------------------------------------------------- */

int Memory_DebugNofAllocs(void) {
   return memory_debug_info.nof_allocs;
}

/* -------------------------------------------------------------------------- */

int Memory_DebugNofDeallocs(void) {
   return memory_debug_info.nof_deallocs;
}

/* -------------------------------------------------------------------------- */

int Memory_DebugCurrentUsage(void) {
   return memory_debug_info.mem_current_usage;
}

/* -------------------------------------------------------------------------- */

#endif

