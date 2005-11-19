#                                               17 November 2005.  SMS.
#
#    Zip 3.0 for VMS - MMS (or MMK) Source Description File.
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
.IFDEF LARGE                        # LARGE
DEST = ALPHAL
.ELSE                               # LARGE
DEST = ALPHA
.ENDIF                              # LARGE
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
DECC = 1
.IFDEF LARGE                            # LARGE
DEST = IA64L
.ELSE                                   # LARGE
DEST = IA64
.ENDIF                                  # LARGE
.ELSE                               # __IA64__
.IFDEF __VAX__                          # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                   # VAXC_OR_FORCE_VAXC
DEST = VAXV
.ELSE                                       # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                     # GNUC
CC = GCC
DEST = VAXG
.ELSE                                           # GNUC
DECC = 1
DEST = VAX
.ENDIF                                          # GNUC
.ENDIF                                      # VAXC_OR_FORCE_VAXC
.ELSE                                   # __VAX__
DEST = UNK
UNK_DEST = 1
.ENDIF                                  # __VAX__
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
.FIRST
	@ write sys$output "   Destination: [.$(DEST)]"
	@ write sys$output ""
	@ define incl_bzip2 $(IZ_BZIP2)
	@ @[.vms]find_bzip2_lib.com $(IZ_BZIP2) $(DEST) lib_bzip2
	@ if (f$trnlnm( "lib_bzip2") .nes. "") then -
	   write sys$output "   BZIP2 dir: ''f$trnlnm( "lib_bzip2")'"
	@ if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	   write sys$output "   Can not find BZIP2 object library."
	@ write sys$output ""
	@ if (f$trnlnm( "lib_bzip2") .eqs. "") then -
	 I_WILL_DIE_NOW.  /$$$$INVALID$$$$
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
LIB_BZIP2_OPTS = lib_bzip2:libbz2.olb /library,
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
OPT_FILE = [.$(DEST)]vaxcshr.opt
LFLAGS_ARCH = $(OPT_FILE) /options,
.ENDIF                                      # NOSHARE
.ELSE                                   # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                 # GNUC
LFLAGS_GNU = GNU_CC:[000000]GCCLIB.OLB /LIBRARY
.IFDEF NOSHARE                                  # NOSHARE
OPT_FILE =
LFLAGS_ARCH = $(LFLAGS_GNU),
.ELSE                                           # NOSHARE
OPT_FILE = [.$(DEST)]vaxcshr.opt
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
 crc32=[.$(DEST)]crc32.obj \
 crctab=[.$(DEST)]crctab.obj \
 crypt=[.$(DEST)]crypt.obj \
 envargs=[.$(DEST)]envargs.obj \
 explode=[.$(DEST)]explode.obj \
 extract=[.$(DEST)]extract.obj \
 fileio=[.$(DEST)]fileio.obj \
 globals=[.$(DEST)]globals.obj \
 inflate=[.$(DEST)]inflate.obj \
 list=[.$(DEST)]list.obj \
 match=[.$(DEST)]match.obj \
 process=[.$(DEST)]process.obj \
 ttyio=[.$(DEST)]ttyio.obj \
 unreduce=[.$(DEST)]unreduce.obj \
 unshrink=[.$(DEST)]unshrink.obj \
 zipinfo=[.$(DEST)]zipinfo.obj

#    Primary object library, [.vms].

MODS_OBJS_LIB_UNZIP_V = \
 vms=[.$(DEST)]vms.obj

MODS_OBJS_LIB_UNZIP = $(MODS_OBJS_LIB_UNZIP_N) $(MODS_OBJS_LIB_UNZIP_V)

#    CLI object library, [.vms].

MODS_OBJS_LIB_UNZIPCLI_C_V = \
 CMDLINE=[.$(DEST)]cmdline.obj

MODS_OBJS_LIB_UNZIPCLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]unz_cli.obj

MODS_OBJS_LIB_UNZIP_CLI = \
 $(MODS_OBJS_LIB_UNZIPCLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPCLI_CLD_V)

# SFX object library, [].

MODS_OBJS_LIB_UNZIPSFX_N = \
 crc32$(GCC_)=[.$(DEST)]crc32_.obj \
 crctab$(GCC_)=[.$(DEST)]crctab_.obj \
 crypt$(GCC_)=[.$(DEST)]crypt_.obj \
 extract$(GCC_)=[.$(DEST)]extract_.obj \
 fileio$(GCC_)=[.$(DEST)]fileio_.obj \
 globals$(GCC_)=[.$(DEST)]globals_.obj \
 inflate$(GCC_)=[.$(DEST)]inflate_.obj \
 match$(GCC_)=[.$(DEST)]match_.obj \
 process$(GCC_)=[.$(DEST)]process_.obj \
 ttyio$(GCC_)=[.$(DEST)]ttyio_.obj

# SFX object library, [.vms].

MODS_OBJS_LIB_UNZIPSFX_V = \
 vms$(GCC_)=[.$(DEST)]vms_.obj

MODS_OBJS_LIB_UNZIPSFX = \
 $(MODS_OBJS_LIB_UNZIPSFX_N) \
 $(MODS_OBJS_LIB_UNZIPSFX_V)

# SFX object library, [.vms] (no []).

MODS_OBJS_LIB_UNZIPSFX_CLI_C_V = \
 CMDLINE$(GCC_)=[.$(DEST)]cmdline_.obj

MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]unz_cli.obj

MODS_OBJS_LIB_UNZIPSFX_CLI = \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V)

# Executables.

UNZIP = [.$(DEST)]unzip.exe

UNZIP_CLI = [.$(DEST)]unzip_cli.exe

UNZIPSFX = [.$(DEST)]unzipsfx.exe

UNZIPSFX_CLI = [.$(DEST)]unzipsfx_cli.exe

