#                                               18 March 2013.  SMS.
#
#    UnZip 6.1 for VMS - MMS Dependency Description File.
#
#    MMS /EXTENDED_SYNTAX description file to generate a C source
#    dependencies file.  Unsightly errors result when /EXTENDED_SYNTAX
#    is not specified.  Typical usage:
#
#    $ L_U = "CRYPT_AES_WG_SFX, BZIP2_SFX, USE_OWN_CRCTAB,"+ -
#       " LZMA_SFX, PPMD_SFX, API_DOC, MEMDIAG"
#    $ MMS /EXTEND /DESCRIP = [.VMS]DESCRIP_MKDEPS.MMS /SKIP -
#       /MACRO = (AES_WG=1, IZ_BZIP2=iz_bzip2, IZ_ZLIB=iz_zlib, -
#       LZMA=1, PPMD=1, LIBUNZIP=1, "LOCAL_UNZIP=''L_U'")
#
# If the IZ_AES_WG encryption source kit has not been installed, then
# the macros AES_WG and CRYPT_AES_WG_SFX should not be defined.
#
# Note that this description file must be used from the main
# distribution directory, not from the [.VMS] subdirectory.
#
# This description file uses these command procedures:
#
#    [.VMS]MOD_DEP.COM
#    [.VMS]COLLECT_DEPS.COM
#
# MMK users without MMS will be unable to generate the dependencies file
# using this description file, however there should be one supplied in
# the kit.  If this file has been deleted, users in this predicament
# will need to recover it from the original distribution kit.
#
# Note:  This dependency generation scheme assumes that the dependencies
# do not depend on host architecture type or other such variables.
# Therefore, no "#include" directive in the C source itself should be
# conditional on such variables.
#
# The default target is the comprehensive source dependency file,
# DEPS_FILE = [.VMS]DESCRIP_DEPS.MMS.
#
# Other targets:
#
#    CLEAN      deletes the individual source dependency files,
#               *.MMSD;*, but leaves the comprehensive source dependency
#               file.
#
#    CLEAN_ALL  deletes all source dependency files, including the
#               individual *.MMSD;* files and the comprehensive file,
#               DESCRIP_DEPS.MMS.*.
#

# Required command procedures.

COLLECT_DEPS = [.VMS]COLLECT_DEPS.COM
MOD_DEP = [.VMS]MOD_DEP.COM

COMS = $(COLLECT_DEPS) $(MOD_DEP)

# Include the source file lists (among other data).

INCL_DESCRIP_SRC = 1
.INCLUDE [.VMS]DESCRIP_SRC.MMS

# The ultimate product, a comprehensive dependency list.

DEPS_FILE = [.VMS]DESCRIP_DEPS.MMS

# Detect valid qualifier and/or macro options.

.IF $(FINDSTRING Skip, $(MMSQUALIFIERS)) .eq Skip
DELETE_MMSD = 1
.ELSIF NOSKIP
PURGE_MMSD = 1
.ELSE
UNK_MMSD = 1
.ENDIF

# Dependency suffixes and rules.
#
# .FIRST is assumed to be used already, so the MMS qualifier/macro check
# is included in each rule (one way or another).

.SUFFIXES_BEFORE .C .MMSD

.C.MMSD :
.IF UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF
	$(CC) $(CFLAGS_INCL) $(CDEFS_UNX) $(MMS$SOURCE) /NOLIST /NOOBJECT -
	 /MMS_DEPENDENCIES = (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)

# List of MMS dependency files.

# In case it's not obvious...
# To extract module name lists from object library module=object lists:
# 1.  Transform "module=[.dest]name.OBJ" into "module=[.dest] name".
# 2.  For [.VMS], add [.VMS] to name.
# 3.  Delete "*]" words.
#
# A similar scheme works for executable lists.

MODS_LIB_UNZIP_N = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_UNZIP_N)))

MODS_LIB_UNZIP_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.VMS]*, $(MODS_OBJS_LIB_UNZIP_V)))

MODS_LIB_UNZIP_AES = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.AES_WG]*, $(MODS_OBJS_LIB_UNZIP_AES)))

MODS_LIB_UNZIP_LZMA = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.SZIP]*, $(MODS_OBJS_LIB_UNZIP_LZMA)))

MODS_LIB_UNZIP_PPMD = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.SZIP]*, $(MODS_OBJS_LIB_UNZIP_PPMD)))

MODS_LIB_UNZIPCLI_N = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_UNZIPCLI_C_N)))

MODS_LIB_UNZIPCLI_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.VMS]*, $(MODS_OBJS_LIB_UNZIPCLI_C_V)))

MODS_LIB_UNZIPSFX_N = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_UNZIPSFX_N)))

MODS_LIB_UNZIPSFX_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.VMS]*, $(MODS_OBJS_LIB_UNZIPSFX_V)))

MODS_LIB_LIBUNZIP_NL = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_LIBUNZIP_NL)))

MODS_LIB_LIBUNZIP_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.VMS]*, $(MODS_OBJS_LIB_LIBUNZIP_V)))

MODS_UNZIP = $(FILTER-OUT *], \
 $(PATSUBST *]*.EXE, *] *, $(UNZIP)))

MODS_UNZIP_CLI = $(FILTER-OUT *], \
 $(PATSUBST *]*.EXE, *] *, $(UNZIP_CLI)))

MODS_UNZIPSFX = $(FILTER-OUT *], \
 $(PATSUBST *]*.EXE, *] *, $(UNZIPSFX)))

MODS_UNZIPSFX_CLI = $(FILTER-OUT *], \
 $(PATSUBST *]*.EXE, *] *, $(UNZIPSFX_CLI)))

# Complete list of C object dependency file names.
# Note that the CLI UnZip main program object file is a special case.

DEPS = $(FOREACH NAME, \
 $(MODS_LIB_UNZIP_N) $(MODS_LIB_UNZIP_V) \
 $(MODS_LIB_UNZIP_AES) \
 $(MODS_LIB_UNZIP_LZMA) \
 $(MODS_LIB_UNZIP_PPMD) \
 $(MODS_LIB_UNZIPCLI_N) $(MODS_LIB_UNZIPCLI_V) \
 $(MODS_LIB_UNZIPSFX_N) $(MODS_LIB_UNZIPSFX_V) \
 $(MODS_UNZIP) $(MODS_UNZIP_CLI) \
 $(MODS_UNZIPSFX) $(MODS_UNZIPSFX_CLI) \
 $(MODS_LIB_LIBUNZIP_NL), \
 $(NAME).mmsd)

# Default target is the comprehensive dependency list.

$(DEPS_FILE) : $(DEPS) $(COMS)
.IF UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete individual .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the individual .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF
#
#       Note that the space in P4, which prevents immediate macro
#       expansion, is removed by COLLECT_DEPS.COM.
#
	@$(COLLECT_DEPS) "UnZip for VMS" "$(MMS$TARGET)" -
         "[...]*.MMSD" "[.$ (DEST)]" $(MMSDESCRIPTION_FILE) -
         "[.AES_WG/[.SZIP]C/[.SZIP]P/[.SZIP]S/[.SZIP]T/[.SZIP" -
         "AES_WG/LZMA_PPMD/PPMD/LZMA_PPMD/LZMA_PPMD/LZMA"
	@ write sys$output -
         "Created a new dependency file: $(MMS$TARGET)"
.IF DELETE_MMSD
	@ write sys$output -
         "Deleting intermediate .MMSD files..."
	if (f$search( "*.MMSD") .nes. "") then -
         delete /log *.MMSD;*
	if (f$search( "[.aes_wg]*.MMSD") .nes. "") then -
         delete /log [.aes_wg]*.MMSD;*
	if (f$search( "[.szip]*.MMSD") .nes. "") then -
         delete /log [.szip]*.MMSD;*
	if (f$search( "[.VMS]*.MMSD") .nes. "") then -
         delete /log [.VMS]*.MMSD;*
.ELSE
	@ write sys$output -
         "Purging intermediate .MMSD files..."
	if (f$search( "*.MMSD;-1") .nes. "") then -
         purge /log *.MMSD
	if (f$search( "[.aes_wg]*.MMSD;-1") .nes. "") then -
         purge /log [.aes_wg]*.MMSD
	if (f$search( "[.szip]*.MMSD;-1") .nes. "") then -
         purge /log [.szip]*.MMSD
	if (f$search( "[.VMS]*.MMSD;-1") .nes. "") then -
         purge /log [.VMS]*.MMSD
.ENDIF

# CLEAN target.  Delete the individual C dependency files.

CLEAN :
	if (f$search( "*.MMSD") .nes. "") then -
         delete /log *.MMSD;*
	if (f$search( "[.aes_wg]*.MMSD") .nes. "") then -
         delete /log [.aes_wg]*.MMSD;*
	if (f$search( "[.szip]*.MMSD") .nes. "") then -
         delete /log [.szip]*.MMSD;*
	if (f$search( "[.VMS]*.MMSD") .nes. "") then -
         delete /log [.VMS]*.MMSD;*

# CLEAN_ALL target.  Delete:
#    The individual C dependency files.
#    The collected source dependency file.

CLEAN_ALL :
	if (f$search( "*.MMSD") .nes. "") then -
         delete /log *.MMSD;*
	if (f$search( "[.aes_wg]*.MMSD") .nes. "") then -
         delete /log [.aes_wg]*.MMSD;*
	if (f$search( "[.szip]*.MMSD") .nes. "") then -
         delete /log [.szip]*.MMSD;*
	if (f$search( "[.VMS]*.MMSD") .nes. "") then -
         delete /log [.VMS]*.MMSD;*
	if (f$search( "[.VMS]DESCRIP_DEPS.MMS") .nes. "") then -
         delete /log [.VMS]DESCRIP_DEPS.MMS;*

# Explicit dependencies and rules for utility variant modules.
#
# The extra dependency on the normal dependency file obviates including
# the /SKIP warning code in each rule here.

CRC32_.MMSD : CRC32.C CRC32.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

CRYPT_.MMSD : CRYPT.C CRYPT.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

EXTRACT_.MMSD : EXTRACT.C EXTRACT.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

FILEIO_.MMSD : FILEIO.C FILEIO.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

GLOBALS_.MMSD : GLOBALS.C GLOBALS.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

INFLATE_.MMSD : INFLATE.C INFLATE.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

MATCH_.MMSD : MATCH.C MATCH.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

PROCESS_.MMSD : PROCESS.C PROCESS.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

TTYIO_.MMSD : TTYIO.C TTYIO.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UBZ2ERR_.MMSD : UBZ2ERR.C UBZ2ERR.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

[.VMS]VMS_.MMSD : [.VMS]VMS.C [.VMS]VMS.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UNZIP_CLI.MMSD : UNZIP.C UNZIP.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UNZIPSFX.MMSD : UNZIP.C UNZIP.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_SFX) $(CDEFS_SFX) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UNZIPSFX_CLI.MMSD : UNZIP.C UNZIP.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CFLAGS_SFX) $(CDEFS_SFX_CLI) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

ZIPINFO_C.MMSD : ZIPINFO.C ZIPINFO.MMSD
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

# Special case.  No normal (non-CLI) version.

[.VMS]CMDLINE.MMSD : [.VMS]CMDLINE.C
.IF UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CDEFS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

# UnZip library modules.

APIHELP_L.MMSD : APIHELP.C
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

API_L.MMSD : API.C API.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

CRYPT_L.MMSD : CRYPT.C CRYPT.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

EXPLODE_L.MMSD : EXPLODE.C EXPLODE.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

EXTRACT_L.MMSD : EXTRACT.C EXTRACT.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

FILEIO_L.MMSD : FILEIO.C FILEIO.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

GLOBALS_L.MMSD : GLOBALS.C GLOBALS.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

INFLATE_L.MMSD : INFLATE.C INFLATE.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

LIST_L.MMSD : LIST.C LIST.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

PROCESS_L.MMSD : PROCESS.C PROCESS.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

TTYIO_L.MMSD : TTYIO.C TTYIO.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UBZ2ERR_L.MMSD : UBZ2ERR.C UBZ2ERR.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UNSHRINK_L.MMSD : UNSHRINK.C UNSHRINK.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

UNZIP_L.MMSD : UNZIP.C UNZIP.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

[.VMS]VMS_L.MMSD : [.VMS]VMS.C [.VMS]VMS.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

ZIPINFO_L.MMSD : ZIPINFO.C ZIPINFO.MMSD
	$(CC) $(CFLAGS_INCL) $(CDEFS_LIBUNZIP) -
         $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@$(MOD_DEP) $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

