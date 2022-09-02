
#ifndef _string_h
#define _string_h

#include "cmod.h"
#ifdef CMOD_SERIALIZED
#include "serialized.h"
#endif
#include <stdarg.h>

#define STRING_DEFAULTLENGTH 16

typedef char String;

extern const String *string_empty;

void         String_Init(String *string, const String *string_org);
String      *String_Create(int len);
String      *String_Recreate(String *old, const String *nw); /* destroy, and clone */
String      *String_CreatePrintf(const String *format, ... );
String      *String_CreateVPrintf(const String *format, va_list a);
String      *String_Clone(const String *string);
String      *String_Realloc(String *s, int length);
String      *String_CloneQuoted(const String *s); /* Returns the first quoted substring, NULL if not nofound */
Bool         String_IsBeginning(const String *string, const String *beg);
#define      String_IsBeginningWith String_IsBeginning
#define      String_BeginsWith String_IsBeginning
//int          String_FirstOccurence(const String *s, char c);
int          String_NextOccurence(const String *s, int prev, char c);
#define      String_FirstOccurence(s, c) String_NextOccurence((s), -1, (c))
#define      String_FirstOccurenceOf String_FirstOccurence
int          String_LastOccurence(const String *s, char c);
int          String_OffsetOfSubstr(const String *s, const String *s2); /* -1 if not found */
const String*String_AfterOccurence(const String *s, const String *pattern); /* returns NULL if not found */
#define      String_Pos String_OffsetOfSubstr
#define      String_CharPos String_FirstOccurence
#define      String_Compare strcmp
#define      String_CompareNoCase strcasecmp
String      *String_CreateFromSubstr(const String *const s, int startix, int length);
String      *String_CreateFromSubstr2(const String *const s, const String *const end);
#define      String_Slice(s, a, b) String_CreateFromSubstr((s), (a), ((b)-(a)+1))
#define      String_Restore(s)
#define      String_CreateFromSubstring(a,b,c) String_CreateFromSubstr(a,b,c)
#define      String_CreateFromSubString(a,b,c) String_CreateFromSubstr(a,b,c)
String      *String_Append(String *string, const String *app);
void         String_ToLower(String *string);
void         String_ToUpper(String *string);
int          String_ToInt(String *string);
//#define      String_ToInt(s) atoi(s)
#define      String_ToFloat(s) atof(s)
#define      String_ToDouble(s) atof(s)
#define      String_Scanf sscanf
#define      String_Printf sprintf
void         String_Reverse(String *s);
#define      String_Rev String_Reverse
unsigned int String_HashVal(String *s);
Bool         String_IsEqual(const String *s1, const String *s2);
Bool         String_IsEqualNoCase(const String *s1, const String *s2);
int          String_EditDistance(String *s, String *t);
String      *String_Trim(String *s); /* Realloc:s the string (inc trailing \0), cutting slack space */
int          String_NofDifferentCharacters(String *s);
String      *String_NextAlnum(String *s); /* Skips ahead until a letter, number, '_', '+' or '-' is found */
const String*String_NextLine(const String *s); /* Skips ahead, past next consequtive group of '\n' and '\r' */
String      *String_CreateLine(const String *s); /* Allocates a new string, until (not including) '\n' or '\r' or eos */
void         String_RemoveDuplicateWhiteSpace(String *s); /* Replaces all sequences of whitespace with ' '*/
void         String_RemoveWhiteSpace(String *s); /* Removes all whitespace */
String      *String_ForwardUntil(String *s, char c);
Bool         String_RegexpMatch(const String *s, String *regexp);
int          String_Parse(const String *s, const String *format, ... );
#ifdef _darray_h
//DArray      *String_Tokenize(const String *s, const char delimiter);
#endif

#ifdef CMOD_SERIALIZED
void         String_Serialize(String *s, Serialized *ser);
String      *String_DeSerialize(Serialized *ser);
#endif

void         String_Destroy(String *string);

#ifndef CMOD_OPTIMIZE_SPEED
   int         String_Length(const String *string);
#else
   #ifndef CMOD_COMPILER_GCC
   #include <string.h>
   #else
   #include_next <string.h>
   #endif
   #define String_Length(s) strlen(s)
#endif

#ifdef CMOD_DEBUG_OUTPUT
void         String_Display(String *string, int n);
#endif

#ifdef CMOD_NOMAKEFILE
#include "string.c"
#endif

#endif
