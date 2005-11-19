#                                               24 May 2005.  SMS.
#
#    UnZip 6.0 for VMS - MMS Dependency Description File.
#
#    MMS /EXTENDED_SYNTAX description file to generate a C source
#    dependencies file.  Unsightly errors result when /EXTENDED_SYNTAX
#    is not specified.  Typical usage:
#
#    $ MMS /EXTEND /DESCRIP = [.vms]descrip_mkdeps.mms /SKIP
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

# Required command procedures.

COMS = [.VMS]MOD_DEP.COM [.VMS]COLLECT_DEPS.COM

# Include the source file lists (among other data).

INCL_DESCRIP_SRC = 1
.INCLUDE [.vms]descrip_src.mms

# The ultimate product, a comprehensive dependency list.

DEPS_FILE = [.vms]descrip_deps.mms

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
	$(CC) $(CFLAGS_INCL) $(MMS$SOURCE) /NOLIST /NOOBJECT -
	 /MMS_DEPENDENCIES = (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)

# List of MMS dependency files.

# In case it's not obvious...
# To extract module name lists from object library module=object lists:
# 1.  Transform "module=[.dest]name.obj" into "module=[.dest] name".
# 2.  For [.vms], add [.vms] to name.
# 3.  Delete "*]" words.
#
# A similar scheme works for executable lists.

MODS_LIB_UNZIP_N = $(FILTER-OUT *], \
 $(PATSUBST *]*.obj, *] *, $(MODS_OBJS_LIB_UNZIP_N)))

MODS_LIB_UNZIP_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.obj, *] [.vms]*, $(MODS_OBJS_LIB_UNZIP_V)))

MODS_LIB_UNZIPCLI_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.obj, *] [.vms]*, $(MODS_OBJS_LIB_UNZIPCLI_C_V)))

MODS_LIB_UNZIPSFX_N = $(FILTER-OUT *], \
 $(PATSUBST *]*.obj, *] *, $(MODS_OBJS_LIB_UNZIPSFX_N)))

MODS_LIB_UNZIPSFX_V = $(FILTER-OUT *], \
 $(PATSUBST *]*.obj, *] [.vms]*, $(MODS_OBJS_LIB_UNZIPSFX_V)))

MODS_UNZIP = $(FILTER-OUT *], \
 $(PATSUBST *]*.exe, *] *, $(UNZIP)))

MODS_UNZIP_CLI = $(FILTER-OUT *], \
 $(PATSUBST *]*.exe, *] *, $(UNZIP_CLI)))

MODS_UNZIPSFX = $(FILTER-OUT *], \
 $(PATSUBST *]*.exe, *] *, $(UNZIPSFX)))

MODS_UNZIPSFX_CLI = $(FILTER-OUT *], \
 $(PATSUBST *]*.exe, *] *, $(UNZIPSFX_CLI)))

# Complete list of C object dependency file names.
# Note that the CLI UnZip main program object file is a special case.

DEPS = $(FOREACH NAME, \
 $(MODS_LIB_UNZIP_N) $(MODS_LIB_UNZIP_V) \
 $(MODS_LIB_UNZIPCLI_V) \
 $(MODS_LIB_UNZIPSFX_N) $(MODS_LIB_UNZIPSFX_V) \
 $(MODS_UNZIP) $(MODS_UNZIP_CLI) \
 $(MODS_UNZIPSFX) $(MODS_UNZIPSFX_CLI), \
 $(NAME).mmsd)

# Target is the comprehensive dependency list.

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
        @[.vms]collect_deps.com "UnZip" -
         "$(MMS$TARGET)" "[...]*.mmsd" "[.$ (DEST)]" $(MMSDESCRIPTION_FILE)
        @ write sys$output -
         "Created a new dependency file: $(MMS$TARGET)"
.IF DELETE_MMSD
	@ write sys$output -
         "Deleting intermediate .MMSD files..."
	delete /log *.mmsd;*, [.vms]*.mmsd;*
.ELSE
	@ write sys$output -
         "Purging intermediate .MMSD files..."
	purge /log *.mmsd, [.vms]*.mmsd
.ENDIF

# Explicit dependencies and rules for utility variant modules.
#
# The extra dependency on the normal dependency file obviates including
# the /SKIP warning code in each rule here.

crc32_.mmsd : crc32.c crc32.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

crctab_.mmsd : crctab.c crc32.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

crypt_.mmsd : crypt.c crypt.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

extract_.mmsd : extract.c extract.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

fileio_.mmsd : fileio.c fileio.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

globals_.mmsd : globals.c globals.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

inflate_.mmsd : inflate.c inflate.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

match_.mmsd : match.c match.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

process_.mmsd : process.c process.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

ttyio_.mmsd : ttyio.c ttyio.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

[.vms]vms_.mmsd : [.vms]vms.c [.vms]vms.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

unzip_cli.mmsd : unzip.c unzip.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

unzipsfx.mmsd : unzip.c unzip.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_SFX) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

unzipsfx_cli.mmsd : unzip.c unzip.mmsd
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(CFLAGS_SFX) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

# Special case.  No normal (non-CLI) version.

[.vms]cmdline.mmsd : [.vms]cmdline.c
.IF UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF
	$(CC) $(CFLAGS_INCL) $(CFLAGS_CLI) $(MMS$SOURCE) -
         /NOLIST /NOOBJECT /MMS_DEPENDENCIES = -
         (FILE = $(MMS$TARGET), NOSYSTEM_INCLUDE_FILES)
	@[.vms]mod_dep.com $(MMS$TARGET) $(MMS$TARGET_NAME).OBJ $(MMS$TARGET)

