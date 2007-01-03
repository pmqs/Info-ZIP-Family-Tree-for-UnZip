!==========================================================================
! MMS description file for UnZip/UnZipSFX 5.5x (x >= 3)          2006-12-29
!==========================================================================
!
! To build UnZip that uses shared libraries, edit the USER CUSTOMIZATION
! lines below to taste, then do
!	mms
! or
!	mmk
! if you use Matt's Make (free MMS-compatible make utility).
!
! (One-time users will find it easier to use the MAKE_UNZ.COM command file,
! which generates both UnZip and UnZipSFX.  Just type "@[.VMS]MAKE_UNZ", or
! "@[.VMS]MAKE_UNZ GCC" if you want to use GNU C.)

! To build the default target
!   "all executables (linked against shareable images), and help file",
! you can simply type "mmk" or "mms".
! (You have to copy the description file to your working directory for MMS,
! with MMK you can alternatively use the /DESCR=[.VMS] qualifier.
!
! In all other cases where you want to explicitely specify a makefile target,
! you have to specify your compiling environment, too. These are:
!
!	$ MMS/MACRO=(__ALPHA__=1)		! Alpha AXP, (DEC C)
!	$ MMS/MACRO=(__IA64__=1)		! IA64, (DEC C)
!	$ MMS/MACRO=(__DECC__=1)		! VAX, using DEC C
!	$ MMS/MACRO=(__FORCE_VAXC__=1)		! VAX, prefering VAXC over DECC
!	$ MMS/MACRO=(__VAXC__=1)		! VAX, where VAXC is default
!	$ MMS/MACRO=(__GNUC__=1)		! VAX, using GNU C
!

! To activate BZIP2 support, add the MMS macro "IZ_BZIP2=dev:[dir]",
! where the macro value ("dev:[dir]", or a suitable logical name) tells
! where to find "bzlib.h".  The BZIP2 object library (LIBBZ2_NS.OLB) is
! expected ! to be in a "[.dest]" directory under that one
! ("dev:[dir.ALPHAL]", for example), or in that directory itself.
!
! By default, the SFX programs are built without BZIP2 support.  Add
! "BZIP2_SFX=1" to the COMMON_DEFS C macros to enable it.  (See
! COMMON_DEFS below.)

! To build UnZip without shared libraries,
!	mms noshare

! To delete all .OBJ, .OLB, .EXE and .HLP files,
!	mms clean

DO_THE_BUILD :
	@ decc = f$search("SYS$SYSTEM:DECC$COMPILER.EXE").nes.""
	@ axp = (f$getsyi("HW_MODEL") .ge. 1024) .and. -
	   (f$getsyi("HW_MODEL") .lt. 4096)
	@ i64 = f$getsyi("HW_MODEL") .ge. 4096
	@ macro = "/MACRO=("
.IFDEF IZ_BZIP2
	@ macro = macro + "IZ_BZIP2=$(IZ_BZIP2),"
.ENDIF
	@ if decc then macro = macro + "__DECC__=1,"
	@ if axp then macro = macro + "__ALPHA__=1,"
	@ if i64 then macro = macro + "__IA64__=1,"
	@ if .not.(axp .or. i64 .or. decc) then macro = macro + "__VAXC__=1,"
	@ macro = f$extract(0,f$length(macro)-1,macro)+ ")"
	$(MMS)$(MMSQUALIFIERS)'macro' default

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

.IFDEF __ALPHA__                # __ALPHA__
DEST = ALPHA
E = .AXP_EXE
O = .AXP_OBJ
A = .AXP_OLB
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
DEST = IA64
E = .I64_EXE
O = .I64_OBJ
A = .I64_OLB
.ELSE                               # __IA64__
.IFDEF __DECC__                         # __DECC__
DEST = VAX
E = .VAX_DECC_EXE
O = .VAX_DECC_OBJ
A = .VAX_DECC_OLB
.ENDIF                                  # __DECC__
.IFDEF __FORCE_VAXC__                   # __FORCE_VAXC__
__VAXC__ = 1
.ENDIF                                  # __FORCE_VAXC__
.IFDEF __VAXC__                         # __VAXC__
DEST = VAX
E = .VAX_VAXC_EXE
O = .VAX_VAXC_OBJ
A = .VAX_VAXC_OLB
.ENDIF                                  # __VAXC__
.IFDEF __GNUC__                         # __GNUC__
DEST = VAX
E = .VAX_GNUC_EXE
O = .VAX_GNUC_OBJ
A = .VAX_GNUC_OLB
.ENDIF                                  # __GNUC__
.ENDIF                              # __IA64__
.ENDIF                          # __ALPHA__
.IFDEF O                        # O
.ELSE                           # O
!If EXE and OBJ extensions aren't defined, define them
E = .EXE
O = .OBJ
A = .OLB
.ENDIF                          # O

!The following preprocessor macros are set to enable the VMS CLI$ interface:
CLI_DEFS = VMSCLI,

!!!!!!!!!!!!!!!!!!!!!!!!!!! USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!!!!!
! add RETURN_CODES, and/or any other optional preprocessor flags (macros)
! except VMSCLI to the following line for a custom version (do not forget
! a trailing comma!!):
COMMON_DEFS =
!
! WARNING: Do not use VMSCLI here!! The creation of an UnZip executable
!          utilizing the VMS CLI$ command interface is handled differently.
!!!!!!!!!!!!!!!!!!!!!!!! END OF USER CUSTOMIZATION !!!!!!!!!!!!!!!!!!!!!!!!

.IFDEF __GNUC__
CC = gcc
LIBS = ,GNU_CC:[000000]GCCLIB.OLB/LIB
.ELSE
CC = cc
LIBS =
.ENDIF

CFLAGS = /NOLIST

OPTFILE = sys$disk:[.vms]vaxcshr.opt

.IFDEF __ALPHA__                # __ALPHA__
CC_OPTIONS = /STANDARD=RELAX/PREFIX=ALL/ANSI
CC_DEFS = MODERN,
OPTFILE_LIST =
OPTIONS = $(LIBS)
NOSHARE_OPTS = $(LIBS)/NOSYSSHR
.ELSE                           # __ALPHA__
.IFDEF __IA64__                     # __IA64__
CC_OPTIONS = /STANDARD=RELAX/PREFIX=ALL/ANSI
CC_DEFS = MODERN,
OPTFILE_LIST =
OPTIONS = $(LIBS)
NOSHARE_OPTS = $(LIBS)/NOSYSSHR
.ELSE                               # __IA64__
.IFDEF __DECC__                         # __DECC__
CC_OPTIONS = /DECC/STANDARD=RELAX/PREFIX=ALL
CC_DEFS = MODERN,
OPTFILE_LIST =
OPTIONS = $(LIBS)
NOSHARE_OPTS = $(LIBS),SYS$LIBRARY:DECCRTL.OLB/LIB/INCL=(CMA$TIS)/NOSYSSHR
.ELSE                                   # __DECC__
.IFDEF __FORCE_VAXC__                       # __FORCE_VAXC__
!Select VAXC on systems where DEC C exists
CC_OPTIONS = /VAXC
.ELSE                                       # __FORCE_VAXC__
!No flag allowed/needed on a pure VAXC system
CC_OPTIONS =
.ENDIF                                      # __FORCE_VAXC__
CC_DEFS =
OPTFILE_LIST = ,$(OPTFILE)
OPTIONS = $(LIBS),$(OPTFILE)/OPTIONS
NOSHARE_OPTS = $(LIBS),SYS$LIBRARY:VAXCRTL.OLB/LIB/NOSYSSHR
.ENDIF                                  # __DECC__
.ENDIF                              # __IA64__
.ENDIF                          # __ALPHA__

!
! The .FIRST target is needed only if we're serious about building,
! and then, only if BZIP2 support was requested.
!
.IFDEF MMSTARGETS               # MMSTARGETS
.IFDEF IZ_BZIP2                     # IZ_BZIP2
CC_DEFS2 = USE_BZIP2,
CFLAGS_INCL = /INCLUDE = ([], [.VMS])
INCL_BZIP2_M = , ubz2err
INCL_BZIP2_Q = /include = (ubz2err)
LIB_BZIP2_OPTS = LIB_BZIP2:LIBBZ2_NS.OLB /library,

.FIRST
	@ define incl_bzip2 $(IZ_BZIP2)
	@ @[.vms]find_bzip2_lib.com $(IZ_BZIP2) $(DEST) LIBBZ2_NS.OLB lib_bzip2
	@ write sys$output ""
	@ if (f$trnlnm("lib_bzip2") .nes. "") then -
	   write sys$output "   BZIP2 dir: ''f$trnlnm("lib_bzip2")'"
	@ if (f$trnlnm("lib_bzip2") .eqs. "") then -
	   write sys$output "   Can not find BZIP2 object library."
	@ write sys$output ""
	@ if (f$trnlnm("lib_bzip2") .eqs. "") then -
	   I_WILL_DIE_NOW.  /$$$$INVALID$$$$
.ELSE                               # IZ_BZIP2
CC_DEFS2 =
CFLAGS_INCL = /INCLUDE = []
LIB_BZIP2_OPTS =
.ENDIF                              # IZ_BZIP2
.ELSE                           # MMSTARGETS
CC_DEFS2 =
CFLAGS_INCL = /INCLUDE = []
LIB_BZIP2_OPTS =
.ENDIF                          # MMSTARGETS

.IFDEF __DEBUG__
CDEB = /DEBUG/NOOPTIMIZE
LDEB = /DEBUG
.ELSE
CDEB =
LDEB = /NOTRACE
.ENDIF

CFLAGS_ALL  = $(CC_OPTIONS) $(CFLAGS) $(CDEB) $(CFLAGS_INCL) -
              /def=($(CC_DEFS) $(CC_DEFS2) $(COMMON_DEFS) VMS)
CFLAGS_SFX  = $(CC_OPTIONS) $(CFLAGS) $(CDEB) $(CFLAGS_INCL) -
              /def=($(CC_DEFS) $(CC_DEFS2) $(COMMON_DEFS) SFX, VMS)
CFLAGS_CLI  = $(CC_OPTIONS) $(CFLAGS) $(CDEB) $(CFLAGS_INCL) -
              /def=($(CC_DEFS) $(CC_DEFS2) $(COMMON_DEFS) $(CLI_DEFS) VMS)
CFLAGS_SXC = $(CC_OPTIONS) $(CFLAGS) $(CDEB) $(CFLAGS_INCL) -
              /def=($(CC_DEFS) $(CC_DEFS2) $(COMMON_DEFS) $(CLI_DEFS) SFX, VMS)

LINKFLAGS   = $(LDEB)


OBJM =		unzip$(O), unzcli$(O), unzipsfx$(O), unzsxcli$(O)
COMMON_OBJS1 =	crc32$(O),crypt$(O),envargs$(O),-
		explode$(O),extract$(O),fileio$(O),globals$(O)
COMMON_OBJS2 =	inflate$(O),list$(O),match$(O),process$(O),ttyio$(O),-
		ubz2err$(O),unreduce$(O),unshrink$(O),zipinfo$(O),-
		vms$(O)
OBJUNZLIB =	$(COMMON_OBJS1),$(COMMON_OBJS2)

COMMON_OBJX1 =	CRC32=crc32_$(O),CRYPT=crypt_$(O),-
		EXTRACT=extract_$(O),-
		FILEIO=fileio_$(O),GLOBALS=globals_$(O)
COMMON_OBJX2 =	INFLATE=inflate_$(O),MATCH=match_$(O),-
		PROCESS=process_$(O),-
		TTYIO=ttyio_$(O),-
		UBZ2ERR=ubz2err_$(O),-
		VMS=vms_$(O)
OBJSFXLIB =	$(COMMON_OBJX1),$(COMMON_OBJX2)

UNZX_UNX = unzip
UNZX_CLI = unzip_cli
UNZSFX_DEF = unzipsfx
UNZSFX_CLI = unzipsfx_cli

OBJS = unzip$(O), $(OBJUNZLIB)
OBJX = UNZIP=unzipsfx$(O), $(OBJSFXLIB)
OBJSCLI = UNZIP=unzipcli$(O), -
	VMS_UNZIP_CLD=unz_cli$(O),-
	VMS_UNZIP_CMDLINE=cmdline$(O)
OBJXCLI = UNZIP=unzsxcli$(O),-
	VMS_UNZIP_CLD=unz_cli$(O),-
	VMS_UNZIP_CMDLINE=cmdline_$(O)
UNZHELP_UNX_RNH = [.vms]unzip_def.rnh
UNZHELP_CLI_RNH = [.vms]unzip_cli.rnh

OLBUNZ = unzip$(A)
OLBCLI = unzipcli$(A)
OLBSFX = unzipsfx$(A)
OLBSXC = unzsxcli$(A)

UNZIP_H = unzip.h unzpriv.h globals.h

UNZIPS = $(UNZX_UNX)$(E), $(UNZX_CLI)$(E), $(UNZSFX_DEF)$(E), $(UNZSFX_CLI)$(E)
UNZIPS_NOSHARE = $(UNZX_UNX)_noshare$(E), $(UNZSFX_DEF)_noshare$(E)
UNZIPHELPS = $(UNZX_UNX).hlp, $(UNZX_CLI).hlp

!!!!!!!!!!!!!!!!!!! override default rules: !!!!!!!!!!!!!!!!!!!
.suffixes :
.suffixes : .ANL $(E) $(A) .MLB .HLB .TLB .FLB $(O) -
	    .FORM .BLI .B32 .C .COB -
	    .FOR .BAS .B16 .PLI .PEN .PAS .MAC .MAR .M64 .CLD .MSG .COR .DBL -
	    .RPG .SCN .IFDL .RBA .RC .RCO .RFO .RPA .SC .SCO .SFO .SPA .SPL -
	    .SQLADA .SQLMOD .RGK .RGC .MEM .RNO .HLP .RNH .L32 .REQ .R32 -
	    .L16 .R16 .TXT .H .FRM .MMS .DDL .COM .DAT .OPT .CDO .SDML .ADF -
	    .GDF .LDF .MDF .RDF .TDF

$(O)$(E) :
	$(LINK) $(LINKFLAGS) /EXE=$(MMS$TARGET) $(MMS$SOURCE)

$(O)$(A) :
	If "''F$Search("$(MMS$TARGET)")'" .EQS. "" Then $(LIBR)/Create $(MMS$TARGET)
	$(LIBR)$(LIBRFLAGS) $(MMS$TARGET) $(MMS$SOURCE)

.CLD$(O) :
	SET COMMAND /OBJECT=$(MMS$TARGET) $(CLDFLAGS) $(MMS$SOURCE)

.c$(O) :
	$(CC) $(CFLAGS_ALL) /OBJ=$(MMS$TARGET) $(MMS$SOURCE)

.RNH.HLP :
	runoff /out=$@ $<

!!!!!!!!!!!!!!!!!! here starts the unzip specific part !!!!!!!!!!!

default :	$(UNZIPS), $(UNZIPHELPS)
	@	!	Do nothing.

noshare :	$(UNZIPS_NOSHARE), $(UNZIPHELPS)
	@	!	Do nothing.

$(UNZX_UNX)$(E) : $(OLBUNZ)($(OBJS))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBUNZ)/INCLUDE=(UNZIP $(INCL_BZIP2_M))/LIBRARY$(OPTIONS), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzip.opt/OPT

$(UNZX_CLI)$(E) : $(OLBCLI)($(OBJSCLI)),$(OLBUNZ)($(OBJUNZLIB))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBCLI)/INCLUDE=UNZIP/LIBRARY, -
	 $(OLBUNZ)/LIBRARY$(OPTIONS) $(INCL_BZIP2_Q), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzip.opt/OPT

$(UNZSFX_DEF)$(E) : $(OLBSFX)($(OBJX))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBSFX)/INCLUDE=(UNZIP $(INCL_BZIP2_M))/LIBRARY$(OPTIONS), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzipsfx.opt/OPT

$(UNZSFX_CLI)$(E) : $(OLBSXC)($(OBJXCLI)),$(OLBSFX)($(OBJSFXLIB))$(OPTFILE_LIST)
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBSXC)/INCLUDE=UNZIP/LIBRARY, -
	 $(OLBSFX)/LIBRARY$(OPTIONS) $(INCL_BZIP2_Q), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzipsfx.opt/OPT

$(UNZX_UNX)_noshare$(E) :	$(OLBUNZ)($(OBJS))
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBUNZ)/INCLUDE=(UNZIP $(INCL_BZIP2_M))/LIBRARY$(NOSHARE_OPTS), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzip.opt/OPT

$(UNZSFX_DEF)_noshare$(E) :	$(OLBSFX)($(OBJX))
	$(LINK)$(LINKFLAGS) /EXE=$(MMS$TARGET) -
	 $(OLBSFX)/INCLUDE=(UNZIP $(INCL_BZIP2_M))/LIBRARY$(NOSHARE_OPTS), -
	 $(LIB_BZIP2_OPTS) -
	 sys$disk:[.vms]unzipsfx.opt/OPT

$(OPTFILE) :
	@ open/write tmp $(MMS$TARGET)
	@ write tmp "SYS$SHARE:VAXCRTL.EXE/SHARE"
	@ close tmp

$(UNZHELP_CLI_RNH) : [.vms]unzip_cli.help
	@ set default [.vms]
	edit/tpu/nosection/nodisplay/command=cvthelp.tpu unzip_cli.help
	@ set default [-]

$(UNZX_UNX).hlp : $(UNZHELP_UNX_RNH)
	runoff /out=$@ $<

$(UNZX_CLI).hlp : $(UNZHELP_CLI_RNH)

clean.com :
	@ open/write tmp $(MMS$TARGET)
	@ write tmp "$!"
	@ write tmp "$!	Clean.com --	procedure to delete files. It always returns success"
	@ write tmp "$!			status despite any error or warnings. Also it extracts"
	@ write tmp "$!			filename from MMS ""module=file"" format."
	@ write tmp "$!"
	@ write tmp "$ on control_y then goto ctly"
	@ write tmp "$ if p1.eqs."""" then exit 1"
	@ write tmp "$ i = -1"
	@ write tmp "$scan_list:"
	@ write tmp "$	i = i+1"
	@ write tmp "$	item = f$elem(i,"","",p1)"
	@ write tmp "$	if item.eqs."""" then goto scan_list"
	@ write tmp "$	if item.eqs."","" then goto done		! End of list"
	@ write tmp "$	item = f$edit(item,""trim"")		! Clean of blanks"
	@ write tmp "$	wild = f$elem(1,""="",item)"
	@ write tmp "$	show sym wild"
	@ write tmp "$	if wild.eqs.""="" then wild = f$elem(0,""="",item)"
	@ write tmp "$	vers = f$parse(wild,,,""version"",""syntax_only"")"
	@ write tmp "$	if vers.eqs."";"" then wild = wild - "";"" + "";*"""
	@ write tmp "$scan:"
	@ write tmp "$		f = f$search(wild)"
	@ write tmp "$		if f.eqs."""" then goto scan_list"
	@ write tmp "$		on error then goto err"
	@ write tmp "$		on warning then goto warn"
	@ write tmp "$		delete/log 'f'"
	@ write tmp "$warn:"
	@ write tmp "$err:"
	@ write tmp "$		goto scan"
	@ write tmp "$done:"
	@ write tmp "$ctly:"
	@ write tmp "$	exit 1"
	@ close tmp

clean : clean.com
	@clean "$(OBJM)"
	@clean "$(COMMON_OBJS1)"
	@clean "$(COMMON_OBJS2)"
	@clean "$(OBJSCLI)"
	@clean "$(COMMON_OBJX1)"
	@clean "$(COMMON_OBJX2)"
	@clean "$(OBJXCLI)"
	@clean "$(OPTFILE)"
	@clean "$(OLBUNZ),$(OLBCLI),$(OLBSFX),$(OLBSXC)"
	@clean "$(UNZIPS)"
	@clean "$(UNZIPS_NOSHARE)"
	@clean "$(UNZHELP_CLI_RNH)"
	@clean "$(UNZIPHELPS)"
	- delete/noconfirm/nolog clean.com;*

crc32$(O)		: crc32.c $(UNZIP_H) zip.h crc32.h
crypt$(O)		: crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
envargs$(O)		: envargs.c $(UNZIP_H)
explode$(O)		: explode.c $(UNZIP_H)
extract$(O)		: extract.c $(UNZIP_H) crc32.h crypt.h
fileio$(O)		: fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
globals$(O)		: globals.c $(UNZIP_H)
inflate$(O)		: inflate.c inflate.h $(UNZIP_H)
list$(O)		: list.c $(UNZIP_H)
match$(O)		: match.c $(UNZIP_H)
process$(O)		: process.c $(UNZIP_H) crc32.h
ttyio$(O)		: ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
ubz2err$(O)		: ubz2err.c $(UNZIP_H)
unreduce$(O)		: unreduce.c $(UNZIP_H)
unshrink$(O)		: unshrink.c $(UNZIP_H)
unzip$(O)		: unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
zipinfo$(O)		: zipinfo.c $(UNZIP_H)

unzipcli$(O)		: unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) $(CFLAGS_CLI) /OBJ=$(MMS$TARGET) $(MMS$SOURCE)

cmdline$(O)		: [.vms]cmdline.c $(UNZIP_H) unzvers.h
	$(CC) $(CFLAGS_CLI) /OBJ=$(MMS$TARGET) $(MMS$SOURCE)

unz_cli$(O)		: [.vms]unz_cli.cld


cmdline_$(O)		: [.vms]cmdline.c $(UNZIP_H) unzvers.h
	$(CC) $(CFLAGS_SXC) /OBJ=$(MMS$TARGET) [.vms]cmdline.c

crc32_$(O)		: crc32.c $(UNZIP_H) zip.h crc32.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) crc32.c

crypt_$(O)		: crypt.c $(UNZIP_H) zip.h crypt.h crc32.h ttyio.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) crypt.c

extract_$(O)		: extract.c $(UNZIP_H) crc32.h crypt.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) extract.c

fileio_$(O)		: fileio.c $(UNZIP_H) crc32.h crypt.h ttyio.h ebcdic.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) fileio.c

globals_$(O)		: globals.c $(UNZIP_H)
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) globals.c

inflate_$(O)		: inflate.c inflate.h $(UNZIP_H)
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) inflate.c

match_$(O)		: match.c $(UNZIP_H)
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) match.c

process_$(O)		: process.c $(UNZIP_H) crc32.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) process.c

ttyio_$(O)		: ttyio.c $(UNZIP_H) zip.h crypt.h ttyio.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) ttyio.c

ubz2err_$(O)		: ubz2err.c $(UNZIP_H)
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) ubz2err.c

unzipsfx$(O)		: unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) unzip.c

unzsxcli$(O)		: unzip.c $(UNZIP_H) crypt.h unzvers.h consts.h
	$(CC) $(CFLAGS_SXC) /OBJ=$(MMS$TARGET) unzip.c

vms$(O)			: [.vms]vms.c [.vms]vms.h [.vms]vmsdefs.h $(UNZIP_H)
!	@ x = ""
!	@ if f$search("SYS$LIBRARY:SYS$LIB_C.TLB").nes."" then x = "+SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY"
	$(CC) $(CFLAGS_ALL) /OBJ=$(MMS$TARGET) [.vms]vms.c

vms_$(O)		: [.vms]vms.c [.vms]vms.h [.vms]vmsdefs.h $(UNZIP_H)
!	@ x = ""
!	@ if f$search("SYS$LIBRARY:SYS$LIB_C.TLB").nes."" then x = "+SYS$LIBRARY:SYS$LIB_C.TLB/LIBRARY"
	$(CC) $(CFLAGS_SFX) /OBJ=$(MMS$TARGET) [.vms]vms.c
