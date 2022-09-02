#include "cmod.h"
#include "memory.h"
#include "stdfunc.h"

#ifdef CMOD_OS_UNIX
 #include <unistd.h>
 #ifdef CMOD_OS_LINUX
  #include <execinfo.h>
  #include <sys/time.h>
  #include <sys/types.h>
 #endif
#endif

#ifdef CMOD_MATH
#include <math.h>
#endif

/* Directions for array maps */
int std_xd[] = {0, 1, 1, 0,-1,-1,-1, 0, 1};
int std_yd[] = {0, 0,-1,-1,-1, 0, 1, 1, 1};
/* Directions for hexagons */
int std_hxd[2][7] = {{0, 1, 0, -1, -1, -1, 0}, {0, 1, 1, 0, -1, 0, 1}};
int std_hyd[] = {0, 0, -1, -1, 0, 1, 1};

unsigned const char std_bit[] = {1,2,4,8,16,32,64,128};
unsigned const char std_notbit[] = {254,253,251,247,239,223,191,127};

Bool std_htmlmode = FALSE;

/*
char std_nofbits[256] = {
0,
1,
1,2,
1,2,2,3,
1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};
*/
#if 0
FILE *std_print = stdout, *std_warning = stdout, *std_error = stderr;
#endif
#define std_print stdout
#define std_warning stdout
#define std_error stdout

/*-------------------------------------------------------------------------*/

void Int_Init(Int *i, Int *i_org) {
   if (i_org == NULL) *i = 0;
   else *i = *i_org;
}

/*-------------------------------------------------------------------------*/

Int *Int_Create(int nof) { return Memory_Reserve(nof, int); }

/*-------------------------------------------------------------------------*/

Int *Int_CreateVal(int nof, int val) {
   int i;
   Int *b = Int_Create(nof);
   for (i=0; i<nof; i++) b[i] = val;
   return b;
}

/*-------------------------------------------------------------------------*/

Int *Int_CreateDim(int nof, ... ) {
   int i ,v;
   Int *a;
   va_list args;
      
   a = Int_Create(nof);
      
   va_start(args, nof);
   for (i=0; i<nof; i++) {
      v = va_arg(args, unsigned int);
      a[i] = v;
   }
   va_end(args);
   return a;
}

/*-------------------------------------------------------------------------*/

Int *Int_Clone(Int *a, int dim) { 
   Int *b;
   int i;
   b = Int_Create(dim);
   for (i=0; i<dim; i++) b[i] = a[i];
   return b;
}

/*-------------------------------------------------------------------------*/

int Int_CompareDescending(const void *a, const void *b) { return *(const int *)b - *(const int *)a; } 

/*-------------------------------------------------------------------------*/

int Int_Compare(const void *a, const void *b) { return *(const int *)a - *(const int *)b; } 

/*-------------------------------------------------------------------------*/

void Int_Sort(int *a, int nof) { qsort(a, nof, sizeof(int), Int_Compare); }

/*-------------------------------------------------------------------------*/

void Int_SortDescending(int *a, int nof) { qsort(a, nof, sizeof(int), Int_CompareDescending); }

/*-------------------------------------------------------------------------*/

int Int_Max(int *arr, int nof) {
   int i, max = arr[0];
   for(i=1; i<nof; i++) if (arr[i]>max) max = arr[i];
   return max;
}

/*-------------------------------------------------------------------------*/

int Int_Min(int *arr, int nof) {
   int i, min = arr[0];
   for(i=1; i<nof; i++) if (arr[i]<min) min = arr[i];
   return min;
}

/*-------------------------------------------------------------------------*/

int Int_ArgMax(int *arr, int nof) {
   int i, argmax = 0, max = arr[0];
   for(i=1; i<nof; i++) if (arr[i]>max) { max = arr[i]; argmax = i; }
   return argmax;
}

/*-------------------------------------------------------------------------*/

int Int_ArgMin(int *arr, int nof)  {
   int i, argmin = 0, min = arr[0];
   for(i=1; i<nof; i++) if (arr[i]<min) { min = arr[i]; argmin = i; }
   return argmin;
}

/*-------------------------------------------------------------------------*/

void Int_Replace(int *a, int nof, int s, int t) {
   int i;
   for (i=0; i<nof; i++) if (a[i] == s) a[i] = t;
}

/*-------------------------------------------------------------------------*/

unsigned int Int_ByteSwap(unsigned int a) {
   return (a>>24) + (((a>>16)&255) << 8) + (((a>>8)&255) << 16) + (a<<24);
}

/*-------------------------------------------------------------------------*/

void Int_Destroy(Int *a) { Memory_Free(a, int); }

/*-------------------------------------------------------------------------*/

Short *Short_Create(int nof) { return Memory_Reserve(nof, short); }

/*-------------------------------------------------------------------------*/

Short *Short_CreateVal(int nof, int val) {
   int i;
   Short *b = Short_Create(nof);
   for (i=0; i<nof; i++) b[i] = val;
   return b;
}

/*-------------------------------------------------------------------------*/

void   Short_Destroy(Short *a) { Memory_Free(a, short); }

/*-------------------------------------------------------------------------*/

Double *Double_Create(int dim) { return Memory_Reserve(dim, Double); }

/*-------------------------------------------------------------------------*/

Double *Double_CreateVal(int dim, double val) { 
   int i;
   Double *d = Memory_Reserve(dim, Double);
   for (i=0; i<dim; i++) d[i] = val;
   return d;
}

/*-------------------------------------------------------------------------*/

Double *Double_Realloc(double *ptr, int new_len) {
   return Memory_Realloc(ptr, new_len*sizeof(double), Double);
}

/*-------------------------------------------------------------------------*/

int     Double_CompareFirst(Double *f1, Double *f2) { 
   if (*f1<*f2) return -1; 
   else
   if (*f1>*f2) return 1; 
   else
   return 0;
}

/*-------------------------------------------------------------------------*/

int Double_CompareSecondDescending(Double *f1, Double *f2) { 
   if (f1[1]<f2[1]) return -1; 
   else
   if (f1[1]>f2[1]) return 1; 
   else
   return 0;
}

/*-------------------------------------------------------------------------*/

int Double_CompareSecondAscending(Double *f1, Double *f2) { 
   if (f1[1]>f2[1]) return -1; 
   else
   if (f1[1]<f2[1]) return 1; 
   else
   return 0;
}

/*-------------------------------------------------------------------------*/

double Double_Max(double *arr, int len) {
   int i = len-1;
   double max = arr[i];
   while(i--) if (arr[i]>max) max = arr[i];
   return max;
}

/*-------------------------------------------------------------------------*/

double Double_Min(double *arr, int len) {
   int i = len-1;
   double min = arr[i];
   while(i--) if (arr[i]<min) min = arr[i];
   return min;
}

/*-------------------------------------------------------------------------*/

#ifdef CMOD_MATH
double Double_AbsMax(double *arr, int len) {
   int i = len-1;
   double max = fabs(arr[i]);
   while(i--) if (fabs(arr[i])>max) max = fabs(arr[i]);
   return max;
}

/*-------------------------------------------------------------------------*/

double Double_AbsMin(double *arr, int len) {
   int i = len-1;
   double min = fabs(arr[i]);
   while(i--) if (fabs(arr[i])<min) min = fabs(arr[i]);
   return min;
}
#endif

/*-------------------------------------------------------------------------*/

int Double_ArgMax(double *arr, int len) {
   int i, argmax = len-1;
   double max = arr[argmax];
   i=argmax;
   while(i--) if (arr[i]>max) { max = arr[i]; argmax = i; }
   return argmax;
}

/*-------------------------------------------------------------------------*/

int Double_ArgMin(double *arr, int len) {
   int argmin = len-1, i = argmin;
   double min = arr[argmin];
   while(i--) if (arr[i]<min) { min = arr[i]; argmin = i; }
   return argmin;
}

/*-------------------------------------------------------------------------*/

void Double_Add(double *ptr, int len, double val) {
   int i;
   for(i=0; i<len; i++) ptr[i]+=val;
}

/*-------------------------------------------------------------------------*/

void Double_Sub(double *ptr, int len, double val) {
   int i;
   for(i=0; i<len; i++) ptr[i]-=val;
}

/*-------------------------------------------------------------------------*/

void Double_Mul(double *ptr, int len, double val) {
   int i;
   for(i=0; i<len; i++) ptr[i]*=val;
}

/*-------------------------------------------------------------------------*/

double Double_Avg(Double *ptr, int len) {
   double sum = 0.0;
   int i;
   for (i=0; i<len; i++) sum+= ptr[i];
   return sum / (double)len;
}

/*-------------------------------------------------------------------------*/

double *Double_Clone(Double *d, int nof) {
   Double *d2;
   int i;
   d2 = Double_Create(nof);
   for (i=0; i<nof; i++) d2[i] = d[i];
   return d2;
}

/*-------------------------------------------------------------------------*/

Double *Double_Clone2D(Double *d) {
   Double *d2 = Memory_Reserve(2, Double);
   d2[0] = d[0];
   d2[1] = d[1];
   return d2;
}

/*-------------------------------------------------------------------------*/

/* Quicksort with middle elem as pivot */
void Double_SortAscending(Double *arr, int first, int one_after_last) {
   int i = first, j = one_after_last-1;
   double z = arr[(first + one_after_last) >> 1];

    do {
       while(arr[i] < z) i++;
       while(arr[j] > z) j--;

       if(i <= j) {
          double y = arr[i];
          arr[i] = arr[j]; 
          arr[j] = y;
          i++; 
          j--;
       }
    } while(i <= j);

    if(first < j) Double_SortAscending(arr, first, j+1);
    if(i < one_after_last) Double_SortAscending(arr, i, one_after_last); 
}

/*-------------------------------------------------------------------------*/

void Double_Destroy(Double *dt) { if (dt != NULL) Memory_Free(dt, Double); }

/*-------------------------------------------------------------------------*/
/*
darray:
   max_elems
   nof_elems
   elem --> [ 
a ->  (int *) -> int, 
      (int *), 
b ->  (int *) , 
      ... , 
      (int *) ]
**a
*/

Bool Int_IsEqual(int *a, int *b) { return (*a == *b); }

/*-------------------------------------------------------------------------*/

int Int_SortKey(int *a) { return *a; }

/*-------------------------------------------------------------------------*/

void Int_Display(int *a) { printf("%i", *a); }

/*-------------------------------------------------------------------------*/

int **Int2_Create(int cols, int rows) {
   int i;
   int **ret;
   ret = Memory_Reserve(rows, int *);
   for (i=0; i<rows; i++) ret[i] = Int_Create(cols);
   return ret;
}

/*-------------------------------------------------------------------------*/

int **Int2_CreateVal(int cols, int rows, int init) {
   int i;
   int **ret;
   ret = Memory_Reserve(rows, int *);
   for (i=0; i<rows; i++) ret[i] = Int_CreateVal(cols, init);
   return ret;
}

/*-------------------------------------------------------------------------*/

int **Int2_Clone(int **ptr, int rows, int cols) {
   int **d2;
   int i,j;
   d2 = Int2_Create(cols, rows);
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         d2[i][j] = ptr[i][j];
      }
   }
   return d2;
}

/*-------------------------------------------------------------------------*/

int Int2_Max(int **ptr, int cols, int rows) {
   int i, j, max = ptr[0][0];
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         if (ptr[i][j]>max) max = ptr[i][j];
      }
   }
   return max;
}

/*-------------------------------------------------------------------------*/

int Int2_Min(int **ptr, int cols, int rows) {
   int i, j, min = ptr[0][0];
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         if (ptr[i][j]<min) min = ptr[i][j];
      }
   }
   return min;
}


/*-------------------------------------------------------------------------*/

void Int2_Destroy(int **ptr, int rows) {
   int i;
   for (i=0; i<rows; i++) Int_Destroy(ptr[i]);
   Memory_Free(ptr, int *);
}

/*-------------------------------------------------------------------------*/

double **Double2_Create(int cols, int rows) {
   int i;
   double **ret;
   ret = Memory_Reserve(rows, double *);
   for (i=0; i<rows; i++) ret[i] = Double_Create(cols);
   return ret;
}

/*-------------------------------------------------------------------------*/

double **Double2_CreateVal(int cols, int rows, double val) {
   int i;
   double **ret;
   ret = Memory_Reserve(rows, double *);
   for (i=0; i<rows; i++) ret[i] = Double_CreateVal(cols, val);
   return ret;
}

/*-------------------------------------------------------------------------*/

void Double2_Destroy(double **ptr, int rows) {
   int i;
   for (i=0; i<rows; i++) Double_Destroy(ptr[i]);
   Memory_Free(ptr, double *);
}

/*-------------------------------------------------------------------------*/

void Double2_Mul(double **ptr, int cols, int rows, double val) {
   int i,j;
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         ptr[i][j] *= val;
      }
   }
}

/*-------------------------------------------------------------------------*/

#ifdef CMOD_MATH
double Double2_AbsMax(double **ptr, int cols, int rows) {
   double max = fabs(ptr[0][0]);
   int i, j;
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         max = MAX(max, fabs(ptr[i][j]));
      }
   }
   return max;
}
#endif

/*-------------------------------------------------------------------------*/

double Double2_Max(double **ptr, int cols, int rows) {
   double max = ptr[0][0];
   int i, j;
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         max = MAX(max, ptr[i][j]);
      }
   }
   return max;
}

/*-------------------------------------------------------------------------*/

double Double2_Min(double **ptr, int cols, int rows) {
   double min = ptr[0][0];
   int i, j;
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         min = MIN(min, ptr[i][j]);
      }
   }
   return min;
}

/*-------------------------------------------------------------------------*/

/* Used Double_ArgMin() to find the min column in the returned row */
int Double2_ArgMinRow(double **ptr, int cols, int rows) {
   double min = ptr[0][0];
   int i, j, minrow = 0;
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         if (ptr[i][j] < min) {
            minrow = i;
            min = ptr[i][j];
         }
      }
   }
   return minrow;
}


/*-------------------------------------------------------------------------*/

double **Double2_Clone(double **ptr, int rows, int cols) {
   double **d2;
   int i,j;
   d2 = Double2_Create(cols, rows);
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         d2[i][j] = ptr[i][j];
      }
   }
   return d2;
}

/*-------------------------------------------------------------------------*/

Char *Char_Create(int nof) { return Memory_Reserve(nof, char); }

/*-------------------------------------------------------------------------*/

Char *Char_CreateVal(int nof, char val) {
   int i;
   Char *a = Char_Create(nof);
   for (i=0; i<nof; i++) a[i] = val;
   return a;
}

/*-------------------------------------------------------------------------*/

void  Char_Destroy(Char *a) { Memory_Free(a, char); }

/*-------------------------------------------------------------------------*/

void Char_Add(Char *a, Char *b, int nof) {
   int i;
   for (i=0; i<nof; i++) a[i]+=b[i];
}

/*-------------------------------------------------------------------------*/

void Char_Sub(Char *a, Char *b, int nof) {
   int i;
   for (i=0; i<nof; i++) a[i]-=b[i];
}

/*-------------------------------------------------------------------------*/

char **Char2_Create(int cols, int rows) {
   int i;
   char **ret;
   ret = Memory_Reserve(rows, char *);
   for (i=0; i<rows; i++) ret[i] = Char_Create(cols);
   return ret;
}

/*-------------------------------------------------------------------------*/

char **Char2_CreateVal(int cols, int rows, char init) {
   int i;
   char **ret;
   ret = Memory_Reserve(rows, char *);
   for (i=0; i<rows; i++) ret[i] = Char_CreateVal(cols, init);
   return ret;
}

/*-------------------------------------------------------------------------*/

void Char2_Destroy(char **ptr, int rows) {
   int i;
   for (i=0; i<rows; i++) Char_Destroy(ptr[i]);
   Memory_Free(ptr, char *);
}

/*-------------------------------------------------------------------------*/

char **Char2_Clone(char **ptr, int rows, int cols) {
   char **d2;
   int i,j;
   d2 = Char2_Create(cols, rows);
   for (i=0; i<rows; i++) {
      for (j=0; j<cols; j++) {
         d2[i][j] = ptr[i][j];
      }
   }
   return d2;
}

/*-------------------------------------------------------------------------*/

float *Float_Create(int dim) { return Memory_Reserve(dim, float); }

/*-------------------------------------------------------------------------*/

void Float_Destroy(float *f) { Memory_Free(f, float); }

/*-------------------------------------------------------------------------*/

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

unsigned int Int_HashFunc(int *n) {
   unsigned int a,b,c;
   char *k = (char *)n;
   b = c = 0x1753850f;
   a = (k[0] << 24) + (k[1] << 16) + (k[2] << 8) + k[3];
   HASH_MIX(a,b,c);
   return c;
}

/*-------------------------------------------------------------------------*/

unsigned int Void_HashFunc(void *p) {
   unsigned int a,b,c;
   char *k = (char *)&p;
   b = c = 0xee876af4;
   a = (k[0] << 24) + (k[1] << 16) + (k[2] << 8) + k[3];
   HASH_MIX(a,b,c);
   return c;
}

/*-------------------------------------------------------------------------*/

Bool Void_IsEqual(void *a, void *b) { return (a==b); }

/*--------------------------------------------------------------------------*/

void Void_Display(void *a) {
   printf("%016llx", (unsigned long long int)a);
}

/*-------------------------------------------------------------------------*/

Bool Bit_Get(void *block, int bit) {
   int byte = bit >> 3;
   return ((unsigned char *)block)[byte] & std_bit[bit & 7];
}

/*-------------------------------------------------------------------------*/

void Bit_Set(void *block, int bit, Bool val) {
   int byte = bit>>3;
   if (val)
      ((unsigned char *)block)[byte] |= std_bit[bit & 7];
   else 
      ((unsigned char *)block)[byte] &= std_notbit[bit & 7];
}

/*-------------------------------------------------------------------------*/

unsigned int Bit_ReadInt(void *block, int *startbit, int nof) {
   unsigned int c = 0;
   int i;
   for (i=0; i<nof; i++) {
      if (Bit_Get(block, (*startbit)++)) c |= (1<<i);
   }
   return c;
}

/*-------------------------------------------------------------------------*/

void Bit_Write(void *block, int *startbit, void *data, int nof_bits) {
   int i;
#ifdef CMOD_DEBUG
   if (nof_bits == 0) Std_Warning("Bit write with 0 bits in length (swapped data and nof_bits?)!\n");
#endif
   for (i=0; i<nof_bits; i++) {
      Bit_Set(block, (*startbit)++, Bit_Get(data, i));
   }
}

/*--------------------------------------------------------------------------*/

void Bit_WriteInt(void *block, int *startbit, unsigned int data, int nof_bits) {
   int i;
#ifdef CMOD_DEBUG
   if (nof_bits == 0) Std_Warning("Bit write with 0 bits in length (swapped data and nof_bits?)!\n");
#endif
   for (i=0; i<nof_bits; i++) {      
      Bit_Set(block, (*startbit)++, data&1);
      data>>=1;
   }
}

/*--------------------------------------------------------------------------*/

/* Handles 0..30 bit ints */

/*
The length of the integer is the number of bits required to code it.
This first stores a length of the integer, and then only the necessary
bits of the int to be stored.
The length is stored in 4 bits, where a value of i means that 2i bits
are to be read as the integer. For <28 bit ints, this saves space cf
storing all 32 bits.
*/

/* 
Observation:
It is better to store half the length in 4 bits than the whole length in 5 bits. 

Proof:
Case analysis:
When the number of bits is even, one bit is gained c.f. storing a 5 bit length.
For odd number of bits, the same number of bits is required as with a 5 bit length.
*/

void Bit_WriteCompressedInt(unsigned char *ptr, int *bitpos, unsigned int int_to_write) {
   unsigned int c = int_to_write;
   int i=0;

   while (c>0) {
      c>>=2;
      i++;
   }
   Bit_WriteInt(ptr, bitpos, 4, i);
   Bit_WriteInt(ptr, bitpos, 2*i, int_to_write);
}

/*--------------------------------------------------------------------------*/

unsigned int Bit_ReadCompressedInt(unsigned char *ptr, int *bitpos) {
   int len = Bit_ReadInt(ptr, bitpos, 4);
   return Bit_ReadInt(ptr, bitpos, 2*len);
}

/*-------------------------------------------------------------------------*/

int Bit_Lowest(unsigned char *c, int n) {
   int i,j;
   for (j=0; j<n/8; j++) {
      for (i=0; i<8; i++) if (*c & std_bit[i]) return j*8+i;
      c++;
   }
   for (i=0; i < (n&7); i++) if (*c & std_bit[i]) return j*8+i;
   return MAXINT;
}

/*-------------------------------------------------------------------------*/

void Bit_Display(unsigned char *ptr, int n) { /* print n bits (0 padded) */
   int i,j;
   for (j=0; j<n/8; j++) {
      for (i=0; i<8; i++) if (*ptr & std_bit[i]) printf("1"); else printf("0");
      ptr++;
   }
   for (i=0; i < (n&7); i++) if (*ptr & std_bit[i]) printf("1"); else printf("0");
   
}
/*-------------------------------------------------------------------------*/

#ifdef CMOD_OS_UNIX

void SleepUnix(double seconds) {
   struct timeval timeout = {(int)seconds,(int)((seconds - (double)(int)seconds)*1000000)};
   select(0, NULL, NULL, NULL, &timeout);
}

/*-------------------------------------------------------------------------*/

double TimeUnix() {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (double)tv.tv_usec / 1000000.0 + (double)tv.tv_sec;
}

/*-------------------------------------------------------------------------*/

int Std_LaunchProcess(const String *command, int *process_stdin, int *process_stdout) {
   int from_prog[2], to_prog[2];
   pid_t pid;

   if (pipe(from_prog) < 0) return -errno;
   if (pipe(to_prog) < 0) return -errno;
   
   pid = vfork();
   if (pid < 0) return -errno;

   if (pid == 0) {
      close(0);
      dup(to_prog[0]);
      close(to_prog[1]);
      close(1);
      dup(from_prog[1]);
      close(from_prog[0]);

      if (execlp("/bin/sh", "/bin/sh", "-c", command, (void *)NULL) == -1) {
         fprintf(stderr, "start_proc: execlp of %s failed\n", command);
         exit(-1);
      }
   }

   *process_stdin = from_prog[0];
   *process_stdout = to_prog[1];

   close(from_prog[1]);
   close(to_prog[0]);
   return pid;
}

/*-------------------------------------------------------------------------*/

/* Returns bitmask of read fds: bit 0 is first arg fd, etc. Negative on error */
int Std_Select(double timeout, int nof_fd, ...) {
   fd_set rfds;
   struct timeval tv;
   int sret, ret, i, maxfd=0, fd;
   va_list fd_arg;

   FD_ZERO(&rfds);
   va_start(fd_arg, nof_fd);

   for (i=0; i<nof_fd; i++) {
      fd = va_arg(fd_arg, int);
      FD_SET(fd, &rfds);
      if (fd >= maxfd) maxfd = fd + 1;
   }
   va_end(fd_arg);
   
   tv.tv_sec = (int)timeout;
   tv.tv_usec = (int)((timeout - (double)tv.tv_sec) * 1e6);

   sret = select(maxfd, &rfds, NULL, NULL, &tv);
   ret = 0;
   if (sret > 0) {
      va_start(fd_arg, nof_fd);
      for (i=0; i<nof_fd; i++) {
         fd = va_arg(fd_arg, int);
         if (FD_ISSET(fd, &rfds)) ret |= (1<<i);
      }
      va_end(fd_arg);
   } else
   if (sret < 0) ret = -1;
   
   return ret;
}

/*-------------------------------------------------------------------------*/

int Std_FdReadLine(const int fd, const int maxlen, char *buf) {
   int nof_read=0;
   int ret=0;

   if (Std_Select(0.0, 1, fd) == 1) {
      do {
         ret = read(fd, buf+nof_read, 1);
         if (buf[nof_read] == '\n') break;
         if (ret>0) {
            nof_read++;
            if (nof_read >= maxlen-1) break;
         }
      } while (ret>0 || (ret == -1 && (errno == EAGAIN || errno == EINTR)));
      buf[nof_read] = 0;
      if (buf[nof_read-1] == '\r') buf[nof_read-1] = '\0';
   }
   return nof_read;
   
}

/*-------------------------------------------------------------------------*/

int fdprintf(const int fd, const char *fmt, ... ) {
    va_list ap;
    int size = 0;
    char * buf = NULL;
 
    va_start(ap, fmt);
 
    size = vsnprintf(buf, size, fmt, ap) + 1; /* add one for the null */
    buf = (char *)realloc(buf, size);
    vsnprintf(buf, size, fmt, ap);
 
    write(fd, buf, size - 1);
    
    va_end(ap);
    free(buf);

    return size;
}

#else

#ifdef CMOD_COMPILER_VISUALC
#ifdef Sleep
#undef Sleep
#endif
#include <windows.h>
#include <winbase.h>
void SleepWin(double seconds) { Sleep((int)(seconds*1000.0)); }
double TimeWin() { 
	double t;
	SYSTEMTIME st;
	GetSystemTime(&st);
	t = (double)st.wMilliseconds;
	t *= 0.001;
	t += (double)time(NULL);
	return t;
}
#else
//void Sleep(double seconds) { sleep((int)seconds); }
//double Time() { return time(NULL); }
#endif


#endif

/*-------------------------------------------------------------------------*/

typedef struct {
   int nof;
   int firstix;
   int currentix;
} Bucket;

Bucket *Bucket_Create() {
   Bucket *b;
   b = Memory_Reserve(1, Bucket);
   b->nof = 0;
   return b;
}

void Bucket_Destroy(Bucket *bucket) { Memory_Free(bucket, Bucket); }

/*-------------------------------------------------------------------------*/

void Void_InsertionSortIx(void **array, void **array2, SortFunc *sortkey, int beg, int end) {
   int nof;
//   int start, stop;
   int i, j, dj, prevj, prev2j;
   int cur, cmp;

   nof = end - beg;

   if (nof <= 0) return;
   if (nof == 1) {
      array2[beg] = array[beg];
      return;
   }
//   start = beg;
//   stop = beg;

   if (sortkey(array[beg]) > sortkey(array[beg+1])) {
      array2[beg] = array[beg+1];
      array2[beg+1] = array[beg];
   } else {
      array2[beg] = array[beg];
      array2[beg+1] = array[beg+1];
   }

   for (i=beg+2; i<end; i++) {
      cur = sortkey(array[i]);
      j = (i + beg) >> 1;
      dj = ((i - beg) >> 2);
/*
      printf("next elm: %i, array = [", cur);

      for (t=beg; t<i-1; t++) printf("%i, ", sortkey(array2[t]));
      printf("%i] ", sortkey(array2[i-1]));
*/
      prev2j = prevj = -MAXINT;
      while (j != prev2j) {
         prev2j = prevj;
         prevj = j;

//         printf("(%i,%i)",j,dj);          
         if (j>=i) {
            j=i-1;
         }
         cmp = sortkey(array2[j]);
         if (cmp > cur) j-=dj;
         else j+=dj;
         dj>>=1;
         if (j<=beg) {
            j=beg;
         }

         if (dj<=0) dj=1;
      }
//      if (j==i-1 && sortkey(array2[j]) < cur) j++;
//      else
      if (j<prevj) j=prevj;
//      printf(", inserting at %i\n", j);
      for (dj=i+1; dj>j; dj--) {
         array2[dj] = array2[dj-1];
      }
      array2[j] = array[i];
   }
}

#define BUCKET_BITS 8
#define BUCKET_MASK 255
#define BUCKET_NOF 256

void Void_RadixSortIxLvl(void **array, void **array2, SortFunc *sortkey, int beg, int end, int currentshift) {
   int i;
   Bucket *bucket;
   int currentix;
   int buck;

   if (currentshift < 0) return;
   if (end - beg < 1) return;
   if (end - beg < 64) {
      Void_InsertionSortIx(array, array2, sortkey, beg, end);
      if ((currentshift&15) != 0) {
         Memory_Copy(&array[beg], &array2[beg], sizeof(void *)*(end - beg));
      }
      return;
   }

   bucket = Memory_Reserve(BUCKET_NOF, Bucket);

   for (i=0; i<BUCKET_NOF; i++) {
      bucket[i].nof = 0;
   }

   for (i=beg; i<end; i++) {
      bucket[(sortkey(array[i])>>currentshift)&BUCKET_MASK].nof++;
   }

   currentix=beg;
   for (i=0; i<BUCKET_NOF; i++) {
      bucket[i].firstix = currentix;
      bucket[i].currentix = currentix;
      currentix+=bucket[i].nof;
//      printf("%i, %i: %i\n", currentshift, i, bucket[i].nof);
   }

   for (i=beg; i<end; i++) {
      buck = (sortkey(array[i])>>currentshift)&BUCKET_MASK;
      array2[bucket[buck].currentix++] = array[i];
   }

   for (i=0; i<BUCKET_NOF; i++) {
      Void_RadixSortIxLvl(array2, array, sortkey, bucket[i].firstix, bucket[i].firstix+bucket[i].nof, currentshift-BUCKET_BITS);
   }

   Memory_Free(bucket, Bucket);
}
/*-------------------------------------------------------------------------*/

void Void_RadixSortIx(void **array, SortFunc *sortkey, int beg, int end) {
   int currentshift;
   void **array2;

   currentshift = 8*sizeof(int);
   array2 = Memory_Reserve(end-beg, void *);

   Void_RadixSortIxLvl(array, array2, sortkey, beg, end, currentshift - BUCKET_BITS);
   Memory_Free(array2, void *);
}

/*-------------------------------------------------------------------------*/

void Void_RadixSort(void **array, SortFunc *sortkey, int nof) {
   Void_RadixSortIx(array, sortkey, 0, nof);
}

/*-------------------------------------------------------------------------*/

void swapelems(float *keys, void **array, int a, int b) {
   float t;
   void *t2;
   
   t = keys[a];
   keys[a] = keys[b];
   keys[b] = t;
   
   t2 = array[a];
   array[a] = array[b];
   array[b] = t2;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Void_QuickSortFloatAsc(float *keys, void **array, int n) {
   float pivot;
   int i;
   int less;
   int dest;
   
   if (n<=1) return;
   if (n==2) {
      if (keys[0] > keys[1]) swapelems(keys, array, 0, 1);
      return;
   }
   
   pivot = keys[0];
   less = 0;
   
   for (i=1; i<n; i++) {
      if (keys[i] < pivot) less++;
   }
   
   swapelems(keys, array, 0, less);
   
   dest = less+1;
   for (i=0; i<less; i++) {
      if (keys[i] > pivot) {
         swapelems(keys, array, i, dest);
         dest++;
         i--;
      }
   }
   Void_QuickSortFloatAsc(keys, array, less);
   Void_QuickSortFloatAsc(&keys[less+1], &array[less+1], n-less-1);
}

/*-------------------------------------------------------------------------*/

void Void_QuickSortFloatDesc(float *keys, void **array, int n) {
   float pivot;
   int i;
   int larger;
   int dest;
   
   if (n<=1) return;
   if (n==2) {
      if (keys[0] < keys[1]) swapelems(keys, array, 0, 1);
      return;
   }
   
   pivot = keys[0];
   larger = 0;
   
   for (i=1; i<n; i++) {
      if (keys[i] > pivot) larger++;
   }
   
   swapelems(keys, array, 0, larger);
   
   dest = larger+1;
   for (i=0; i<larger; i++) {
      if (keys[i] < pivot) {
         swapelems(keys, array, i, dest);
         dest++;
         i--;
      }
   }
   Void_QuickSortFloatDesc(keys, array, larger);
   Void_QuickSortFloatDesc(&keys[larger+1], &array[larger+1], n-larger-1);
}

/* CODE TO TEST SORTERS */
/* 
   float keys[10] = {1, 5, 7, 2, 6, 3, 9, 0, 4, 8};
   int i;
      
   for (i=0; i<10; i++) printf("(%.1f %s), ", keys[i], words[i]);

   Void_QuickSortFloatDesc(keys, (void **)&words[0], 10);
   
   printf("\n");
   for (i=0; i<10; i++) printf("(%.1f %s), ", keys[i], words[i]);
*/

/*-------------------------------------------------------------------------*/
#ifdef CMOD_COMPILER_GCC
void StdError(const char *where, const char* format, ...) {
  va_list   params;

  fprintf(std_error, "ERROR (%s): ", where);
  va_start(params, format);
  vfprintf(std_error, format, params);
  va_end(params);

}

/*-------------------------------------------------------------------------*/

void StdWarning(const char *where, const char* format, ...) {
  va_list   params;

  fprintf(std_warning, "Warning (%s): ", where);
  va_start(params, format);
  vfprintf(std_warning, format, params);
  va_end(params);
}
#else

/*-------------------------------------------------------------------------*/

void Std_Error(char* format, ...) {
  va_list   params;

  fprintf(std_error, "ERROR : ");
  va_start(params, format);
  vfprintf(std_error, format, params);
  va_end(params);

}

/*-------------------------------------------------------------------------*/

void Std_Warning(char* format, ...) {
  va_list   params;

  fprintf(std_warning, "Warning : ");
  va_start(params, format);
  vfprintf(std_warning, format, params);
  va_end(params);
}
#endif

/*-------------------------------------------------------------------------*/

void Error_Fatal(char* format, ...) {
  va_list   params;

  va_start(params, format);
  vfprintf(std_error, format, params);
  va_end(params);

  exit(-1);
}

#define STD_STACKTRACESIZE 8192
#define STD_MAXFILENAME 512
#define STD_MAXFUNCNAME 250

void Std_StackDumpFull(FILE *out, const char *message, const char *color, int toskip, int maxtoprint, Bool box) {
#ifdef CMOD_OS_LINUX
   static void *stacktrace[STD_STACKTRACESIZE];
   static char exe[STD_MAXFILENAME];
   static char cmd[STD_MAXFILENAME+128];
   static char line[STD_MAXFUNCNAME];
   static char func[STD_MAXFUNCNAME];
   static int size,ret,i,len,recursion;
   static FILE *addr;

   ret = readlink("/proc/self/exe", exe, STD_MAXFILENAME);
   if (ret <= 0 || ret >= STD_MAXFILENAME || exe[0] != '/') {
      fprintf(out, "%s\n", message);
      fprintf(out, "Cannot dump stack (Unrecognized /proc filesystem)\n");
   } else {
      exe[ret] = 0;      
      size = backtrace(stacktrace, STD_STACKTRACESIZE);
//      backtrace_symbols_fd(stacktrace, size, fileno(out));
      if (fileno(out) == 1 || fileno(out) == 2) fprintf(out, "%s", color);
      if (box) {
         fprintf(out, "\n+------------------------------------------------------------------------------+\n");
         fprintf(out, "| %-76s |\n",message);
         fprintf(out, "+------------------------------------------------------------------------------+\n");
      } else {
         fprintf(out, "%s\n", message);
      }
      
      recursion = 0;
      size = MIN(size, maxtoprint);
      for (i=toskip; i<size; i++) {
         if (i>toskip && i<size-1 && stacktrace[i] == stacktrace[i-1]) recursion++; 
         else {
            if (recursion>0) {
               if (box) fprintf(out, "| ");
               len = fprintf(out, "          :  +%i recursive calls ", recursion);
               if (box) {
                  for(;len<77;len++) fprintf(out, " "); 
                  fprintf(out, "|");
               }
               fprintf(out, "\n");
            }
            /* addr2line no longer works in 64-bit mode, the stack addresses are virtual not actual. use backtrace_symbols if needed */
            sprintf(cmd, "addr2line -e %s -s -i -f -C %p", exe, stacktrace[i]);
            addr = popen(cmd, "r");
            if (addr <= 0 || fgets(func, STD_MAXFUNCNAME, addr) == 0 || fgets(line, STD_MAXFUNCNAME, addr) == 0) {
               if (box) fprintf(out, "| ");
               fprintf(out, "Cannot obtain file/line/function information for adress %p",stacktrace[i]);
               if (box) fprintf(out, "                   |");
               fprintf(out, "\n");
            } else {
               len = strlen(func);
               while(len>0 && func[len-1] == '\n') len--;
               func[len++]='(';
               func[len++]=')';
               func[len]=0;
               if (strcmp(func, "Memory_SigSegvHandler()") == 0 || strcmp(func, "Std_StackDumpFull()")==0) continue;

               len = strlen(line);
               while(len>0 && line[len-1] == '\n') len--;
               line[len]=0;
           
               if (line[0] != '?') {
                  static int lineno, t;
                  t = 0;
                  while (line[t] != 0) {
                     if (line[t++] == ':') {
                        /* Correct line number by one */
                        lineno = atoi(line+t); 
                        sprintf(line+t, "%i", lineno-1);
                        break;
                     }
                  }
                  if (box) fprintf(out, "| ");
                  fprintf(out, "%p : %-36s @  %-24s", stacktrace[i], func, line);
                  if (box) fprintf(out, " |");
                  fprintf(out, "\n");
               }
               recursion = 0;
            }
         }
      }
      if (box)
         fprintf(out, "+------------------------------------------------------------------------------+\n");
      if (fileno(out) == 1 || fileno(out) == 2) {
         if (!std_htmlmode) fprintf(out, ANSI_GRAY ANSI_NORMAL ANSI_DEFAULT);
         else fprintf(out, "</pre></font>");
      }
//      backtrace_symbols_fd(stacktrace, STD_STACKTRACESIZE, fileno(out));
   }
#else
   fprintf(out, "%s\n", message);
   fprintf(out, "Stack dump not available.\n");
#endif
}

/*-------------------------------------------------------------------------*/

void Std_Print(char* format, ...) {
  va_list   params;

  va_start(params, format);
  vfprintf(std_print, format, params);
  va_end(params);
}

/*-------------------------------------------------------------------------*/

void Std_PrintChar(int n, char c) {
   int i;
   for (i=0; i<n; i++) fputc(c, std_print);
}

/*-------------------------------------------------------------------------*/

void Std_HexDump(void *object, int size) {
   int i;
   unsigned char *data;
   
   data = (unsigned char *)object;
   for (i=0; i<size; i++) {
      if ((i&15) == 0) {
         fprintf(std_print, "%04x : ", i);
      }
      if ((i&7) == 0) fprintf(std_print, ANSI_CYAN);
      else
      if ((i&7) == 4) fprintf(std_print, ANSI_GRAY);
      fprintf(std_print, "%02x ", data[i]);
      if ((i&15) == 15) fprintf(std_print, "\n");
   }
   if ((i&15) != 0) fprintf(std_print, "%s\n", ANSI_GRAY);
}

/* ------------------------------------------------------------------------- */

void Std_FloatingPointException(int signum) {
   Std_StackDump("Floating point exception!\n");
   exit(-1);
}

/* ------------------------------------------------------------------------- */

#ifdef CMOD_OS_UNIX
#include <signal.h>
void Std_SetupFPE(void) { signal(SIGFPE, Std_FloatingPointException); }
#endif

/* ------------------------------------------------------------------------- */

int Std_GCD(int a, int b) {
   while (a > 0 && b > 0) {
      if (a>b) a %= b;
      else b %= a;
   }
   if (a == 0) return b; else return a;
}

/* ------------------------------------------------------------------------- */

int StdIn_ReadInt() {
   int a;
   if (fscanf(stdin, "%i", &a) == 1) return a;
   return -1;
}

/* ------------------------------------------------------------------------- */

int  *StdIn_ReadInts(int nof) {
   int i;
   int *res;
   
   res = (int *)malloc(sizeof(int) * nof);
   
   for (i=0; i<nof; i++) {
      if (fscanf(stdin, "%i", &res[i]) != 1) {
         free(res);
         return NULL;
      }
   }
   return res;
}

/* ------------------------------------------------------------------------- */

char *StdIn_ReadString() {
   char *line = (char*)malloc(1004);
   char *ret;
   int i;
   ret = fgets(line, 1004, stdin);
   if (ret == NULL) free(line);
   else {
      i = 0;
      while(ret[i] != 0) {
         if (ret[i]==0xa || ret[i]==0xd) { ret[i] = 0; break; }
         i++;
      }
   }
   return ret;
}

/* ------------------------------------------------------------------------- */

void  StdIn_SkipLine() {
   char temp[81];
   fgets(temp, 80, stdin);
}

/* ------------------------------------------------------------------------- */

#ifndef CMOD_OPTIMIZE_SIZE
short sin1024[] __attribute__ ((aligned (4))) = {
    0x000, 0x006, 0x00c, 0x012, 0x019, 0x01f, 0x025, 0x02b, 0x032, 0x038, 0x03e, 0x045, 0x04b, 0x051, 0x057, 0x05e,
     0x064, 0x06a, 0x070, 0x077, 0x07d, 0x083, 0x089, 0x090, 0x096, 0x09c, 0x0a2, 0x0a8, 0x0af, 0x0b5, 0x0bb, 0x0c1,
     0x0c7, 0x0cd, 0x0d4, 0x0da, 0x0e0, 0x0e6, 0x0ec, 0x0f2, 0x0f8, 0x0fe, 0x104, 0x10b, 0x111, 0x117, 0x11d, 0x123,
     0x129, 0x12f, 0x135, 0x13b, 0x141, 0x147, 0x14d, 0x153, 0x158, 0x15e, 0x164, 0x16a, 0x170, 0x176, 0x17c, 0x182,
     0x187, 0x18d, 0x193, 0x199, 0x19e, 0x1a4, 0x1aa, 0x1b0, 0x1b5, 0x1bb, 0x1c1, 0x1c6, 0x1cc, 0x1d2, 0x1d7, 0x1dd,
     0x1e2, 0x1e8, 0x1ed, 0x1f3, 0x1f8, 0x1fe, 0x203, 0x209, 0x20e, 0x213, 0x219, 0x21e, 0x223, 0x229, 0x22e, 0x233,
     0x238, 0x23e, 0x243, 0x248, 0x24d, 0x252, 0x257, 0x25c, 0x261, 0x267, 0x26c, 0x271, 0x276, 0x27a, 0x27f, 0x284,
     0x289, 0x28e, 0x293, 0x298, 0x29c, 0x2a1, 0x2a6, 0x2ab, 0x2af, 0x2b4, 0x2b8, 0x2bd, 0x2c2, 0x2c6, 0x2cb, 0x2cf,
     0x2d4, 0x2d8, 0x2dc, 0x2e1, 0x2e5, 0x2e9, 0x2ee, 0x2f2, 0x2f6, 0x2fa, 0x2ff, 0x303, 0x307, 0x30b, 0x30f, 0x313,
     0x317, 0x31b, 0x31f, 0x323, 0x327, 0x32b, 0x32e, 0x332, 0x336, 0x33a, 0x33d, 0x341, 0x345, 0x348, 0x34c, 0x34f,
     0x353, 0x356, 0x35a, 0x35d, 0x361, 0x364, 0x367, 0x36b, 0x36e, 0x371, 0x374, 0x377, 0x37a, 0x37e, 0x381, 0x384,
     0x387, 0x38a, 0x38c, 0x38f, 0x392, 0x395, 0x398, 0x39a, 0x39d, 0x3a0, 0x3a2, 0x3a5, 0x3a8, 0x3aa, 0x3ad, 0x3af,
     0x3b2, 0x3b4, 0x3b6, 0x3b9, 0x3bb, 0x3bd, 0x3bf, 0x3c2, 0x3c4, 0x3c6, 0x3c8, 0x3ca, 0x3cc, 0x3ce, 0x3d0, 0x3d2,
     0x3d3, 0x3d5, 0x3d7, 0x3d9, 0x3da, 0x3dc, 0x3de, 0x3df, 0x3e1, 0x3e2, 0x3e4, 0x3e5, 0x3e7, 0x3e8, 0x3e9, 0x3eb,
     0x3ec, 0x3ed, 0x3ee, 0x3ef, 0x3f0, 0x3f1, 0x3f3, 0x3f3, 0x3f4, 0x3f5, 0x3f6, 0x3f7, 0x3f8, 0x3f9, 0x3f9, 0x3fa,
     0x3fb, 0x3fb, 0x3fc, 0x3fc, 0x3fd, 0x3fd, 0x3fe, 0x3fe, 0x3fe, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff
};
short cos1024[] __attribute__ ((aligned (2))) = {
    0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3fe, 0x3fe, 0x3fe, 0x3fd, 0x3fd, 0x3fc, 0x3fc, 0x3fb,
     0x3fb, 0x3fa, 0x3f9, 0x3f9, 0x3f8, 0x3f7, 0x3f6, 0x3f5, 0x3f4, 0x3f3, 0x3f3, 0x3f1, 0x3f0, 0x3ef, 0x3ee, 0x3ed,
     0x3ec, 0x3eb, 0x3e9, 0x3e8, 0x3e7, 0x3e5, 0x3e4, 0x3e2, 0x3e1, 0x3df, 0x3de, 0x3dc, 0x3da, 0x3d9, 0x3d7, 0x3d5,
     0x3d3, 0x3d2, 0x3d0, 0x3ce, 0x3cc, 0x3ca, 0x3c8, 0x3c6, 0x3c4, 0x3c2, 0x3bf, 0x3bd, 0x3bb, 0x3b9, 0x3b6, 0x3b4,
     0x3b2, 0x3af, 0x3ad, 0x3aa, 0x3a8, 0x3a5, 0x3a2, 0x3a0, 0x39d, 0x39a, 0x398, 0x395, 0x392, 0x38f, 0x38c, 0x38a,
     0x387, 0x384, 0x381, 0x37e, 0x37a, 0x377, 0x374, 0x371, 0x36e, 0x36b, 0x367, 0x364, 0x361, 0x35d, 0x35a, 0x356,
     0x353, 0x34f, 0x34c, 0x348, 0x345, 0x341, 0x33d, 0x33a, 0x336, 0x332, 0x32e, 0x32b, 0x327, 0x323, 0x31f, 0x31b,
     0x317, 0x313, 0x30f, 0x30b, 0x307, 0x303, 0x2ff, 0x2fa, 0x2f6, 0x2f2, 0x2ee, 0x2e9, 0x2e5, 0x2e1, 0x2dc, 0x2d8,
     0x2d4, 0x2cf, 0x2cb, 0x2c6, 0x2c2, 0x2bd, 0x2b8, 0x2b4, 0x2af, 0x2ab, 0x2a6, 0x2a1, 0x29c, 0x298, 0x293, 0x28e,
     0x289, 0x284, 0x27f, 0x27a, 0x275, 0x271, 0x26c, 0x267, 0x261, 0x25c, 0x257, 0x252, 0x24d, 0x248, 0x243, 0x23e,
     0x238, 0x233, 0x22e, 0x229, 0x223, 0x21e, 0x219, 0x213, 0x20e, 0x209, 0x203, 0x1fe, 0x1f8, 0x1f3, 0x1ed, 0x1e8,
     0x1e2, 0x1dd, 0x1d7, 0x1d2, 0x1cc, 0x1c6, 0x1c1, 0x1bb, 0x1b5, 0x1b0, 0x1aa, 0x1a4, 0x19e, 0x199, 0x193, 0x18d,
     0x187, 0x182, 0x17c, 0x176, 0x170, 0x16a, 0x164, 0x15e, 0x158, 0x153, 0x14d, 0x147, 0x141, 0x13b, 0x135, 0x12f,
     0x129, 0x123, 0x11d, 0x117, 0x111, 0x10b, 0x104, 0x0fe, 0x0f8, 0x0f2, 0x0ec, 0x0e6, 0x0e0, 0x0da, 0x0d4, 0x0cd,
     0x0c7, 0x0c1, 0x0bb, 0x0b5, 0x0af, 0x0a8, 0x0a2, 0x09c, 0x096, 0x090, 0x089, 0x083, 0x07d, 0x077, 0x070, 0x06a,
     0x064, 0x05e, 0x057, 0x051, 0x04b, 0x045, 0x03e, 0x038, 0x032, 0x02b, 0x025, 0x01f, 0x019, 0x012, 0x00c, 0x006,
     0x000,-0x006,-0x00c,-0x012,-0x019,-0x01f,-0x025,-0x02b,-0x032,-0x038,-0x03e,-0x045,-0x04b,-0x051,-0x057,-0x05e,
    -0x064,-0x06a,-0x070,-0x077,-0x07d,-0x083,-0x089,-0x090,-0x096,-0x09c,-0x0a2,-0x0a8,-0x0af,-0x0b5,-0x0bb,-0x0c1,
    -0x0c7,-0x0cd,-0x0d4,-0x0da,-0x0e0,-0x0e6,-0x0ec,-0x0f2,-0x0f8,-0x0fe,-0x104,-0x10b,-0x111,-0x117,-0x11d,-0x123,
    -0x129,-0x12f,-0x135,-0x13b,-0x141,-0x147,-0x14d,-0x153,-0x158,-0x15e,-0x164,-0x16a,-0x170,-0x176,-0x17c,-0x182,
    -0x187,-0x18d,-0x193,-0x199,-0x19e,-0x1a4,-0x1aa,-0x1b0,-0x1b5,-0x1bb,-0x1c1,-0x1c6,-0x1cc,-0x1d2,-0x1d7,-0x1dd,
    -0x1e2,-0x1e8,-0x1ed,-0x1f3,-0x1f8,-0x1fe,-0x203,-0x209,-0x20e,-0x213,-0x219,-0x21e,-0x223,-0x229,-0x22e,-0x233,
    -0x238,-0x23e,-0x243,-0x248,-0x24d,-0x252,-0x257,-0x25c,-0x262,-0x267,-0x26c,-0x271,-0x276,-0x27a,-0x27f,-0x284,
    -0x289,-0x28e,-0x293,-0x298,-0x29c,-0x2a1,-0x2a6,-0x2ab,-0x2af,-0x2b4,-0x2b8,-0x2bd,-0x2c2,-0x2c6,-0x2cb,-0x2cf,
    -0x2d4,-0x2d8,-0x2dc,-0x2e1,-0x2e5,-0x2e9,-0x2ee,-0x2f2,-0x2f6,-0x2fa,-0x2ff,-0x303,-0x307,-0x30b,-0x30f,-0x313,
    -0x317,-0x31b,-0x31f,-0x323,-0x327,-0x32b,-0x32e,-0x332,-0x336,-0x33a,-0x33d,-0x341,-0x345,-0x348,-0x34c,-0x34f,
    -0x353,-0x356,-0x35a,-0x35d,-0x361,-0x364,-0x367,-0x36b,-0x36e,-0x371,-0x374,-0x377,-0x37a,-0x37e,-0x381,-0x384,
    -0x387,-0x38a,-0x38c,-0x38f,-0x392,-0x395,-0x398,-0x39a,-0x39d,-0x3a0,-0x3a2,-0x3a5,-0x3a8,-0x3aa,-0x3ad,-0x3af,
    -0x3b2,-0x3b4,-0x3b6,-0x3b9,-0x3bb,-0x3bd,-0x3bf,-0x3c2,-0x3c4,-0x3c6,-0x3c8,-0x3ca,-0x3cc,-0x3ce,-0x3d0,-0x3d2,
    -0x3d3,-0x3d5,-0x3d7,-0x3d9,-0x3da,-0x3dc,-0x3de,-0x3df,-0x3e1,-0x3e2,-0x3e4,-0x3e5,-0x3e7,-0x3e8,-0x3e9,-0x3eb,
    -0x3ec,-0x3ed,-0x3ee,-0x3ef,-0x3f0,-0x3f1,-0x3f3,-0x3f3,-0x3f4,-0x3f5,-0x3f6,-0x3f7,-0x3f8,-0x3f9,-0x3f9,-0x3fa,
    -0x3fb,-0x3fb,-0x3fc,-0x3fc,-0x3fd,-0x3fd,-0x3fe,-0x3fe,-0x3fe,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,
    -0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3ff,-0x3fe,-0x3fe,-0x3fe,-0x3fd,-0x3fd,-0x3fc,-0x3fc,-0x3fb,
    -0x3fb,-0x3fa,-0x3f9,-0x3f9,-0x3f8,-0x3f7,-0x3f6,-0x3f5,-0x3f4,-0x3f3,-0x3f2,-0x3f1,-0x3f0,-0x3ef,-0x3ee,-0x3ed,
    -0x3ec,-0x3eb,-0x3e9,-0x3e8,-0x3e7,-0x3e5,-0x3e4,-0x3e2,-0x3e1,-0x3df,-0x3de,-0x3dc,-0x3da,-0x3d9,-0x3d7,-0x3d5,
    -0x3d3,-0x3d2,-0x3d0,-0x3ce,-0x3cc,-0x3ca,-0x3c8,-0x3c6,-0x3c4,-0x3c2,-0x3bf,-0x3bd,-0x3bb,-0x3b9,-0x3b6,-0x3b4,
    -0x3b2,-0x3af,-0x3ad,-0x3aa,-0x3a8,-0x3a5,-0x3a2,-0x3a0,-0x39d,-0x39a,-0x398,-0x395,-0x392,-0x38f,-0x38c,-0x38a,
    -0x387,-0x384,-0x381,-0x37e,-0x37a,-0x377,-0x374,-0x371,-0x36e,-0x36b,-0x367,-0x364,-0x361,-0x35d,-0x35a,-0x356,
    -0x353,-0x34f,-0x34c,-0x348,-0x345,-0x341,-0x33d,-0x33a,-0x336,-0x332,-0x32e,-0x32b,-0x327,-0x323,-0x31f,-0x31b,
    -0x317,-0x313,-0x30f,-0x30b,-0x307,-0x303,-0x2ff,-0x2fa,-0x2f6,-0x2f2,-0x2ee,-0x2e9,-0x2e5,-0x2e1,-0x2dc,-0x2d8,
    -0x2d4,-0x2cf,-0x2cb,-0x2c6,-0x2c2,-0x2bd,-0x2b8,-0x2b4,-0x2af,-0x2ab,-0x2a6,-0x2a1,-0x29c,-0x298,-0x293,-0x28e,
    -0x289,-0x284,-0x27f,-0x27a,-0x275,-0x271,-0x26c,-0x267,-0x261,-0x25c,-0x257,-0x252,-0x24d,-0x248,-0x243,-0x23e,
    -0x238,-0x233,-0x22e,-0x229,-0x223,-0x21e,-0x219,-0x213,-0x20e,-0x209,-0x203,-0x1fe,-0x1f8,-0x1f3,-0x1ed,-0x1e8,
    -0x1e2,-0x1dd,-0x1d7,-0x1d1,-0x1cc,-0x1c6,-0x1c1,-0x1bb,-0x1b5,-0x1b0,-0x1aa,-0x1a4,-0x19e,-0x199,-0x193,-0x18d,
    -0x187,-0x182,-0x17c,-0x176,-0x170,-0x16a,-0x164,-0x15e,-0x158,-0x153,-0x14d,-0x147,-0x141,-0x13b,-0x135,-0x12f,
    -0x129,-0x123,-0x11d,-0x117,-0x111,-0x10b,-0x104,-0x0fe,-0x0f8,-0x0f2,-0x0ec,-0x0e6,-0x0e0,-0x0da,-0x0d4,-0x0cd,
    -0x0c7,-0x0c1,-0x0bb,-0x0b5,-0x0af,-0x0a8,-0x0a2,-0x09c,-0x096,-0x090,-0x089,-0x083,-0x07d,-0x077,-0x070,-0x06a,
    -0x064,-0x05e,-0x057,-0x051,-0x04b,-0x045,-0x03e,-0x038,-0x032,-0x02b,-0x025,-0x01f,-0x019,-0x012,-0x00c,-0x006,
     0x000, 0x006, 0x00c, 0x012, 0x019, 0x01f, 0x025, 0x02b, 0x032, 0x038, 0x03e, 0x045, 0x04b, 0x051, 0x057, 0x05e,
     0x064, 0x06a, 0x070, 0x077, 0x07d, 0x083, 0x089, 0x090, 0x096, 0x09c, 0x0a2, 0x0a8, 0x0af, 0x0b5, 0x0bb, 0x0c1,
     0x0c7, 0x0cd, 0x0d4, 0x0da, 0x0e0, 0x0e6, 0x0ec, 0x0f2, 0x0f8, 0x0fe, 0x105, 0x10b, 0x111, 0x117, 0x11d, 0x123,
     0x129, 0x12f, 0x135, 0x13b, 0x141, 0x147, 0x14d, 0x153, 0x158, 0x15e, 0x164, 0x16a, 0x170, 0x176, 0x17c, 0x182,
     0x187, 0x18d, 0x193, 0x199, 0x19e, 0x1a4, 0x1aa, 0x1b0, 0x1b5, 0x1bb, 0x1c1, 0x1c6, 0x1cc, 0x1d2, 0x1d7, 0x1dd,
     0x1e2, 0x1e8, 0x1ed, 0x1f3, 0x1f8, 0x1fe, 0x203, 0x209, 0x20e, 0x213, 0x219, 0x21e, 0x223, 0x229, 0x22e, 0x233,
     0x238, 0x23e, 0x243, 0x248, 0x24d, 0x252, 0x257, 0x25c, 0x262, 0x267, 0x26c, 0x271, 0x276, 0x27a, 0x27f, 0x284,
     0x289, 0x28e, 0x293, 0x298, 0x29c, 0x2a1, 0x2a6, 0x2ab, 0x2af, 0x2b4, 0x2b8, 0x2bd, 0x2c2, 0x2c6, 0x2cb, 0x2cf,
     0x2d4, 0x2d8, 0x2dc, 0x2e1, 0x2e5, 0x2e9, 0x2ee, 0x2f2, 0x2f6, 0x2fa, 0x2ff, 0x303, 0x307, 0x30b, 0x30f, 0x313,
     0x317, 0x31b, 0x31f, 0x323, 0x327, 0x32b, 0x32e, 0x332, 0x336, 0x33a, 0x33d, 0x341, 0x345, 0x348, 0x34c, 0x34f,
     0x353, 0x356, 0x35a, 0x35d, 0x361, 0x364, 0x367, 0x36b, 0x36e, 0x371, 0x374, 0x377, 0x37a, 0x37e, 0x381, 0x384,
     0x387, 0x38a, 0x38c, 0x38f, 0x392, 0x395, 0x398, 0x39a, 0x39d, 0x3a0, 0x3a3, 0x3a5, 0x3a8, 0x3aa, 0x3ad, 0x3af,
     0x3b2, 0x3b4, 0x3b6, 0x3b9, 0x3bb, 0x3bd, 0x3bf, 0x3c2, 0x3c4, 0x3c6, 0x3c8, 0x3ca, 0x3cc, 0x3ce, 0x3d0, 0x3d2,
     0x3d3, 0x3d5, 0x3d7, 0x3d9, 0x3da, 0x3dc, 0x3de, 0x3df, 0x3e1, 0x3e2, 0x3e4, 0x3e5, 0x3e7, 0x3e8, 0x3e9, 0x3eb,
     0x3ec, 0x3ed, 0x3ee, 0x3ef, 0x3f0, 0x3f1, 0x3f3, 0x3f3, 0x3f4, 0x3f5, 0x3f6, 0x3f7, 0x3f8, 0x3f9, 0x3f9, 0x3fa,
     0x3fb, 0x3fb, 0x3fc, 0x3fc, 0x3fd, 0x3fd, 0x3fe, 0x3fe, 0x3fe, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff
};
#endif
/*-------------------------------------------------------------------------*/
