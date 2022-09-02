
#include "cmod.h"
#include "memory.h"
#include "string.h"
#include <stdarg.h>
#include <stdio.h>

const String *string_empty = "";

void String_Init(String *string, const String *string_org) {
   int i;
   register char c;
   if (string_org == NULL) {
      string[0] = 0;
      return;
   }
   i=0;
   do {
      c = string_org[i];
      string[i] = c;
      i++;
   } while(c != 0);
}

/*---------------------------------------------------------------------------*/

String *String_Create(int len) {
	String *string;
   
	string = Memory_Reserve(len + 1, String);
	
   return string;
}

/*---------------------------------------------------------------------------*/

String *String_Recreate(String *old, const String *nw) {
   String_Destroy(old); 
   return String_Clone(nw);
}

/*---------------------------------------------------------------------------*/


String *String_CreatePrintf(const String *format, ... ) {
   String *newh;
   va_list a;
   int len;
   
   newh = String_Create(STRING_DEFAULTLENGTH);
   
   va_start(a, format);
   len = vsnprintf(newh, STRING_DEFAULTLENGTH, format, a);
   va_end(a);
   
   if (len >= STRING_DEFAULTLENGTH) {
      newh = Memory_Realloc(newh, len+2, String);
      va_start(a, format);   
      len = vsnprintf(newh, len+1, format, a);
      va_end(a);
   }
   return newh;
}

/*---------------------------------------------------------------------------*/

String *String_CreateVPrintf(const String *format, va_list a) {
   String *newh;
   int len;
   
   len = vsnprintf(NULL, 0, format, a);   
   newh = Memory_Reserve(len+2, String);
   vsnprintf(newh, len+1, format, a);
   return newh;
}

/*---------------------------------------------------------------------------*/

Bool String_IsBeginning(const String *string, const String *beg) {
   int i;

   i=-1;
   do {
      i++;
      if (beg[i] == 0) return TRUE;
   } while(string[i] == beg[i]);
   return FALSE;
}

/*---------------------------------------------------------------------------*/

String *String_CreateFromSubstr(const String *s, int startix, int length) {
   String *s2 = String_Create(length + 1);
   Memory_Copy(s2, &s[startix], length);
   s2[length] = 0;
   return s2;
}

/*---------------------------------------------------------------------------*/
/* not including the character *end*/
String *String_CreateFromSubstr2(const String *const s, const String *const end) {
   String *s2 = String_Create(end - s + 1);
   Memory_Copy(s2, s, end - s);
   s2[end - s] = 0;
   return s2;
}

/*---------------------------------------------------------------------------*/

String *String_Clone(const String *string) {
   int length = String_Length(string);
   String *s2 = String_Create(length + 1);
   Memory_Copy(s2, (String *)string, length + 1);
   return s2;
}

/*---------------------------------------------------------------------------*/

String *String_Append(String *string, const String *app) {
   int l1 = String_Length(string);
   int l2 = String_Length(app);

   string = Memory_Realloc(string, l1 + l2 + 1, String);
   Memory_Copy(&string[l1], (String *)app, l2+1);

   return string;
}

/*---------------------------------------------------------------------------*/

int String_NextOccurence(const String *s, int prev, char c) {
   register int i = prev+1;
   while (s[i] != 0) {
      if (s[i] == c) return i;
      i++;
   }
   return -1;
}

/*---------------------------------------------------------------------------*/

/* Returns NULL if no quoted substring is found */
String *String_CloneQuoted(const String *s) {
   int first, last=-1;
   char c = '"';
   if (s == NULL) return NULL;
   if (s[0] == '\0') return String_Clone(s);

   /* First try double quotes, then single quotes */
   first = String_NextOccurence(s, -1, c);   
   if (first < 0) c = '\'';
   first = String_NextOccurence(s, -1, c);   
   
   if (first >= 0)
      last = String_NextOccurence(s, first, c);

   if (last > 0)
      return String_CreateFromSubstr(s, first+1, last - first - 1);
   else 
      return NULL;
}

/*---------------------------------------------------------------------------*/

//int String_FirstOccurence(const String *s, char c) {
//   return String_NextOccurence(s, -1, c); 
//}

int String_LastOccurence(const String *s, char c) {
   int i=0, pos = -1;
   while (s[i] != '\0') {
      if (s[i] == c) pos=i;
      i++;
   }
   return pos; 
}

/*---------------------------------------------------------------------------*/
/* looks for s2 in s */

int String_OffsetOfSubstr(const String *s, const String *s2) {
   int i;
   int j;
   
   i=0;
   
   while (s[i] != 0) {
      j=0;
      while (s[i+j] == s2[j]) {
         if (s[i+j] == 0) break;         
         j++;
         if (s2[j] == 0) return i;
      }
      i++;
   }
   return -1;
}

/*---------------------------------------------------------------------------*/

const String *String_AfterOccurence(const String *s, const String *pattern) {
   int p = String_OffsetOfSubstr(s, pattern);
   return (p>=0) ? s+p+String_Length(pattern) : NULL; 
}

/*---------------------------------------------------------------------------*/

void String_ToLower(String *string) {
   int i;
   i = 0;
   while (string[i] != 0) {
      if (string[i] >= 'A' && string[i] <= 'Z') string[i] += 'a' - 'A';
      i++;
   }
}

/*---------------------------------------------------------------------------*/

void String_ToUpper(String *string) {
   int i;
   i=0;
   while (string[i] != 0) {
      if (string[i] >= 'a' && string[i] <= 'z') string[i] += 'A' - 'a';
      i++;
   }
}

/*---------------------------------------------------------------------------*/

Bool	String_IsEqual(const String *s1, const String *s2) { return (strcmp(s1, s2) == 0); }

/*---------------------------------------------------------------------------*/

Bool String_IsEqualNoCase(const String *s1, const String *s2) {
   int i;
   register char c1, c2;
   i=0;
   
   while (s1[i] != 0) {
      c2 = s2[i];
      if (c2 == 0) return FALSE;
      c1 = s1[i]; 
      if (c1 < 'a') {
         if (c2 >= 'a' && c2 <= 'z') c2-='a'-'A';
      } else
         if (c2 >= 'A' && c2 <= 'Z') c2+='a'-'A';
      
      if (c1 != c2) return FALSE;
      i++;
   }
   
   if (s2[i] != 0) return FALSE;
   return TRUE;
}

/*---------------------------------------------------------------------------*/

#ifdef CMOD_SERIALIZED
void String_Serialize(String *s, Serialized *ser) {
   unsigned short len;
   len = String_Length(s);
   Serialized_AppendWrapped(ser, &len, sizeof(len));
   Serialized_AppendData(ser, s, String_Length(s)+1);
}

/*---------------------------------------------------------------------------*/

String      *String_DeSerialize(Serialized *ser) {
   unsigned short len;
   String *s;
   Serialized_ReadWrapped(ser, &len, sizeof(len));
   s = String_Create(len);
   Serialized_ReadData(ser, s, len+1);
   return s;
}
#endif

/*---------------------------------------------------------------------------*/

int  String_ToInt(String *s) {
   int base = 10;
   int i, len, val=0;
   Bool neg;
   
   while (s[0] == ' ' || s[0] == 9) s++;
   neg = (s[0]=='-');
   if (s[0] == '-' || s[0] == '+') s++;
   i=0;
   if (s[0] == '0' && s[1] == 'x') base = 16;
   while (s[i] != 0) {
      if ((s[i] >= 'a' && s[i] <= 'f') || (s[i] >= 'A' && s[i] <= 'F')) base = 16;
      else
      if (s[i] == 'h') { base = 16; break; }
      else
      if ((s[i] < '0' || s[i] > '9') && s[i] != 'x') break;
      i++;
   }
   len = i;
   for (i=0; i<len; i++) {
      if (s[i] >= '0' && s[i] <= '9') { val = val*base + s[i] - '0'; }
      if (s[i] >= 'a' && s[i] <= 'f') { val = val*base + 10 + s[i] - 'a'; }
      if (s[i] >= 'A' && s[i] <= 'F') { val = val*base + 10 + s[i] - 'A'; }
   }
   if (neg) val=-val;
   return val;
}

/*---------------------------------------------------------------------------*/

#ifndef CMOD_OPTIMIZE_SPEED
int  String_Length(const String *string) {
   return (int)strlen(string);
}
#endif

/*---------------------------------------------------------------------------*/

/* 
   This uses Robert Jenkins hasvalue generator.
   See www.burtleburtle.net/bob for further details.
*/
#ifndef _hashtable_h 
/* To avoid same code existing in two places */
#define String_HashMix(a,b,c) { \
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

/* This has been taken from doobs web page, 6n + 36 cycles */
//typedef unsigned long int ub4;
static unsigned long int string_initval = 0x14f5e32a; /* the previous hash value */

unsigned int String_HashVal(String *k) {
   register unsigned int a, b, c;
   int len, length;

   /* Set up the internal state */
   length = len = String_Length(k);
   a = b = c = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
//   c = string_initval;         /* the previous hash value */

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((unsigned long int)k[1]<<8) +((unsigned long int)k[2]<<16) +((unsigned long int)k[3]<<24));
      b += (k[4] +((unsigned long int)k[5]<<8) +((unsigned long int)k[6]<<16) +((unsigned long int)k[7]<<24));
      c += (k[8] +((unsigned long int)k[9]<<8) +((unsigned long int)k[10]<<16)+((unsigned long int)k[11]<<24));
      String_HashMix(a,b,c);
      k += 12; len -= 12;
   }

   /* handle the last 11 bytes */
   c += length;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((unsigned long int)k[10]<<24);
   case 10: c+=((unsigned long int)k[9]<<16);
   case 9 : c+=((unsigned long int)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((unsigned long int)k[7]<<24);
   case 7 : b+=((unsigned long int)k[6]<<16);
   case 6 : b+=((unsigned long int)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((unsigned long int)k[3]<<24);
   case 3 : a+=((unsigned long int)k[2]<<16);
   case 2 : a+=((unsigned long int)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   String_HashMix(a,b,c);
   string_initval = c;
   /* report the result */
   return c;
}
#else
unsigned int String_HashVal(String *k) { 
   return HashTable_BlockHashValue((unsigned char *)k, String_Length(k));
}
#endif

#if 0
/*
Consider the following hash instead? Better than Bob J (less collisions) and 66% faster

#include "pstdint.h"
*/
/* Replace with <stdint.h> if appropriate */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const char * data, int len) {
uint32_t hash = len, tmp;
int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
#endif


/*---------------------------------------------------------------------------*/

/***********************************************/
/*    Implementation of Levenshtein distance   */
/*                                             */
/* by Lorenzo Seidenari (sixmoney@virgilio.it) */
/***********************************************/

int String_EditDistance(String *s, String *t) {
   int i, j, k, n, m;
   int cost, *d, distance;

   n = String_Length(s) + 1;
   m = String_Length(t) + 1;
   if (n<=1 || m<=1) return n+m-2;

   d = Memory_Reserve(m*n, int);
   for (k = 0; k < n; k++) d[k] = k;
   for (k = 0; k < m; k++) d[k*n] = k;

   for (i = 1; i < n; i++) {
      for (j = 1; j < m; j++) {
         if (s[i-1] == t[j-1]) cost=0; else cost=1;
         d[j*n + i] = MIN(d[(j-1)*n+i]+1 , MIN(d[j*n+i-1]+1,d[(j-1)*n+i-1]+cost));
      }
   }
   distance = d[n*m - 1];
   Memory_Free(d, int);
   return distance;
}

/*---------------------------------------------------------------------------*/

void String_Reverse(String *s) {
   int i,l;
   char c;
   l = String_Length(s);
   for (i=0; i<l/2; i++) {
      c = s[i];
      s[i] = s[l-i-1];
      s[l-i-1]=c;
   }
}

/*---------------------------------------------------------------------------*/

String *String_Realloc(String *s, int length) { return Memory_Realloc(s, length, String); }

/*---------------------------------------------------------------------------*/

String *String_Trim(String *s) { return String_Realloc(s, String_Length(s)+1); }

/*---------------------------------------------------------------------------*/

int String_NofDifferentCharacters(String *s) {
   int i = 0,nof[256] = { 0 };
   while(s[i] != 0) nof[(unsigned int)s[i++]]++;
   for (i=0; i<256; i++) if (nof[i]>0) nof[0]++;
   return nof[0];
}

/*---------------------------------------------------------------------------*/

String *String_NextAlnum(String *s) {
   if (s != NULL) {
/*
      while(*s != 0) {
         if (*s >= '0' && *s <= '9') s++; 
         else
         if (*s >= 'A' && *s <= 'Z') s++; 
         else
         if (*s >= 'a' && *s <= 'z') s++; 
         else
         break;
      }
*/
      while(*s != 0) {
         if (*s >= '0' && *s <= '9') return s;
         if (*s >= 'A' && *s <= 'Z') return s;
         if (*s >= 'a' && *s <= 'z') return s;
         if (*s == '+' || *s == '-') return s;
         if (*s == '_') return s;
         s++;
      }
   }
   return NULL;
}

/*---------------------------------------------------------------------------*/

const String *String_NextLine(const String *s) {
   if (s != NULL) {
      const String *d = s;
      
      while (*d != '\n' && *d != '\r' && *d != '\0') d++;
      if (*d == '\0') return NULL;
      if (*d == '\r') d++;
      if (*d == '\n') d++;
      return d;
   }
   return NULL;
}

/*---------------------------------------------------------------------------*/

String *String_CreateLine(const String *s) {
   String *ret;
   if (s != NULL) {
      const String *d = s;
      int len;
      
      while (*d != '\n' && *d != '\r' && *d != '\0') d++;
      len = d - s;
      
      ret = String_Create(len+1);
      Memory_Copy(ret, s, len);
      ret[len] = '\0';

      return ret;
   }
   return NULL;  
}

/*---------------------------------------------------------------------------*/
/*
String *String_ExtractToken(String *s) {
   int n = String_Length(s);
   String *r = String_Create(n);
   int i;
   r[0] = 0;
   
   
}
*/
/*
int String_Parse(const String *s, const String *format, ... );
   va_list al;
   int i;
   char type;

   va_start(al, format);
   i=0;
   while (s[i] != 0) {
      if (s[i++] == '%') {
         type = s[i++];
         if (s[i] == 0) break;
         
      }
   }
}
*/
/*---------------------------------------------------------------------------*/

void String_RemoveDuplicateWhiteSpace(String *s) { /* Replaces all sequences of whitespace with ' '*/
   int i, j, n, mode;
   String *s2;

   n = String_Length(s);
   s2 = String_Create(n);
   j = mode = 0;
   for (i=0; i<n; i++) {
      if ((s[i] != ' ' && s[i] != 9) || mode == 0) {
         s2[j++] = s[i];
         mode = 0;
      }
   }
   s2[j++]=0;
   Memory_Copy(s, s2, j);
   String_Destroy(s2);
}

/*---------------------------------------------------------------------------*/

void String_RemoveWhiteSpace(String *s) { /* Removes all whitespace */
   int i, j, n;
   String *s2;

   n = String_Length(s);
   s2 = String_Create(n+1);
   j = 0;
   for (i=0; i<n; i++) {
      if (s[i] != ' ' && s[i] != 9) s2[j++] = s[i];
   }
   s2[j++] = 0;
   Memory_Copy(s, s2, j);
   String_Destroy(s2);
}

/*---------------------------------------------------------------------------*/

String *String_ForwardUntil(String *s, char c) {
   while (*s != 0 && *s != c) s++;
   return s;
}

#ifndef _darray_h
#include "darray.h"
#endif

#if 0
DArray *String_Tokenize(const String *s, const char delimiter) {
   DArray *res = DArray_Create(32);
   int b, e;
   b = 0;
   e = String_FirstOccurenceOf(s, delimiter);
   while (e>0) {
      DArray_Add(res, String_Slice(s, b, e-1));
      b = e+1;
      e = String_NextOccurence(s, e, delimiter);      
   }
   DArray_Add(res, String_Slice(s, b, String_Length(s)));
   return res;
}
#endif

/*---------------------------------------------------------------------------*/
/*
int String_FindMatching(String *s, char lparen) {
   char rparen;
   int i,n;
   switch(lparen) {
   case '[': rparen=']'; break;
   case '{': rparen='}'; break;
   case '(': rparen=')'; 
   default:
      break;
  }
  n = String_Length(s);
  for (i=0; i<n; i++) {
     
  }
}
*/
/*---------------------------------------------------------------------------*/
#include <regex.h>

Bool String_RegexpMatch(const String *s, String *regexp) {
   regex_t re;
   int res;

   if (regcomp(&re, regexp, REG_EXTENDED|REG_NOSUB) != 0) return FALSE;
   res = regexec(&re, s, 0, NULL, 0);
   regfree(&re);
   return (res == 0);
}

#if 0
/* match: search for regexp anywhere in text */
int match(char *regexp, char *text)
{
    if (regexp[0] == '^')
        return matchhere(regexp+1, text);
    do {    /* must look even if string is empty */
        if (matchhere(regexp, text))
            return 1;
    } while (*text++ != '\0');
    return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *regexp, char *text)
{
    if (regexp[0] == '\0')
        return 1;
    if (regexp[1] == '*')
        return matchstar(regexp[0], regexp+2, text);
    if (regexp[0] == '$' && regexp[1] == '\0')
        return *text == '\0';
    if (*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
        return matchhere(regexp+1, text+1);
    return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(int c, char *regexp, char *text)
{
    do {    /* a * matches zero or more instances */
        if (matchhere(regexp, text))
            return 1;
    } while (*text != '\0' && (*text++ == c || c == '.'));
    return 0;
}
#endif

/*---------------------------------------------------------------------------*/

void String_Destroy(String *string) { 
   if (string != NULL) Memory_Free(string, String); 
}

#ifdef CMOD_DEBUG_OUTPUT
void         String_Display(String *string, int n) {
   int i;
   
   for (i=0; i<n; i++) printf(" ");
   printf("%s", string);
}
#endif

