#                                               21 December 2004.  SMS.
#
#    UnZip 6.0 for VMS - MMS (or MMK) Description File.
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
#    CCOPTS=xxx     Compile with CC options xxx.  For example:
#                   CCOPTS=/ARCH=HOST
#
#    DBG=1          Compile with /DEBUG /NOOPTIMIZE.
#                   Link with /DEBUG /TRACEBACK.
#                   (Default is /NOTRACEBACK.)
#
#    LARGE=1        Enable large-file (>2GB) support.  Non-VAX-only.
#
#    LINKOPTS=xxx   Link with LINK options xxx.  For example:
#                   LINKOPTS=/NOINFO
#
#    LIST=1         Compile with /LIST /SHOW = (ALL, NOMESSAGES).
#                   Link with /MAP /CROSS_REFERENCE /FULL.
#
#    NOSHARE=1      Link /NOSYSSHR (not using shareable images).
#    NOSHARE=OLDVAX Link /NOSYSSHR on VAX for:
#                      DEC C with VMS before V7.3.
#                      VAX C without DEC C RTL (DEC C not installed).
#
#    "LOCAL_UNZIP= c_macro_1=value1 [, c_macro_2=value2 [...]]"
#                   Compile with these additional C macros defined. 
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
# Example commands:
#
# To build the conventional small-file product using the DEC/Compaq/HP C
# compiler (Note: DESCRIP.MMS is the default description file name.):
#
#    MMS /DESCRIP = [.VMS]
#
# To get the large-file executables (on a non-VAX system):
#
#    MMS /DESCRIP = [.VMS] /MACRO = (LARGE=1)
#
# To delete the architecture-specific generated files for this system
# type:
#
#    MMS /DESCRIP = [.VMS] /MACRO = (LARGE=1)   ! Large-file product.
# or
#    MMS /DESCRIP = [.VMS]                      ! Small-file product.
#
# To build a complete small-file product for debug with compiler
# listings and link maps:
#
#    MMS /DESCRIP = [.VMS] CLEAN
#    MMS /DESCRIP = [.VMS] /MACRO = (DBG=1, LIST=1)
#
########################################################################

# Include primary product description file.

INCL_DESCRIP_SRC = 1
.INCLUDE [.vms]descrip_src.mms

# Object library names.

LIB_UNZIP = [.$(DEST)]unzip.olb
LIB_UNZIP_CLI = [.$(DEST)]unzipcli.olb
LIB_UNZIPSFX = [.$(DEST)]unzipsfx.olb
LIB_UNZIPSFX_CLI = [.$(DEST)]unzsxcli.olb

# Help file names.

UNZIP_HELP = unzip.hlp unzip_cli.hlp


# TARGETS.

# Default target, ALL.  Build All Zip executables, utility executables,
# and help files.

ALL : $(UNZIP) $(UNZIP_CLI) $(UNZIPSFX) $(UNZIPSFX_CLI) $(UNZIP_HELP)
	@ write sys$output "Done."

# CLEAN target.  Delete the [.$(DEST)] directory and everything in it.

CLEAN :
	if (f$search( "[.$(DEST)]*.*") .nes. "") then -
	 delete [.$(DEST)]*.*;*
	if (f$search( "$(DEST).dir") .nes. "") then -
	 set protection = w:d $(DEST).dir;*
	if (f$search( "$(DEST).dir") .nes. "") then -
	 delete $(DEST).dir;*

# CLEAN_ALL target.  Delete:
#    The [.$(DEST)] directories and everything in them.
#    All help-related derived files,
#    All individual C dependency files.
# Also mention:
#    Comprehensive dependency file.
#
CLEAN_ALL :
	if (f$search( "[.ALPHA*]*.*") .nes. "") then -
	 delete [.ALPHA*]*.*;*
	if (f$search( "ALPHA.dir") .nes. "") then -
	 set protection = w:d ALPHA.dir;*
	if (f$search( "ALPHA.dir") .nes. "") then -
	 delete ALPHA.dir;*
	if (f$search( "[.IA64]*.*") .nes. "") then -
	 delete [.IA64*]*.*;*
	if (f$search( "IA64.dir") .nes. "") then -
	 set protection = w:d IA64.dir;*
	if (f$search( "IA64.dir") .nes. "") then -
	 delete IA64.dir;*
	if (f$search( "[.VAX*]*.*") .nes. "") then -
	 delete [.VAX*]*.*;*
	if (f$search( "VAX.dir") .nes. "") then -
	 set protection = w:d VAX.dir;*
	if (f$search( "VAX.dir") .nes. "") then -
	 delete VAX.dir;*
	if (f$search( "[.vms]ZIP_CLI.RNH") .nes. "") then -
	 delete [.vms]ZIP_CLI.RNH;*
	if (f$search( "ZIP_CLI.HLP") .nes. "") then -
	 delete ZIP_CLI.HLP;*
	if (f$search( "ZIP.HLP") .nes. "") then -
	 delete ZIP.HLP;*
	if (f$search( "*.MMSD") .nes. "") then -
	 delete *.MMSD;*
	if (f$search( "[.vms]*.MMSD") .nes. "") then -
	 delete [.vms]*.MMSD;*
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
 "generating the [.VMS]DESCRIP_DEPS.MMS."
	@ write sys$output ""

# CLEAN_EXE target.  Delete the executables in [.$(DEST)].

CLEAN_EXE :
        if (f$search( "[.$(DEST)]*.exe") .nes. "") then -
         delete [.$(DEST)]*.exe;*


# Object library module dependencies.

$(LIB_UNZIP) : $(LIB_UNZIP)($(MODS_OBJS_LIB_UNZIP))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIP_CLI) : $(LIB_UNZIP_CLI)($(MODS_OBJS_LIB_UNZIP_CLI))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIPSFX) : $(LIB_UNZIPSFX)($(MODS_OBJS_LIB_UNZIPSFX))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_UNZIPSFX_CLI) : $(LIB_UNZIPSFX_CLI)($(MODS_OBJS_LIB_UNZIPSFX_CLI))
	@ write sys$output "$(MMS$TARGET) updated."


# Module ID options files.

OPT_ID = SYS$DISK:[.vms]unzip.opt
OPT_ID_SFX = SYS$DISK:[.vms]unzipsfx.opt

# Default C compile rule.

.C.OBJ :
        $(CC) $(CFLAGS) $(CDEFS_UNX) $(MMS$SOURCE)


# Normal sources in [.VMS].

[.$(DEST)]vms.obj : [.vms]vms.c

# Command-line interface files.

[.$(DEST)]cmdline.obj : [.vms]cmdline.c
	$(CC) $(CFLAGS) $(CDEFS_CLI) $(MMS$SOURCE)

[.$(DEST)]unzipcli.obj : unzip.c
	$(CC) $(CFLAGS) $(CDEFS_CLI) $(MMS$SOURCE)

[.$(DEST)]unz_cli.obj : [.vms]unz_cli.cld

# SFX variant sources.

[.$(DEST)]crc32_.obj : crc32.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]crctab_.obj : crctab.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]crypt_.obj : crypt.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]extract_.obj : extract.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]fileio_.obj : fileio.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]globals_.obj : globals.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]inflate_.obj : inflate.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]match_.obj : match.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]process_.obj : process.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]ttyio_.obj : ttyio.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]vms_.obj : [.vms]vms.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

[.$(DEST)]unzipsfx.obj : unzip.c
	$(CC) $(CFLAGS) $(CDEFS_SFX) $(MMS$SOURCE)

# SFX CLI variant sources.

[.$(DEST)]cmdline_.obj : [.vms]cmdline.c
	$(CC) $(CFLAGS) $(CDEFS_SFX_CLI) $(MMS$SOURCE)

[.$(DEST)]unzsxcli.obj : unzip.c
	$(CC) $(CFLAGS) $(CDEFS_SFX_CLI) $(MMS$SOURCE)

# VAX C LINK options file.

.IFDEF OPT_FILE
$(OPT_FILE) :
	open /write opt_file_ln  $(OPT_FILE)
	write opt_file_ln "SYS$SHARE:VAXCRTL.EXE /SHARE"
	close opt_file_ln
.ENDIF

# Normal UnZip executable.

$(UNZIP) : [.$(DEST)]unzip.obj \
           $(LIB_UNZIP) $(OPT_FILE) $(OPT_ID)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIP) /library, -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID) /options -
	 $(NOSHARE_OPTS)

# CLI Zip executable.

$(UNZIP_CLI) : [.$(DEST)]unzipcli.obj \
               $(LIB_UNZIP_CLI) $(OPT_FILE) $(OPT_ID)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIP_CLI) /library, $(LIB_UNZIP) /library, -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID) /options -
	 $(NOSHARE_OPTS)

# SFX UnZip executable.

$(UNZIPSFX) : [.$(DEST)]unzipsfx.obj \
              $(LIB_UNZIPSFX) $(OPT_FILE) $(OPT_ID_SFX)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIPSFX) /library, -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID_SFX) /options -
	 $(NOSHARE_OPTS)

# SFX CLI UnZip executable.

$(UNZIPSFX_CLI) : [.$(DEST)]unzsxcli.obj \
                  $(LIB_UNZIPSFX_CLI) $(LIB_UNZIPSFX) \
                  $(OPT_FILE) $(OPT_ID_SFX)
	$(LINK) $(LINKFLAGS) $(MMS$SOURCE), -
	 $(LIB_UNZIPSFX_CLI) /library, $(LIB_UNZIPSFX) /library, -
	 $(LFLAGS_ARCH) -
	 $(OPT_ID_SFX) /options -
	 $(NOSHARE_OPTS)


# Help files.

unzip.hlp : [.vms]unzip_def.rnh
	runoff /output = $(MMS$TARGET) $(MMS$SOURCE)

unzip_cli.hlp : [.vms]unzip_cli.help [.vms]cvthelp.tpu
	edit /tpu /nosection /nodisplay /command = [.vms]cvthelp.tpu -
	 $(MMS$SOURCE)
	rename unzip_cli.rnh [.vms]
	runoff /output = $(MMS$TARGET) [.vms]unzip_cli.rnh

# Include generated source dependencies.

INCL_DESCRIP_DEPS = 1
.INCLUDE [.vms]descrip_deps.mms

