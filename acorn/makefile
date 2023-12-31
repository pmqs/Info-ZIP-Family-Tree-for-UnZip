# Makefile for UnZip, UnZipSFX, ZipInfo and fUnZip (6.0 or later)
# last modified:  18 Nov 2010

# add -g to CC to debug
# add -d to BIND to debug
CC   = cc
BIND = link
AS   = $(CC) -c
SQUEEZE = squeeze -v
E    =

# flags
#   CFLAGS    flags for C compile
#   LFLAGS1   flags after output file spec, before obj file list
#   LFLAGS2   flags after obj file list (libraries, etc)
#
LIB	 =
CBASE	 = -throwback -wn -DNO_STRNICMP -memaccess -L22-S22-L41
CFLAGS	 = $(CBASE) -IC:,@.
LFLAGS1  =
LFLAGS2  = $(LIB) C:o.Stubs

# object file lists
OBJS1 = unzip.o crc32.o $(CRCA_O) crypt.o envargs.o explode.o
OBJS2 = extract.o fileio.o globals.o inflate.o list.o match.o
OBJS3 = process.o ttyio.o ubz2err.o unreduce.o unshrink.o zipinfo.o
OBJS = $(OBJS1) $(OBJS2) $(OBJS3) riscos.o swiven.o acorn.o
LOBJS = $(OBJS)
OBJSDLL = $(OBJS:.o=.pic.o) api.pic.o
OBJX = unzipsfx.o crc32_.o $(CRCA_O) crypt_.o extract_.o fileio_.o \
	globals_.o inflate_.o match_.o process_.o ttyio_.o ubz2err_.o \
	 riscos_.o swiven.o acorn_.o
LOBJX = $(OBJX)
OBJF = funzip.o crc32.o $(CRCA_O) cryptf.o globalsf.o inflatef.o ttyiof.o \
	 swiven.o
UNZIP_H = unzip.h unzpriv.h globals.h acorn/riscos.h

all:	  unzip funzip unzipsfx


install:  unzip funzip unzipsfx
	$(SQUEEZE) unzip %.unzip
	$(SQUEEZE) funzip %.funzip
	$(SQUEEZE) unzipsfx unzipsfx
	copy %.unzip %.zipinfo ~CVF

# suffix rules
.SUFFIXES: _.o .o .c
.c_.o:
	$(CC) $(CFLAGS) -DUTIL -o $@ -c $<
.c.o:
	$(CC) $(CFLAGS) -c $<

# rules for common stuff
crc32.o:	crc32.c $(UNZIP_H) zip.h crc32.h
crypt.o:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
envargs.o:	envargs.c $(UNZIP_H)
explode.o:	explode.c $(UNZIP_H)
extract.o:	extract.c $(UNZIP_H) crc32.h crypt.h
fileio.o:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
funzip.o:	funzip.c $(UNZIP_H) crc32.h crypt.h ttyio.h
globals.o:	globals.c $(UNZIP_H)
inflate.o:	inflate.c inflate.h $(UNZIP_H)
list.o:		list.c $(UNZIP_H)
match.o:	match.c $(UNZIP_H)
process.o:	process.c $(UNZIP_H) crc32.h
ttyio.o:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
ubz2err.o:	ubz2err.c $(UNZIP_H)
unreduce.o:	unreduce.c $(UNZIP_H)
unshrink.o:	unshrink.c $(UNZIP_H)
unzip.o:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
zipinfo.o:	zipinfo.c $(UNZIP_H)

# unzipsfx compilation section
unzipsfx.o:	unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ unzip.c

crc32_.o:	crc32.c $(UNZIP_H) zip.h crc32.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ crc32.c

crypt_.o:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ crypt.c

extract_.o:	extract.c $(UNZIP_H) crc32.h crypt.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ extract.c

fileio_.o:	fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ fileio.c

globals_.o:	globals.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ globals.c

inflate_.o:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ inflate.c

match_.o:	match.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ match.c

process_.o:	process.c $(UNZIP_H) crc32.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ process.c

ttyio_.o:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CFLAGS) -DSFX -o $@ ttyio.c

ubz2err_.o:	ubz2err.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DSFX -o $@ ubz2err.c


# funzip compilation section
cryptf.o:	crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ crypt.c

globalsf.o:	globals.c $(UNZIP_H)
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ globals.c

inflatef.o:	inflate.c inflate.h $(UNZIP_H) crypt.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ inflate.c

ttyiof.o:	ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) -c $(CFLAGS) -DFUNZIP -o $@ ttyio.c


acorn.o:       acorn/acorn.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c acorn/acorn.c
acorn_.o:	acorn/acorn.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c -DSFX -DSFX_EXDIR -o acorn_.o acorn/acorn.c
acornf.o:	acorn/acorn.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c -DFUNZIP -o acornf.o acorn/acorn.c

riscos.o:	acorn/riscos.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c acorn/riscos.c
riscos_.o:	acorn/riscos.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c -DSFX -DSFX_EXDIR -o riscos_.o acorn/riscos.c
riscosf.o:	acorn/riscos.c $(UNZIP_H)
	$(CC) $(CFLAGS) -c -DFUNZIP -o riscosf.o acorn/riscos.c

swiven.o:	acorn/swiven.c
	$(CC) $(CFLAGS) -c acorn/swiven.c

unzip:	  $(OBJS)
	  $(BIND) -o unzip$(E) $(LFLAGS1) $(OBJS) $(LFLAGS2)
funzip:   $(OBJF)
	  $(BIND) -o funzip$(E) $(LFLAGS1) $(OBJF) $(LFLAGS2)
unzipsfx: $(OBJX)
	  $(BIND) -o unzipsfx$(E) $(LFLAGS1) $(OBJX) $(LFLAGS2)

clean:
	remove unzip
	remove funzip
	remove zipinfo
	remove unzipsfx
	create o.!fake! 0
	wipe o.* ~cf

# end of Makefile
