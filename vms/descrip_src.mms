#                                               16 February 2008.  CS.
#
#    UnZip 6.0 for VMS - MMS (or MMK) Source Description File.
#

# This description file is included by other description files.  It is
# not intended to be used alone.  Verify proper inclusion.

.IFDEF INCL_DESCRIP_SRC
.ELSE
$$$$ THIS DESCRIPTION FILE IS NOT INTENDED TO BE USED THIS WAY.
.ENDIF


# Define MMK architecture macros when using MMS.

.IFDEF __MMK__                  # __MMK__
.ELSE                           # __MMK__
ALPHA_X_ALPHA = 1
IA64_X_IA64 = 1
VAX_X_VAX = 1
.IFDEF $(MMS$ARCH_NAME)_X_ALPHA     # $(MMS$ARCH_NAME)_X_ALPHA
__ALPHA__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_ALPHA
.IFDEF $(MMS$ARCH_NAME)_X_IA64      # $(MMS$ARCH_NAME)_X_IA64
__IA64__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_IA64
.IFDEF $(MMS$ARCH_NAME)_X_VAX       # $(MMS$ARCH_NAME)_X_VAX
__VAX__ = 1
.ENDIF                              # $(MMS$ARCH_NAME)_X_VAX
.ENDIF                          # __MMK__

# Combine command-line VAX C compiler macros.

.IFDEF VAXC                     # VAXC
VAXC_OR_FORCE_VAXC = 1
.ELSE                           # VAXC
.IFDEF FORCE_VAXC                   # FORCE_VAXC
VAXC_OR_FORCE_VAXC = 1
.ENDIF                              # FORCE_VAXC
.ENDIF                          # VAXC

# Analyze architecture-related and option macros.

.IFDEF __ALPHA__                # __ALPHA__
DECC = 1
DESTC = ALPHA
.IFDEF LARGE                        # LARGE
DEST = $(DESTC)L
.ELSE                               # LARGE
DEST = $(DESTC)
.ENDIF                              # LARGE
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
DECC = 1
DESTC = IA64
.IFDEF LARGE                            # LARGE
DEST = $(DESTC)L
.ELSE                                   # LARGE
DEST = $(DESTC)
.ENDIF                                  # LARGE
.ELSE                               # __IA64__
.IFDEF __VAX__                          # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                   # VAXC_OR_FORCE_VAXC
DESTC = VAXV
.ELSE                                       # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                     # GNUC
CC = GCC
DESTC = VAXG
.ELSE                                           # GNUC
DECC = 1
DESTC = VAX
.ENDIF                                          # GNUC
.ENDIF                                      # VAXC_OR_FORCE_VAXC
.ELSE                                   # __VAX__
DESTC = UNK
UNK_DEST = 1
.ENDIF                                  # __VAX__
DEST = $(DESTC)
.ENDIF                              # __IA64__
.ENDIF                          # __ALPHA__

# Library module name suffix for XXX_.OBJ with GNU C.

.IFDEF GNUC                     # GNUC
GCC_ = _
.ELSE                           # GNUC
GCC_ =
.ENDIF                          # GNUC

# Check for option problems.

.IFDEF __VAX__                  # __VAX__
.IFDEF LARGE                        # LARGE
LARGE_VAX = 1
.ENDIF                              # LARGE
.IFDEF VAXC_OR_FORCE_VAXC           # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                             # GNUC
VAX_MULTI_CMPL = 1
.ENDIF                                  # GNUC
.ENDIF                              # VAXC_OR_FORCE_VAXC
.ELSE                           # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC           # VAXC_OR_FORCE_VAXC
NON_VAX_CMPL = 1
.ELSE                               # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                             # GNUC
NON_VAX_CMPL = 1
.ENDIF                                  # GNUC
.ENDIF                              # VAXC_OR_FORCE_VAXC
.ENDIF                          # __VAX__

# Shortcut to include BZIP2 support from the optional bzip2 source subdir
# in the UnZip source location.
.IFDEF USEBZ2
.IFDEF IZ_BZIP2
.ELSE
IZ_BZIP2 = SYS$DISK:[.BZIP2]
BZ2DIR_BIN = [.BZIP2.$(DESTC)]
BZIP2_INTEGRATED_BUILD = 1
.ENDIF                                     # IZ_BZIP2
.ENDIF                                  # USEBZ2

# Complain if warranted.  Otherwise, show destination directory.
# Make the destination directory, if necessary.

.IFDEF UNK_DEST                 # UNK_DEST
.FIRST
	@ write sys$output -
 "   Unknown system architecture."
.IFDEF __MMK__                      # __MMK__
	@ write sys$output -
 "   MMK on IA64?  Try adding ""/MACRO = __IA64__""."
.ELSE                               # __MMK__
	@ write sys$output -
 "   MMS too old?  Try adding ""/MACRO = MMS$ARCH_NAME=ALPHA"","
	@ write sys$output -
 "   or ""/MACRO = MMS$ARCH_NAME=IA64"", or ""/MACRO = MMS$ARCH_NAME=VAX"","
	@ write sys$output -
 "   as appropriate.  (Or try a newer version of MMS.)"
.ENDIF                              # __MMK__
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                           # UNK_DEST
.IFDEF VAX_MULTI_CMPL               # VAX_MULTI_CMPL
.FIRST
	@ write sys$output -
 "   Macro ""GNUC"" is incompatible with ""VAXC"" or ""FORCE_VAXC""."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                               # VAX_MULTI_CMPL
.IFDEF NON_VAX_CMPL                     # NON_VAX_CMPL
.FIRST
	@ write sys$output -
 "   Macros ""GNUC"", ""VAXC"", and ""FORCE_VAXC"" are valid only on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                   # NON_VAX_CMPL
.IFDEF LARGE_VAX                            # LARGE_VAX
.FIRST
	@ write sys$output -
 "   Macro ""LARGE"" is invalid on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                       # LARGE_VAX
.IFDEF IZ_BZIP2                                 # IZ_BZIP2
BZ2_OLB = LIBBZ2_NS.OLB
INCL_BZIP2_M = , UBZ2ERR
INCL_BZIP2_Q = /include = (UBZ2ERR)
.FIRST
	@ write sys$output "   Destination: [.$(DEST)]"
	@ write sys$output ""
	@ define incl_bzip2 $(IZ_BZIP2)
	@ if (f$search( "incl_bzip2:bzlib.h;") .eqs. "") then -
	   write sys$output "   Cannot find bzlib.h header file."
	@ if (f$search( "incl_bzip2:bzlib.h;") .eqs. "") then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.IFDEF BZIP2_INTEGRATED_BUILD
	@ $(MMS)$(MMSQUALIFIERS)/DESCR=$(IZ_BZIP2)descrbz2.mms'macro' -
	 /MACRO=(SRCDIR=$(IZ_BZIP2),DSTDIR=$(BZ2DIR_BIN), -
	   DEST=$(IZ_BZIP2)$(DESTC)) $(MMSTARGETS)
	@ if ("$(MMSTARGETS)" .nes. "CLEAN") then -
	  @[.VMS]FIND_BZIP2_LIB.COM $(IZ_BZIP2) $(DESTC) $(BZ2_OLB) lib_bzip2
	@ if (f$trnlnm( "lib_bzip2") .nes. "") then -
	   write sys$output "   BZIP2 dir: ''f$trnlnm( "lib_bzip2")'"
	@ if ("$(MMSTARGETS)" .nes. "CLEAN") then -
	   if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	     write sys$output "   Can not find BZIP2 object library."
	@ write sys$output ""
	@ if ("$(MMSTARGETS)" .nes. "CLEAN") then -
	   if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                             # BZIP2_INTEGRATED_BUILD
	@ @[.VMS]FIND_BZIP2_LIB.COM $(IZ_BZIP2) $(DESTC) $(BZ2_OLB) lib_bzip2
	@ if (f$trnlnm( "lib_bzip2") .nes. "") then -
	   write sys$output "   BZIP2 dir: ''f$trnlnm( "lib_bzip2")'"
	@ if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	   write sys$output "   Can not find BZIP2 object library."
	@ write sys$output ""
	@ if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	 I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ENDIF                                            # BZIP2_INTEGRATED_BUILD
	if (f$search( "$(DEST).DIR;1") .eqs. "") then -
	 create /directory [.$(DEST)]
.ELSE                                           # IZ_BZIP2
.FIRST
	@ write sys$output "   Destination: [.$(DEST)]"
	@ write sys$output ""
	if (f$search( "$(DEST).DIR;1") .eqs. "") then -
	 create /directory [.$(DEST)]
.ENDIF                                          # IZ_BZIP2
.ENDIF                                      # LARGE_VAX
.ENDIF                                  # NON_VAX_CMPL
.ENDIF                              # VAX_MULTI_CMPL
.ENDIF                          # UNK_DEST

# BZIP2 options.

.IFDEF IZ_BZIP2                            # IZ_BZIP2
CDEFS_BZIP2 = , USE_BZIP2
CFLAGS_INCL = /include = ([], [.VMS])
LIB_BZIP2_OPTS = lib_bzip2:$(BZ2_OLB) /library,
.ELSE                                   # IZ_BZIP2
CDEFS_BZIP2 =
CFLAGS_INCL = /include = []
LIB_BZIP2_OPTS =
.ENDIF                                  # IZ_BZIP2

# DBG options.

.IFDEF DBG                      # DBG
CFLAGS_DBG = /debug /nooptimize
LINKFLAGS_DBG = /debug /traceback
.ELSE                           # DBG
CFLAGS_DBG =
LINKFLAGS_DBG = /notraceback
.ENDIF                          # DBG

# Large-file options.

.IFDEF LARGE                    # LARGE
CDEFS_LARGE = , LARGE_FILE_SUPPORT
.ELSE                           # LARGE
CDEFS_LARGE =
.ENDIF                          # LARGE

# C compiler defines.

.IFDEF LOCAL_UNZIP
C_LOCAL_UNZIP = , $(LOCAL_UNZIP)
.ELSE
C_LOCAL_UNZIP =
.ENDIF

CDEFS = VMS $(CDEFS_BZIP2) $(CDEFS_LARGE) $(C_LOCAL_UNZIP)

CDEFS_UNX = /define = ($(CDEFS))

CDEFS_CLI = /define = ($(CDEFS), VMSCLI)

CDEFS_SFX = /define = ($(CDEFS), SFX)

CDEFS_SFX_CLI = /define = ($(CDEFS), SFX, VMSCLI)

# Other C compiler options.

.IFDEF DECC                             # DECC
CFLAGS_ARCH = /decc /prefix = (all)
.ELSE                                   # DECC
.IFDEF FORCE_VAXC                           # FORCE_VAXC
CFLAGS_ARCH = /vaxc
.IFDEF VAXC                                     # VAXC
.ELSE                                           # VAXC
VAXC = 1
.ENDIF                                          # VAXC
.ELSE                                       # FORCE_VAXC
CFLAGS_ARCH =
.ENDIF                                      # FORCE_VAXC
.ENDIF                                  # DECC

# LINK (share) library options.
# Omit shareable image options file for NOSHARE.

.IFDEF VAXC_OR_FORCE_VAXC               # VAXC_OR_FORCE_VAXC
.IFDEF NOSHARE                              # NOSHARE
OPT_FILE =
LFLAGS_ARCH =
.ELSE                                       # NOSHARE
OPT_FILE = [.$(DEST)]VAXCSHR.OPT
LFLAGS_ARCH = $(OPT_FILE) /options,
.ENDIF                                      # NOSHARE
.ELSE                                   # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                 # GNUC
LFLAGS_GNU = GNU_CC:[000000]GCCLIB.OLB /LIBRARY
.IFDEF NOSHARE                                  # NOSHARE
OPT_FILE =
LFLAGS_ARCH = $(LFLAGS_GNU),
.ELSE                                           # NOSHARE
OPT_FILE = [.$(DEST)]VAXCSHR.OPT
LFLAGS_ARCH = $(LFLAGS_GNU), SYS$DISK:$(OPT_FILE) /options,
.ENDIF                                          # NOSHARE
.ELSE                                       # GNUC
OPT_FILE =
LFLAGS_ARCH =
.ENDIF                                      # GNUC
.ENDIF                                  # VAXC_OR_FORCE_VAXC

# LINK NOSHARE options.

.IFDEF NOSHARE                  # NOSHARE
.IFDEF __ALPHA__                    # __ALPHA__
NOSHARE_OPTS = , SYS$LIBRARY:STARLET.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                               # __ALPHA__
.IFDEF __IA64__                         # __IA64__
NOSHARE_OPTS = , SYS$LIBRARY:STARLET.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                                   # __IA64__
OLDVAX_OLDVAX = 1
.IFDEF DECC                                 # DECC
.IFDEF OLDVAX_$(NOSHARE)                        # OLDVAX_$(NOSHARE)
NOSHARE_OPTS = , SYS$LIBRARY:DECCRTL.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                                           # OLDVAX_$(NOSHARE)
NOSHARE_OPTS = , SYS$LIBRARY:DECCRTL.OLB /LIBRARY\
 /INCLUDE = (CMA$TIS, CMA$TIS_VEC) /NOSYSSHR
.ENDIF                                          # OLDVAX_$(NOSHARE)
.ELSE                                       # DECC
.IFDEF OLDVAX_$(NOSHARE)                        # OLDVAX_$(NOSHARE)
NOSHARE_OPTS = , SYS$LIBRARY:VAXCRTL.OLB /LIBRARY,\
 SYS$LIBRARY:IMAGELIB.OLB /LIBRARY /NOSYSSHR
.ELSE                                           # OLDVAX_$(NOSHARE)
NOSHARE_OPTS = , SYS$LIBRARY:VAXCRTL.OLB /LIBRARY,\
 SYS$LIBRARY:DECCRTL.OLB /LIBRARY /INCLUDE = CMA$TIS,\
 SYS$LIBRARY:IMAGELIB.OLB /LIBRARY /NOSYSSHR
.ENDIF                                          # OLDVAX_$(NOSHARE)
.ENDIF                                      # DECC
.ENDIF                                  # __IA64__
.ENDIF                              # __ALPHA__
.ELSE                           # NOSHARE
NOSHARE_OPTS =
.ENDIF                          # NOSHARE

# LIST options.

.IFDEF LIST                     # LIST
.IFDEF DECC                         # DECC
CFLAGS_LIST = /list = $*.LIS /show = (all, nomessages)
.ELSE                               # DECC
CFLAGS_LIST = /list = $*.LIS /show = (all)
.ENDIF                              # DECC
LINKFLAGS_LIST = /map = $*.MAP /cross_reference /full
.ELSE                           # LIST
CFLAGS_LIST =
LINKFLAGS_LIST =
.ENDIF                          # LIST

# Common CFLAGS and LINKFLAGS.

CFLAGS = \
 $(CFLAGS_ARCH) $(CFLAGS_DBG) $(CFLAGS_INCL) $(CFLAGS_LIST) $(CCOPTS) \
 /object = $(MMS$TARGET)

LINKFLAGS = \
 $(LINKFLAGS_DBG) $(LINKFLAGS_LIST) $(LINKOPTS) \
 /executable = $(MMS$TARGET)

# Object library module=object lists.

#    Primary object library, [].

MODS_OBJS_LIB_UNZIP_N = \
 CRC32=[.$(DEST)]CRC32.OBJ \
 CRYPT=[.$(DEST)]CRYPT.OBJ \
 ENVARGS=[.$(DEST)]ENVARGS.OBJ \
 EXPLODE=[.$(DEST)]EXPLODE.OBJ \
 EXTRACT=[.$(DEST)]EXTRACT.OBJ \
 FILEIO=[.$(DEST)]FILEIO.OBJ \
 GLOBALS=[.$(DEST)]GLOBALS.OBJ \
 INFLATE=[.$(DEST)]INFLATE.OBJ \
 LIST=[.$(DEST)]LIST.OBJ \
 MATCH=[.$(DEST)]MATCH.OBJ \
 PROCESS=[.$(DEST)]PROCESS.OBJ \
 TTYIO=[.$(DEST)]TTYIO.OBJ \
 UBZ2ERR=[.$(DEST)]UBZ2ERR.OBJ \
 UNREDUCE=[.$(DEST)]UNREDUCE.OBJ \
 UNSHRINK=[.$(DEST)]UNSHRINK.OBJ \
 ZIPINFO=[.$(DEST)]ZIPINFO.OBJ

#    Primary object library, [.VMS].

MODS_OBJS_LIB_UNZIP_V = \
 VMS=[.$(DEST)]VMS.OBJ

MODS_OBJS_LIB_UNZIP = $(MODS_OBJS_LIB_UNZIP_N) $(MODS_OBJS_LIB_UNZIP_V)

#    CLI object library, [.VMS].

MODS_OBJS_LIB_UNZIPCLI_C_V = \
 CMDLINE=[.$(DEST)]CMDLINE.OBJ

MODS_OBJS_LIB_UNZIPCLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]UNZ_CLI.OBJ

MODS_OBJS_LIB_UNZIP_CLI = \
 $(MODS_OBJS_LIB_UNZIPCLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPCLI_CLD_V)

# SFX object library, [].

MODS_OBJS_LIB_UNZIPSFX_N = \
 CRC32$(GCC_)=[.$(DEST)]CRC32_.OBJ \
 CRYPT$(GCC_)=[.$(DEST)]CRYPT_.OBJ \
 EXTRACT$(GCC_)=[.$(DEST)]EXTRACT_.OBJ \
 FILEIO$(GCC_)=[.$(DEST)]FILEIO_.OBJ \
 GLOBALS$(GCC_)=[.$(DEST)]GLOBALS_.OBJ \
 INFLATE$(GCC_)=[.$(DEST)]INFLATE_.OBJ \
 MATCH$(GCC_)=[.$(DEST)]MATCH_.OBJ \
 PROCESS$(GCC_)=[.$(DEST)]PROCESS_.OBJ \
 TTYIO$(GCC_)=[.$(DEST)]TTYIO_.OBJ \
 UBZ2ERR$(GCC_)=[.$(DEST)]UBZ2ERR_.OBJ

# SFX object library, [.VMS].

MODS_OBJS_LIB_UNZIPSFX_V = \
 VMS$(GCC_)=[.$(DEST)]VMS_.OBJ

MODS_OBJS_LIB_UNZIPSFX = \
 $(MODS_OBJS_LIB_UNZIPSFX_N) \
 $(MODS_OBJS_LIB_UNZIPSFX_V)

# SFX object library, [.VMS] (no []).

MODS_OBJS_LIB_UNZIPSFX_CLI_C_V = \
 CMDLINE$(GCC_)=[.$(DEST)]CMDLINE_.OBJ

MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]UNZ_CLI.OBJ

MODS_OBJS_LIB_UNZIPSFX_CLI = \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V)

# Executables.

UNZIP = [.$(DEST)]UNZIP.EXE

UNZIP_CLI = [.$(DEST)]UNZIP_CLI.EXE

UNZIPSFX = [.$(DEST)]UNZIPSFX.EXE

UNZIPSFX_CLI = [.$(DEST)]UNZIPSFX_CLI.EXE

