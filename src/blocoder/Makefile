BINNAME=blocoder

DEBUG=1

OBJECT_FILES=	\
	blocoder.o 

LIBS= -lm -lpng -lz

ifdef DEBUG

ONEFILE=
OBJECT_FILES +=	\
	bitmap.o \
	blochunter.o \
	blocundo.o \
	blockdata.o \
	darray.o \
	fsfont.o \
	intcoord.o \
	isl.o \
	islseq.o \
	file.o \
	hashtable.o \
	list.o \
	memory.o \
	pixelcolor.o \
	serialized.o \
	stdfunc.o \
	string.o

LIBS+=
CFLAGS= -O0 -g -march=native -Wall
CC=gcc
DEFINES+=-DCMOD_DEBUG_MEMORY 

else

ONEFILE=1
LIBS+= 
CFLAGS = -O3 -fwhole-program -fomit-frame-pointer -march=native -s 
CC=gcc
LDFLAGS=
DEFINES+=-DCMOD_OPTIMIZE_SIZE

endif

ifdef ONEFILE
DEFINES+= -DCMOD_NOMAKEFILE
OBJECT_FILES=$(BINNAME).o
endif

.PHONY : all clean

all :   $(BINNAME) 

lib :
	$(CC) $(CFLAGS) -shared $(DEFINES) -DCMOD_NOMAKEFILE -c $(BINNAME).c -o lib$(BINNAME).so

clean :
	rm -rf $(BINNAME) *.o lib$(BINNAME).so

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $^ -o $@

$(BINNAME): $(OBJECT_FILES)  
	$(CC) $(LDFLAGS) $^ $(INCLUDES) $(LIBS) -o $@
ifndef DEBUG
	strip $(BINNAME)
	ls -l $(BINNAME)
endif
