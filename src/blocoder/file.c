
#include "cmod.h"
#include "file.h"
#include "memory.h"
#include "stdfunc.h"
#include "string.h"
#include <dirent.h>
#include <stdarg.h>

#ifdef CMOD_OS_UNIX
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif

#define FILE_BLOCKSIZE 32768

/*-------------------------------------------------------------------------*/
#ifdef CMOD_OS_UNIX
File   *File_LockAndOpen(String *name, String *mode) {
   File *f;

   f = File_Open(name, mode);
   flock(File_Descriptor(f), LOCK_EX);
   return f;
}
#endif
/*-------------------------------------------------------------------------*/

String *File_ReadLine(File *f) {
   char c;
   int pos, len;
   String *temp;
   String *temp2;

   len = 128;
   temp = String_Create(len);
   pos = 0;

   while(!File_Eof(f)) {
      if (fscanf(f, "%c", &c) <= 0) break;
      if (c == '\n') break;
      if (c == 0xc) continue;
      temp[pos] = c;
      pos++;
      if (pos>=len) {
         temp[len] = 0;
         temp2 = String_Create(2*len);
         Memory_Copy(temp2, temp, len);
         len *= 2;
         String_Destroy(temp);
         temp = temp2;
      }
   }
   temp[pos] = 0;
   return temp;
}

/*-------------------------------------------------------------------------*/

void File_SkipLine(File *file) {
   char c;
   do {
      c = File_ReadByte(file);
   } while (c != '\n' && c != '\0' && !File_Eof(file));
}

/*-------------------------------------------------------------------------*/

int     File_Size(File *f) {
   int oldpos;
   int newpos;

   oldpos = ftell(f);
   fseek(f, 0L, SEEK_END);
   newpos = ftell(f);
   fseek(f, oldpos, SEEK_SET);

   return newpos;
}

/*-------------------------------------------------------------------------*/

static Bool file_strmatch(unsigned char *buf, const unsigned char *pattern, int slen) {
   int i;
   for (i=0; i<slen; i++) {
      if (buf[i] != pattern[i]) return FALSE;
   }
   return TRUE;
}

filepos File_Search(File *file, const unsigned char *pattern, int len) {
   filepos startpos = File_Pos(file);
   filepos curpos = 0;
   filepos retval = -1;
   int blocksize;
   int lastread;
   unsigned char *block;
   int i;

   blocksize = MAX(2*len+3, FILE_BLOCKSIZE);
   if (blocksize & 1) blocksize++;

   block = Memory_ReserveSize(blocksize, unsigned char);

   lastread = File_Read(file, block, blocksize);
   while (!File_Eof(file)) {
      for (i=0; i < blocksize/2; i++) {
         if (block[i] == pattern[0]) {
            if (file_strmatch(&block[i], pattern, len)) {
               retval = startpos + curpos + i;
               goto leave;
            }
         }
      }
      Memory_Move(block, &block[blocksize/2],  blocksize/2);
      lastread = File_Read(file, &block[blocksize/2], blocksize/2);
      curpos += blocksize/2;
   }

   Memory_Move(block, &block[blocksize/2],  blocksize/2);
   curpos += blocksize/2;
   for (i=0; i < lastread - len - 1; i++) {
      if (block[i] == pattern[0]) {
         if (file_strmatch(&block[i], pattern, len)) {
            retval =  startpos + curpos + i;
            goto leave;

         }
      }
   }
leave:
   Memory_Free(block, unsigned char);
   File_SetPos(file, startpos);
   return retval;
}

/*-------------------------------------------------------------------------*/

int File_Close(File *file) {
   if (file == NULL) return -1;
   fflush(file);
#ifdef CMOD_OS_UNIX
   fsync(File_Descriptor(file));
   flock(File_Descriptor(file), LOCK_UN);
#endif
   return fclose(file);
}

/*-------------------------------------------------------------------------*/

File   *File_CreateLineIterator(String *filename) {
   File *f;
   f = File_Open(filename, "r");
   if (f == NULL) Std_Warning("Cannot open file %s\n", filename);
   return f;
}

/*-------------------------------------------------------------------------*/

String *File_NextLine(File *file) {
   if (file == NULL || File_Eof(file)) return NULL;
   return File_ReadLine(file);
}

/*-------------------------------------------------------------------------*/

String *File_NextLineMax(File *file, int len) {
   String *temp, *temp2;
   
   if (file == NULL || File_Eof(file)) return NULL;
   temp = String_Create(len+1);
   temp2 = fgets(temp, len, file);
   if (temp2 == NULL) String_Destroy(temp);
   return temp2;
}

/*-------------------------------------------------------------------------*/

int File_ReadInt(File *file) {
   int a;
   File_Read(file, &a, sizeof(int));
   return a;
}

/*-------------------------------------------------------------------------*/

unsigned char File_ReadByte(File *file) {
   unsigned char c;
   File_Read(file, &c, sizeof(unsigned char));
   return c;
}

/* ----------------------------------------------------------------------- */

String *File_ReadToString(const String *filename) {
   int len;
   String *s;
   File *f;
   
   f = File_Open(filename, "r");
   if (f == NULL) return NULL;
   
   len = File_Size(f)+1;
   s = String_Create(len);   
   s[File_Read(f, s, len)] = 0;
   s[len] = 0;
   
   File_Close(f);
   return s;
}

/* ----------------------------------------------------------------------- */

unsigned char *File_ReadToBuf(const String *filename, int *len) {
   unsigned char *buf;
   File *f;
   
   f = File_Open(filename, "rb");
   if (f == NULL) {
      *len = -1;
      return NULL;
   }
   *len = File_Size(f);
   buf = Memory_Reserve(*len, unsigned char);
   *len = fread(buf, 1, *len, f);
   return buf;
}

/* ----------------------------------------------------------------------- */

int File_Dump(const String *filename, void *ofs, int len) {
   File *f = File_Open(filename, "w");
   if (f == NULL) return -1;
   File_Write(f, ofs, len);
   File_Close(f);
   return 0;
}

/* ----------------------------------------------------------------------- */

int File_Undump(const String *filename, void *ofs) {
   int len;
   File *f = File_Open(filename, "r");
   if (f == NULL) return -1;
   len = File_Size(f);
   File_Read(f, ofs, len);
   File_Close(f);
   return len;
}

/* ----------------------------------------------------------------------- */

void    File_WriteBytes(File *f, int nof, ...) {
   int i;
   va_list args;
   unsigned int a;
   unsigned char c;
      
   va_start(args, nof);
   for (i=0; i<nof; i++) {
      a = va_arg(args, unsigned int);
      c = a;
      File_WriteByte(f, c);
   }
   va_end(args);
}

/* ----------------------------------------------------------------------- */

void    File_WriteByte(File *f, unsigned char byte) { File_Write(f, &byte, 1); }

/* ----------------------------------------------------------------------- */

void    File_WriteInt(File *f, unsigned int dword) { File_Write(f, &dword, sizeof(dword)); }

/* ----------------------------------------------------------------------- */

long long File_ReadLongLong(File *file) { 
   long long ll;
   File_Read(file, &ll, sizeof(long long));
   return ll;   
}

/* ----------------------------------------------------------------------- */

void File_WriteLongLong(File *file, long long ll) { File_Write(file, &ll, sizeof(long long)); }

/* ----------------------------------------------------------------------- */

void    File_WriteDouble(File *f, double x) { File_Write(f, &x, sizeof(double)); }

/* ----------------------------------------------------------------------- */

double  File_ReadDouble(File *file) {
   double x;
   File_Read(file, &x, sizeof(double));
   return x;
}

/* ----------------------------------------------------------------------- */

int File_ReadAsciiInt(File *f, int *ans) {
   char c=' ';
   int sig = 1, ret = 0;
   *ans = 0;
   while (!File_Eof(f)) {
      do { 
         c = File_ReadByte(f);
         if (c == '-') { sig = -1; c = ' '; }
         if (c == '+') c = ' ';
      } while (c == '\t' || c == ' ');
   }
   
   while (!File_Eof(f) && c>='0' && c<= '9') {
      *ans *= 10;
      *ans += 'c' - '0';
      ret = 1;
   }
   *ans *= sig;
   File_Seek(f, -1, SEEK_CUR);
   return ret;
}

/* ----------------------------------------------------------------------- */

int File_Parse(File *file, const char *format, ... ) {
   va_list al;
   const char *cf;
   char c;
   String *s;
   int *ip;
   float *f;
   double *d;
   int i;
   int variables_parsed = 0;
   int field_width = 4096;
   
   va_start(al, format);
   cf = format;
   while (!File_Eof(file) && *cf) {
      if (*cf != '%') {
         do {
            c = File_ReadByte(file);
         } while (!File_Eof(file) && c != *cf);
         cf++;
         field_width = 0;
      } else {
         cf++;
         switch(*cf) {
         case 's':
            cf++;
            i = -1;
            s = va_arg(al, String*);
            s[0] = 0;
            if (!File_Eof(file)) {
               do {
                  s[++i] = File_ReadByte(file);
               } while(!File_Eof(file) && *cf != s[i] && (i<field_width && field_width>0));
               variables_parsed++;
               s[i] = 0;
            }
            break;
         case 'i':
         case 'l':
         case 'd':
            ip = va_arg(al, int*);
//            variables_parsed += File_ReadAsciiInt(file, ip);
            variables_parsed += File_Scanf(file, "%d", ip);
            break;
         case 'x':
         case 'X':
            ip = va_arg(al, int*);
            variables_parsed += File_Scanf(file, "%x", ip);
            break;
         case 'f':
            f = va_arg(al, float*);
            variables_parsed += File_Scanf(file, "%f", f);
            break;
         case 'g':
            d = va_arg(al, double*);
            variables_parsed += File_Scanf(file, "%lf", d);
            break;
         case 'u':
            ip = va_arg(al, int*);
            variables_parsed += File_Scanf(file, "%u", ip);
            break;
         case 'c':
            s = va_arg(al, char*);
            variables_parsed += File_Scanf(file, "%c", s);
            break;
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            field_width *= 10;
            field_width += *cf - '0';
            break;
         default: ;
            fprintf(stderr, "Unknown format %%%c to File_Parse\n", *cf); fflush(stdout);
         }
         cf++;
      }
   }
   va_end(al);
   return variables_parsed;
}

/* ----------------------------------------------------------------------- */


char File_Type(const char *filename) {
   struct stat buf;
   char ret = 0;
   
   if (lstat(filename, &buf) == 0) {
      if (S_ISREG(buf.st_mode)) ret = 'f';
      else
      if (S_ISDIR(buf.st_mode)) ret = 'd';
      else
      if (S_ISCHR(buf.st_mode)) ret = 'c';
      else
      if (S_ISFIFO(buf.st_mode)) ret = 'p';
      else
      if (S_ISLNK(buf.st_mode)) ret = 'l';
      else
      if (S_ISSOCK(buf.st_mode)) ret = 's';
   }
   return ret;
}

/* ----------------------------------------------------------------------- */

long long File_StatSize(const char *filename) {
   struct stat buf;
   long long size;
   
   if (lstat(filename, &buf) == 0) size = buf.st_size;
   else size = -1;
   
   return size;
}

/* ----------------------------------------------------------------------- */

/* Returns an array to String * with entry names, terminated with a NULL ptr. User frees */
String **File_ReadDir(const char *dirname) {
   DIR *dp;
   struct dirent *ep;
   String **ret;
   int i;
   int n;
   
   /* TODO: Not MT-safe! Use readdir_r instead of readdir.... */
   
   dp = opendir(dirname);
   if (dp != NULL) {
      n = 16; i = 0;
      ret = Memory_Reserve(n, String *);
      while ((ep = readdir(dp))) {
         ret[i++] = String_Clone(ep->d_name);
         if (i==n) {
            n <<= 1;
            ret = Memory_Realloc(ret, n, String *);
         }
         free(ep);
      }
      
      closedir(dp);
   } else 
      ret = NULL;

   return ret;
}

void File_ClearDirStrings(String **readdir_res) {
   int i=0;
   while(readdir_res[i] != NULL) Memory_Free(readdir_res[i++], String);
   Memory_Free(readdir_res, String *);
}

