
#ifndef _serialized_h
#define _serialized_h

#include "cmod.h"

#define SERIALIZED_HEADER 32

typedef struct {
   unsigned int total_size;
   unsigned int used_size;
   unsigned int current_ofs;
   char *int_size;
   char *byte_order;
   unsigned char *buf;
} Serialized;

typedef void (SerializeFunc)(void *, Serialized *);

Serialized    *Serialized_Create(void);
Serialized    *Serialized_CreateFromBuf(char *buf);
#define Serialized_CreateFromBuffer Serialized_CreateFromBuf
Serialized    *Serialized_Clone(Serialized *serialized);
Bool           Serialized_IsEqual(Serialized *serialized1, Serialized *serialized2);
void           Serialized_AppendData(Serialized *ser, void *from, unsigned int nof_bytes);
void           Serialized_ReadData(Serialized *ser, void *to_where, unsigned int nof_bytes);
#ifndef CMOD_ARCHITECTURE_LITTLEENDIAN
 #define       Serialized_AppendWrapped Serialized_AppendData
 #define       Serialized_ReadWrapped Serialized_ReadData
#else
 void          Serialized_AppendWrapped(Serialized *ser, void *from, unsigned int nof_bytes);
 void          Serialized_ReadWrapped(Serialized *ser, void *to_where, unsigned int nof_bytes);
#endif
unsigned char *Serialized_Buffer(Serialized *ser);
unsigned int   Serialized_BufferSize(Serialized *ser);
unsigned int   Serialized_BytesToRead(unsigned char *header);

void           Serialized_Rewind(Serialized *ser);
void           Serialized_Destroy(Serialized *serialized);

#ifdef CMOD_NOMAKEFILE
#include "serialized.c"
#endif
#endif

