
#ifndef _stdfunc_h
#define _stdfunc_h

#include "cmod.h"
#include "stdtypes.h"
#include "string.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#ifndef _stdtypes_h
typedef int Int;
typedef double Double;
typedef short Short;
typedef int Bool;
#endif

#define SQR(x) ((x)*(x))
#define IS_IN(x,a,b) ((a) <= (x) && (x) < (b))
#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define ANSI_BLACK "\x1b[30m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_GRAY "\x1b[37m"
#define ANSI_DEFAULT "\x1b[39m"
#define ANSI_BRIGHT "\x1b[1m"
#define ANSI_NORMAL "\x1b[22m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_WHITE ANSI_GRAY""ANSI_BRIGHT

#define ANSI_BG_BLACK "\x1b[40m"
#define ANSI_BG_RED "\x1b[41m"
#define ANSI_BG_GREEN "\x1b[42m"
#define ANSI_BG_YELLOW "\x1b[43m"
#define ANSI_BG_BLUE "\x1b[44m"
#define ANSI_BG_MAGENTA "\x1b[45m"
#define ANSI_BG_CYAN "\x1b[46m"
#define ANSI_BG_GRAY "\x1b[47m"
#define ANSI_BG_DEFAULT "\x1b[49m"

#define ANSI_CURSOR_BACK(k) "\x1b["#k"D"
#define ANSI_CURSOR_BACKFORMAT "\x1b[%iD"
#define ANSI_CURSOR_FORWARD(k) "\x1b["#k"C"
#define ANSI_CURSOR_UP(k) "\x1b["#k"A"
#define ANSI_CURSOR_DOWN(k) "\x1b["#k"B"

#define ANSI_SAVE_POS "\x1b[s"
#define ANSI_RESTORE_POS "\x1b[u"
#define ANSI_CLEAR "\x1b[2J"
#define ANSI_CLEAR_LINE "\x1b[K"

#define ANSI_SET_HPOS(k) "\x1b["#k"G"
//#define ANSI_SET_VPOS(k) "\x1b[#kG"

extern int std_xd[], std_yd[];
extern int std_hxd[2][7], std_hyd[];
extern unsigned const char std_bit[], std_notbit[];
//extern FILE *std_print, *std_warning, *std_error;

#ifndef CMOD_OPTIMIZE_SIZE
extern short sin1024[], cos1024[];
#endif

void         Int_Init(Int *i, Int *i_org);
Int         *Int_Create(int a);
Int         *Int_CreateVal(int a, int val);
Int         *Int_CreateDim(int dim, ...);
Int         *Int_Clone(Int *a, int dim);
void         Int_Destroy(Int *i);
int          Int_CompareDescending(const void *a, const void *b);
int          Int_Compare(const void *a, const void *b);
#define      Int_CompareAscending Int_Compare
void         Int_Sort(int *a, int nof);
#define      Int_SortAscending Int_Sort
void         Int_SortDescending(int *a, int nof);
void         Int_Replace(int *a, int nof, int s, int t); /* Replaces all occurences of s to t */
int          Int_SortKey(int *a);
unsigned int Int_HashFunc(int *a);
Bool         Int_IsEqual(int *a, int *b);
unsigned int Int_ByteSwap(unsigned int a);
void         Int_Display(int *a);

int        **Int2_Create(int cols, int rows);
int        **Int2_CreateVal(int cols, int rows, int init);
int        **Int2_Clone(int **ptr, int rows, int cols);
int          Int2_Max(int **ptr, int cols, int rows);
int          Int2_Min(int **ptr, int cols, int rows);
void         Int2_Destroy(int **ptr, int rows);

Short       *Short_Create(int nof);
Short       *Short_CreateVal(int nof, int val);
void         Short_Destroy(Short *a);

Short      **Short2_Create(int cols, int rows);
Short      **Short2_CreateVal(int cols, int rows, int val);
void         Short2_Destroy(Short **a, int rows);

Char        *Char_Create(int nof);
Char        *Char_CreateVal(int nof, char val);
void         Char_Destroy(Char *a);
void         Char_Add(Char *a, Char *b, int nof);
void         Char_Sub(Char *a, Char *b, int nof);

Char       **Char2_Create(int cols, int rows);
Char       **Char2_CreateVal(int cols, int rows, char val);
Char       **Char2_Clone(Char **a, int rows, int cols);
void         Char2_Destroy(Char **a, int rows);

Double      *Double_Create(int dim);
Double      *Double_CreateVal(int dim, double val);
int          Double_CompareFirst(Double *f1, Double *f2);
int          Double_CompareSecondAscending(Double *f1, Double *f2); 
int          Double_CompareSecondDescending(Double *f1, Double *f2);
#define      Double_CompareDescending Double_CompareFirst
Double      *Double_Clone(Double *ptr, int len);
Double      *Double_Realloc(double *ptr, int new_len);
double       Double_Avg(Double *ptr, int len);
double       Double_Max(double *ptr, int len);
double       Double_Min(double *ptr, int len);
double       Double_AbsMax(double *ptr, int len);
double       Double_AbsMin(double *ptr, int len);
int          Double_ArgMax(double *ptr, int len);
int          Double_ArgMin(double *ptr, int len);
void         Double_Add(double *ptr, int len, double val);
void         Double_Sub(double *ptr, int len, double val);
void         Double_Mul(double *ptr, int len, double val);
void         Double_SortAscending(Double *arr, int first, int one_after_last);
//double      *Double_Clone(Double *d, int nof);

Double      *Double_Clone2D(Double *d);
#define      Double_Clone2 Double_Clone2D
void         Double_Destroy(Double *dt);

double     **Double2_Create(int cols, int rows);
double     **Double2_CreateVal(int cols, int rows, double val);
void         Double2_Destroy(double **ptr, int rows);
double     **Double2_Clone(double **ptr, int rows, int cols);
void         Double2_Mul(double **ptr, int ros, int cols, double val);
double       Double2_AbsMax(double **ptr, int ros, int cols);
double       Double2_Max(double **ptr, int ros, int cols);
double       Double2_Min(double **ptr, int ros, int cols);
int          Double2_ArgMinRow(double **ptr, int cols, int rows); /* Use Double_Min() to get min column */

float       *Float_Create(int dim);
void         Float_Destroy(float *f);

unsigned int Void_HashFunc(void *a);
#define      Void_HashVal Void_HashFunc
Bool         Void_IsEqual(void *a, void *b);
void         Void_Display(void *a);

void         Void_InsertionSortIx(void **array, void **array2, SortFunc *sortkey, int beg, int end);
void         Void_RadixSort(void **array, SortFunc *sortkey, int nof);
void         Void_QuickSortFloatAsc(float *keys, void **array, int n);
void         Void_QuickSortFloatDesc(float *keys, void **array, int n);

Bool         Bit_Get(void *block, int bit);
void         Bit_Set(void *block, int bit, Bool val);
void         Bit_Write(void *block, int *startbit, void *data, int nof_bits);
unsigned int Bit_ReadInt(void *block, int *startbit, int nof_bits);
void         Bit_WriteInt(void *block, int *startbit, unsigned int int_to_write, int nof_bits);
/* Handles up to 30 bit ints */
void         Bit_WriteCompressedInt(unsigned char *ptr, int *bitpos, unsigned int int_to_write);
unsigned int Bit_ReadCompressedInt(unsigned char *ptr, int *bitpos);
int          Bit_Lowest(unsigned char *ptr, int n); /* n is smallest bit not considered */
void         Bit_Display(unsigned char *ptr, int n); /* print n bits (0 padded) */

#ifdef CMOD_COMPILER_VISUALC
void         SleepWin(double seconds);
double       TimeWin();
#define      Std_Sleep SleepWin
#define      Std_Time TimeWin
#else
#define      Std_Sleep SleepUnix
#define      Std_Time TimeUnix
void         SleepUnix(double seconds);
double       TimeUnix();
#endif
//double       Std_Time();
int          Std_LaunchProcess(const String *command, int *process_stdin, int *process_stdout);
/* Returns bitmask of read fds: bit 0 is first arg fd, etc. Negative on error */
int          Std_Select(double timeout, int nof_fd, ... );
int          Std_FdReadLine(const int fd, const int maxlen, char *buf);
int          fdprintf(const int fd, const char * fmt, ... );
#define      Std_FdPrintf  fdprintf

#ifdef CMOD_COMPILER_GCC
void         StdError(const char *where, const char* format, ...);
void         StdWarning(const char *where, const char* format, ...);
#define      Std_Error(a...) StdError(__FILE__, a)
#define      Std_Warning(a...) StdWarning(__FILE__, a)
#else
void         Std_Error(char* format, ...);
void         Std_Warning(char* format, ...);
#endif
void         Error_Fatal(char* format, ...);

void         Std_Print(char* format, ...);
void         Std_PrintChar(int n, char c);
void         Std_HexDump(void *object, int size);
int          Std_GCD(int a, int b);
int          StdIn_ReadInt();
int         *StdIn_ReadInts(int nof);
void         StdIn_SkipLine();
void         Std_StackDumpFull(FILE *out, const char *message, const char *color, int toskip, int maxtoprint, Bool box);
#define      Std_StackDump(m) Std_StackDumpFull(stdout, m, ANSI_GRAY, 1, 999, TRUE)
#define      Std_StackTrace Std_StackDump
void         Std_FloatingPointException(int signum); // signal handler for FPE
void         Std_SetupFPE();
#define      PING printf("PING from %s:%i\n", __FILE__, __LINE__);

#ifdef CMOD_NOMAKEFILE
#include "stdfunc.c"
#endif
#endif
