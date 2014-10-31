# DESCRIP_SRC.MMS
#
#    UnZip 6.10 for VMS -- MMS (or MMK) Source Description File.
#
#    Last revised:  2014-10-20
#
#----------------------------------------------------------------------
# Copyright (c) 2004-2014 Info-ZIP.  All rights reserved.
#
# See the accompanying file LICENSE, version 2009-Jan-2 or later (the
# contents of which are also included in zip.h) for terms of use.  If,
# for some reason, all these files are missing, the Info-ZIP license
# may also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
#----------------------------------------------------------------------

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
DESTM = ALPHA
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
DECC = 1
DESTM = IA64
.ELSE                               # __IA64__
.IFDEF __VAX__                          # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                   # VAXC_OR_FORCE_VAXC
DESTM = VAXV
.ELSE                                       # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                     # GNUC
CC = GCC
DESTM = VAXG
.ELSE                                           # GNUC
DECC = 1
DESTM = VAX
.ENDIF                                          # GNUC
.ENDIF                                      # VAXC_OR_FORCE_VAXC
.ELSE                                   # __VAX__
DESTM = UNK
UNK_DEST = 1
.ENDIF                                  # __VAX__
.ENDIF                              # __IA64__
.ENDIF                          # __ALPHA__

# AES_WG support.  Default to disabled, but the .FIRST rule will detect
# the presence of the optional source kit, and advise accordingly.

# Translate NO_AES_WG to NOAES_WG.
.IFDEF NO_AES_WG                # NO_AES_WG
.IFDEF NOAES_WG                     # NOAES_WG
.ELSE                               # NOAES_WG
NOAES_WG = 1
.ENDIF                              # NOAES_WG
.ENDIF                          # NO_AES_WG

# Skip AES_WG check if AES_WG is explicitly disabled.
.IFDEF NOAES_WG                 # NOAES_WG
.IFDEF NOCHECK_AES_WG               # NOCHECK_AES_WG
.ELSE                               # NOCHECK_AES_WG
NOCHECK_AES_WG = 1
.ENDIF                              # NOCHECK_AES_WG
.ELSE                           # NOAES_WG
.IFDEF AES_WG                       # AES_WG
.ELSE                               # AES_WG
.IFDEF NOCHECK_AES_WG                   # NOCHECK_AES_WG
.ELSE                                   # NOCHECK_AES_WG
CHECK_AES_WG = 1
.ENDIF                                  # NOCHECK_AES_WG
.ENDIF                              # AES_WG
.ENDIF                          # NOAES_WG

# Large-file support.  Always no on VAX.  Assume yes elsewhere.  On
# Alpha, the .FIRST rule will detect incompatibility (before VMS V7.2).

# Targets which can bypass the AES_WG test and, on Alpha, the (slow)
# large-file test. 
# (Not "" or ALL.  Could add help- and message-related.)

TRGT_CLEAN = 1
TRGT_CLEAN_ALL = 1
TRGT_CLEAN_EXE = 1
TRGT_CLEAN_OLB = 1
TRGT_CLEAN_TEST = 1
TRGT_DASHV = 1
TRGT_HELP = 1
TRGT_HELP_TEXT = 1
TRGT_SLASHV = 1
TRGT_TEST = 1
TRGT_TEST_PPMD = 1

TRGT = TRGT_$(MMSTARGETS)

.IFDEF $(TRGT)                  # $(TRGT)
.IFDEF NOCHECK_AES_WG               # NOCHECK_AES_WG
.ELSE                               # NOCHECK_AES_WG
NOCHECK_AES_WG = 1
.ENDIF                              # NOCHECK_AES_WG
.IFDEF __ALPHA__                    # __ALPHA__
.IFDEF NOCHECK_LARGE                    # NOCHECK_LARGE
.ELSE                                   # NOCHECK_LARGE
NOCHECK_LARGE = 1
.ENDIF                                  # NOCHECK_LARGE
.ENDIF                              # __ALPHA__
.ENDIF                          # $(TRGT)

.IFDEF __VAX__                  # __VAX__
.IFDEF NOLARGE                      # NOLARGE
.ELSE                               # NOLARGE
NOLARGE = 1
.ENDIF                              # NOLARGE
.ENDIF                          # __VAX__

# Translate NO_LARGE to NOLARGE.
.IFDEF NO_LARGE                 # NO_LARGE
.IFDEF NOLARGE                      # NOLARGE
.ELSE                               # NOLARGE
NOLARGE = 1
.ENDIF                              # NOLARGE
.ENDIF                          # NO_LARGE

.IFDEF NOLARGE                  # NOLARGE
.ELSE                           # NOLARGE
.IFDEF LARGE                        # LARGE
.ELSE                               # LARGE
LARGE = 1
.IFDEF __ALPHA__                        # __ALPHA__
.IFDEF NOCHECK_LARGE                        # NOCHECK_LARGE
.ELSE                                       # NOCHECK_LARGE
CHECK_LARGE = 1
.ENDIF                                      # NOCHECK_LARGE
.ENDIF                                  # __ALPHA__
.ENDIF                              # LARGE
DESTL = L
.ENDIF                          # NOLARGE

DEST_STD = $(DESTM)$(DESTL)
.IFDEF PROD                     # PROD
DEST = $(PROD)
.ELSE                           # PROD
DEST = $(DEST_STD)
.ENDIF                          # PROD
SEEK_BZ = $(DESTM)

# Library module name suffix for XXX_.OBJ with GNU C.

.IFDEF GNUC                     # GNUC
GCC_ = _
GCC_C = _C
GCC_L = _L
.ELSE                           # GNUC
GCC_ =
GCC_C =
GCC_L =
.ENDIF                          # GNUC

# BZIP2 options.  (Default: IZ_BZIP2=[.bzip2].  To disable, define
# NO_IZ_BZIP2 or NOIZ_BZIP2.)

BZ2DIR_BIN = [.BZIP2.$(DEST_STD)]
BZ2DIR_BIN_DIR = [.BZIP2]$(DEST_STD).DIR
BZ2_OLB = LIBBZ2_NS.OLB
LIB_BZ2_LOCAL = $(BZ2DIR_BIN)$(BZ2_OLB)

# Translate NO_IZ_BZIP2 to NOIZ_BZIP2.
.IFDEF NO_IZ_BZIP2              # NO_IZ_BZIP2
.IFDEF NOIZ_BZIP2                   # NOIZ_BZIP2
.ELSE                               # NOIZ_BZIP2
NOIZ_BZIP2 = 1
.ENDIF                              # NOIZ_BZIP2
.ENDIF                          # NO_IZ_BZIP2

.IFDEF NOIZ_BZIP2               # NOIZ_BZIP2
.IFDEF IZ_BZIP2                     # IZ_BZIP2
IZ_BZIP2_ERR = 1
.ELSE                               # IZ_BZIP2
.ENDIF                              # IZ_BZIP2
.ELSE                           # NOIZ_BZIP2
.IFDEF IZ_BZIP2                     # IZ_BZIP2
.ELSE                               # IZ_BZIP2
IZ_BZIP2 = SYS$DISK:[.BZIP2]
LIB_BZ2_DEP = $(LIB_BZ2_LOCAL)
BUILD_BZIP2 = 1
.IFDEF LARGE                            # LARGE
IZ_BZIP2_MACROS = /MACRO = (LARGE=1)
.ENDIF                                  # LARGE
.ENDIF                              # IZ_BZIP2
.ENDIF                          # NOIZ_BZIP2

.IFDEF IZ_BZIP2                 # IZ_BZIP2
CDEFS_BZ = , BZIP2_SUPPORT
CFLAGS_INCL = /include = ([], [.VMS])
LIB_BZIP2_OPTS = LIB_BZIP2:$(BZ2_OLB) /library,
.ENDIF                          # IZ_BZIP2

# LZMA options.  (Default: LZMA.  To disable, define NO_LZMA or NOLZMA.)

# Translate NO_LZMA to NOLZMA.
.IFDEF NO_LZMA                  # NO_LZMA
.IFDEF NOLZMA                       # NOLZMA
.ELSE                               # NOLZMA
NOLZMA = 1
.ENDIF                              # NOLZMA
.ENDIF                          # NO_LZMA

.IFDEF NOLZMA                   # NOLZMA
.IFDEF LZMA                         # LZMA
LZMA_ERR = 1
.ELSE                               # LZMA
.ENDIF                              # LZMA
.ELSE                           # NOLZMA
.IFDEF LZMA                         # LZMA
.ELSE                               # LZMA
LZMA = 1
.ENDIF                              # LZMA
.ENDIF                          # NOLZMA

.IFDEF LZMA                     # LZMA
LZMA_PPMD = 1
.IFDEF __VAX__                      # __VAX__
CDEFS_LZMA = , LZMA_SUPPORT, _SZ_NO_INT_64
.ELSE                               # __VAX__
CDEFS_LZMA = , LZMA_SUPPORT
.ENDIF                              # __VAX__
.IFDEF CFLAGS_INCL                  # CFLAGS_INCL
.ELSE                               # CFLAGS_INCL
CFLAGS_INCL = /include = ([], [.VMS])
.ENDIF                              # CFLAGS_INCL
.ENDIF                          # LZMA

# PPMd options.  (Default: PPMD.  To disable, define NO_PPMD or NOPPMD.)

# Translate NO_PPMD to NOPPMD.
.IFDEF NO_PPMD                  # NO_PPMD
.IFDEF NOPPMD                       # NOPPMD
.ELSE                               # NOPPMD
NOPPMD = 1
.ENDIF                              # NOPPMD
.ENDIF                          # NO_PPMD

.IFDEF NOPPMD                   # NOPPMD
.IFDEF PPMD                         # PPMD
PPMD_ERR = 1
.ELSE                               # PPMD
.ENDIF                              # PPMD
.ELSE                           # NOPPMD
.IFDEF PPMD                         # PPMD
.ELSE                               # PPMD
PPMD = 1
.ENDIF                              # PPMD
.ENDIF                          # NOPPMD

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

# Complain about any problems (and die) if warranted.  Otherwise, show
# optional package directories being used, and the destination
# directory.  Make the destination directory, if necessary.

.FIRST
.IFDEF __MMK__                  # __MMK__
	@ write sys$output ""
.ENDIF                          # __MMK__
.IFDEF UNK_DEST                 # UNK_DEST
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
	@ write sys$output -
 "   Macro ""GNUC"" is incompatible with ""VAXC"" or ""FORCE_VAXC""."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                               # VAX_MULTI_CMPL
.IFDEF NON_VAX_CMPL                     # NON_VAX_CMPL
	@ write sys$output -
 "   Macros ""GNUC"", ""VAXC"", and ""FORCE_VAXC"" are valid only on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                   # NON_VAX_CMPL
.IFDEF LARGE_VAX                            # LARGE_VAX
	@ write sys$output -
 "   Macro ""LARGE"" is invalid on VAX."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                       # LARGE_VAX
.IFDEF IZ_BZIP2_ERR                             # IZ_BZIP2_ERR
	@ write sys$output -
 "   Macro ""IZ_BZIP2"" conflicts with ""NOIZ_BZIP2"" or ""NO_IZ_BZIP2""."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                           # IZ_BZIP2_ERR
.IFDEF LZMA_ERR                                     # LZMA_ERR
	@ write sys$output -
 "   Macro ""LZMA"" conflicts with ""NOLZMA"" or ""NO_LZMA""."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                               # LZMA_ERR
.IFDEF PPMD_ERR                                         # PPMD_ERR
	@ write sys$output -
 "   Macro ""PPMD"" conflicts with ""NOPPMD"" or ""NO_PPMD""."
	@ write sys$output ""
	I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                                   # PPMD_ERR
.IFDEF IZ_BZIP2                                             # IZ_BZIP2
.IFDEF BUILD_BZIP2                                              # BUILD_BZIP2
	@ no_bzlib_h = (f$search( "$(IZ_BZIP2)bzlib.h") .eqs. "")
	@ if (no_bzlib_h) then -
	   write sys$output "   Can not find header file $(IZ_BZIP2)bzlib.h"
	@ if (no_bzlib_h) then -
	   write sys$output ""
	@ if (no_bzlib_h) then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
	@ write sys$output "   BZIP2 dir: $(BZ2DIR_BIN)"
	@ define lib_bzip2 $(BZ2DIR_BIN)
 	@ if (f$search( "$(BZ2DIR_BIN_DIR)") .eqs. "") then -
	   create /directory $(BZ2DIR_BIN)
.ELSE                                                           # BUILD_BZIP2
	@ @[.VMS]FIND_BZIP2_LIB.COM $(IZ_BZIP2) $(SEEK_BZ) $(BZ2_OLB) lib_bzip2
	@ no_lib_bzip2 = (f$trnlnm( "lib_bzip2") .eqs. "")
	@ if (no_lib_bzip2) then -
	   write sys$output "   Can not find BZIP2 object library."
	@ if (no_lib_bzip2) then -
	   write sys$output ""
	@ if (no_lib_bzip2) then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
	@ write sys$output "   BZIP2 dir: ''f$trnlnm( "lib_bzip2")'"
.ENDIF                                                          # BUILD_BZIP2
	@ write sys$output ""
	@ define incl_bzip2 $(IZ_BZIP2)
.ENDIF                                                      # IZ_BZIP2
.IFDEF IZ_ZLIB                                              # IZ_ZLIB
	@ @[.VMS]FIND_BZIP2_LIB.COM $(IZ_ZLIB) $(SEEK_BZ) LIBZ.OLB lib_zlib
	@ no_lib_zlib = (f$trnlnm( "lib_zlib") .eqs. "")
	@ if (no_lib_zlib) then -
	   write sys$output "   Can not find ZLIB object library."
	@ if (no_lib_zlib) then -
	   write sys$output ""
	@ if (no_lib_zlib) then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
	@ write sys$output "   ZLIB dir:  ''f$trnlnm( "lib_zlib")'"
	@ write sys$output ""
	@ define incl_zlib $(IZ_ZLIB)
	@ @[.VMS]FIND_BZIP2_LIB.COM $(IZ_ZLIB) -
	   contrib.infback9 infback9.h incl_zlib_contrib_infback9
.ENDIF                                                      # IZ_ZLIB
	@ write sys$output "   Destination: [.$(DEST)]"
	@ write sys$output ""
	if (f$search( "$(DEST).DIR;1") .eqs. "") then -
	 create /directory [.$(DEST)]
.IFDEF CHECK_LARGE                                          # CHECK_LARGE
	@ write sys$output ""
	@ write sys$output "   Verifying large-file support..."
	@ @[.VMS]CHECK_LARGE.COM $(DEST) large_ok
	@ no_large = (f$trnlnm( "large_ok") .eqs. "")
	@ if (no_large) then -
	   write sys$output -
	    "   Large-file support not available with this VMS/CRTL version."
	@ if (no_large) then -
	   write sys$output "   Add ""/MACRO = NOLARGE=1""."
	@ if (no_large) then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
	@ write sys$output "   Large-file support ok."
	@ write sys$output ""
.ENDIF                                                      # CHECK_LARGE
.IFDEF NOCHECK_AES_WG                                       # NOCHECK_AES_WG
.ELSE                                                       # NOCHECK_AES_WG
.IFDEF AES_WG                                                   # AES_WG
	@ no_aes_wg_kit = (f$search( "[.aes_wg]aes.h") .eqs. "")
	@ if (no_aes_wg_kit) then -
	   write sys$output ""
	@ if (no_aes_wg_kit) then -
	   write sys$output -
	    "   Optional AES_WG source kit ([.aes_wg]) not found."
	@ if (no_aes_wg_kit) then -
	   write sys$output ""
	@ if (no_aes_wg_kit) then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                                                           # AES_WG
	@ aes_wg_kit = (f$search( "[.aes_wg]aes.h") .nes. "")
	@ if (aes_wg_kit) then -
	   write sys$output ""
	@ if (aes_wg_kit) then -
	   write sys$output -
	    "   Optional AES_WG source kit found, but support not enabled."
	@ if (aes_wg_kit) then -
	   write sys$output -
	    "   To enable AES_WG support, add ""/MACRO = AES_WG=1""."
	@ if (aes_wg_kit) then -
	   write sys$output ""
.ENDIF                                                          # AES_WG
.ENDIF                                                      # NOCHECK_AES_WG
.ENDIF                                                  # PPMD_ERR
.ENDIF                                              # LZMA_ERR
.ENDIF                                          # IZ_BZIP2_ERR
.ENDIF                                      # LARGE_VAX
.ENDIF                                  # NON_VAX_CMPL
.ENDIF                              # VAX_MULTI_CMPL
.ENDIF                          # UNK_DEST

# AES_WG options.

.IFDEF AES_WG                   # AES_WG
CDEFS_AES = , CRYPT_AES_WG
.ENDIF                          # AES_WG

# PPMd options.

.IFDEF PPMD                     # PPMD
.IFDEF LZMA                         # LZMA
.IFDEF __VAX__                          # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                   # VAXC_OR_FORCE_VAXC
CDEFS_PPMD = , PPMD_SUPPORT, NO_SIGNED_CHAR
.ELSE                                       # VAXC_OR_FORCE_VAXC
CDEFS_PPMD = , PPMD_SUPPORT
.ENDIF                                      # VAXC_OR_FORCE_VAXC
.ELSE                                   # __VAX__
CDEFS_PPMD = , PPMD_SUPPORT
.ENDIF                                  # __VAX__
.ELSE                               # LZMA
LZMA_PPMD = 1
.IFDEF __VAX__                          # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                   # VAXC_OR_FORCE_VAXC
CDEFS_PPMD = , PPMD_SUPPORT, NO_SIGNED_CHAR, _SZ_NO_INT_64
.ELSE                                       # VAXC_OR_FORCE_VAXC
CDEFS_PPMD = , PPMD_SUPPORT, _SZ_NO_INT_64
.ENDIF                                      # VAXC_OR_FORCE_VAXC
.ELSE                                   # __VAX__
CDEFS_PPMD = , PPMD_SUPPORT
.ENDIF                                  # __VAX__
.ENDIF                              # LZMA
.IFDEF CFLAGS_INCL                  # CFLAGS_INCL
.ELSE                               # CFLAGS_INCL
CFLAGS_INCL = /include = ([], [.VMS])
.ENDIF                              # CFLAGS_INCL
.ENDIF                          # PPMD

# ZLIB options.

.IFDEF IZ_ZLIB                  # IZ_ZLIB
CDEFS_ZL = , USE_ZLIB
.IFDEF CFLAGS_INCL                  # CFLAGS_INCL
.ELSE                               # CFLAGS_INCL
CFLAGS_INCL = /include = ([], [.VMS])
.ENDIF                              # CFLAGS_INCL
LIB_ZLIB_OPTS = LIB_ZLIB:LIBZ.OLB /library,
.ENDIF                          # IZ_ZLIB

.IFDEF CFLAGS_INCL              # CFLAGS_INCL
.ELSE                           # CFLAGS_INCL
CFLAGS_INCL = /include = []
.ENDIF                          # CFLAGS_INCL


# DBG, TRC options.

.IFDEF DBG                      # DBG
CFLAGS_DBG = /debug /nooptimize
LINKFLAGS_DBG = /debug /traceback
.ELSE                           # DBG
CFLAGS_DBG =
.IFDEF TRC                          # TRC
LINKFLAGS_DBG = /traceback
.ELSE                               # TRC
LINKFLAGS_DBG = /notraceback
.ENDIF                              # TRC
.ENDIF                          # DBG

# Large-file options.

.IFDEF LARGE                    # LARGE
CDEFS_LARGE = , LARGE_FILE_SUPPORT
.ENDIF                          # LARGE

# C compiler defines.

.IFDEF LOCAL_UNZIP
C_LOCAL_UNZIP = , $(LOCAL_UNZIP)
.ENDIF

CDEFS = VMS $(CDEFS_AES) $(CDEFS_BZ) $(CDEFS_LARGE) \
 $(CDEFS_LZMA) $(CDEFS_PPMD) $(CDEFS_ZL) $(C_LOCAL_UNZIP)

CDEFS_UNX = /define = ($(CDEFS))

CDEFS_CLI = /define = ($(CDEFS), VMSCLI)

CDEFS_SFX = /define = ($(CDEFS), SFX)

CDEFS_SFX_CLI = /define = ($(CDEFS), SFX, VMSCLI)

CDEFS_LIBUNZIP = /define = ($(CDEFS), DLL)

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

# LINK (sysshr) library options.
# Omit shareable image options file for NOSYSSHR.

.IFDEF VAXC_OR_FORCE_VAXC               # VAXC_OR_FORCE_VAXC
.IFDEF NOSYSSHR                             # NOSYSSHR
OPT_FILE =
LFLAGS_ARCH =
.ELSE                                       # NOSYSSHR
OPT_FILE = [.$(DEST)]VAXCSHR.OPT
LFLAGS_ARCH = $(OPT_FILE) /options,
.ENDIF                                      # NOSYSSHR
.ELSE                                   # VAXC_OR_FORCE_VAXC
.IFDEF GNUC                                 # GNUC
LFLAGS_GNU = GNU_CC:[000000]GCCLIB.OLB /LIBRARY
.IFDEF NOSYSSHR                                 # NOSYSSHR
OPT_FILE =
LFLAGS_ARCH = $(LFLAGS_GNU),
.ELSE                                           # NOSYSSHR
OPT_FILE = [.$(DEST)]VAXCSHR.OPT
LFLAGS_ARCH = $(LFLAGS_GNU), SYS$DISK:$(OPT_FILE) /options,
.ENDIF                                          # NOSYSSHR
.ELSE                                       # GNUC
OPT_FILE =
LFLAGS_ARCH =
.ENDIF                                      # GNUC
.ENDIF                                  # VAXC_OR_FORCE_VAXC

# LINK NOSYSSHR options.

.IFDEF NOSYSSHR                 # NOSYSSHR
.IFDEF __ALPHA__                    # __ALPHA__
NOSYSSHR_OPTS = , SYS$LIBRARY:STARLET.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                               # __ALPHA__
.IFDEF __IA64__                         # __IA64__
NOSYSSHR_OPTS = , SYS$LIBRARY:STARLET.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                                   # __IA64__
OLDVAX_OLDVAX = 1
.IFDEF DECC                                 # DECC
.IFDEF OLDVAX_$(NOSYSSHR)                       # OLDVAX_$(NOSYSSHR)
NOSYSSHR_OPTS = , SYS$LIBRARY:DECCRTL.OLB /LIBRARY\
 /INCLUDE = CMA$TIS /NOSYSSHR
.ELSE                                           # OLDVAX_$(NOSYSSHR)
NOSYSSHR_OPTS = , SYS$LIBRARY:DECCRTL.OLB /LIBRARY\
 /INCLUDE = (CMA$TIS, CMA$TIS_VEC) /NOSYSSHR
.ENDIF                                          # OLDVAX_$(NOSYSSHR)
.ELSE                                       # DECC
.IFDEF OLDVAX_$(NOSYSSHR)                       # OLDVAX_$(NOSYSSHR)
NOSYSSHR_OPTS = , SYS$LIBRARY:VAXCRTL.OLB /LIBRARY,\
 SYS$LIBRARY:IMAGELIB.OLB /LIBRARY /NOSYSSHR
.ELSE                                           # OLDVAX_$(NOSYSSHR)
NOSYSSHR_OPTS = , SYS$LIBRARY:VAXCRTL.OLB /LIBRARY,\
 SYS$LIBRARY:DECCRTL.OLB /LIBRARY /INCLUDE = CMA$TIS,\
 SYS$LIBRARY:IMAGELIB.OLB /LIBRARY /NOSYSSHR
.ENDIF                                          # OLDVAX_$(NOSYSSHR)
.ENDIF                                      # DECC
.ENDIF                                  # __IA64__
.ENDIF                              # __ALPHA__
.ELSE                           # NOSYSSHR
NOSYSSHR_OPTS =
.ENDIF                          # NOSYSSHR

# LIST options.

.IFDEF LIST                     # LIST
.IFDEF DECC                         # DECC
CFLAGS_LIST = /list = $*.LIS /show = (all, nomessages)
.ELSE                               # DECC
CFLAGS_LIST = /list = $*.LIS /show = (all)
.ENDIF                              # DECC
LINKFLAGS_LIST = /map = $*.MAP /cross_reference /full
.ELSE                           # LIST
CFLAGS_LIST = /nolist
LINKFLAGS_LIST = /nomap
.ENDIF                          # LIST

# Common CFLAGS and LINKFLAGS.

CFLAGS = \
 $(CFLAGS_ARCH) $(CFLAGS_DBG) $(CFLAGS_INCL) $(CFLAGS_LIST) $(CCOPTS) \
 /object = $(MMS$TARGET)

CFLAGS_DEP = $(CFLAGS_ARCH) $(CFLAGS_INCL) $(CCOPTS)

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

#    Primary object library, [.AES_WG].

.IFDEF AES_WG                   # AES_WG
MODS_OBJS_LIB_UNZIP_AES = \
 AESCRYPT=[.$(DEST)]AESCRYPT.OBJ \
 AESKEY=[.$(DEST)]AESKEY.OBJ \
 AESTAB=[.$(DEST)]AESTAB.OBJ \
 FILEENC=[.$(DEST)]FILEENC.OBJ \
 HMAC=[.$(DEST)]HMAC.OBJ \
 PRNG=[.$(DEST)]PRNG.OBJ \
 PWD2KEY=[.$(DEST)]PWD2KEY.OBJ \
 SHA1=[.$(DEST)]SHA1.OBJ
.ENDIF                          # AES_WG

#    Primary object library, [.SZIP], LZMA.

.IFDEF LZMA                     # LZMA
MODS_OBJS_LIB_UNZIP_LZMA = \
 LZFIND=[.$(DEST)]LZFIND.OBJ \
 LZMADEC=[.$(DEST)]LZMADEC.OBJ
.ENDIF                          # LZMA

#    Primary object library, [.SZIP], PPMd.

.IFDEF PPMD                     # PPMD
MODS_OBJS_LIB_UNZIP_PPMD = \
 PPMD8=[.$(DEST)]PPMD8.OBJ \
 PPMD8DEC=[.$(DEST)]PPMD8DEC.OBJ
.ENDIF                          # PPMD

MODS_OBJS_LIB_UNZIP = $(MODS_OBJS_LIB_UNZIP_N) $(MODS_OBJS_LIB_UNZIP_V) \
 $(MODS_OBJS_LIB_UNZIP_AES) $(MODS_OBJS_LIB_UNZIP_LZMA) \
 $(MODS_OBJS_LIB_UNZIP_PPMD)

#    CLI object library, [].

MODS_OBJS_LIB_UNZIPCLI_C_N = \
 ZIPINFO$(GCC_C)=[.$(DEST)]ZIPINFO_C.OBJ

#    CLI object library, [.VMS].

MODS_OBJS_LIB_UNZIPCLI_C_V = \
 CMDLINE=[.$(DEST)]CMDLINE.OBJ

MODS_OBJS_LIB_UNZIPCLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]UNZ_CLI.OBJ

MODS_OBJS_LIB_UNZIP_CLI = \
 $(MODS_OBJS_LIB_UNZIPCLI_C_N) \
 $(MODS_OBJS_LIB_UNZIPCLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPCLI_CLD_V)

#    SFX object library, [].

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

#    SFX object library, [.VMS].

MODS_OBJS_LIB_UNZIPSFX_V = \
 VMS$(GCC_)=[.$(DEST)]VMS_.OBJ

#    SFX object library, [.AES_WG].

.IFDEF AES_WG                   # AES_WG
MODS_OBJS_LIB_UNZIPSFX_AES = \
 AESCRYPT=[.$(DEST)]AESCRYPT.OBJ \
 AESKEY=[.$(DEST)]AESKEY.OBJ \
 AESTAB=[.$(DEST)]AESTAB.OBJ \
 FILEENC=[.$(DEST)]FILEENC.OBJ \
 HMAC=[.$(DEST)]HMAC.OBJ \
 PRNG=[.$(DEST)]PRNG.OBJ \
 PWD2KEY=[.$(DEST)]PWD2KEY.OBJ \
 SHA1=[.$(DEST)]SHA1.OBJ
.ENDIF                          # AES_WG

#    Primary object library, [.SZIP], LZMA.

.IFDEF LZMA                     # LZMA
MODS_OBJS_LIB_UNZIPSFX_LZMA = \
 LZFIND=[.$(DEST)]LZFIND.OBJ \
 LZMADEC=[.$(DEST)]LZMADEC.OBJ
.ENDIF                          # LZMA

#    Primary object library, [.SZIP], PPMd.

.IFDEF PPMD                     # PPMD
MODS_OBJS_LIB_UNZIPSFX_PPMD = \
 PPMD8=[.$(DEST)]PPMD8.OBJ \
 PPMD8DEC=[.$(DEST)]PPMD8DEC.OBJ
.ENDIF                          # PPMD

MODS_OBJS_LIB_UNZIPSFX = \
 $(MODS_OBJS_LIB_UNZIPSFX_N) \
 $(MODS_OBJS_LIB_UNZIPSFX_V) \
 $(MODS_OBJS_LIB_UNZIPSFX_AES) \
 $(MODS_OBJS_LIB_UNZIPSFX_LZMA) \
 $(MODS_OBJS_LIB_UNZIPSFX_PPMD)

# SFX object library, [.VMS] (no []).

MODS_OBJS_LIB_UNZIPSFX_CLI_C_V = \
 CMDLINE$(GCC_)=[.$(DEST)]CMDLINE_.OBJ

MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V = \
 VMS_UNZIP_CLD=[.$(DEST)]UNZ_CLI.OBJ

MODS_OBJS_LIB_UNZIPSFX_CLI = \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_C) \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_C_V) \
 $(MODS_OBJS_LIB_UNZIPSFX_CLI_CLD_V)

# LIBUNZIP object library.

.IFDEF LIBUNZIP                 # LIBUNZIP

#    Modules sensitive to DLL/REENTRANT.

MODS_OBJS_LIB_LIBUNZIP_NL = \
 API$(GCC_L)=[.$(DEST)]API_L.OBJ \
 APIHELP$(GCC_L)=[.$(DEST)]APIHELP_L.OBJ \
 CRYPT$(GCC_L)=[.$(DEST)]CRYPT_L.OBJ \
 EXPLODE$(GCC_L)=[.$(DEST)]EXPLODE_L.OBJ \
 EXTRACT$(GCC_L)=[.$(DEST)]EXTRACT_L.OBJ \
 FILEIO$(GCC_L)=[.$(DEST)]FILEIO_L.OBJ \
 GLOBALS$(GCC_L)=[.$(DEST)]GLOBALS_L.OBJ \
 INFLATE$(GCC_L)=[.$(DEST)]INFLATE_L.OBJ \
 LIST$(GCC_L)=[.$(DEST)]LIST_L.OBJ \
 PROCESS$(GCC_L)=[.$(DEST)]PROCESS_L.OBJ \
 TTYIO$(GCC_L)=[.$(DEST)]TTYIO_L.OBJ \
 UBZ2ERR$(GCC_L)=[.$(DEST)]UBZ2ERR_L.OBJ \
 UNSHRINK$(GCC_L)=[.$(DEST)]UNSHRINK_L.OBJ \
 UNZIP$(GCC_L)=[.$(DEST)]UNZIP_L.OBJ \
 ZIPINFO$(GCC_L)=[.$(DEST)]ZIPINFO_L.OBJ

MODS_OBJS_LIB_LIBUNZIP_V = \
 VMS$(GCC_)=[.$(DEST)]VMS_L.OBJ

#    Modules insensitive to DLL/REENTRANT.

MODS_OBJS_LIB_LIBUNZIP_N = \
 CRC32=[.$(DEST)]CRC32.OBJ \
 ENVARGS=[.$(DEST)]ENVARGS.OBJ \
 MATCH=[.$(DEST)]MATCH.OBJ \
 UNREDUCE=[.$(DEST)]UNREDUCE.OBJ

MODS_OBJS_LIB_LIBUNZIP_A = $(MODS_OBJS_LIB_UNZIP_AES)
MODS_OBJS_LIB_LIBUNZIP_L = $(MODS_OBJS_LIB_UNZIP_LZMA)
MODS_OBJS_LIB_LIBUNZIP_P = $(MODS_OBJS_LIB_UNZIP_PPMD)

MODS_OBJS_LIB_LIBUNZIP = \
 $(MODS_OBJS_LIB_LIBUNZIP_NL) \
 $(MODS_OBJS_LIB_LIBUNZIP_V) \
 $(MODS_OBJS_LIB_LIBUNZIP_N) \
 $(MODS_OBJS_LIB_LIBUNZIP_A) \
 $(MODS_OBJS_LIB_LIBUNZIP_L) \
 $(MODS_OBJS_LIB_LIBUNZIP_P)
.ENDIF                          # LIBUNZIP

# Executables.

UNZIP = [.$(DEST)]UNZIP.EXE

UNZIP_CLI = [.$(DEST)]UNZIP_CLI.EXE

UNZIPSFX = [.$(DEST)]UNZIPSFX.EXE

UNZIPSFX_CLI = [.$(DEST)]UNZIPSFX_CLI.EXE

