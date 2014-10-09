#==============================================================================
# unix/Makefile
# - For UnZip, fUnZip, UnZipSFX, and ZipInfo            Revised: 2014-10-10
#
# Copyright (c) 2004-2014 Info-ZIP.  All rights reserved.
#
# See the accompanying file LICENSE, version 2009-Jan-2 or later (the
# contents of which are also included in zip.h) for terms of use.  If,
# for some reason, all these files are missing, the Info-ZIP license may
# also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
#==============================================================================

#==============================================================================
#
# INSTRUCTIONS
#
# "make -f unix/Makefile generic"         Make UnZip on most systems using cc.
#
# "make -f unix/Makefile generic CC=gcc"  Make UnZip on most systems using gcc.
#
# "make -f unix/Makefile list"            List all supported targets.
#
# "make -f unix/Makefile help"            Limited advice on which targets to
#                                         try if problems occur.
#
# CF are flags for the C compiler.  LF are flags for the linker.  LF2
# are more flags for the linker, if they need to be at the end of the
# line instead of at the beginning (for example, some libraries).
# LOCAL_UNZIP is a "make" macro that can be used to add default C flags
# to your compile without editing the "make" file (e.g., -DDEBUG_STRUC,
# or -FPi87 on a PC using Microsoft C).
#
# The "generic" target invokes the "unix/configure" script to test the
# compile and other parts of the environment to see what's supported.
# The results are used to adjust the build procedure and to enable or
# disable some program features.  unix/configure builds a $(PROD)/flags
# file with appropriate values for a number of "make" macros, which is
# used by this "make" file when performing the actual builds.  A separate
# flags file for building BZIP2 may also be created.  This "make" file
# recursively invokes itself to perform the actual builds.
#
# While oriented towards building Unix (and Unix work-alike) ports of
# UnZip, this file and/or unix/configure are also used by other ports of
# UnZip.  See below.
#
# If at all possible, use the generic targets.  The other (older)
# targets are less well tested, and may need considerable help from the
# user to get optimal results (or any success at all).
#
# Optional UnZip features are controlled by setting "make" macros as
# described below or by the results of tests done by configure (such as
# checking if the needed files are in an appropriate directory).
#
# Optional features include (see INSTALL for installation instructions):
#
# AES encryption - See the files INSTALL and aes_wg/README_AES_WG.txt
#   for more information.  To include AES_WG (WinZip/Gladman) encryption
#   support, unpack a compatible AES_WG source kit (iz_aes_wg13.zip, or
#   the latest compatible kit available on the Info-ZIP server) to
#   populate the local ./aes_wg subdirectory.  To disable AES_WG
#   encryption support (when the source kit is present), set the "make"
#   macro NO_AES_WG to 1.
#
# LZMA compression - See the files INSTALL and szip/README_LZMA.txt for
#   more information.  To disable LZMA compression support, set the
#   "make" macro NO_LZMA to 1.
#
# PPMd compression - See the files INSTALL and szip/README_LZMA.txt for
#   more information.  To disable PPMd compression support, set the
#   "make" macro NO_PPMD to 1.
#
# Bzip2 compression - See the files INSTALL and bzip2/README_BZIP2.txt
#   for more information.  To disable bzip2 compression support, set the
#   "make" macro NO_IZ_BZIP2 to 1.
#   To include bzip2 compression support, there are three options:
#   - Let the UnZip builders build a bzip2 object library from the
#     source kit in the bzip2 subdirectory.
#   - Use an existing build of the bzip2 library by setting the "make"
#     macro IZ_BZIP2 to a directory containing the bzip2 header file and
#     object (and/or shared) library files.
#   - Use the system bzip2 library.  To do this, ensure that the local
#     ./bzip2 subdirectory exists and does not contain the files
#     Makefile, libbz2.a, or bzlib.h.  It may be safer to use one of the
#     above options first, if possible, to avoid a possible problem with
#     error handling that may not be done as expected by the system
#     library.
#
# Zlib deflation - To use zlib to handle Deflate compression instead of
#   UnZip's built-in Deflate code, there are two options:
#   - Use an existing build of the zlib support by setting the "make"
#     macro IZ_ZLIB to a directory containing suitable zlib header and
#     object/shared library files.
#   - Use the system zlib support by ensuring that the local ./zlib
#     subdirectory exists, and does not contain the files libz.a or
#     bzlib.h.  Set the "make" macro IZ_ZLIB to "zlib".
#
# UnZip Ports:
#
# This "make" file and/or the unix/configure script support a number of
# non-Unix UnZip ports, where the UnZip program build is under Unix (or a
# Unix work-alike), but the UnZip program execution may be on another
# system or execution environment.  The unix/configure script will be
# invoked from either unix/Makefile, or from a "make" file located in
# another port directory.
#
# The port may be explicitly identified by the PORT parameter, or
# implicitly detected by compiler configuration tests.
#
# Supported ports include:
#   msdos : DOS port built using the DJGPP (or Cygwin) cross-compiler
#   os2   : OS/2 port
#   qdos  : Sinclair Q-DOS port built using a gcc cross-compiler
#   zos   : z/OS MVS port built under z/OS UNIX
#   zvm   : z/VM CMS port built under z/VM OpenExtensions
#
# Be sure to test your new UnZip (and UnZipSFX and fUnZip and ZipInfo).  A
# successful build does not guarantee a working program.

###########
# TARGETS #
###########

# List of supported systems/targets in this version
GENERIC_TARGETS1 = generic  generic_pkg  config_flags
GENERIC_TARGETS2 = generic1  generic2  generic3

MISC_TARGETS = clean  clean_bzip2  clean_docs  clean_exe  dashv

SYS_TARGETS01 = 386i 3Bx 7300 7300_gcc amdahl amdahl_eft apollo aviion
SYS_TARGETS02 = bsd bsdi bsdi_noasm bull coherent convex cray cray_opt
SYS_TARGETS03 = cyber_sgi cygwin dec dnix encore eta freebsd gcc gould
SYS_TARGETS04 = hk68 hp hpux isc isc_gcc isi linux linux_dos linux_noasm
SYS_TARGETS05 = lynx macosx macosx_gcc minix mips
SYS_TARGETS06 = mpeix next next10 next2x next3x nextfat osf1 pixel ptx
SYS_TARGETS07 = pyramid qnxnto realix regulus sco sco_dos sco_sl sco_x286
SYS_TARGETS08 = sequent sgi solaris stardent stellar sunos3 sunos4
SYS_TARGETS09 = sysv sysv_gcc sysv6300 tahoe ti_sysv ultrix vax v7
SYS_TARGETS10 = xenix xos

# By default, print help on which "make" targets to try.

# What you can make ...
help:
	@echo ''
	@echo 'Make what?  You must specify which system to make UnZip for.'
	@echo 'The "make" generic target choices are:'
	@echo ''
	@echo "  $(GENERIC_TARGETS1)"
	@echo "  $(GENERIC_TARGETS2)"
	@echo ''
	@echo 'Additional "make" targets include:'
	@echo ''
	@echo "  $(MISC_TARGETS)"
	@echo ''
	@echo 'For a list including specific targets for older systems:'
	@echo '   make -f unix/Makefile list'
	@echo ''
	@echo '"make" macros to control various optional features:'
	@echo '    "NO_AES_WG=1"     Disable AES_WG encryption.'
	@echo '    "BINS=[L][M][U]   Binaries build list:'
	@echo '                       L: object library (libizunzip.a)'
	@echo '                       M: main program (UnZip)'
	@echo '                       U: utility programs (fUnZip, UnZipSFX)'
	@echo '                       Default: MU.'
	@echo '    "CC=gcc"          Use "gcc" instead of "cc" for C compiler.'
	@echo '    "NO_ICONV=1"      Disable ISO/OEM (iconv) conversion.'
	@echo '    "IZ_BZIP2=bz2dir" Use bzip2 compression from dir "bz2dir".'
	@echo '    "NO_IZ_BZIP2=1"   Disable bzip2 compression.'
	@echo '    "IZ_ZLIB=zdir"    Use zlib from dir "zdir" for Deflate.'
	@echo '    "LIST=1"          Generate C compiler and linker listing.'
	@echo '    "LOCAL_UNZIP=opts" Add C compiler options, "opts".'
	@echo '    "NO_LZMA=1"       Disable LZMA compression.'
	@echo '    "NO_PPMD=1"       Disable PPMd compression.'
	@echo '    "PROD=subdir"     Build product files in "subdir", not ".".'
	@echo ''
	@echo 'Examples:'
	@echo '   Make UnZip programs using cc:'
	@echo '      make -f unix/Makefile generic'
	@echo '   Make UnZip programs using gcc, without LZMA compression.'
	@echo '      make -f unix/Makefile generic CC=gcc NO_LZMA=1'
	@echo ''
	@echo 'Note that many of these optional "make" macros direct the'
	@echo '"unix/configure" script, and so may work only with the'
	@echo '"generic" target.'
	@echo ''
	@echo 'First, try "make -f unix/Makefile generic" (to use "cc")'
	@echo '(or "make -f unix/Makefile generic CC=gcc" (to use "gcc")),'
	@echo 'as "generic" should automatically set the proper flags.'
	@echo ''
	@echo 'Use the "clean" target to delete any product files before'
	@echo 'using one of the other "make" targets.  The generic targets'
	@echo 'use the unix/configure script to generate an intermediate'
	@echo '"flags" file which is used by unix/Makefile.  For debugging'
	@echo 'purposes, the "config_flags" target can be used to generate'
	@echo 'the "flags" file only.'
	@echo ''
	@echo 'Use the "docs" target to make the plain-text "man" output'
	@echo 'files.  These are the files, like "unzip.txt", that are'
	@echo 'supplied in the "docs" directory.  Newly generated ".txt"'
	@echo 'files will be placed in the "$(PROD)/manout" directory.'
	@echo ''
	@echo 'After UnZip has been built, use dashv to get an "unzip -v"'
	@echo 'report from the created executable.'
	@echo ''
	@echo 'See the files INSTALL, unzip.txt, and unix/Makefile for more'
	@echo 'information.'
	@echo ''
	@echo '  For most users, the best build method is the generic target'
	@echo '  using the default compiler (cc is often gcc), or by specifying'
	@echo '  an explicit "CC=gcc" override.'
	@echo ''
	@echo '  Other (older) targets are less well tested, and may need'
	@echo '  considerable help from the user to get optimal results (or'
	@echo '  any success at all).  Use "make -f unix/Makefile list" to'
	@echo '  get a list of valid targets.'
	@echo ''
	@echo '  Always do "make -f unix/Makefile clean clean_bzip2" before'
	@echo '  attempting to build UnZip using a different target.'
	@echo ''
	@echo '  If the "generic" target fails, you might try an older generic'
	@echo '  target that is now called "generic1".'
	@echo ''
	@echo '  If the compiler complains about "timezone redefined", then'
	@echo '  the "generic2" target may work better.  If the linker'
	@echo '  complains about an undefined symbol, "_ftime", then the'
	@echo '  "generic3" target may work better.'
	@echo ''
	@echo '  One of these actions should produce working UnZip programs on'
	@echo '  most Unix(-like) systems.  If you know a bit more about the'
	@echo '  system on which you work, then you might try "make -f'
	@echo '  unix/Makefile list" for a list of the system-specific targets'
	@echo '  supported herein.  (Many of them do exactly the same thing,'
	@echo '  so do not agonize too much over which to pick if two or more'
	@echo '  sound equally likely.)'
	@echo ''

# list
# - Display list of supported systems/targets in this version.
# - The SYSTEM variable is no longer supported.  Instead, use:
#      make -f unix/Makefile <target>
list:
	@echo ''
	@echo 'Type "make -f unix/Makefile <target>", where <target> is one of'
	@echo 'the following:'
	@echo ''
	@echo 'Generic targets:'
	@echo "  $(GENERIC_TARGETS1)"
	@echo "  $(GENERIC_TARGETS2)"
	@echo ''
	@echo 'Specific targets:'
	@echo "  $(SYS_TARGETS01)"
	@echo "  $(SYS_TARGETS02)"
	@echo "  $(SYS_TARGETS03)"
	@echo "  $(SYS_TARGETS04)"
	@echo "  $(SYS_TARGETS05)"
	@echo "  $(SYS_TARGETS06)"
	@echo "  $(SYS_TARGETS07)"
	@echo "  $(SYS_TARGETS08)"
	@echo "  $(SYS_TARGETS09)"
	@echo "  $(SYS_TARGETS10)"
	@echo ''
	@echo 'Miscellaneous targets:'
	@echo "  $(MISC_TARGETS)"
	@echo ''
	@echo 'Use one of the generic targets if possible, as the specific'
	@echo 'targets tend to be woefully out of date.'
	@echo ''
	@echo 'For further (very useful) information, please read the comments'
	@echo 'in unix/Makefile.'

#####################
# MACRO DEFINITIONS #
#####################

#----------------------------------------------------------------------------
# Defaults most systems use:
# - The generic target invokes unix/configure to build $(PROD)/flags and
#   $(PROD)/flags_bz containing system derived settings that are used by
#   this "make" file.
# - Use "make" macro LOCAL_UNZIP to add C compiler flags (such as
#   -DDOSWILD).
#----------------------------------------------------------------------------

# This file name as "make" parameter, for recursion.
MAKEF = '-f unix/Makefile'

# Product file directory default is "."
PROD = .

# Build Dependencies for "make"
# - Optional object library names
#    AES (WinZip/Gladman) encryption
#    BZIP2 compression
#    LZMA compression
#    PPMd compression
#    ZLIB compression
LIB_AES_WG  =
LIB_BZ      =
LIB_LZMA    =
LIB_PPMD    =
LIB_Z       =

# Binaries build list (executables and/or object library).
# - Default: main program and utilities.
BINS = MU

# Build "make" files
# - Alternatively, use bzip2/Makebz2.iz
#    BZIP2 compression
MK_BZ   = Makefile

# OS-dependent files
# - Default:  Unix port.
# - Compilation and execution are both for Unix (or a Unix-like) environment.
#   The files in the UnZip root and unix directories are used.
# - The vast majority of you out there will likely use this
#   configuration, and do not have to ever worry about this.
# - Where required by a specific UnZip port, a generic target (in this
#   "make" file or a "make" file located in another port directory) uses
#   the PORT macro to set the correct OS dependent files via
#   unix/configure.  Or you can (very carefully) override as required.
#    C header files
#    C object files for funzip
#    C object files for unzipsfx
#    C object files for unzip
#    Assembler object files
OSDEP_H  = unix/unxcfg.h
OSDEP_OCF =
OSDEP_OCS = $(PROD)/unix.o
OSDEP_OCU = $(PROD)/unix.o
OSDEP_OS =

# OS-dependent file extensions
# - This default may be customized by unix/configure based on PORT
#    Executable programs
#    DLL (.so) (currently not used)
#    man page (section) suffix
PGMEXT =
DLLEXT = .so
MANSUFX =

# OS-dependent installation directories: executables and man pages
# - This default may be customized by unix/configure based on PORT
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/man

MANDIR1 = $(MANDIR)/man1

# OS-dependent installation file permissions: executables and man pages
# - This default may be customized by unix/configure based on PORT
BINPERMS = 755
MANPERMS = 644

# C compiler and flags: core and bzip2
# - CC and LD must match, else "unresolved symbol: ___main" is possible.
# - To use "gcc", specify "make" macro, "CC=gcc", instead of editing here.
CC      = cc
CC_BZ   = $(CC)

CF_NOOPT = -I. -DUNIX $(LOCAL_UNZIP)
CFLAGS  = -O
CF      = $(CFLAGS) $(CF_NOOPT)         # C compiler flags (core components)
CF_BZ   = $(CF)                         # C compiler flags (BZIP2 components)

# C preprocessor
CPP     = $(CC) -E

# Assembler and assembler flags
AS      = as
AF      = $(LOCAL_UNZIP)

# Linker and linker flags (early and late)
# - CC and LD must match, else "unresolved symbol: ___main" is possible.
# - To use "gcc", specify "make" macro, "CC=gcc", instead of editing here.
LD      = $(CC)
LF1     =
LF2     = -s

# Other utility programs
AR      = ar cq
CHMOD   = chmod
COL     = col
LN      = ln
RANLIB  = ranlib
RM      = rm -f
SHELL   = /bin/sh
STRIP   = strip
PGM_ZIP = zip

# Default for dependency on unix/configure result, is an empty symbol
# so that the static non-unix/configure targets continue to work
ACONF_DEP =

#----------------------------------------------------------------------------
#  Build configuration:  header files
#----------------------------------------------------------------------------

# OSDEP_H is port-dependent and can be updated by unix/configure

# C header files: unzip
H_UNZIP  = unzip.h          \
           unzpriv.h        \
           globals.h        \
           $(OSDEP_H)       \
           $(ACONF_DEP)

H_UNZIPL = api.h $(H_UNZIP)

# C header files: AES (WinZip/Gladman) encryption
H_AES    = aes_wg/aes.h     \
           aes_wg/aesopt.h  \
           aes_wg/fileenc.h \
           aes_wg/hmac.h    \
           aes_wg/prng.h    \
           aes_wg/pwd2key.h \
           aes_wg/sha1.h

# C header files: LZMA compression
H_LZMA   = szip/SzVersion.h \
           szip/LzFind.h    \
           szip/LzHash.h    \
           szip/LzmaDec.h   \
           szip/Types.h

# C header files: PPMd compression
H_PPMD   = szip/Ppmd.h      \
           szip/Ppmd8.h

#----------------------------------------------------------------------------
#  Build configuration:  object files
#----------------------------------------------------------------------------

# OSDEP_* are port-dependent and can be updated by unix/configure.

# object files: unzip
O_UNZIP  = $(PROD)/unzip.o      \
           $(PROD)/crc32.o      \
           $(PROD)/crypt.o      \
           $(PROD)/envargs.o    \
           $(PROD)/explode.o    \
           $(PROD)/extract.o    \
           $(PROD)/fileio.o     \
           $(PROD)/globals.o    \
           $(PROD)/inflate.o    \
           $(PROD)/list.o       \
           $(PROD)/match.o      \
           $(PROD)/process.o    \
           $(PROD)/ttyio.o      \
           $(PROD)/ubz2err.o    \
           $(PROD)/unreduce.o   \
           $(PROD)/unshrink.o   \
           $(PROD)/zipinfo.o    \
           $(OSDEP_OCU)         \
           $(OSDEP_OS)

# object files: UnZip object library.  Modules sensitive to DLL/REENTRANT.
O_UNZIPL = $(PROD)/api_l.o      \
           $(PROD)/apihelp_l.o  \
           $(PROD)/crypt_l.o    \
           $(PROD)/explode_l.o  \
           $(PROD)/extract_l.o  \
           $(PROD)/fileio_l.o   \
           $(PROD)/globals_l.o  \
           $(PROD)/inflate_l.o  \
           $(PROD)/list_l.o     \
           $(PROD)/process_l.o  \
           $(PROD)/ttyio_l.o    \
           $(PROD)/ubz2err_l.o  \
           $(PROD)/unshrink_l.o \
           $(PROD)/unzip_l.o    \
           $(PROD)/zipinfo_l.o  \
           $(OSDEP_OCU:.o=_l.o)

# object files: UnZip object library.  Modules insensitive to DLL/REENTRANT.
O_UNZIPN = $(PROD)/crc32.o      \
           $(PROD)/envargs.o    \
           $(PROD)/match.o      \
           $(PROD)/unreduce.o   \
           $(OSDEP_OS)

# object files: UnZip DLL
O_DLL    = $(O_UNZIP)           \
           $(PROD)/api.o        \
           $(PROD)/apihelp.o

O_STUB   = $(PROD)/unzipstb.o

# object files: unzipsfx
O_SFX    = $(PROD)/unzipsfx.o   \
           $(PROD)/crc32_s.o    \
           $(PROD)/crypt_s.o    \
           $(PROD)/extract_s.o  \
           $(PROD)/fileio_s.o   \
           $(PROD)/globals_s.o  \
           $(PROD)/inflate_s.o  \
           $(PROD)/match_s.o    \
           $(PROD)/process_s.o  \
           $(PROD)/ttyio_s.o    \
           $(PROD)/ubz2err_s.o  \
           $(OSDEP_OCS:.o=_s.o) \
           $(OSDEP_OS)

# object files: funzip
O_FUZ    = $(PROD)/funzip.o     \
           $(PROD)/crc32_f.o    \
           $(PROD)/crypt_f.o    \
           $(PROD)/globals_f.o  \
           $(PROD)/inflate_f.o  \
           $(PROD)/ttyio_f.o    \
           $(OSDEP_OCF:.o=_f.o) \
           $(OSDEP_OS)

# object files: Assembler CRC (Source: msdos/crc_i86.S)
# - All OS-dependent, by unix/configure based on PORT
O_CRC_I  = $(PROD)/crc_i86.o

# object files: Assembler CRC (Source: crc_i386.S)
# - All OS-dependent, by unix/configure based on PORT
O_CRC_G  = $(PROD)/crc_gcc.o
O_CRC_SV = $(PROD)/crc_sysv.o

# object files: AES (WinZip/Gladman) encryption
O_AES    = $(PROD)/aescrypt.o   \
           $(PROD)/aeskey.o     \
           $(PROD)/aestab.o     \
           $(PROD)/fileenc.o    \
           $(PROD)/hmac.o       \
           $(PROD)/prng.o       \
           $(PROD)/pwd2key.o    \
           $(PROD)/sha1.o

# object files: LZMA compression
O_LZMA   = $(PROD)/LzFind.o     \
           $(PROD)/LzmaDec.o

# object files: PPMd compression
O_PPMD   = $(PROD)/Ppmd8.o      \
           $(PROD)/Ppmd8Dec.o

# object files: ZLIB compression
# - N/A.  ZLIB is not built as part of UnZip.

#----------------------------------------------------------------------------
#  Build configuration:  object libraries
#----------------------------------------------------------------------------

# object library: AES (WinZip/Gladman) encryption
AR_AES   = $(PROD)/libiz_aes_wg.a

# object library: BZIP2 compression
AR_BZ    = bzip2/libbz2.a

# object library: LZMA compression
AR_LZMA  = $(PROD)/libiz_lzma.a

# object library: PPMd compression
AR_PPMD  = $(PROD)/libiz_ppmd.a

# object library: Callable UnZip
AR_UNZIP = $(PROD)/libizunzip.a

# object library: ZLIB compression
AR_Z     = zlib/libz.a

#----------------------------------------------------------------------------
#  Build configuration:  Executable programs
#----------------------------------------------------------------------------
LIB_UNZIP   = $(PROD)/libizunzip.a

PGM_FUZ     = funzip$(PGMEXT)
PGM_SFX     = unzipsfx$(PGMEXT)
PGM_UNZIP   = unzip$(PGMEXT)
PGM_ZGREP   = zipgrep
PGM_ZINFO   = zipinfo$(PGMEXT)

PPGM_FUZ    = $(PROD)/$(PGM_FUZ)
PPGM_SFX    = $(PROD)/$(PGM_SFX)
PPGM_UNZIP  = $(PROD)/$(PGM_UNZIP)
PPGM_ZGREP  = unix/zipgrep
PPGM_ZINFO  = $(PROD)/$(PGM_ZINFO)

UNZIP_PPGMS = $(PPGM_FUZ)   \
              $(PPGM_SFX)   \
              $(PPGM_UNZIP)

#----------------------------------------------------------------------------
#  Build configuration:  Manuals
#----------------------------------------------------------------------------
MAN_FUZ     = funzip.1$(MANSUFX)
MAN_SFX     = unzipsfx.1$(MANSUFX)
MAN_UNZIP   = unzip.1$(MANSUFX)
MAN_ZGREP   = zipgrep.1$(MANSUFX)
MAN_ZINFO   = zipinfo.1$(MANSUFX)

PMAN_FUZ    = $(PROD)/manout/funzip.txtr
PMAN_SFX    = $(PROD)/manout/unzipsfx.txtr
PMAN_UNZIP  = $(PROD)/manout/unzip.txtr
PMAN_ZGREP  = $(PROD)/manout/zipgrep.txtr
PMAN_ZINFO  = $(PROD)/manout/zipinfo.txtr

UNZIP_PMANS = $(PMAN_UNZIP) \
              $(PMAN_SFX)   \
              $(PMAN_FUZ)   \
              $(PMAN_ZGREP) \
              $(PMAN_ZINFO)

DOC_FUZ     = $(PROD)/manout/funzip.txt
DOC_SFX     = $(PROD)/manout/unzipsfx.txt
DOC_UNZIP   = $(PROD)/manout/unzip.txt
DOC_ZGREP   = $(PROD)/manout/zipgrep.txt
DOC_ZINFO   = $(PROD)/manout/zipinfo.txt

UNZIP_DOCS  = $(DOC_UNZIP) \
              $(DOC_SFX)   \
              $(DOC_FUZ)   \
              $(DOC_ZINFO) \
              $(DOC_ZGREP)

#----------------------------------------------------------------------------
#  Build configuration:  Installation
#----------------------------------------------------------------------------

# (Probably can change next two to "install" and "install -d", if you have it.)
INSTALL   = cp
INSTALL_PROGRAM = $(INSTALL)
INSTALL_D = mkdir -p

BIN_FUZ   = $(BINDIR)/$(PGM_FUZ)
BIN_SFX   = $(BINDIR)/$(PGM_SFX)
BIN_UNZIP = $(BINDIR)/$(PGM_UNZIP)
BIN_ZGREP = $(BINDIR)/$(PGM_ZGREP)
BIN_ZINFO = $(BINDIR)/$(PGM_ZINFO)

INSTALLEDBIN = $(BIN_FUZ)   \
               $(BIN_SFX)   \
               $(BIN_UNZIP) \
               $(BIN_ZGREP) \
               $(BIN_ZINFO)

INSTALLEDMAN = $(MANDIR1)/$(MAN_FUZ)   \
               $(MANDIR1)/$(MAN_SFX)   \
               $(MANDIR1)/$(MAN_UNZIP) \
               $(MANDIR1)/$(MAN_ZGREP) \
               $(MANDIR1)/$(MAN_ZINFO)

INSTALLEDMAN_GZ = $(MANDIR1)/$(MAN_FUZ).gz   \
                  $(MANDIR1)/$(MAN_SFX).gz   \
                  $(MANDIR1)/$(MAN_UNZIP).gz \
                  $(MANDIR1)/$(MAN_ZGREP).gz \
                  $(MANDIR1)/$(MAN_ZINFO).gz

# Solaris 2.x package stuff:
PKGDIR  = IZunzip
VERSION = Version 6.1


###############################################
# BASIC COMPILE INSTRUCTIONS AND DEPENDENCIES #
###############################################

#----------------------------------------------------------------------------
#  Build rules: File suffixes
#  - Disable all built-in rules.
#  - Only explicit 1:1 rules are used.
#----------------------------------------------------------------------------
.SUFFIXES:

#----------------------------------------------------------------------------
#  Build rules: C compile (.c -> .o)
#  - Explicit 1:1 rules are used.
#  - Across various "make" utilities, pattern rule support is inconsistent.
#  - Sys V "make" may have problems with $< when input and output names differ.
#----------------------------------------------------------------------------

# C Compile rules: UnZip

# A) Core UnZip C files

$(PROD)/unzip.o:      unzip.c     $(H_UNZIP) consts.h crypt.h unzvers.h
	$(CC) -c $(CF) -o $@ unzip.c

$(PROD)/crc32.o:      crc32.c     $(H_UNZIP) crc32.h zip.h
	$(CC) -c $(CF) -o $@ crc32.c

$(PROD)/crypt.o:      crypt.c     $(H_UNZIP) crc32.h crypt.h ttyio.h zip.h
	$(CC) -c $(CF) -o $@ crypt.c

$(PROD)/envargs.o:    envargs.c   $(H_UNZIP)
	$(CC) -c $(CF) -o $@ envargs.c

$(PROD)/explode.o:    explode.c   $(H_UNZIP)
	$(CC) -c $(CF) -o $@ explode.c

$(PROD)/extract.o:    extract.c   $(H_UNZIP) crc32.h crypt.h
	$(CC) -c $(CF) -o $@ extract.c

$(PROD)/fileio.o:     fileio.c    $(H_UNZIP) crc32.h crypt.h ebcdic.h ttyio.h
	$(CC) -c $(CF) -o $@ fileio.c

$(PROD)/globals.o:    globals.c   $(H_UNZIP)
	$(CC) -c $(CF) -o $@ globals.c

$(PROD)/inflate.o:    inflate.c   $(H_UNZIP) inflate.h
	$(CC) -c $(CF) -o $@ inflate.c

$(PROD)/list.o:       list.c      $(H_UNZIP)
	$(CC) -c $(CF) -o $@ list.c

$(PROD)/match.o:      match.c     $(H_UNZIP)
	$(CC) -c $(CF) -o $@ match.c

$(PROD)/process.o:    process.c   $(H_UNZIP) crc32.h
	$(CC) -c $(CF) -o $@ process.c

$(PROD)/ttyio.o:      ttyio.c     $(H_UNZIP) crypt.h ttyio.h zip.h
	$(CC) -c $(CF) -o $@ ttyio.c

$(PROD)/ubz2err.o:    ubz2err.c   $(H_UNZIP)
	$(CC) -c $(CF) -o $@ ubz2err.c

$(PROD)/unreduce.o:   unreduce.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ unreduce.c

$(PROD)/unshrink.o:   unshrink.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ unshrink.c

$(PROD)/zipinfo.o:    zipinfo.c   $(H_UNZIP)
	$(CC) -c $(CF) -o $@ zipinfo.c

# A') Callable UnZip C files

$(PROD)/api_l.o:      api.c       $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ api.c

$(PROD)/apihelp_l.o:  apihelp.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ apihelp.c

$(PROD)/crypt_l.o:    crypt.c     $(H_UNZIPL) crc32.h crypt.h ttyio.h zip.h
	$(CC) -c $(CF) -DDLL -o $@ crypt.c

$(PROD)/explode_l.o:  explode.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ explode.c

$(PROD)/extract_l.o:  extract.c   $(H_UNZIPL) crc32.h crypt.h
	$(CC) -c $(CF) -DDLL -o $@ extract.c

$(PROD)/fileio_l.o:   fileio.c    $(H_UNZIPL) crc32.h crypt.h ebcdic.h ttyio.h
	$(CC) -c $(CF) -DDLL -o $@ fileio.c

$(PROD)/globals_l.o:  globals.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ globals.c

$(PROD)/inflate_l.o:  inflate.c   $(H_UNZIPL) inflate.h
	$(CC) -c $(CF) -DDLL -o $@ inflate.c

$(PROD)/list_l.o:     list.c      $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ list.c

$(PROD)/process_l.o:  process.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ process.c

$(PROD)/ttyio_l.o:    ttyio.c     $(H_UNZIPL) crypt.h ttyio.h zip.h
	$(CC) -c $(CF) -DDLL -o $@ ttyio.c

$(PROD)/ubz2err_l.o:  ubz2err.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ ubz2err.c

$(PROD)/unreduce_l.o: unreduce.c  $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ unreduce.c

$(PROD)/unshrink_l.o: unshrink.c  $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ unshrink.c

$(PROD)/unzip_l.o:    unzip.c     $(H_UNZIPL) consts.h crypt.h unzvers.h
	$(CC) -c $(CF) -DDLL -o $@ unzip.c

$(PROD)/zipinfo_l.o:  zipinfo.c   $(H_UNZIPL)
	$(CC) -c $(CF) -DDLL -o $@ zipinfo.c

$(PROD)/unix_l.o:   unix/unix.c $(H_UNZIPL) unix/macosx.h
	$(CC) -c $(CF) -DDLL -o $@ unix/unix.c

# B) UnZip C files for specific ports
#    - Files under msdos and os2 are used when cross compiling for those ports.
#    - Generated when referenced by OSDEP_OC*.
#    - OSDEP_OC* are customized by unix/configure, based on PORT macro.

$(PROD)/msdos.o:    msdos/msdos.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ msdos/msdos.c

$(PROD)/os2.o:      os2/os2.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ os2/os2.c

$(PROD)/unix.o:     unix/unix.c  $(H_UNZIP) unix/macosx.h
	$(CC) -c $(CF) -o $@ unix/unix.c

$(PROD)/nt.o:       win32/nt.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ win32/nt.c

$(PROD)/win32.o:    win32/win32.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ win32/win32.c

$(PROD)/win32i64.o:  win32/win32i64.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ win32/win32i64.c

# C Compile rules: UnZip DLL
$(PROD)/api.o:        api.c       $(H_UNZIP)
	$(CC) -c $(CF) -DDLL -o $@ api.c

$(PROD)/apihelp.o:    apihelp.c   $(H_UNZIP)
	$(CC) -c $(CF) -DDLL -o $@ apihelp.c

$(PROD)/unzipstb.o:   unzipstb.c  $(H_UNZIP)
	$(CC) -c $(CF) -o $@ unzipstb.c

# B) UnZip DLL C files for specific ports
#    - Generated when referenced by OSDEP_OC*
#    - OSDEP_OC* are customized by unix/configure, based on PORT macro

$(PROD)/nt_l.o:       win32/nt.c  $(H_UNZIP)
	$(CC) -c $(CF) -DDLL -o $@ win32/nt.c

$(PROD)/win32_l.o:    win32/win32.c  $(H_UNZIP)
	$(CC) -c $(CF) -DDLL -o $@ win32/win32.c

$(PROD)/win32i64_l.o: win32/win32i64.c  $(H_UNZIP)
	$(CC) -c $(CF) -DDLL -o $@ win32/win32i64.c

# C Compile rules: UnZipSFX

# A) Core UnZipSFX C files
$(PROD)/unzipsfx.o:   unzip.c     $(H_UNZIP) crypt.h unzvers.h consts.h
	$(CC) -c $(CF) -DSFX -o $@ unzip.c

$(PROD)/crc32_s.o:    crc32.c     $(H_UNZIP) zip.h crc32.h
	$(CC) -c $(CF) -DSFX -o $@ crc32.c

$(PROD)/crypt_s.o:    crypt.c     $(H_UNZIP) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CF) -DSFX -o $@ crypt.c

$(PROD)/extract_s.o:  extract.c   $(H_UNZIP) crc32.h crypt.h
	$(CC) -c $(CF) -DSFX -o $@ extract.c

$(PROD)/fileio_s.o:   fileio.c    $(H_UNZIP) crc32.h crypt.h ttyio.h ebcdic.h
	$(CC) -c $(CF) -DSFX -o $@ fileio.c

$(PROD)/globals_s.o:  globals.c   $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ globals.c

$(PROD)/inflate_s.o:  inflate.c   $(H_UNZIP) inflate.h
	$(CC) -c $(CF) -DSFX -o $@ inflate.c

$(PROD)/match_s.o:    match.c     $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ match.c

$(PROD)/process_s.o:  process.c   $(H_UNZIP) crc32.h
	$(CC) -c $(CF) -DSFX -o $@ process.c

$(PROD)/ttyio_s.o:    ttyio.c     $(H_UNZIP) zip.h crypt.h ttyio.h
	$(CC) -c $(CF) -DSFX -o $@ ttyio.c

$(PROD)/ubz2err_s.o:  ubz2err.c   $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ ubz2err.c

# B) UnZipSFX C files for specific ports
#    - Generated when referenced by OSDEP_OC*.
#    - OSDEP_OC* are customized by unix/configure, based on PORT macro.

$(PROD)/msdos_s.o:  msdos/msdos.c $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ msdos/msdos.c

$(PROD)/os2_s.o:    os2/os2.c   $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ os2/os2.c

$(PROD)/unix_s.o:   unix/unix.c $(H_UNZIP) unix/macosx.h
	$(CC) -c $(CF) -DSFX -o $@ unix/unix.c

$(PROD)/nt_s.o:       win32/nt.c  $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ win32/nt.c

$(PROD)/win32_s.o:    win32/win32.c  $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ win32/win32.c

$(PROD)/win32i64_s.o:  win32/win32i64.c  $(H_UNZIP)
	$(CC) -c $(CF) -DSFX -o $@ win32/win32i64.c

# C Compile rules: fUnZip
$(PROD)/funzip.o:     funzip.c    $(H_UNZIP) crc32.h crypt.h ttyio.h
	$(CC) -c $(CF) -DFUNZIP -o $@ funzip.c

$(PROD)/crc32_f.o:    crc32.c     $(H_UNZIP) zip.h crc32.h
	$(CC) -c $(CF) -DFUNZIP -o $@ crc32.c

$(PROD)/crypt_f.o:    crypt.c     $(H_UNZIP) zip.h crypt.h crc32.h ttyio.h
	$(CC) -c $(CF) -DFUNZIP -o $@ crypt.c

$(PROD)/globals_f.o:  globals.c   $(H_UNZIP)
	$(CC) -c $(CF) -DFUNZIP -o $@ globals.c

$(PROD)/inflate_f.o:  inflate.c   $(H_UNZIP) inflate.h
	$(CC) -c $(CF) -DFUNZIP -o $@ inflate.c

$(PROD)/ttyio_f.o:    ttyio.c     $(H_UNZIP) zip.h crypt.h ttyio.h
	$(CC) -c $(CF) -DFUNZIP -o $@ ttyio.c

# B) fUnZip C files for specific ports
#    - Generated when referenced by OSDEP_OC*
#    - OSDEP_OC* are customized by unix/configure, based on PORT macro

$(PROD)/unix_f.o:   unix/unix.c $(H_UNZIP) unix/macosx.h
	$(CC) -c $(CF) -DFUNZIP -o $@ unix/unix.c

$(PROD)/win32_f.o:    win32/win32.c  $(H_UNZIP)
	$(CC) -c $(CF) -DFUNZIP -o $@ win32/win32.c

$(PROD)/win32i64_f.o:  win32/win32i64.c  $(H_UNZIP)
	$(CC) -c $(CF) -DFUNZIP -o $@ win32/win32i64.c

#----------------------------------------------------------------------------

# C Compile rules: AES (WinZip/Gladman) encryption

$(PROD)/aescrypt.o: aes_wg/aescrypt.c $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/aescrypt.c

$(PROD)/aeskey.o:   aes_wg/aeskey.c   $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/aeskey.c

$(PROD)/aestab.o:   aes_wg/aestab.c   $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/aestab.c

$(PROD)/fileenc.o:  aes_wg/fileenc.c  $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/fileenc.c

$(PROD)/hmac.o:     aes_wg/hmac.c     $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/hmac.c

$(PROD)/prng.o:     aes_wg/prng.c     $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/prng.c

$(PROD)/pwd2key.o:  aes_wg/pwd2key.c  $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/pwd2key.c

$(PROD)/sha1.o:     aes_wg/sha1.c     $(H_AES)
	$(CC) -c $(CF) -o $@ aes_wg/sha1.c

#----------------------------------------------------------------------------

# C Compile rules: LZMA compression

$(PROD)/LzFind.o:   szip/LzFind.c     $(H_LZMA)
	$(CC) -c $(CF) -o $@ szip/LzFind.c

$(PROD)/LzmaDec.o:  szip/LzmaDec.c    $(H_LZMA)
	$(CC) -c $(CF) -o $@ szip/LzmaDec.c

#----------------------------------------------------------------------------

# C Compile rules: PPMd compression

$(PROD)/Ppmd8.o:    szip/Ppmd8.c      $(H_PPMD)
	$(CC) -c $(CF) -o $@ szip/Ppmd8.c

$(PROD)/Ppmd8Dec.o: szip/Ppmd8Dec.c   $(H_PPMD)
	$(CC) -c $(CF) -o $@ szip/Ppmd8Dec.c

#----------------------------------------------------------------------------
#  Build rules: Assemble (.S -> .o)
#  - Optional assembler replacements for C routines.
#  - All are port-specific.
#    - Generated when referenced by OSDEP_OS.
#    - OSDEP_OS is customized by unix/configure, based on PORT macro.
#----------------------------------------------------------------------------

# Assembler rule: CRC-32
#
# 1) Assembler rule: CRC, 16-bit only, MSDOS
$(O_CRC_I):  msdos/crc_i86.asm
	$(AS) $(AF) -c -o $@ msdos/crc_i86.asm $(ASEOL)

# 2) Assembler rule: CRC 32bit, GNU AS
$(O_CRC_G):  crc_i386.S $(ACONF_DEP)
	$(AS) $(AF) -c -o $@ crc_i386.S

# 3) Assembler rule: CRC 32bit, SysV AS
$(O_CRC_SV): crc_i386.S $(ACONF_DEP)
	$(CPP) $(AF) crc_i386.S > $(PROD)/_crc_i386s.s
	$(AS) -c -o $@ $(PROD)/_crc_i386s.s
	$(RM) $(PROD)/_crc_i386s.s

#----------------------------------------------------------------------------
#  Build rules: Object libraries (.o -> .a)
#----------------------------------------------------------------------------

# Object library rule: AES (WinZip/Gladman) encryption

$(AR_AES): $(O_AES)
	@echo 'Building/updating AES (WinZip/Gladman) object library...'
	-$(RM) $@
	$(AR) $@ $(O_AES)
	-$(RANLIB) $@

# Object library rule: LZMA compression

$(AR_LZMA): $(O_LZMA)
	@echo 'Building/updating LZMA object library...'
	-$(RM) $@
	$(AR) $@ $(O_LZMA)
	-$(RANLIB) $@

# Object library rule: PPMd compression

$(AR_PPMD): $(O_PPMD)
	@echo 'Building/updating PPMd object library...'
	-$(RM) $@
	$(AR) $@ $(O_PPMD)
	-$(RANLIB) $@

# Object library rule: BZIP2 compression

$(AR_BZ):
	@echo 'Building/updating BZIP2 object library...'
	@if [ -f $(PROD)/flags_bz ]; then \
          ( cd bzip2/; \
            echo "Using flags_bz overrides"; \
            eval $(MAKE) -f $(MK_BZ) libbz2.a ACONF_DEP=$(PROD)/flags_bz \
             `cat ../$(PROD)/flags_bz`; \
          ); \
        else \
          ( cd bzip2/; \
            eval $(MAKE) -f $(MK_BZ) CC="$(CC_BZ)" CFLAGS="$(CF_BZ)" \
             RANLIB="$(RANLIB)" RM="rm -f" libbz2.a; \
          ); \
        fi

# Object library rule: ZLIB compression

$(AR_Z):
	@echo 'Building/updating ZLIB object library...'
	( cd zlib/; \
          eval $(MAKE) -f Makefile libz.a ACONF_DEP=$(PROD)/flags \
           `cat ../$(PROD)/flags`; \
        )

# Object library rule: Callable UnZip

$(AR_UNZIP): $(O_UNZIPL) $(O_UNZIPN) $(LIB_AES_WG) $(LIB_LZMA) $(LIB_PPMD)
	@echo 'Building/updating Callable UnZip object library...'
	sh unix/mlz.sh "$(AR_UNZIP)" "$(O_UNZIPL) $(O_UNZIPN)" \
         $(LIB_AES_WG) $(LIB_LZMA) $(LIB_PPMD)
	-$(RANLIB) $@

#----------------------------------------------------------------------------
#  Build rules: Link programs (.o + .a -> executables)
#----------------------------------------------------------------------------

# Link rule: unzip

$(PPGM_UNZIP): $(O_UNZIP) $(LIB_AES_WG) $(LIB_BZ) $(LIB_LZMA) \
                $(LIB_PPMD) $(LIB_Z)
	$(LD) -o $@ $(LF1) $(O_UNZIP) $(LIB_AES_WG) $(LIB_LZMA) \
         $(LIB_PPMD) $(LIB_Z) $(LF2)

# Link rule: unzipsfx

$(PPGM_SFX): $(O_SFX) $(LIB_AES_WG) $(LIB_BZ) $(LIB_LZMA) \
              $(LIB_PPMD) $(LIB_Z)
	$(LD) -o $@ $(LF1) $(O_SFX) $(LIB_AES_WG) $(LIB_LZMA) \
         $(LIB_PPMD) $(LIB_Z) $(LF2)

# Link rule: funzip

$(PPGM_FUZ): $(O_FUZ) $(LIB_AES_WG) $(LIB_BZ) $(LIB_LZMA) \
              $(LIB_PPMD) $(LIB_Z)
	$(LD) -o $@ $(LF1) $(O_FUZ) $(LIB_AES_WG) $(LIB_LZMA) \
         $(LIB_PPMD) $(LIB_Z) $(LF2)

# Link rule: zipinfo

$(PPGM_ZINFO): $(PPGM_UNZIP)
	@echo '  This is a Unix-specific target.'
	@echo ''
	@echo '  ZipInfo is not enabled in some MS-DOS versions of UnZip;'
	@echo '  if it is in yours, copy unzip.exe to zipinfo.exe, or else'
	@echo '  invoke as "unzip -Z" (in a batch file, for example).'
	$(LN) $(PPGM_UNZIP) $(PPGM_ZINFO)

#----------------------------------------------------------------------------
#  Build rules: Documentation
#  - Explicit 1:1 rules are used.
#  - Across various "make" utilities, pattern rule support is inconsistent.
#  - Sys V "make" may have problems with $< when input and output names differ.
#----------------------------------------------------------------------------

# UnZip documentation output directory rules
# - Creates $(PROD)/manout directory if unix/configure does not.

$(PROD)/manout:
	if [ ! -d "$(PROD)" ]; then mkdir "$(PROD)"; fi
	if [ ! -d "$@" ]; then mkdir "$@"; fi

# UnZip text documentation generation rules
#
# SunOS 4.x quirks:
#    "man -M" requires an absolute path, hence "`pwd`/".
#    /bin/man does not support "-x".  Specify "COL=/usr/5bin/col".

$(DOC_UNZIP): man/man1/unzip.1
	man -M ` pwd `/man unzip    | $(COL) -bx | uniq | expand > $@
	@echo ''

$(DOC_SFX):   man/man1/unzipsfx.1
	man -M ` pwd `/man unzipsfx | $(COL) -bx | uniq | expand > $@
	@echo ''

$(DOC_FUZ):   man/man1/funzip.1
	man -M ` pwd `/man funzip   | $(COL) -bx | uniq | expand > $@
	@echo ''

$(DOC_ZINFO): man/man1/zipinfo.1
	man -M ` pwd `/man zipinfo  | $(COL) -bx | uniq | expand > $@
	@echo ''

$(DOC_ZGREP): man/man1/zipgrep.1
	man -M ` pwd `/man zipgrep  | $(COL) -bx | uniq | expand > $@
	@echo ''

# UnZip alternate documentation generation rules
# - Requires nroff, or GNU groff package.

$(PMAN_UNZIP): man/man1/unzip.1
	nroff -man man/man1/unzip.1    | $(COL) -bx | uniq > $@

$(PMAN_SFX):   man/man1/unzipsfx.1
	nroff -man man/man1/unzipsfx.1 | $(COL) -bx | uniq > $@

$(PMAN_FUZ):   man/man1/funzip.1
	nroff -man man/man1/funzip.1   | $(COL) -bx | uniq > $@

$(PMAN_ZINFO): man/man1/zipinfo.1
	nroff -man man/man1/zipinfo.1  | $(COL) -bx | uniq > $@

$(PMAN_ZGREP): man/man1/zipgrep.1
	nroff -man man/man1/zipgrep.1  | $(COL) -bx | uniq > $@

#----------------------------------------------------------------------------
#  Build rules: Standard build targets
#----------------------------------------------------------------------------

all:        generic_msg generic
docs:       $(PROD)/manout $(UNZIP_DOCS)
docsrof:    $(PROD)/manout $(UNZIP_PMANS)

# Binaries build list (BINS) options:
L:          $(LIB_UNZIP)
LM:         $(LIB_UNZIP) $(PPGM_UNZIP)
LMU:        $(LIB_UNZIP) $(UNZIP_PPGMS)
LU:         $(LIB_UNZIP) $(PPGM_FUZ) $(PPGM_SFX)
M:          $(PPGM_UNZIP)
MU:         $(UNZIP_PPGMS)
U:          $(PPGM_FUZ) $(PPGM_SFX)

#----------------------------------------------------------------------------
#  Build rules: ???????
#  FIX_ME: ??????
#----------------------------------------------------------------------------
unix_make:
# @echo \
# '(Ignore any errors from `make'"' due to the following command; it's harmless.)"
	-@2>&1 $(LN) unix/Makefile . > /dev/null || echo > /dev/null
	if [ ! -d $(PROD) ]; then mkdir "$(PROD)"; fi

#----------------------------------------------------------------------------
#  Build rules: Configuration flags
#  - Creates files containing system-dependent flag values.
#  - Generated by unix/configure based on build utility program,
#    platform and features selected.
#----------------------------------------------------------------------------

# Main flags

config_flags: $(PROD)/flags

$(PROD)/flags: unix/configure
	@echo "Running:  unix/configure"
	@sh unix/configure \
         "PROD='$(PROD)'" \
         "LIST='$(LIST)'" \
         "CC='$(CC)'" \
         "CFLAGS='$(LOCAL_UNZIP)'" \
         "LFLAGS1='$(LFLAGS1)'" \
         "LFLAGS2='$(LFLAGS2)'" \
         "NO_AES_WG='$(NO_AES_WG)'" \
         "IZ_BZIP2='$(IZ_BZIP2)'" \
         "NO_IZ_BZIP2='$(NO_IZ_BZIP2)'" \
         "NO_LZMA='$(NO_LZMA)'" \
         "NO_PPMD='$(NO_PPMD)'" \
         "IZ_ZLIB='$(IZ_ZLIB)'"

# BZIP2 flags

config_flags_bz: $(PROD)/flags_bz

$(PROD)/flags_bz: $(PROD)/flags

# C macros (-Dxxxx, or edit a header file), and their effects:
#
# HAVE_DIRENT_H         - Use <dirent.h> instead of <sys/dir.h>.
# HAVE_NDIR_H           - Use <ndir.h> (unix/unix.c).
# HAVE_SYS_DIR_H        - Use <sys/dir.h>.
# HAVE_SYS_NDIR_H       - Use <sys/ndir.h>.
# NODIR                 - For 3B1, which has neither getdents() nor opendir().
# NO_BZIP2_SUPPORT      - Do not compile in bzip2 code even if available.
# NO_CONST              - Cannot handle ANSI "const" keyword.
# NO_LARGE_FILE_SUPPORT - Do not enable Large File support even if available.
# NO_PROTO              - Cannot handle ANSI function prototypes.
# NO_RMDIR              - Remove directories using a system("rmdir ...") call.
# NO_UNICODE_SUPPORT    - Do not enable Unicode support even if available.
# NO_ZIP64_SUPPORT      - Do not enable Zip64 archive support even if available.
# ZMEM                  - Include C language versions of memset(),
#                         memcpy(), and memcmp() (util.c).

#----------------------------------------------------------------------------
#  Build rules: Clean-up targets
#  - Used to remove old output files before performing a new build.
#  - Used when incompatible build options (or source changes) are required.
#----------------------------------------------------------------------------

clean:
	@if sh unix/unsafe_prod.sh "$(PROD)"; then \
          echo "Won't clean absolute (/x) or rising (../) PROD: $(PROD)"; \
        else \
          echo "rm -f \"$(PROD)\"/*.o \"$(PROD)\"/*.a $(UNZIP_PPGMS)"; \
          rm -f "$(PROD)"/*.o "$(PROD)"/*.a $(UNZIP_PPGMS); \
          echo "rm -f \"$(PROD)\"/flags \"$(PROD)\"/flags_bz"; \
          rm -f "$(PROD)"/flags "$(PROD)"/flags_bz; \
          echo "rm -f $(PROD)/manout"; \
          rm -rf "$(PROD)"/manout; \
        fi
#       # Remove empty $(PROD) directory.
	if [ -d "$(PROD)" ]; then \
          ls -1 "${PROD}"/* > /dev/null 2>&1 || \
          rm -r "$(PROD)" ; \
        fi
	rm -rf $(PKGDIR)

clean_bzip2:
	@if [ -f bzip2/$(MK_BZ) ]; then \
          ( cd bzip2; make -f $(MK_BZ) clean ); \
        else \
          echo "Expected bzip2 \"make\" file not found: bzip2/$(MK_BZ)"; \
        fi;

clean_exe:
	rm -f $(UNZIP_PPGMS) $(PPGM_ZINFO)

clean_docs:
	rm -f $(UNZIP_DOCS)

#----------------------------------------------------------------------------
#  Build rules: Installation-related
#----------------------------------------------------------------------------

install: install_bin install_man

install_gz: install_bin install_man_gz

install_bin:
	-$(INSTALL_D) $(BINDIR)
	$(INSTALL_PROGRAM) $(UNZIP_PPGMS) $(PPGM_ZGREP) $(BINDIR)
	$(RM) $(BIN_ZINFO)
	$(LN) $(BIN_UNZIP) $(BIN_ZINFO)
	$(CHMOD) $(BINPERMS) $(INSTALLEDBIN)

install_man:
	-$(INSTALL_D) $(MANDIR1)
	$(INSTALL) man/man1/funzip.1   $(MANDIR1)/$(MAN_FUZ)
	$(INSTALL) man/man1/unzip.1    $(MANDIR1)/$(MAN_UNZIP)
	$(INSTALL) man/man1/unzipsfx.1 $(MANDIR1)/$(MAN_SFX)
	$(INSTALL) man/man1/zipgrep.1  $(MANDIR1)/$(MAN_ZGREP)
	$(INSTALL) man/man1/zipinfo.1  $(MANDIR1)/$(MAN_ZINFO)
	$(CHMOD) $(MANPERMS) $(INSTALLEDMAN)

install_man_gz: install_man
	gzip $(INSTALLEDMAN)
	$(CHMOD) $(MANPERMS) $(INSTALLEDMAN_GZ)

uninstall:
	$(RM) $(INSTALLEDBIN) $(INSTALLEDMAN) $(INSTALLEDMAN_GZ)

#----------------------------------------------------------------------------
#  Build rules: SVR4 Package generation Interface
#  - Originally tested under Sun Solaris 2.x (by JBush).
#  - Other SVr4s may be very similar, and could possibly use this.
#  Note:  Expects version info to be stored in VERSION macro variable.
#         See "README" under unix/Packaging/.
#----------------------------------------------------------------------------

svr4package: $(BINS)
	@echo 'Creating SVR4 package for Unix ...'
	-@rm -rf $(PKGDIR) $(PKGDIR)_`uname -p`.pkg
	-@sed -e "s/.VERSION./$(VERSION)/g" \
         -e "s/.PSTAMP./$(LOGNAME)_`date | tr  ' ' '_'`/g" \
         -e "s/.ARCH./Solaris_`uname -rp | tr ' ' ','`/g" \
         unix/Packaging/pkginfo.in > unix/Packaging/pkginfo
	-@sed -e "s/.ARCH./`uname -p`/g" \
         unix/Packaging/preinstall.in > unix/Packaging/preinstall
	/usr/bin/pkgmk -d . -b . -r . -f unix/Packaging/prototype $(PKGDIR)
	/usr/bin/pkgtrans -o -s . $(PKGDIR)_`uname -p`.pkg $(PKGDIR)
	@echo ''
	@echo \
 "To install, copy $(PKGDIR)_`uname -p`.pkg to the target system, and"
	@echo \
 "issue the command (as root):  pkgadd -d $(PKGDIR)_`uname -p`.pkg"
	@echo ''

#----------------------------------------------------------------------------
#  Build rules: Distribution kit
#----------------------------------------------------------------------------

dist: $(DOC_UNZIP)
	eval unzip -r9 unzip`sed -e '/VERSION/!d' -e 's/.*"\(.*\)".*/\1/' \
         -e 's/[.]//g' -e 's/ .*//g' -e q revision.h` *

#----------------------------------------------------------------------------
#  Build rules: Unzip validation tests
#----------------------------------------------------------------------------

# Test archives.
TESTZIP = testmake.zip
TESTZIP_PPMD = testmake_ppmd.zip

# Test some basic features of the build.
check: test

dashv:
	$(PROD)/unzip -v

test:
	sh unix/test_unzip.sh $(TESTZIP) $(PROD)

test_ppmd:
	sh unix/test_unzip.sh $(TESTZIP_PPMD) $(PROD) NOFUNSFX

# Create a new testmake.zip or testmake_ppmd.zip.
#
# These targets are intended for use by UnZip developers, not by normal
# users.  The method used relies on an existing testmake.zip file to
# provide "notes", which historically has non-Unix line endings.
#
# Note that this is typically an iterative process, because the test
# archive includes an "unzip -Z" report on itself, which often results
# in mismatches on date-time and file size data.  If the results don't
# converge within a few iterations, then it may help to wait a minute (to
# get a different date-time), or to replace the current testmake.zip with
# the original (or some other old version).
#
# While crude, the method used here does try to save the original
# testmake[_ppmd].zip file as testmake[_ppmd].zip_orig, which is deleted
# only if the "diff" succeeds, which normally allows a manual restoration
# to the original state, if success remains elusive.

testmake_zip:
	@if test ! -f $(TESTZIP) ; then \
          echo "***" ; \
          echo "*** Missing \"$(TESTZIP)\".  Expect chronic failure. ***" ; \
          echo "***" ; \
        fi
	if test -f $(TESTZIP) -a ! -f $(TESTZIP)_orig ; then \
          cp -p $(TESTZIP) $(TESTZIP)_orig ; \
        fi
	@rm -rf testmake
	@mkdir testmake
	@( cd testmake ; \
         ../$(PROD)/unzip ../$(TESTZIP) > /dev/null ; \
         touch testmake.zipinfo ; \
         $(PGM_ZIP) -X $(TESTZIP) notes testmake.zipinfo > /dev/null ; \
         ../$(PROD)/unzip -Z -mc- $(TESTZIP) > testmake.zipinfo ; \
         $(PGM_ZIP) -X $(TESTZIP) notes testmake.zipinfo > /dev/null ; \
         ../$(PROD)/unzip -Z -mc- $(TESTZIP) > testmake.zipinfo_new ; \
         mv $(TESTZIP) ../$(TESTZIP) )
	@if diff testmake/testmake.zipinfo \
          testmake/testmake.zipinfo_new 2> /dev/null ; then \
            echo '' ; echo "   It's a miracle!" ; echo '' ; \
            rm $(TESTZIP)_orig ; \
        else \
          echo '' ; echo '   Try again?' ; echo '' ; \
        fi
	@rm -r testmake

testmake_ppmd_zip:
	@if test ! -f $(TESTZIP_PPMD) ; then \
           cp -p $(TESTZIP) $(TESTZIP_PPMD) ; \
        fi
	if test -f $(TESTZIP_PPMD) -a ! -f $(TESTZIP_PPMD)_orig ; then \
          cp -p $(TESTZIP_PPMD) $(TESTZIP_PPMD)_orig ; \
        fi
	@rm -rf testmake
	@mkdir testmake
	@( cd testmake ; \
         ../$(PROD)/unzip ../$(TESTZIP_PPMD) > /dev/null ; \
         touch testmake.zipinfo ; \
         $(PGM_ZIP) -X -Z ppmd $(TESTZIP_PPMD) notes testmake.zipinfo > \
         /dev/null ; \
         ../$(PROD)/unzip -Z -mc- $(TESTZIP_PPMD) > testmake.zipinfo ; \
         $(PGM_ZIP) -X -Z ppmd $(TESTZIP_PPMD) notes testmake.zipinfo > \
         /dev/null ; \
         ../$(PROD)/unzip -Z -mc- $(TESTZIP_PPMD) > testmake.zipinfo_new ; \
         mv $(TESTZIP_PPMD) ../$(TESTZIP_PPMD) )
	@if diff testmake/testmake.zipinfo \
          testmake/testmake.zipinfo_new 2> /dev/null ; then \
            echo '' ; echo "   It's a miracle!" ; echo '' ; \
            rm $(TESTZIP_PPMD)_orig ; \
        else \
          echo '' ; echo '   Try again?' ; echo '' ; \
        fi
	@rm -r testmake

############################
# INDIVIDUAL MACHINE RULES #
############################

#----------------------------------------------------------------------------
#  Generic targets
#  - These invoke the unix/configure script to determine configuration.
#----------------------------------------------------------------------------

# generic_msg
# - Issue warning message before generic build

generic_msg:
	@echo ''
	@echo '  Attempting "make generic" now.  If this fails for some reason, type'
	@echo '  "make help" for suggestions.  If none of the targets work for you,'
	@echo '  contact us using the information in README so we can work the'
	@echo '  problems.'
	@echo ''

# generic
# - Generate flags file (if doesn't exist), then UnZip modules.

generic: $(PROD)/flags
	eval $(MAKE) $(MAKEF) $(BINS) ACONF_DEP=$(PROD)/flags \
         `cat $(PROD)/flags`

# generic_pkg
# - Generate flags file (if doesn't exist), then UnZip modules, then perform
#   SVR4 package-creation

generic_pkg: generic svr4package

#----------------------------------------------------------------------------
#  Old, non-"generic" targets
#  - For those systems which do not work with the "generic" targets.
#  - Can not assume that the "make" utility groks "$(MAKE)".
#----------------------------------------------------------------------------

generic1: $(BINS)     # first try if unknown

generic2: unix_make  # second try if unknown:  hope make is called "make"
	make $(MAKEF) $(BINS) CF="$(CF) -DBSD"

generic3: unix_make  # third try if unknown:  hope make is called "make"
	make $(MAKEF) $(BINS) CF="$(CF) -DSYSV"

#----------------------------------------------------------------------------
#  "Autoconfig" group, aliases for the generic targets using configure:
#----------------------------------------------------------------------------

# Solaris:  generic, plus generation of installable package.
solaris: generic_pkg

#----------------------------------------------------------------------------
#  "Normal" group (BSD vs. SysV may be set in unzip.h via predefined macros):
#----------------------------------------------------------------------------

386i: $(BINS)     # sun386i, SunOS 4.0.2
#3Bx: $(BINS)     # AT&T 3B2/1000-80; should work on any WE32XXX machine
bull: $(BINS)     # Bull DPX/2, BOS 2.00.45 (doesn't require -Xk switch)
convex: $(BINS)   # Convex C-120 and C-210 (-O is enough; -ext is default)
cray: $(BINS)     # Cray-2 and Y-MP, using default (possibly old) compiler
dec: $(BINS)      # DEC 5820 (MIPS RISC), test version of Ultrix v4.0
encore: $(BINS)   # Multimax
eta: $(BINS)      # ETA-10P*, hybrid SysV with BSD 4.3 enhancements
gould: $(BINS)    # Gould PN9000 running UTX/32 2.1Bu01
hp: $(BINS)       # HP 9000 series (68020), 4.3BSD or HP-UX A.B3.10 Ver D
hpux: $(BINS)     # (to match Zip's makefile entry)
mips: $(BINS)     # MIPS M120-5(?), SysV.3 [error in sys/param.h file?]
next10: $(BINS)   # NeXT (generic; use next2x or next3x for better opt.)
osf1: $(BINS)     # DECstation, including Alpha-based; DEC OSF/1 v1.x
pyr_: $(BINS)     # [failsafe target for pyramid target below]
pyr_ucb: $(BINS)  # Pyramids running BSD universe by default (see below)
realix: $(BINS)   # Modcomp Real/IX (SysV.3); note "gcc" = GLS C, not GNU
sco: $(BINS)      # Xenix/386 (tested on 2.3.1); SCO Unix 3.2.0.
sgi: $(BINS)      # Silicon Graphics; Irix 3.3.2, 4.0.x, 5.2, etc.
stellar: $(BINS)  # gs-2000
sun: $(BINS)      # old target; no good with solaris...use "sunos" now
sunos: $(BINS)    # no good with SunOS 3.x...use "sunos3" or "sunos4" now
sunos4: $(BINS)   # Sun 3, 4; SunOS 4.x (SOME SYSTEMS ARE SYSTEM V!)
tahoe: $(BINS)    # tahoe (CCI Power6/32), 4.3BSD
ultrix: $(BINS)   # VAXen, DEC 58x0 (MIPS guts), DECstation 2100; v4.x
vax: $(BINS)      # general-purpose VAX target (not counting VMS)

#----------------------------------------------------------------------------
#  BSD group (for timezone structs [struct timeb]):
#----------------------------------------------------------------------------

bsd: _bsd        # generic BSD (BSD 4.2 & Ultrix handled in unzip.h)

_bsd: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DBSD"

#----------------------------------------------------------------------------
#  SysV group (for extern long timezone and ioctl.h instead of sgtty.h):
#----------------------------------------------------------------------------

aviion: _sysv   # Data General AViiONs, DG/UX 4.3x
pyr_att: _sysv  # Pyramids running AT&T (SysV) universe by default
stardent: _sysv # Stardent ...
sysv: _sysv     # generic System V Unix (Xenix handled in unzip.h)
xos: _sysv      # Olivetti LSX-3005..3045, X/OS 2.3 and 2.4

_sysv: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DSYSV"

# extension to perform SVR4 package-creation after compilation
_sysvp: _sysv svr4package

#----------------------------------------------------------------------------
#  Version 7 group (old/obsolescent):
#----------------------------------------------------------------------------

pixel: _v7      # Pixel 80, 100 (68000-based, V7/mostly BSD4.1 compat.)
v7: _v7         # generic Unix Version 7 box (prob. only Pixel...)

_v7:
	if [ ! -d $(PROD) ]; then mkdir "$(PROD)"; fi
	make $(MAKEF) $(BINS) CF="$(CF) -DV7 -DNO_PARAM_H \
         -DSHORT_NAMES -DBSD -DZMEM -DNO_LCHOWN -DNO_LCHMOD"

#----------------------------------------------------------------------------
#  "Unique" group (require non-standard options):
#----------------------------------------------------------------------------

# AT&T 3B2/1000-80; should work on any WE32XXX machine
3Bx: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DCBREAK=2"

# AT&T 7300 (M68000/SysV.3) (add -DSYSV? -DNO_LIMITS?)
7300: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DNO_DIR -DNO_MKDIR -DNO_STRNICMP \
         -DNO_UID_GID -DNO_FCHMOD -DNO_LCHOWN -DNO_LCHMOD -DCBREAK=2"

7300_gcc: unix_make
	$(MAKE) $(BINS) CC=gcc LD=gcc LF2="" CFLAGS="-O2" \
         LOC="-DNO_DIR -DNO_MKDIR -DNO_STDLIB_H -DNO_STRNICMP \
         -DNO_UID_GID -DNO_FCHMOD -DNO_LCHOWN -DNO_LCHMOD -DCBREAK=2 $(LOC)"
	$(STRIP) $(UNZIP_PPGMS)

# Amdahl (IBMish) mainframe, UTS (SysV) 1.2.4, 2.0.1, 3.x
amdahl: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DSYSV -DNO_UID_GID -DNO_LCHOWN -DNO_LCHMOD"

# Amdahl UTS 2.1.4 with "extended file types" filesystem (aarrrggghhhh...)
amdahl_eft: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -eft -DSYSV -DNO_UID_GID -DNO_LCHOWN -DNO_LCHMOD"

# Apollo Domain/OS machines (added -D...SOURCE options) [Gordon Fox, 960810]
apollo: unix_make
	$(MAKE) $(BINS) CF="$(CF) -D_INCLUDE_BSD_SOURCE \
         -D_INCLUDE_XOPEN_SOURCE -DNO_LCHOWN -DNO_LCHMOD"

# BSDI BSD/OS on 386 platform, using the assembler replacement for crc32.c
bsdi: unix_make
	@echo 'NOTE:  use bsdi_noasm target for non-Intel BSD/OS compiles.'
	$(MAKE) $(BINS) CC=gcc2 LD=shlicc2 AS=gcc2 \
         CFLAGS="-O3 -Wall -DASM_CRC -DBSD" OSDEP_OS="$(PROD)/crc_gcc.o"

# BSDI BSD/OS
bsdi_noasm: unix_make
# @echo 'NOTE:  use bsd target for non-Intel BSD/OS compiles.'
	$(MAKE) $(BINS) CC=gcc2 LD=shlicc2 AS=gcc2 \
         CFLAGS="-O3 -Wall -DBSD"

# Coherent 3.x/4.x, Mark Williams C.  ``For Coherent's CC, it needs either
# -T0 or -T150000 (or bigger) added to the CFLAGS, otherwise the compiler
# runs out of memory and dies in zipinfo.c.'' [Fred "Fredex" Smith, 940719]
coherent: unix_make
	$(MAKE) $(BINS) CFLAGS="$(CFLAGS) -T0 -DNO_LCHOWN -DNO_LCHMOD"

# Cray-2, Y-MP or C90, running Unicos 5.x to 8.x (SysV + BSD enhancements)
# and Standard (ANSI) C compiler 3.0 or later.
cray_opt: unix_make
	$(MAKE) $(BINS) \
         CFLAGS="$(CFLAGS) -h scalar3 -h vector3 -DNO_LCHOWN -DNO_LCHMOD"

# The unzip41 build on a Cyber 910/SGI running Irix v3.3.3 was successful
# with the following change to Makefile:
cyber_sgi: unix_make
	$(MAKE) $(BINS) \
         CFLAGS="$(CFLAGS) -I/usr/include/bsd -DNO_LCHOWN -DNO_LCHMOD" \
         LF="-lbsd $(LF)" SL="-lbsd $(SL)"

# The "generic" target should work with Cygwin on Windows.  The
# following instructons are old, and probably obsolete...
# The Cygwin environment on a Win32 system, treated as a Unix emulator.
# This port does not offer full access to the Windows file system.
# Info-ZIP recommends using "win32/Makefile.gcc" instead.
cygwin: unix_make
	$(MAKE) $(BINS) CC=gcc LD=gcc AS=gcc \
         CFLAGS="-O3 -DASM_CRC -DNO_LCHOWN -DNO_LCHMOD" \
         AF="-Di386 $(AF)" OSDEP_OS="$(PROD)/crc_gcc.o" \
         E=".exe" CP="cp" LN="ln -s"

# 680x0, DIAB dnix 5.2/5.3 (a Swedish System V clone)
#
# Options for the dnix cc:
#  -X7 = cc is strict ANSI C
#  -X9 = warnings if a function is used without a declaration
#
dnix: unix_make
	$(MAKE) $(BINS) CFLAGS="$(CFLAGS) -X7 -X9 -DDNIX"

# FreeBSD on Intel:
freebsd: unix_make
	@echo 'NOTE:  use bsd target for non-Intel FreeBSD compiles (if any).'
	$(MAKE) $(BINS) CC=gcc LD=gcc AS=gcc \
         CFLAGS="-O3 -Wall -DASM_CRC -DBSD" \
         AF="-Di386 $(AF)" OSDEP_OS="$(PROD)/crc_gcc.o"

# Generic BSDish Unix gcc.  ``The -O3 only works with later versions of gcc;
# you may have to use -O2 or -O for earlier versions.  I have no idea why
# -s causes this bug in gcc.''  [Bug:  "nm: unzip: no name list", "collect:
# /usr/bin/nm returned 1 exit status".]  If you don't have strip, don't
# worry about it (it just makes the executable smaller and can be replaced
# with "echo" instead).
#
gcc: unix_make
	$(MAKE) $(BINS) CC=gcc LD=gcc CFLAGS="-O3" LF2=""
	$(STRIP) $(UNZIP_PPGMS)

# Heurikon HK68 (68010), UniPlus+ System V 5.0, Green Hills C-68000
hk68: unix_make
	$(MAKE) $(BINS) CC="gcc" LD="gcc" \
         LF="-n $(LF)" SL="-n $(SL)" FL="-n $(FL)" \
         CFLAGS="-ga -X138 -Dlocaltime=localti -Dtimezone=timezon"

# ISC Unix on 386 platform
isc: unix_make
	$(MAKE) $(BINS) LF2="-lc_s $(LF2)" OSDEP_OS="$(PROD)/crc_sysv.o" \
         CFLAGS="-O" \
         LOC="-DASM_CRC -DSYSV -DNO_UID_GID -DNEED_PTEM -DNO_LCHOWN \
         -DNO_LCHMOD $(LOC)" \
         AF="-DNO_UNDERLINE -Djecxz=jcxz -DALIGNMENT='.align 16' $(AF)"

isc_gcc: unix_make
	$(MAKE) $(BINS) AS=gcc CC=gcc LD=gcc OSDEP_OS="$(PROD)/crc_gcc.o" \
         LF="-shlib $(LF)" SL="-shlib $(SL)" FL="-shlib $(FL)" LF2="" \
         CFLAGS="-O3" \
         LOC="-DSYSV -DASM_CRC -DNO_UID_GID -DNEED_PTEM -DNO_LCHOWN \
          -DNO_LCHMOD $(LOC)" \
         AF="-DNO_UNDERLINE -Djecxz=jcxz -DALIGNMENT='.align 16' $(AF)"
	$(STRIP) $(UNZIP_PPGMS)

# "ISI machine (68025 CPU)" (based on e-mail from Rob White <rsw@tfs.com>;
# no further information).  May also need DIRENT defined.
isi: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DDECLARE_ERRNO -DNO_LCHOWN -DNO_LCHMOD"

# The "generic" target should work with GNU/Linux.  The following
# instructons are old, and probably obsolete...
# Linux on 386 platform, using the assembler replacement for crc32.c. (-O4 and
# -fno-strength-reduce have virtually no effect beyond -O3.  Add "-m486
# -malign-functions=2 -malign-jumps=2 -malign-loops=2" for Pentium [Pro]
# systems.)
linux: unix_make
	@echo 'NOTE:  use linux_noasm target for non-Intel Linux compiles.'
	$(MAKE) $(BINS) CC=gcc LD=gcc AS=gcc \
         CFLAGS="-O3 -Wall -DASM_CRC" \
         AF="-Di386 $(AF)" OSDEP_OS="$(PROD)/crc_gcc.o"

linux_asm: linux

# Linux (Posix, approximately SysV):  virtually any version since before 0.96,
# for any platform.  Change "-O" to "-O3" or whatever, as desired...
linux_noasm: unix_make
	$(MAKE) $(BINS) CC=gcc LD=gcc CFLAGS="-O -Wall"

# Linux with lcc compiler:  __inline__ (stat.h) not recognized, and must edit
# /usr/include/gnu/types.h to get rid of "long long" if __LCC__ defined.  -O3
# (or -O2 or -O) is ignored.  [GRR 960828: test target only]
#
linux_lcc: unix_make
	$(MAKE) $(BINS) CC=lcc LD=lcc CFLAGS="-O3 -Wall -D__inline__= "

# Linux host with go32 (djgpp) cross-compiler (go32crs.tgz) for 32-bit DOS.
linux_dos: unix_make
	$(MAKE) $(BINS) CC=go32gcc CFLAGS="-O2 -Wall" \
         LD=go32gcc \
         OSDEP_H="msdos/doscfg.h" \
         OSDEP_OCF="$(PROD)/msdos.o" \
         OSDEP_OCS="$(PROD)/msdos.o" \
         OSDEP_OCU="$(PROD)/msdos.o" \
         OSDEP_OS=""
# go32-strip unzip
# Due to limitations of the cross-compiling package, this has to be
# done manually:
	@echo "Copy $(UNZIP_PPGMS) to your DOS partition and use coff2exe."

# LynxOS-x86 2.3.0 and newer, a real-time BSD-like OS; uses gcc.
lynx: unix_make
	$(MAKE) $(BINS) CC=gcc CF="$(CF) -DLynx -DLYNX -DBSD -DUNIX"

# Macintosh MacOS X (Unix-compatible enviroment), using standard compiler
macosx: unix_make
	$(MAKE) $(BINS) CFLAGS="-O3 -Wall -DBSD" LF2=""
	$(STRIP) $(UNZIP_PPGMS)

# Macintosh MacOS X (Unix-compatible enviroment), using gcc
macosx_gcc: unix_make
	$(MAKE) $(BINS) CC=gcc CFLAGS="-O3 -Wall -DBSD" LF2=""
	$(STRIP) $(UNZIP_PPGMS)

# Minix 1.5 PC for the 386.  Invoke as is to use default cc, or as "make
# minix CC=gcc" to use gcc.  Try "make linux" if you have a working termios.h.
minix: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -DMINIX -DSHORT_NAMES -DNO_LCHOWN -DNO_LCHMOD" \
         CC=$(CC) LD=$(CC)

# MPE/iX, the Unix variant for HP 3000 systems.
mpeix: unix_make
	$(MAKE) $(BINS) CC=c89 \
         CF="$(CF) -DUNIX -D_POSIX_SOURCE -DHAVE_TERMIOS_H \
          -DPASSWD_FROM_STDIN -DNO_PARAM_H -DNO_LCHOWN -DNO_LCHMOD" \
         LF2=-lbsd CP=cp LN="ln -s"

# NeXT info.
next:
	@echo ''
	@echo \
 '  Please pick a specific NeXT target:  "make next10" will create a generic'
	@echo \
 '  NeXT executable; "make next2x" will create a smaller executable (for'
	@echo \
 '  NeXTstep 2.0 and higher); "make next3x" will create a small executable'
	@echo \
 '  with significantly better optimization (NeXTstep 3.0 and higher only);'
	@echo \
 '  "make nextfat" will create a fat, multi-architecture (NeXT plus Intel)'
	@echo \
 '  executable (NeXTstep 3.1 and higher only).'
	@echo ''

# 68030 BSD 4.3+Mach.  NeXT 2.x: make the executable smaller.
next2x: unix_make
	$(MAKE) $(BINS) LF2="-object -s"

# NeXT 3.x: as above, plus better optimization.
next3x: unix_make
	$(MAKE) $(BINS) CFLAGS="-O2" LF2="-object -s"

# NeXT 3.1+: make the executable fat (multi-architecture binary [MAB],
# for "black" [NeXT] and "white" [x86] hardware, so far).
nextfat: unix_make
	$(MAKE) $(BINS) CFLAGS="-O2 -arch i386 -arch m68k" \
         LF2="-arch i386 -arch m68k -object -s"

# Sequent Symmetry running Dynix/ptx (sort of SysV.3):  needs to link
# with libseq to get symlink().
ptx: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -DSYSV -DTERMIO -DPTX -DNO_LCHOWN -DNO_LCHMOD" \
         LF2="$(LF2) -lseq"

# Pyramid 90X (probably all) under >= OSx4.1, either universe.  (This is an
# experimental target!  If it fails, use either pyr_ucb or pyr_att instead.)
# The "make" in the BSD half is too stupid to understand $(MAKE), sigh...
pyramid: unix_make
	-make $(MAKEF) pyr_`universe`

# QNX/Neutrino
# - No native development tools, so cross-build environement is used.
# - Use gcc and set ARCH as follows:
#      "x86"    to produce x86
#      "ppcbe"             PowerPC (big-endian)
#      "ppcle"             PowerPC (little-endian)
#      "mipsbe"            MIPS    (big-endian)
#      "mipsle"            MIPS    (little-endian)
qnxnto: unix_make
	@if [ "$(ARCH)" = "" ] ; then \
          echo "You didn't set ARCH; I'll assume you meant ARCH=x86..." ; \
          echo '' ; \
          $(MAKE) $(MAKEF) CC="qcc -Vgcc_ntox86" $(BINS) ; \
        else \
          echo "Making UnZip for $(ARCH)..." ; \
          echo '' ; \
          $(MAKE) $(MAKEF) CC="qcc -Vgcc_nto$(ARCH)" $(BINS) ; \
        fi

# REGULUS:  68040-based, "real-time" SysV.3 mutant; uses gcc, with "REGULUS"
# predefined.
regulus: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -traditional -DSYSV -DNO_MKDIR -DNO_LCHOWN -DNO_LCHMOD"

# SCO UNIX with shared libraries and no international support.  If you are
# not using a USA-style keyboard and display, you may want to remove -nointl
# to get support.  It adds quite a bit to the size of the executable.
sco_sl: unix_make
	$(MAKE) $(BINS) LF="$(LF) -nointl" LF2="$(LF2) -lc_s" \
         SL="$(SL) -nointl" FL="$(FL) -nointl"

# SCO Xenix/286 2.2.3 or later with development system 2.2.1 or later
sco_x286: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -Mel2 -LARGE -DNO_MKDIR -DNO_LCHOWN -DNO_LCHMOD" \
         LF="$(LF) -Mel2 -LARGE -lx" SL="$(SL) -Mel2 -LARGE" \
         FL="$(FL) -Mel2 -LARGE"

# Sequent Symmetry with Dynix.  (386, but needs -DZMEM)
# This should also work on Balance but I can't test it just yet.
sequent: unix_make
	$(MAKE) $(BINS) CF="$(CF) -DBSD -DZMEM -DNO_LCHOWN -DNO_LCHMOD"

# Sun 2, 3, 4 running SunOS 3.x
sunos3: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -DNO_UID_GID -DUID_USHORT -DNO_LCHOWN -DNO_LCHMOD"

# Generic System V + GNU C
sysv_gcc: unix_make
	$(MAKE) $(BINS) CC=gcc LD=gcc CFLAGS="-O2 -DSYSV" LF2=""
	$(STRIP) $(UNZIP_PPGMS)

# AT&T 6300+, System V.2 Unix:  run-time out-of-memory error if don't use -Ml;
# also compile-time error if work arrays dimensioned at HSIZE+2 (>32K)
sysv6300: unix_make
	$(MAKE) $(BINS) CF="$(CF) -Ml -DSYSV -DNO_LCHOWN -DNO_LCHMOD" \
         LF="$(LF) -Ml" SL="$(SL) -Ml" FL="$(FL) -Ml"

# Texas Instruments System V.3 (running on HP 9000-1500)
ti_sysv: unix_make
	$(MAKE) $(BINS) \
         CF="$(CF) -DSYSV -DNO_UID_GID -DUID_USHORT -DNO_LCHOWN -DNO_LCHMOD"

# SCO Xenix
# - Joe Foster 1995/05/08
# - unzip needs to be linked with -lx [for the opendir(), readdir(),
#   telldir(), rewinddir(), and closedir() calls]")
xenix: unix_make
	$(MAKE) $(BINS) LF2="$(LF2) -lx"

