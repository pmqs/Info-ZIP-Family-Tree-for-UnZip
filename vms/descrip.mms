# DESCRIP.MMS
#
#    UnZip 6.1 for VMS -- MMS (or MMK) Description File.
#
#    Last revised:  2018-12-04
#
#----------------------------------------------------------------------
# Copyright (c) 2001-2018 Info-ZIP.  All rights reserved.
#
# See the accompanying file LICENSE, version 2009-Jan-2 or later (the
# contents of which are also included in zip.h) for terms of use.  If,
# for some reason, all these files are missing, the Info-ZIP license
# may also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
#----------------------------------------------------------------------
#
# Usage:
#
#    MMS /DESCRIP = [.VMS]DESCRIP.MMS [/MACRO = (<see_below>)] [target]
#
# Note that this description file must be used from the main
# distribution directory, not from the [.VMS] subdirectory.
#
# Optional macros:
#
#    AES_WG=1       Enable/disable AES (WinZip/Gladman) encryption
#    NOAES_WG=1     support.  Specify either AES_WG=1 or NOAES_WG=1 to
#                   skip the [.aes_wg] source directory test.
#                   By default, the SFX programs are built without
#                   AES_WG support.  Add "CRYPT_AES_WG_SFX=1" to the
#                   LOCAL_UNZIP C macros to enable it.  (See
#                   LOCAL_UNZIP, below.)
#
#    CCOPTS=xxx     Compile with CC options xxx.  For example:
#                   CCOPTS=/ARCH=HOST
#
#    DBG=1          Compile /DEBUG /NOOPTIMIZE.  Link /DEBUG /TRACEBACK.
#    TRC=1          Default is /NOTRACEBACK, but TRC=1 enables link with
#                   /TRACEBACK without compiling for debug.
#
#    IZ_BZIP2=dev:[dir]  Direct/disable optional bzip2 support.  By
#    NOIZ_BZIP2=1        default, bzip2 support is enabled, and uses the
#                   bzip2 source kit supplied in the [.bzip2] directory.
#                   Specify NOIZ_BZIP2=1 to disable bzip2 support.
#                   Specify IZ_BZIP2 with a value ("dev:[dir]", or a
#                   suitable logical name) to use the bzip2 header file
#                   and object library found there.  The bzip2 object
#                   library (LIBBZ2_NS.OLB) is expected to be in a
#                   simple "[.dest]" directory under that one
#                   ("dev:[dir.ALPHAL]", for example), or in that
#                   directory itself.)  By default, the SFX programs are
#                   built without bzip2 support.  Add "BZIP2_SFX=1" to
#                   the LOCAL_UNZIP C macros to enable it.  (See
#                   LOCAL_UNZIP, below.)
#
#    IZ_ZLIB=dev:[dir]  Use ZLIB compression library instead of internal
#                       Deflate compression routines.  The value of the
#                   MMS macro IZ_ZLIB ("dev:[dir]", or a suitable
#                   logical name) tells where to find "zlib.h".  The
#                   ZLIB object library (LIBZ.OLB) is expected to be in
#                   a "[.dest]" directory under that one
#                   ("dev:[dir.ALPHAL]", for example), or in that
#                   directory itself.
#
#    LARGE=1        Enable/disable large-file (>2GB) support.  Always
#    NOLARGE=1      disabled on VAX.  Enabled by default on Alpha and
#                   IA64.  On Alpha, by default, large-file support is
#                   tested, and the build will fail if that test fails.
#                   Specify NOLARGE=1 explicitly to disable support (and
#                   to skip the test on Alpha).
#
#    LIBUNZIP=1     Build LIBIZUNZIP.OLB as a callable UnZip library.
#
#    LINKOPTS=xxx   Link with LINK options xxx.  For example:
#                   LINKOPTS=/NOINFO
#
#    LIST=1         Compile with /LIST /SHOW = (ALL, NOMESSAGES).
#                   Link with /MAP /CROSS_REFERENCE /FULL.
#
#    "LOCAL_UNZIP= c_macro_1=value1 [, c_macro_2=value2 [...]]"
#                   Compile with these additional C macros defined.  For
#                   example:
#                   "LOCAL_UNZIP=NO_EXCEPT_SIGNALS=1, NO_SYMLINKS=1"
#
#    NOLZMA=1       Disable LZMA compression support, which is enabled
#                   by default for the normal UnZip programs.  By
#                   default, the SFX programs are built without LZMA
#                   support.  Add "LZMA_SFX=1" to the LOCAL_UNZIP C
#                   macros to enable it.  (See LOCAL_UNZIP, above.)
#
#    NOPPMD=1       Disable PPMd compression support, which is enabled
#                   by default for the normal UnZip programs.  By
#                   default, the SFX programs are built without PPMd
#                   support.  Add "PPMD_SFX=1" to the LOCAL_UNZIP C
#                   macros to enable it.  (See LOCAL_UNZIP, above.)
#
#    NOSYSSHR=1     Link /NOSYSSHR (not using shareable images).
#    NOSYSSHR=OLDVAX  Link /NOSYSSHR on VAX for:
#                      DEC C with VMS before V7.3.
#                      VAX C without DEC C RTL (DEC C not installed).
#
#    PROD=subdir    Use [.subdir] as the destination for
#                   architecture-specific product files (.EXE, .OBJ,
#                   .OLB, and so on).  The default is a name
#                   automatically generated using rules defined in
#                   [.VMS]DESCRIP_SRC.MMS.  Note that using this option
#                   carelessly can confound the CLEAN* targets.
#
# VAX-specific optional macros:
#
#    VAXC=1         Use the VAX C compiler, assuming "CC" runs it.
#                   (That is, DEC C is not installed, or else DEC C is
#                   installed, but VAX C is the default.)
#
#    FORCE_VAXC=1   Use the VAX C compiler, assuming "CC /VAXC" runs it.
#                   (That is, DEC C is installed, and it is the
#                   default, but you want VAX C anyway, you fool.)
#
#    GNUC=1         Use the GNU C compiler.  (Seriously under-tested.)
#
#
# The default target, ALL, builds the selected product executables and
# help files.
#
# Other targets:
#
#    CLEAN      deletes architecture-specific files, but leaves any
#               individual source dependency files and the help files.
#
#    CLEAN_ALL  deletes all generated files, except the main (collected)
#               source dependency file.
#
#    CLEAN_EXE  deletes only the architecture-specific executables.
#               Handy if all you wish to do is re-link the executables.
#
#    CLEAN_OLB  deletes only the architecture-specific object libraries.
#
#    CLEAN_TEST deletes all test directories.
#
#    DASHV      generates an "unzip -v" report.
#
#    HELP       generates HELP library source files (.HLP).
#
#    HELP_TEXT  generates HELP output text files (.HTX).
#
#    SLASHV     generates an "unzip_cli /verbose" report.
#
#    TEST       runs a brief test.
#
#    TEST_PPMD  runs a brief PPMd test.
#
# Example commands:
#
# To build the large-file product (except on VAX) with all the available
# optional compression methods using the DEC/Compaq/HP C compiler (Note:
# DESCRIP.MMS is the default description file name.):
#
#    MMS /DESCRIP = [.VMS]
#
# To get small-file executables (on a non-VAX system):
#
#    MMS /DESCRIP = [.VMS] /MACRO = (NOLARGE=1)
#
# To delete the architecture-specific generated files for this system
# type:
#
#    MMS /DESCRIP = [.VMS] CLEAN
#    MMS /DESCRIP = [.VMS] /MACRO = (NOLARGE=1) CLEAN   ! Non-VAX,
#                                                       ! small-file.
#
# To build a complete product for debug with compiler listings and link
# maps:
#
#    MMS /DESCRIP = [.VMS] CLEAN
#    MMS /DESCRIP = [.VMS] /MACRO = (DBG=1, LIST=1)
#
#
#    Note that option macros like NOLARGE or PROD affect the destination
#    directory for various product files, including executables, and
#    various clean and test targets (CLEAN, DASHV, TEST, and so on) need
#    to use the proper destination directory.  Thus, if NOLARGE or PROD
#    is specified for a build, then the same macro must be specified for
#    the various clean and test targets, too.
#
#    Note that on a Unix system, LOCAL_UNZIP contains compiler
#    options, such as "-g" or "-DCRYPT_AES_WG_SFX", but on a VMS
#    system, LOCAL_UNZIP contains only C macros, such as
#    "CRYPT_AES_WG_SFX", and CCOPTS is used for any other kinds of
#    compiler options, such as "/ARCHITECTURE".  Unix compilers accept
#    multiple "-D" options, but VMS compilers consider only the last
#    /DEFINE qualifier, so the C macros must be handled differently
#    from other compiler options on VMS.  Thus, when using the generic
#    installation instructions as a guide for controlling various
#    optional features, some adjustment may be needed to adapt them to
#    a VMS build environment.
#
########################################################################

# Explicit suffix list.  (Added .HTX before .HLB.)

.SUFFIXES
.SUFFIXES : .EXE .OLB .HTX .HLB .OBJ .C .CLD .MSG .HLP .RNH

# Include primary product description file.

INCL_DESCRIP_SRC = 1
.INCLUDE [.VMS]DESCRIP_SRC.MMS

# Object library names.

LIB_UNZIP_NAME = UNZIP.OLB
LIB_UNZIP_CLI_NAME = UNZIPCLI.OLB
LIB_UNZIPSFX_NAME = UNZIPSFX.OLB
LIB_UNZIPSFX_CLI_NAME = UNZSFXCLI.OLB
.IFDEF LIBUNZIP                 # LIBUNZIP
LIB_LIBUNZIP_NAME = LIBIZUNZIP.OLB
.ENDIF                          # LIBUNZIP

LIB_UNZIP = SYS$DISK:[.$(DEST)]$(LIB_UNZIP_NAME)
LIB_UNZIP_CLI = SYS$DISK:[.$(DEST)]$(LIB_UNZIP_CLI_NAME)
LIB_UNZIPSFX = SYS$DISK:[.$(DEST)]$(LIB_UNZIPSFX_NAME)
LIB_UNZIPSFX_CLI = SYS$DISK:[.$(DEST)]$(LIB_UNZIPSFX_CLI_NAME)
.IFDEF LIBUNZIP                 # LIBUNZIP
LIB_LIBUNZIP = SYS$DISK:[.$(DEST)]$(LIB_LIBUNZIP_NAME)
.ENDIF                          # LIBUNZIP

# Help library source file names.

UNZIP_HELP = UNZIP.HLP UNZIP_CLI.HLP

# Help output text file names.

UNZIP_HELP_TEXT = UNZIP.HTX UNZIP_CLI.HTX

# Message file names.

UNZIP_MSG_MSG = [.VMS]UNZIP_MSG.MSG
UNZIP_MSG_EXE = [.$(DEST)]UNZIP_MSG.EXE
UNZIP_MSG_OBJ = [.$(DEST)]UNZIP_MSG.OBJ

# Library link options file.

.IFDEF LIBUNZIP                 # LIBUNZIP
LIBUNZIP_OPT = [.$(DEST)]LIB_IZUNZIP.OPT
.ENDIF                          # LIBUNZIP

# TARGETS.

# Default target, ALL.  Build All executables,
# and help files.

ALL : $(UNZIP) $(UNZIP_CLI) $(UNZIPSFX) $(UNZIPSFX_CLI) $(UNZIP_HELP) \
      $(UNZIP_MSG_EXE) $(LIB_LIBUNZIP) $(LIBUNZIP_OPT)
	@ write sys$output "Done."

# CLEAN* targets.  These also similarly clean a local bzip2 directory.

# CLEAN target.  Delete:
#    The [.$(DEST)] directory and everything in it.

CLEAN :
	if (f$search( "[.$(DEST)]*.*") .nes. "") then -
	 delete /noconfirm [.$(DEST)]*.*;*
	if (f$search( "$(DEST).DIR") .nes. "") then -
	 set protection = w:d $(DEST).DIR;*
	if (f$search( "$(DEST).DIR") .nes. "") then -
	 delete /noconfirm $(DEST).DIR;*
.IFDEF BUILD_BZIP2              # BUILD_BZIP2
	@ write sys$output ""
	@ write sys$output "Cleaning bzip2..."
	def_dev_dir_orig = f$environment( "default")
	set default $(IZ_BZIP2)
	$(MMS) $(MMSQUALIFIERS) /DESCR=[.vms]descrip.mms -
	 $(IZ_BZIP2_MACROS) -
	 $(MMSTARGETS)
	set default 'def_dev_dir_orig'
.ENDIF                          # BUILD_BZIP2

# CLEAN_ALL target.  Delete:
#    The [.$(DEST)] directory and everything in it (CLEAN),
#    The standard [.$(DEST)] directories and everything in them,
#    All help-related derived files,
#    All individual C dependency files,
#    All test_dir_* directories.
# Also mention:
#    Comprehensive dependency file.
#
CLEAN_ALL : CLEAN
	if (f$search( "[.ALPHA*]*.*") .nes. "") then -
	 delete /noconfirm [.ALPHA*]*.*;*
	if (f$search( "ALPHA*.DIR", 1) .nes. "") then -
	 set protection = w:d ALPHA*.DIR;*
	if (f$search( "ALPHA*.DIR", 2) .nes. "") then -
	 delete /noconfirm ALPHA*.DIR;*
	if (f$search( "[.IA64*]*.*") .nes. "") then -
	 delete /noconfirm [.IA64*]*.*;*
	if (f$search( "IA64*.DIR", 1) .nes. "") then -
	 set protection = w:d IA64*.DIR;*
	if (f$search( "IA64*.DIR", 2) .nes. "") then -
	 delete /noconfirm IA64*.DIR;*
	if (f$search( "[.VAX*]*.*") .nes. "") then -
	 delete /noconfirm [.VAX*]*.*;*
	if (f$search( "VAX*.DIR", 1) .nes. "") then -
	 set protection = w:d VAX*.DIR;*
	if (f$search( "VAX*.DIR", 2) .nes. "") then -
	 delete /noconfirm VAX*.DIR;*
	if (f$search( "help_temp_*.*") .nes. "") then -
	 delete /noconfirm help_temp_*.*;*
	if (f$search( "[.VMS]UNZIP_CLI.RNH") .nes. "") then -
	 delete /noconfirm [.VMS]UNZIP_CLI.RNH;*
	if (f$search( "UNZIP_CLI.HLP") .nes. "") then -
	 delete /noconfirm UNZIP_CLI.HLP;*
	if (f$search( "UNZIP_CLI.HTX") .nes. "") then -
	 delete /noconfirm UNZIP_CLI.HTX;*
	if (f$search( "UNZIP.HLP") .nes. "") then -
	 delete /noconfirm UNZIP.HLP;*
	if (f$search( "UNZIP.HTX") .nes. "") then -
	 delete /noconfirm UNZIP.HTX;*
	if (f$search( "test_dir_*.DIR;*") .nes. "") then -
	 set protection = w:d [.test_dir_*...]*.*;*
	if (f$search( "test_dir_*.DIR;*") .nes. "") then -
	 set protection = w:d test_dir_*.DIR;*
	if (f$search( "[.test_dir_*.*]*.*") .nes. "") then -
	 delete /noconfirm [.test_dir_*.*]*.*;*
	if (f$search( "[.test_dir_*]*.*") .nes. "") then -
	 delete /noconfirm [.test_dir_*]*.*;*
	if (f$search( "test_dir_*.DIR;*") .nes. "") then -
	 delete /noconfirm test_dir_*.DIR;*
	if (f$search( "*.MMSD") .nes. "") then -
	 delete /noconfirm *.MMSD;*
	if (f$search( "[.VMS]*.MMSD") .nes. "") then -
	 delete /noconfirm [.VMS]*.MMSD;*
	@ write sys$output ""
	@ write sys$output "Note:  This procedure will not"
	@ write sys$output "   DELETE [.VMS]DESCRIP_DEPS.MMS;*"
	@ write sys$output -
 "You may choose to, but a recent version of MMS (V3.5 or newer?) is"
	@ write sys$output -
 "needed to regenerate it.  (It may also be recovered from the original"
	@ write sys$output -
 "distribution kit.)  See [.VMS]DESCRIP_MKDEPS.MMS for instructions on"
	@ write sys$output -
 "generating [.VMS]DESCRIP_DEPS.MMS."
	@ write sys$output ""

# CLEAN_EXE target.  Delete the executables in [.$(DEST)].

CLEAN_EXE :
	if (f$search( "[.$(DEST)]*.EXE") .nes. "") then -
	 delete /noconfirm [.$(DEST)]*.EXE;*
.IFDEF BUILD_BZIP2              # BUILD_BZIP2
	@ write sys$output ""
	@ write sys$output "Cleaning bzip2..."
	def_dev_dir_orig = f$environment( "default")
	set default $(IZ_BZIP2)
	$(MMS) $(MMSQUALIFIERS) /DESCR=[.vms]descrip.mms -
	 $(IZ_BZIP2_MACROS) -
	 $(MMSTARGETS)
	set default 'def_dev_dir_orig'
.ENDIF                          # BUILD_BZIP2

# CLEAN_OLB target.  Delete the object libraries in [.$(DEST)].

CLEAN_OLB :
	if (f$search( "[.$(DEST)]*.OLB") .nes. "") then -
	 delete /noconfirm [.$(DEST)]*.OLB;*
.IFDEF BUILD_BZIP2              # BUILD_BZIP2
	@ write sys$output ""
	@ write sys$output "Cleaning bzip2..."
	def_dev_dir_orig = f$environment( "default")
	set default $(IZ_BZIP2)
	$(MMS) $(MMSQUALIFIERS) /DESCR=[.vms]descrip.mms -
	 $(IZ_BZIP2_MACROS) -
	 $(MMSTARGETS)
	set default 'def_dev_dir_orig'
.ENDIF                          # BUILD_BZIP2

# CLEAN_TEST target.  Delete the test directories, [.test_dir_*...].

CLEAN_TEST :
	if (f$search( "[.test_dir_*...]*.*;*") .nes. "") then -
	 set protection = w:d [.test_dir_*...]*.*;*
	if (f$search( "test_dir_*.DIR;*") .nes. "") then -
	 set protection = w:d test_dir_*.DIR;*
	if (f$search( "[.test_dir_*.*]*.*;*") .nes. "") then -
	 delete /noconfirm [.test_dir_*.*]*.*;*
	if (f$search( "[.test_dir_*]*.*;*") .nes. "") then -
	 delete /noconfirm [.test_dir_*]*.*;*
	if (f$search( "test_dir_*.DIR;*") .nes. "") then -
	 delete /noconfirm test_dir_*.DIR;*
.IFDEF BUILD_BZIP2              # BUILD_BZIP2
	@ write sys$output ""
	@ write sys$output "Cleaning bzip2..."
	def_dev_dir_orig = f$environment( "default")
	set default $(IZ_BZIP2)
	$(MMS) $(MMSQUALIFIERS) /DESCR=[.vms]descrip.mms -
	 $(IZ_BZIP2_MACROS) -
	 $(MMSTARGETS)
	set default 'def_dev_dir_orig'
.ENDIF                          # BUILD_BZIP2

# DASHV target.  Generate an "unzip -v" report.

DASHV :
	mcr [.$(DEST)]unzip -v

# HELP target.  Generate the HELP library source files.

HELP : $(UNZIP_HELP)
	@ write sys$output "Done."

# HELP_TEXT target.  Generate the HELP output text files.

HELP_TEXT : $(UNZIP_HELP_TEXT)
	@ write sys$output "Done."

# SLASHV target.  Generate an "unzip_cli /verbose" report.

SLASHV :
	mcr [.$(DEST)]unzip_cli /verbose

# TEST target.  Runs a test procedure.

TEST :
	@[.vms]test_unzip.com testmake.zip [.$(DEST)]

# TEST_PPMD target.  Runs a PPMd test procedure.

TEST_PPMD :
	@[.vms]test_unzip.com testmake_ppmd.zip [.$(DEST)] NOSFX


# Object library module dependencies.

$(LIB_UNZIP) : $(LIB_UNZIP)($(MODS_OBJS_LIB_UNZIP))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIP_CLI) : $(LIB_UNZIP_CLI)($(MODS_OBJS_LIB_UNZIP_CLI))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIPSFX) : $(LIB_UNZIPSFX)($(MODS_OBJS_LIB_UNZIPSFX))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIPSFX_CLI) : $(LIB_UNZIPSFX_CLI)($(MODS_OBJS_LIB_UNZIPSFX_CLI))
	@ write sys$output "$(MMS$TARGET) updated."

.IFDEF LIBUNZIP                 # LIBUNZIP
$(LIB_LIBUNZIP) : $(LIB_LIBUNZIP)($(MODS_OBJS_LIB_LIBUNZIP))
	@ write sys$output "$(MMS$TARGET) updated."
.ENDIF                          # LIBUNZIP

# Module ID options files.

OPT_ID = SYS$DISK:[.$(DEST)]UNZIP.OPT
OPT_ID_SFX = SYS$DISK:[.$(DEST)]UNZIPSFX.OPT

# Default C compile rule.

.C.OBJ :
	$(CC) $(CFLAGS) $(CDEFS_UNX) $(MMS$SOURCE)

# Normal sources in [.VMS].

[.$(DEST)]VMS.OBJ : [.VMS]VMS.C

# Command-line interface files.

[.$(DEST)]CMDLINE.OBJ : [.VMS]CMDLINE.C
	$(CC) $(CFLAGS) $(CDEFS_CLI) $(MMS$SOURCE)

[.$(DEST)]UNZIPCLI.OBJ : UNZIP.C
	$(CC) $(CFLAGS) $(CDEFS_CLI) $(MMS$SOURCE)

[.$(DEST)]UNZ_CLI.OBJ : [.$(DEST)]UNZ_CLI.CLD

[.$(DEST)]UNZ_CLI.CLD : [.VMS]UNZ_CLI.CLD
	@[.vms]cppcld.com "$(CC) $(CFLAGS_ARCH)" -
	 $(MMS$SOURCE) $(MMS$TARGET) "$(CDEFS)"

[.$(DEST)]ZIPINFO_C.OBJ : ZIPINFO.C
	$(CC) $(CFLAGS) $(CDEFS_CLI) $(MMS$SOURCE)

# SFX variant sources.

[.$(DEST)]CRC32_.OBJ : CRC32.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]CRYPT_.OBJ : CRYPT.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]EXTRACT_.OBJ : EXTRACT.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]FILEIO_.OBJ : FILEIO.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]GLOBALS_.OBJ : GLOBALS.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]ICONV_MAP_.OBJ : ICONV_MAP.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]IF_LZMA_.OBJ : IF_LZMA.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]IF_PPMD_.OBJ : IF_PPMD.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]INFLATE_.OBJ : INFLATE.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]MATCH_.OBJ : MATCH.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]PROCESS_.OBJ : PROCESS.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]TTYIO_.OBJ : TTYIO.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]UBZ2ERR_.OBJ : UBZ2ERR.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]VMS_.OBJ : [.VMS]VMS.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]UNZIPSFX.OBJ : UNZIP.C
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

# SFX CLI variant sources.

[.$(DEST)]CMDLINE_.OBJ : [.VMS]CMDLINE.C
	$(CC) $(CFLAGS) $(CDEFS_SFX_CLI) $(MMS$SOURCE)

[.$(DEST)]UNZSFXCLI.OBJ : UNZIP.C
	$(CC) $(CFLAGS) $(CDEFS_SFX_CLI) $(MMS$SOURCE)

# LIBUNZIP variant sources in [].

[.$(DEST)]API_L.OBJ : API.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]APIHELP_L.OBJ : APIHELP.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]CRYPT_L.OBJ : CRYPT.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]EXPLODE_L.OBJ : EXPLODE.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]EXTRACT_L.OBJ : EXTRACT.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]FILEIO_L.OBJ : FILEIO.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]GLOBALS_L.OBJ : GLOBALS.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]ICONV_MAP_L.OBJ : ICONV_MAP.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]IF_LZMA_L.OBJ : IF_LZMA.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]IF_PPMD_L.OBJ : IF_PPMD.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]INFLATE_L.OBJ : INFLATE.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]LIST_L.OBJ : LIST.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]PROCESS_L.OBJ : PROCESS.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]TTYIO_L.OBJ : TTYIO.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]UNSHRINK_L.OBJ : UNSHRINK.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]UBZ2ERR_L.OBJ : UBZ2ERR.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]UNZIP_L.OBJ : UNZIP.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

[.$(DEST)]ZIPINFO_L.OBJ : ZIPINFO.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

# LIBUNZIP variant sources in [.VMS].

[.$(DEST)]VMS_L.OBJ : [.VMS]VMS.C
	$(CC) $(CFLAGS) $(CDEFS_LIBUNZIP) $(MMS$SOURCE)

# VAX C LINK options file.

.IFDEF OPT_FILE
$(OPT_FILE) :
	open /write opt_file_ln $(OPT_FILE)
	write opt_file_ln "SYS$SHARE:VAXCRTL.EXE /SHARE"
	close opt_file_ln
.ENDIF

# Module ID options files.

$(OPT_ID) :
	@[.vms]optgen.com UnZip iz_unzip_versn
	open /write opt_file_ln $(OPT_ID)
	write opt_file_ln "Ident = ""UnZip ''f$trnlnm( "iz_unzip_versn")'"""
	close opt_file_ln

$(OPT_ID_SFX) :
	@[.vms]optgen.com UnZip iz_unzip_versn
	open /write opt_file_ln $(OPT_ID_SFX)
	write opt_file_ln "Ident = ""UnZipSFX ''f$trnlnm( "iz_unzip_versn")'"""
	close opt_file_ln

# Local BZIP2 object library.

$(LIB_BZ2_LOCAL) :
	@ write sys$output ""
	@ write sys$output "Building bzip2..."
	def_dev_dir_orig = f$environment( "default")
	set default $(IZ_BZIP2)
	$(MMS) $(MMSQUALIFIERS) /DESCR=[.vms]descrip.mms -
	 $(IZ_BZIP2_MACROS) -
	 $(MMSTARGETS)
	set default 'def_dev_dir_orig'
	@ write sys$output ""

# Normal UnZip executable.

$(UNZIP) : [.$(DEST)]UNZIP.OBJ \
            $(LIB_UNZIP) $(LIB_BZ2_DEP) \
            $(OPT_FILE) $(OPT_ID)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIP) /library, -
	 $(LIB_BZIP2_OPTS) -
	 $(LIB_UNZIP) /library, -
	 $(LIB_ZLIB_OPTS) -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID) /options -
	 $(NOSYSSHR_OPTS)

# CLI UnZip executable.

$(UNZIP_CLI) : [.$(DEST)]UNZIPCLI.OBJ \
                $(LIB_UNZIP_CLI) $(LIB_UNZIP) $(LIB_BZ2_DEP) \
                $(OPT_FILE) $(OPT_ID)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIP_CLI) /library, -
	 $(LIB_UNZIP) /library, -
	 $(LIB_BZIP2_OPTS) -
	 $(LIB_UNZIP) /library, -
	 $(LIB_ZLIB_OPTS) -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID) /options -
	 $(NOSYSSHR_OPTS)

# SFX UnZip executable.

$(UNZIPSFX) : [.$(DEST)]UNZIPSFX.OBJ \
               $(LIB_UNZIPSFX) $(LIB_BZ2_DEP) \
               $(OPT_FILE) $(OPT_ID_SFX)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIPSFX) /library, -
	 $(LIB_BZIP2_OPTS) -
	 $(LIB_UNZIPSFX) /library, -
	 $(LIB_ZLIB_OPTS) -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID_SFX) /options -
	 $(NOSYSSHR_OPTS)

# SFX CLI UnZip executable.

$(UNZIPSFX_CLI) : [.$(DEST)]UNZSFXCLI.OBJ \
                   $(LIB_UNZIPSFX_CLI) $(LIB_UNZIPSFX) $(LIB_BZ2_DEP) \
                   $(OPT_FILE) $(OPT_ID_SFX)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIPSFX_CLI) /library, -
	 $(LIB_UNZIPSFX) /library, -
	 $(LIB_BZIP2_OPTS) -
	 $(LIB_UNZIPSFX) /library, -
	 $(LIB_ZLIB_OPTS) -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID_SFX) /options -
	 $(NOSYSSHR_OPTS)


# Help library source files.

UNZIP.HLP : [.VMS]UNZIP_DEF.RNH
	runoff /output = $(MMS$TARGET) $(MMS$SOURCE)

UNZIP_CLI.HLP : [.VMS]UNZIP_CLI.HELP [.VMS]CVTHELP.TPU
	edit /tpu /nosection /nodisplay /command = [.VMS]CVTHELP.TPU -
	 $(MMS$SOURCE)
	rename UNZIP_CLI.RNH [.VMS]
	purge /nolog /keep = 1 [.VMS]UNZIP_CLI.RNH
	runoff /output = $(MMS$TARGET) [.VMS]UNZIP_CLI.RNH

# Help output text files.

.HLP.HTX :
	help_temp_name = "help_temp_"+ f$getjpi( 0, "PID")
	if (f$search( help_temp_name+ ".HLB") .nes. "") then -
         delete /noconfirm 'help_temp_name'.HLB;*
	library /create /help 'help_temp_name'.HLB $(MMS$SOURCE)
	help /library = sys$disk:[]'help_temp_name'.HLB -
         /output = 'help_temp_name'.OUT unzip...
	delete /noconfirm 'help_temp_name'.HLB;*
	create /fdl = [.VMS]STREAM_LF.FDL $(MMS$TARGET)
	open /append help_temp $(MMS$TARGET)
	copy 'help_temp_name'.OUT help_temp
	close help_temp
	delete /noconfirm 'help_temp_name'.OUT;*

UNZIP.HTX : UNZIP.HLP [.VMS]STREAM_LF.FDL

UNZIP_CLI.HTX : UNZIP_CLI.HLP [.VMS]STREAM_LF.FDL

# Message file.

$(UNZIP_MSG_EXE) : $(UNZIP_MSG_OBJ)
	link /shareable = $(MMS$TARGET) $(UNZIP_MSG_OBJ)

$(UNZIP_MSG_OBJ) : $(UNZIP_MSG_MSG)
	message /object = $(MMS$TARGET) /nosymbols $(UNZIP_MSG_MSG)

# Library link options file.

.IFDEF LIBUNZIP                 # LIBUNZIP
$(LIBUNZIP_OPT) : [.VMS]STREAM_LF.FDL
	def_dev_dir_orig = f$environment( "default")
	set default [.$(DEST)]
	def_dev_dir = f$environment( "default")
	set default 'def_dev_dir_orig'
	create /fdl = [.VMS]STREAM_LF.FDL $(MMS$TARGET)
	open /append opt_file_lib $(MMS$TARGET)
	write opt_file_lib "! DEFINE LIB_IZUNZIP ''def_dev_dir'"
	if ("$(LIB_BZIP2_OPTS)" .nes. "") then -
         write opt_file_lib "! DEFINE LIB_BZIP2 ''f$trnlnm( "lib_bzip2")'"
	if ("$(LIB_ZLIB_OPTS)" .nes. "") then -
         write opt_file_lib "! DEFINE LIB_ZLIB ''f$trnlnm( "lib_zlib")'"
	write opt_file_lib "LIB_IZUNZIP:$(LIB_LIBUNZIP_NAME) /library"
	if ("$(LIB_BZIP2_OPTS)" .nes. "") then -
         write opt_file_lib "$(LIB_BZIP2_OPTS)" - ","
	write opt_file_lib "LIB_IZUNZIP:$(LIB_LIBUNZIP_NAME) /library"
	if ("$(LIB_ZLIB_OPTS)" .nes. "") then -
         write opt_file_lib "$(LIB_ZLIB_OPTS)" - ","
	close opt_file_lib
.ENDIF                          # LIBUNZIP

# Include generated source dependencies.

INCL_DESCRIP_DEPS = 1
.INCLUDE [.VMS]DESCRIP_DEPS.MMS

