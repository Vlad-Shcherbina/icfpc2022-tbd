
#include "cmod.h"
#include "memory.h"
#include "serialized.h"

#define SERIALIZED_HEADER 32
#define SERIALIZED_INITIALSIZE (256 + SERIALIZED_HEADER)
#define SERIALIZED_OFS_INTSIZE 0
#define SERIALIZED_OFS_BYTEORDER sizeof(char)
#define SERIALIZED_OFS_USEDSIZE (2*sizeof(char))

/*-------------------------------------------------------------------------*/

void Serialized_SetPointers(Serialized *serialized) {
   serialized->int_size = (char *)serialized->buf;
   serialized->byte_order = (char *)(serialized->buf + sizeof(*serialized->int_size));
}

/*-------------------------------------------------------------------------*/

Serialized *Serialized_Create() {
   Serialized *serialized;

	serialized = Memory_Reserve(1, Serialized);
   serialized->total_size = SERIALIZED_INITIALSIZE;
   serialized->buf = Memory_Reserve(SERIALIZED_INITIALSIZE, unsigned char);

   Serialized_SetPointers(serialized);

#ifdef CMOD_ARCHITECTURE_BIGENDIAN
	*serialized->byte_order = 0;
#else
  	*serialized->byte_order = 1;
#endif
  	*serialized->int_size = sizeof(int);

   serialized->used_size = SERIALIZED_HEADER;
   serialized->current_ofs = SERIALIZED_HEADER;
   
	return serialized;
}

/*-------------------------------------------------------------------------*/

Serialized *Serialized_CreateFromBuf(char *buf) {
   Serialized *serialized;

   if (buf == NULL) {
      return Serialized_Create();
   }
   
   serialized = Memory_Reserve(1, Serialized);
   serialized->buf = (unsigned char *)buf;
   serialized->current_ofs = SERIALIZED_OFS_USEDSIZE;

   Serialized_ReadWrapped(serialized, &serialized->used_size, sizeof(serialized->used_size));
   serialized->total_size = serialized->used_size;
   serialized->current_ofs = SERIALIZED_HEADER;

   serialized->buf = Memory_Reserve(serialized->total_size, unsigned char);
   Memory_Copy(serialized->buf, buf, serialized->used_size);
      
   Serialized_SetPointers(serialized);   
   
	return serialized;
}

/*-------------------------------------------------------------------------*/

Serialized *Serialized_Clone(Serialized *serialized) {
	Serialized *serialized2;

	serialized2 = Memory_Reserve(1, Serialized);
   Serialized_SetPointers(serialized2);
   serialized2->total_size = serialized->total_size;
   serialized2->buf = Memory_Reserve(serialized2->total_size, unsigned char);   
   *serialized2->byte_order = *serialized->byte_order;
   *serialized2->int_size = *serialized->int_size;
   serialized2->used_size = serialized->used_size;
   serialized2->current_ofs = serialized->current_ofs;
   
	Memory_Copy(serialized2->buf, serialized->buf, serialized2->current_ofs);

	return serialized2;
}

/*-------------------------------------------------------------------------*/

Bool Serialized_IsEqual(Serialized *serialized1, Serialized *serialized2) {

	return TRUE;
}

/*-------------------------------------------------------------------------*/

void Serialized_Grow(Serialized *ser, float factor) {
   unsigned char *buf2;
   /* TODO: Use realloc? */
   buf2 = Memory_Reserve(ser->total_size * factor, unsigned char);
   Memory_Copy(buf2, ser->buf, ser->current_ofs);
   
   Serialized_SetPointers(ser);
   Memory_Free(ser->buf, unsigned char);
   ser->buf = buf2;
   ser->total_size *= factor;
}

/*-------------------------------------------------------------------------*/

void Serialized_AppendData(Serialized *ser, void *from, unsigned int nof_bytes) {
   float f;
   
   if (ser->current_ofs + nof_bytes > ser->total_size) {
      f = 1.75;
      while(ser->total_size * f < ser->current_ofs + nof_bytes) f*=1.75;
      Serialized_Grow(ser, f);
   }
   
   Memory_Copy(&ser->buf[ser->current_ofs], from, nof_bytes);
//   printf("appended %i bytes to ofs %i\n", nof_bytes, ser->current_ofs);
   ser->current_ofs+=nof_bytes;
}

/*-------------------------------------------------------------------------*/

void Serialized_ReadData(Serialized *ser, void *to_where, unsigned int nof_bytes) {
   Memory_Copy(to_where, &ser->buf[ser->current_ofs], nof_bytes);
   ser->current_ofs+=nof_bytes;
}

/*-------------------------------------------------------------------------*/

#ifdef CMOD_ARCHITECTURE_LITTLEENDIAN
void Serialized_AppendWrapped(Serialized *ser, void *from, int nof_bytes) {
   int i;
   if (ser->current_ofs + nof_bytes > ser->total_size) 
      Serialized_Grow(ser, 1.5);
   
   //Memory_Copy(&ser->buf[ser->current_ofs], from, nof_bytes);
   switch(nof_bytes) {
   case sizeof(short):
      ser->buf[ser->current_ofs] = *((char *)from + 1);
      ser->buf[ser->current_ofs + 1] = *(char *)from;
      break;
   case sizeof(long):
      ser->buf[ser->current_ofs] = *((char *)from + 3);
      ser->buf[ser->current_ofs + 1] = *((char *)from + 2);
      ser->buf[ser->current_ofs + 2] = *((char *)from + 1);
      ser->buf[ser->current_ofs + 3] = *(char *)from;
      break;
   case sizeof(long long):
      ser->buf[ser->current_ofs] = *((char *)from + 7);
      ser->buf[ser->current_ofs + 1] = *((char *)from + 6);
      ser->buf[ser->current_ofs + 2] = *((char *)from + 5);
      ser->buf[ser->current_ofs + 3] = *((char *)from + 4);
      ser->buf[ser->current_ofs + 4] = *((char *)from + 3);
      ser->buf[ser->current_ofs + 5] = *((char *)from + 2);
      ser->buf[ser->current_ofs + 6] = *((char *)from + 1);
      ser->buf[ser->current_ofs + 7] = *(char *)from;
      break;
   default:
      for (i=0; i<nof_bytes; i++) {
         ser->buf[ser->current_ofs + i] = *(char *)from + nof_bytes - i - 1;
      }
   }
   ser->current_ofs+=nof_bytes;
}

/*-------------------------------------------------------------------------*/

void Serialized_ReadWrapped(Serialized *ser, void *to_where, int nof_bytes) {
   int i;
   switch(nof_bytes) {
   case sizeof(short):
      *(char *)to_where = ser->buf[ser->current_ofs + 1];
      *((char *)to_where + 1) = ser->buf[ser->current_ofs];
      break;
   case sizeof(long):
      *(char *)to_where = ser->buf[ser->current_ofs + 3];
      *((char *)to_where + 1) = ser->buf[ser->current_ofs + 2];
      *((char *)to_where + 2) = ser->buf[ser->current_ofs + 1];
      *((char *)to_where + 3) = ser->buf[ser->current_ofs];
      break;
   case sizeof(long long):
      *(char *)to_where = ser->buf[ser->current_ofs + 7];
      *((char *)to_where + 1) = ser->buf[ser->current_ofs + 6];
      *((char *)to_where + 2) = ser->buf[ser->current_ofs + 5];
      *((char *)to_where + 3) = ser->buf[ser->current_ofs + 4];
      *((char *)to_where + 4) = ser->buf[ser->current_ofs + 3];
      *((char *)to_where + 5) = ser->buf[ser->current_ofs + 2];
      *((char *)to_where + 6) = ser->buf[ser->current_ofs + 1];
      *((char *)to_where + 7) = ser->buf[ser->current_ofs];
      break;
   default:
      for (i=0; i<nof_bytes; i++) {
         *((char *)to_where + i) = ser->buf[ser->current_ofs + nof_bytes - i - 1];
      }
   }
   ser->current_ofs+=nof_bytes;
}
#endif
/*-------------------------------------------------------------------------*/

unsigned char *Serialized_Buffer(Serialized *ser) {
   int temp;
   
   if (ser->current_ofs > ser->used_size)
      ser->used_size = ser->current_ofs;

   /* TODO: Not MT safe! */
   temp = ser->current_ofs;
   ser->current_ofs = SERIALIZED_OFS_USEDSIZE;

   Serialized_AppendWrapped(ser, &ser->used_size, sizeof(ser->used_size));
   ser->current_ofs = temp;

   return ser->buf;
}

/*-------------------------------------------------------------------------*/

unsigned int   Serialized_BufferSize(Serialized *ser) {
   if (ser->current_ofs > ser->used_size)
      ser->used_size = ser->current_ofs;

   return ser->used_size;
}

/*-------------------------------------------------------------------------*/

unsigned int   Serialized_BytesToRead(unsigned char *header) {
#ifdef CMOD_ARCHITECTURE_LITTLEENDIAN
   unsigned int to_read;
   unsigned int *to_where;
   
   to_where = &to_read;

   *(char *)to_where = header[SERIALIZED_OFS_USEDSIZE+3];
   *((char *)to_where + 1) = header[SERIALIZED_OFS_USEDSIZE+2];
   *((char *)to_where + 2) = header[SERIALIZED_OFS_USEDSIZE+1];
   *((char *)to_where + 3) = header[SERIALIZED_OFS_USEDSIZE];
   
   return to_read;
#else
   return *((unsigned int *)&(header[SERIALIZED_OFS_USEDSIZE]));
#endif
}

/*-------------------------------------------------------------------------*/

void Serialized_Rewind(Serialized *ser) {
   if (ser->current_ofs > ser->used_size)
      ser->used_size = ser->current_ofs;
      
   ser->current_ofs = SERIALIZED_HEADER;
}

/*-------------------------------------------------------------------------*/

void Serialized_Destroy(Serialized *serialized) {
   Memory_Free(serialized->buf, unsigned char);
	Memory_Free(serialized, Serialized);
}

