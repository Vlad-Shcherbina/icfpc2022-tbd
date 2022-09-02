
#ifndef _file_h
#define _file_h

#include "cmod.h"
#include "string.h"

#include <stdarg.h>
#include <stdio.h>

typedef long filepos;
typedef FILE File;

#define File_Open(s,t) fopen(s,t)
#define File_ReOpen(f,s,t) freopen(s,t,f)
#define File_Reopen File_ReOpen
#define File_Seek(s,t,u) fseek(s,t,u)
int     File_Close(File *file);
#define File_Eof(f) feof(f)
#define File_Error(f) ferror(f)
#define File_IsEof(f) feof(f)
#define File_IsError(f) ferror(f)
#define File_Descriptor(f) fileno(f)
#define File_Read(f, buf, n) fread(buf, 1, n, f)
#define File_Write(f, buf, n) fwrite(buf, 1, n, f)
#define File_Scanf fscanf
#define File_Printf fprintf
#define File_Pos ftell
#define File_GetPos File_Pos
#define File_SetPos(f, a) fseek(f, a, SEEK_SET)
#define File_Flush fflush
#ifdef CMOD_OS_UNIX
/* Blocking call! Does not lock NFS.. should be done properly.. */
File   *File_LockAndOpen(String *name, String *mode);
#endif
filepos File_Search(File *file, const unsigned char *pattern, int len);
String *File_ReadLine(File *f);
/*void    File_ReadLineToString(File *f, String *s, int maxlen); */
#define File_ReadLineToString(f, s, n) fgets(s, n, f)
int     File_Size(File *f);

File   *File_CreateLineIterator(String *filename);
String *File_NextLine(File *file);
String *File_NextLineMax(File *file, int len);

void    File_WriteBytes(File *file, int nof, ...);
void    File_WriteByte(File *file, unsigned char byte);
void    File_WriteInt(File *file, unsigned int dword);
void    File_WriteDouble(File *file, double x);
void    File_WriteLongLong(File *file, long long ll);
int     File_ReadInt(File *file);
void    File_SkipLine(File *file);
unsigned char File_ReadByte(File *file);
double  File_ReadDouble(File *file);
long long File_ReadLongLong(File *file);
String *File_ReadToString(const String *filename);
unsigned char *File_ReadToBuf(const String *filename, int *len);
int File_Dump(const String *filename, void *ofs, int len); // Dump block to file
int File_Undump(const String *filename, void *ofs); // Read a block from file to mem, length returned

int File_ReadAsciiInt(File *f, int *ans); // helper for File_Parse

/* Similar to scanf, but strings are terminated by what the format specifies 
   ex: format "%s|" reads until a '|' is encountered -- even if it takes several lines
   '\n' matches both '\n', '\r', "\r\n" and "\n\r"
   "%[{}]" reads until either a { or } is encountered
   "%i,%i" will parse an integer, skip until a ',' is found, and then parse another integer

   Returns number of % entries parsed

   NOT FULLY IMPLEMENTED. %s and %i work so far.. probably %c as well.
   Field width only honoured for %s (i.e "%8s" limits reading to 8 characters)
*/
int File_Parse(File *file, const char *format, ...);
#ifdef CMOD_NOMAKEFILE
#include "file.c"
#endif

/* Wrappers for 'stat' calls */
char      File_Type(const char *fileame);
long long File_StatSize(const char *filename);

/* Returns an array to String * with entry names, terminated with a NULL ptr. User frees. */
String  **File_ReadDir(const char *dirname);
void      File_ClearDirStrings(String **readdir_res); /* To free the String ** from File_ReadDir */
#define   File_FreeDirStrings File_ClearDirStrings
#endif

