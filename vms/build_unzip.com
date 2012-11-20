$! BUILD_UNZIP.COM
$!
$!     Build procedure for VMS versions of UnZip/ZipInfo and UnZipSFX.
$!
$!     Last revised:  2012-11-19  SMS.
$!
$!     Command arguments:
$!     - Suppress C compilation (re-link): "NOCOMPILE"
$!     - Suppress linking executables: "NOLINK"
$!     - Suppress help file processing: "NOHELP"
$!     - Suppress message file processing: "NOMSG"
$!     - Define DCL symbols: "SYMBOLS"  (Was default before UnZip 6.1.)
$!     - Select compiler environment: "VAXC", "DECC", "GNUC"
$!     - Select AES_WG encryption support: "AES_WG"
$!       By default, the SFX programs are built without AES_WG support.
$!       Add "AES_WG_SFX=1" to the LOCAL_UNZIP C macros to enable it.
$!       (See LOCAL_UNZIP, below.)
$!     - Select BZIP2 support: "USEBZ2"
$!       This option is a shortcut for "IZ_BZIP2=SYS$DISK:[.bzip2]", and
$!       runs the DCL build procedure there,
$!     - Select BZIP2 support: "IZ_BZIP2=dev:[dir]", where "dev:[dir]"
$!       (or a suitable logical name) tells where to find "bzlib.h".
$!       The BZIP2 object library (LIBBZ2_NS.OLB) is expected to be in
$!       a "[.dest]" directory under that one ("dev:[dir.ALPHAL]", for
$!       example), or in that directory itself.
$!       By default, the SFX programs are built without BZIP2 support.
$!       Add "BZIP2_SFX=1" to the LOCAL_UNZIP C macros to enable it.
$!       (See LOCAL_UNZIP, below.)
$!     - Select LZMA compression support: "LZMA"
$!       By default, the SFX programs are built without LZMA support.
$!       Add "LZMA_SFX=1" to the LOCAL_UNZIP C macros to enable it.
$!       (See LOCAL_UNZIP, below.)
$!     - Select PPMd compression support: "PPMD"
$!       By default, the SFX programs are built without PPMd support.
$!       Add "PPMD_SFX=1" to the LOCAL_UNZIP C macros to enable it.
$!       (See LOCAL_UNZIP, below.)
$!     - Use ZLIB compression library: "IZ_ZLIB=dev:[dir]", where
$!       "dev:[dir]" (or a suitable logical name) tells where to find
$!       "zlib.h".  The ZLIB object library (LIBZ.OLB) is expected to be
$!       in a "[.dest]" directory under that one ("dev:[dir.ALPHAL]",
$!       for example), or in that directory itself.
$!     - Select large-file support: "LARGE"
$!     - Select compiler listings: "LIST"  Note that the whole argument
$!       is added to the compiler command, so more elaborate options
$!       like "LIST/SHOW=ALL" (quoted or space-free) may be specified.
$!     - Supply additional compiler options: "CCOPTS=xxx"  Allows the
$!       user to add compiler command options like /ARCHITECTURE or
$!       /[NO]OPTIMIZE.  For example, CCOPTS=/ARCH=HOST/OPTI=TUNE=HOST
$!       or CCOPTS=/DEBUG/NOOPTI.  These options must be quoted or
$!       space-free.
$!     - Supply additional linker options: "LINKOPTS=xxx"  Allows the
$!       user to add linker command options like /DEBUG or /MAP.  For
$!       example: LINKOPTS=/DEBUG or LINKOPTS=/MAP/CROSS.  These options
$!       must be quoted or space-free.  Default is
$!       LINKOPTS=/NOTRACEBACK, but if the user specifies a LINKOPTS
$!       string, /NOTRACEBACK will not be included unless specified by
$!       the user.
$!     - Select installation of CLI interface version of UnZip:
$!       "VMSCLI" or "CLI"
$!     - Force installation of UNIX interface version of UnZip
$!       (override LOCAL_UNZIP environment): "NOVMSCLI" or "NOCLI"
$!     - Build a callable-UnZip library, LIBIZUNZIP.OLB: "LIBUNZIP"
$!     - Choose a destination directory for architecture-specific
$!       product files (.EXE, .OBJ,.OLB, and so on): "PROD=subdir", to
$!       use "[.subdir]".  The default is a name automatically generated
$!       using rules defined below.
$!     - Run basic UnZip tests: "TEST", "TEST_PPMD"
$!     - Create help output text files: "HELP_TEXT"
$!
$!     To specify additional options, define the symbol LOCAL_UNZIP
$!     as a comma-separated list of the C macros to be defined, and
$!     then run BUILD_UNZIP.COM.  For example:
$!
$!             $ LOCAL_UNZIP = "RETURN_CODES"
$!             $ @ [.VMS]BUILD_UNZIP.COM
$!
$!     VMS-specific options include VMSWILD and RETURN_CODES.  See the
$!     INSTALL file for other options (for example, CHECK_VERSIONS).
$!
$!     If you edit this procedure to set LOCAL_UNZIP here, be sure to
$!     use only one "=", to avoid affecting other procedures.
$!
$!     Note: This command procedure always generates both the "default"
$!     UnZip having the UNIX style command interface and the "VMSCLI"
$!     UnZip having the CLI compatible command interface.  There is no
$!     need to add "VMSCLI" to the LOCAL_UNZIP symbol.  (The only effect
$!     of "VMSCLI" now is the selection of the CLI style UnZip
$!     executable in the foreign command definition.)
$!
$!
$ on error then goto error
$ on control_y then goto error
$ OLD_VERIFY = f$verify( 0)
$!
$ edit := edit                  ! override customized edit commands
$ say := write sys$output
$!
$!##################### Read settings from environment ########################
$!
$ if (f$type( LOCAL_UNZIP) .eqs. "")
$ then
$     LOCAL_UNZIP = ""
$ else  ! Trim blanks and append comma if missing
$     LOCAL_UNZIP = f$edit( LOCAL_UNZIP, "TRIM")
$     if (f$extract( (f$length( LOCAL_UNZIP)- 1), 1, LOCAL_UNZIP) .nes. ",")
$     then
$         LOCAL_UNZIP = LOCAL_UNZIP + ", "
$     endif
$ endif
$!
$! Check for the presence of "VMSCLI" in LOCAL_UNZIP.  If yes, we will
$! define the foreign command for "unzip" to use the executable
$! containing the CLI interface.
$!
$ pos_cli = f$locate( "VMSCLI", LOCAL_UNZIP)
$ len_local_unzip = f$length( LOCAL_UNZIP)
$ if (pos_cli .ne. len_local_unzip)
$ then
$     CLI_IS_DEFAULT = 1
$     ! Remove "VMSCLI" macro from LOCAL_UNZIP. The UnZip executable
$     ! including the CLI interface is now created unconditionally.
$     LOCAL_UNZIP = f$extract( 0, pos_cli, LOCAL_UNZIP)+ -
       f$extract( pos_cli+7, len_local_unzip- (pos_cli+ 7), LOCAL_UNZIP)
$ else
$     CLI_IS_DEFAULT = 0
$ endif
$ delete /symbol /local pos_cli
$ delete /symbol /local len_local_unzip
$!
$!##################### Customizing section #############################
$!
$ unzx_unx = "UNZIP"
$ unzx_cli = "UNZIP_CLI"
$ unzsfx_unx = "UNZIPSFX"
$ unzsfx_cli = "UNZIPSFX_CLI"
$ lib_libunzip_name = "LIBIZUNZIP.OLB"
$ lib_unzip_name = "UNZIP.OLB"
$ lib_unzipcli_name = "UNZIPCLI.OLB"
$ lib_unzipsfx_name = "UNZIPSFX.OLB"
$ lib_unzipsfxcli_name = "UNZSFXCLI.OLB"
$!
$ AES_WG = ""
$ CCOPTS = ""
$ IZ_BZIP2 = ""
$ BUILD_BZIP2 = 0
$ IZ_ZLIB = ""
$ LINKOPTS = "/notraceback"
$ LISTING = " /nolist"
$ LARGE_FILE = 0
$ LIBUNZIP = 0
$ LZMA = 0
$ MAKE_EXE = 1
$ MAKE_HELP = 1
$ MAKE_HELP_TEXT = 0
$ MAKE_MSG = 1
$ MAKE_OBJ = 1
$ MAKE_SYM = 0
$ MAY_USE_DECC = 1
$ MAY_USE_GNUC = 0
$ PPMD = 0
$ PROD = ""
$ TEST = 0
$ TEST_PPMD = 0
$!
$! Process command line parameters requesting optional features.
$!
$ arg_cnt = 1
$ argloop:
$     current_arg_name = "P''arg_cnt'"
$     curr_arg = f$edit( 'current_arg_name', "UPCASE")
$     if (curr_arg .eqs. "") then goto argloop_out
$!
$     if (f$extract( 0, 6, curr_arg) .eqs. "AES_WG")
$     then
$         AES_WG = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 5, curr_arg) .eqs. "CCOPT")
$     then
$         opts = f$edit( curr_arg, "COLLAPSE")
$         eq = f$locate( "=", opts)
$         CCOPTS = f$extract( (eq+ 1), 1000, opts)
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 7, curr_arg) .eqs. "IZ_BZIP")
$     then
$         opts = f$edit( curr_arg, "COLLAPSE")
$         eq = f$locate( "=", opts)
$         IZ_BZIP2 = f$extract( (eq+ 1), 1000, opts)
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 6, curr_arg) .eqs. "USEBZ2")
$     then
$         if (IZ_BZIP2 .eqs. "")
$         then
$             IZ_BZIP2 = "SYS$DISK:[.BZIP2]"
$             BUILD_BZIP2 = 1
$         endif
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 9, curr_arg) .eqs. "HELP_TEXT")
$     then
$         MAKE_HELP_TEXT = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 7, curr_arg) .eqs. "IZ_ZLIB")
$     then
$         opts = f$edit( curr_arg, "COLLAPSE")
$         eq = f$locate( "=", opts)
$         IZ_ZLIB = f$extract( (eq+ 1), 1000, opts)
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 5, curr_arg) .eqs. "LARGE")
$     then
$         LARGE_FILE = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 7, curr_arg) .eqs. "LINKOPT")
$     then
$         opts = f$edit( curr_arg, "COLLAPSE")
$         eq = f$locate( "=", opts)
$         LINKOPTS = f$extract( (eq+ 1), 1000, opts)
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "LIST")
$     then
$         LISTING = "/''curr_arg'"      ! But see below for mods.
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 8, curr_arg) .eqs. "LIBUNZIP")
$     then
$         LIBUNZIP = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "LZMA")
$     then
$         LZMA = 1
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "NOCOMPILE")
$     then
$         MAKE_OBJ = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "NOLINK")
$     then
$         MAKE_EXE = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "NOHELP")
$     then
$         MAKE_HELP = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "NOMSG")
$     then
$         MAKE_MSG = 0
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "PPMD")
$     then
$         PPMD = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "PROD")
$     then
$         opts = f$edit( curr_arg, "COLLAPSE")
$         eq = f$locate( "=", opts)
$         PROD = f$extract( (eq+ 1), 1000, opts)
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "SYMBOLS")
$     then
$         MAKE_SYM = 1
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "VAXC")
$     then
$         MAY_USE_DECC = 0
$         MAY_USE_GNUC = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "DECC")
$     then
$         MAY_USE_DECC = 1
$         MAY_USE_GNUC = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "GNUC")
$     then
$         MAY_USE_DECC = 0
$         MAY_USE_GNUC = 1
$         goto argloop_end
$     endif
$!
$     if ((curr_arg .eqs. "VMSCLI") .or. (curr_arg .eqs. "CLI"))
$     then
$         CLI_IS_DEFAULT = 1
$         goto argloop_end
$     endif
$!
$     if ((curr_arg .eqs. "NOVMSCLI") .or. (curr_arg .eqs. "NOCLI"))
$     then
$         CLI_IS_DEFAULT = 0
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "TEST")
$     then
$         TEST = 1
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "TEST_PPMD")
$     then
$         TEST_PPMD = 1
$         goto argloop_end
$     endif
$!
$     say "Unrecognized command-line option: ''curr_arg'"
$     goto error
$!
$     argloop_end:
$     arg_cnt = arg_cnt + 1
$ goto argloop
$ argloop_out:
$!
$ if (CLI_IS_DEFAULT)
$ then
$     UNZEXEC = unzx_cli
$ else
$     UNZEXEC = unzx_unx
$ endif
$!
$!#######################################################################
$!
$! Find out current disk, directory, compiler and options
$!
$ workdir = f$environment( "default")
$ here = f$parse( workdir, , , "device")+ f$parse( workdir, , , "directory")
$!
$! Sense the host architecture (Alpha, Itanium, or VAX).
$!
$ if (f$getsyi( "HW_MODEL") .lt. 1024)
$ then
$     arch = "VAX"
$ else
$     if (f$getsyi( "ARCH_TYPE") .eq. 2)
$     then
$         arch = "ALPHA"
$     else
$         if (f$getsyi( "ARCH_TYPE") .eq. 3)
$         then
$             arch = "IA64"
$         else
$             arch = "unknown_arch"
$         endif
$     endif
$ endif
$!
$ destl = ""
$ destm = arch
$ cmpl = "DEC/Compaq/HP C"
$ opts = ""
$ if (arch .nes. "VAX")
$ then
$     HAVE_DECC_VAX = 0
$     USE_DECC_VAX = 0
$!
$     if (MAY_USE_GNUC)
$     then
$         say "GNU C is not supported for ''arch'."
$         say "You must use DEC/Compaq/HP C to build UnZip."
$         goto error
$     endif
$!
$     if (.not. MAY_USE_DECC)
$     then
$         say "VAX C is not supported for ''arch'."
$         say "You must use DEC/Compaq/HP C to build UnZip."
$         goto error
$     endif
$!
$     cc = "cc /standard = relax /prefix = all /ansi"
$     defs = "''LOCAL_UNZIP'MODERN"
$     if (LARGE_FILE .ne. 0)
$     then
$         defs = "LARGE_FILE_SUPPORT, ''defs'"
$     endif
$ else
$     if (LARGE_FILE .ne. 0)
$     then
$        say "LARGE_FILE_SUPPORT is not available on VAX."
$        LARGE_FILE = 0
$     endif
$     HAVE_DECC_VAX = (f$search( "SYS$SYSTEM:DECC$COMPILER.EXE") .nes. "")
$     HAVE_VAXC_VAX = (f$search( "SYS$SYSTEM:VAXC.EXE") .nes. "")
$     MAY_HAVE_GNUC = (f$trnlnm( "GNU_CC") .nes. "")
$     if (HAVE_DECC_VAX .and. MAY_USE_DECC)
$     then
$         ! We use DECC:
$         USE_DECC_VAX = 1
$         cc = "cc /decc /prefix = all"
$         defs = "''LOCAL_UNZIP'MODERN"
$     else
$         ! We use VAXC (or GNU C):
$         USE_DECC_VAX = 0
$         defs = "''LOCAL_UNZIP'VMS"
$         if ((.not. HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. MAY_USE_GNUC)
$         then
$             cc = "gcc"
$             destm = "''destm'G"
$             cmpl = "GNU C"
$             opts = "GNU_CC:[000000]GCCLIB.OLB /LIBRARY,"
$         else
$             if (HAVE_DECC_VAX)
$             then
$                 cc = "cc /vaxc"
$             else
$                 cc = "cc"
$             endif
$             destm = "''destm'V"
$             cmpl = "VAX C"
$         endif
$         opts = "''opts' SYS$DISK:[.''destm']VAXCSHR.OPT /OPTIONS,"
$     endif
$ endif
$!
$ if (IZ_BZIP2 .nes. "")
$ then
$     defs = "BZIP2_SUPPORT, ''defs'"
$ endif
$!
$! Set AES_WG-related data.
$!
$ if (AES_WG .ne. 0)
$ then
$     defs = defs+ ", CRYPT_AES_WG"
$ endif
$!
$! Set LZMA-related data.
$!
$ if (LZMA .ne. 0)
$ then
$     defs = defs+ ", LZMA_SUPPORT"
$     if (arch .eqs. "VAX")
$     then
$         defs = defs+ ", _SZ_NO_INT_64"
$     endif
$ endif
$!
$! Set PPMD-related data.
$!
$ if (PPMD .ne. 0)
$ then
$     defs = defs+ ", PPMD_SUPPORT"
$     if ((arch .eqs. "VAX") .and. (LZMA .eq. 0))
$     then
$         defs = defs+ ", _SZ_NO_INT_64"
$     endif
$ endif
$!
$! Change the destination directory, if the large-file option is enabled.
$!
$ destb = destm
$ if (LARGE_FILE .ne. 0)
$ then
$     destl = "L"
$ endif
$!
$ dest_std = destm+ destl
$ if (PROD .eqs. "")
$ then
$     dest = dest_std
$ else
$     dest = PROD
$ endif
$!
$ lib_libunzip = "SYS$DISK:[.''dest']''lib_libunzip_name'"
$ lib_unzip = "SYS$DISK:[.''dest']''lib_unzip_name'"
$ lib_unzipcli = "SYS$DISK:[.''dest']''lib_unzipcli_name'"
$ lib_unzipsfx = "SYS$DISK:[.''dest']''lib_unzipsfx_name'"
$ lib_unzipsfxcli = "SYS$DISK:[.''dest']''lib_unzipsfxcli_name'"
$ libunzip_opt = "[.''dest']LIB_IZUNZIP.OPT"
$!
$! If TEST was requested, then run the basic tests (and exit).
$!
$ if (test)
$ then
$     @ [.vms]test_unzip.com "" [.'dest']
$     goto error
$ endif
$!
$! If TEST_PPMD was requested, then run the PPMd tests (and exit).
$!
$ if (test_ppmd)
$ then
$     @ [.vms]test_unzip.com "" [.'dest'] NOSFX
$     goto error
$ endif
$!
$! Reveal the plan.  If compiling, set some compiler options.
$!
$ if (MAKE_OBJ)
$ then
$     say "Compiling on ''arch' using ''cmpl'."
$!
$     DEF_UNX = "/define = (''defs')"
$     DEF_CLI = "/define = (''defs', VMSCLI)"
$     DEF_SXUNX = "/define = (''defs', SFX)"
$     DEF_SXCLI = "/define = (''defs', VMSCLI, SFX)"
$     DEF_LIBUNZIP = "/define = (''defs', DLL)
$ else
$     if (MAKE_EXE .or. MAKE_MSG)
$     then
$         say "Linking on ''arch' for ''cmpl'."
$     endif
$ endif
$!
$! Search directory for BZIP2.
$!
$ if (BUILD_BZIP2)
$ then
$!    Our own BZIP2 directory.
$     seek_bz = destb
$ else
$!    User-specified BZIP2 directory.
$     seek_bz = arch
$ endif
$!
$! Search directory for ZLIB.
$!
$ seek_zl = arch
$!
$! If BZIP2 support was selected, find the header file and object
$! library.  Complain if things fail.
$!
$ cc_incl = "[], [.VMS]"
$ lib_bzip2_opts = ""
$ if (IZ_BZIP2 .nes. "")
$ then
$     bz2_olb = "LIBBZ2_NS.OLB"
$     if (MAKE_OBJ .or. MAKE_EXE)
$     then
$         define incl_bzip2 'IZ_BZIP2'
$         if (BUILD_BZIP2 .and. (IZ_BZIP2 .eqs. "SYS$DISK:[.BZIP2]"))
$         then
$             set def [.BZIP2]
$             @buildbz2.com
$             set def [-]
$         endif
$     endif
$!
$     if (MAKE_EXE)
$     then
$         @ [.VMS]FIND_BZIP2_LIB.COM 'IZ_BZIP2' 'seek_bz' 'bz2_olb' lib_bzip2
$         if (f$trnlnm( "lib_bzip2") .eqs. "")
$         then
$             say "Can't find BZIP2 object library.  Can't link."
$             goto error
$         else
$             lib_bzip2_opts = "LIB_BZIP2:''bz2_olb' /library,"
$         endif
$     endif
$ endif
$!
$! If ZLIB use was selected, find the object library.
$! Complain if things fail.
$!
$ lib_zlib_opts = ""
$ if (IZ_ZLIB .nes. "")
$ then
$     zlib_olb = "LIBZ.OLB"
$     define incl_zlib 'IZ_ZLIB'
$     defs = "''defs', USE_ZLIB"
$     @ [.VMS]FIND_BZIP2_LIB.COM 'IZ_ZLIB' 'seek_zl' 'zlib_olb' lib_zlib
$     if (f$trnlnm( "lib_zlib") .eqs. "")
$     then
$         say "Can't find ZLIB object library.  Can't link."
$         goto error
$     else
$         lib_zlib_opts = "LIB_ZLIB:''zlib_olb' /library, "
$         @ [.VMS]FIND_BZIP2_LIB.COM 'IZ_ZLIB' -
           contrib.infback9 infback9.h'zlib_olb' incl_zlib_contrib_infback9
$     endif
$ endif
$!
$! If [.'dest'] does not exist, either complain (link-only) or make it.
$!
$ if (f$search( "''dest'.DIR;1") .eqs. "")
$ then
$     if (MAKE_OBJ)
$     then
$         create /directory [.'dest']
$     else
$         if (MAKE_EXE .or. MAKE_MSG)
$         then
$             say "Can't find directory ""[.''dest']"".  Can't link."
$             goto error
$         endif
$     endif
$ endif
$!
$ if (MAKE_OBJ)
$ then
$!
$! Arrange to get arch-specific list file placement, if listing, and if
$! the user didn't specify a particular "/LIST =" destination.
$!
$     L = f$edit( LISTING, "COLLAPSE")
$     if ((f$extract( 0, 5, L) .eqs. "/LIST") .and. -
       (f$extract( 4, 1, L) .nes. "="))
$     then
$         LISTING = " /LIST = [.''dest']"+ f$extract( 5, 1000, LISTING)
$     endif
$!
$! Define compiler command.
$!
$     cc = cc+ " /include = (''cc_incl')"+ LISTING+ CCOPTS
$!
$ endif
$!
$! Define linker command.
$!
$ link = "link ''LINKOPTS'"
$!
$! Make a VAXCRTL options file for GNU C or VAC C, if needed.
$!
$ if ((opts .nes. "") .and. -
   (f$locate( "VAXCSHR", f$edit( opts, "UPCASE")) .lt. f$length( opts)) .and. -
   (f$search( "[.''dest']VAXCSHR.OPT") .eqs. ""))
$ then
$     open /write opt_file_ln [.'dest']VAXCSHR.OPT
$     write opt_file_ln "SYS$SHARE:VAXCRTL.EXE /SHARE"
$     close opt_file_ln
$ endif
$!
$! Show interesting facts.
$!
$ say ""
$ say "   architecture = ''arch' (destination = [.''dest'])"
$!
$ if (MAKE_OBJ)
$ then
$     say "   cc = ''cc'"
$ endif
$!
$ if (MAKE_EXE)
$ then
$     say "   link = ''link'"
$ endif
$!
$ if (.not. MAKE_HELP)
$ then
$     say "   Not making new help files."
$ endif
$!
$ if (.not. MAKE_MSG)
$ then
$     say "   Not making new message files."
$ endif
$!
$ if (IZ_BZIP2 .nes. "")
$ then
$     if (MAKE_EXE)
$     then
$         say "   BZIP2 include dir: ''f$trnlnm( "incl_bzip2")'"
$     endif
$     say "   BZIP2 library dir: ''f$trnlnm( "lib_bzip2")'"
$ endif
$ if (IZ_ZLIB .nes. "")
$ then
$     if (MAKE_EXE)
$     then
$         say "   ZLIB include dir:  ''f$trnlnm( "incl_zlib")'"
$     endif
$     say "   ZLIB library dir:  ''f$trnlnm( "lib_zlib")'"
$ endif
$ say ""
$!
$ tmp = f$verify( 1)    ! Turn echo on to see what's happening.
$!
$!------------------------------- UnZip section ------------------------------
$!
$!
$ if (MAKE_HELP)
$ then
$!
$! Process the Unix-style help file.
$!
$     runoff /out = UNZIP.HLP [.VMS]UNZIP_DEF.RNH
$!
$ endif
$!
$ if (MAKE_HELP .and. MAKE_HELP_TEXT)
$ then
$!
$! Make the Unix-style help output text file.
$!
$     help_temp_name = "help_temp_"+ f$getjpi( 0, "PID")
$     if (f$search( help_temp_name+ ".HLB") .nes. "") then -
       delete 'help_temp_name'.HLB;*
$     library /create /help 'help_temp_name'.HLB UNZIP.HLP
$     help /library = sys$disk:[]'help_temp_name'.HLB -
       /output = 'help_temp_name'.OUT unzip...
$     delete 'help_temp_name'.HLB;*
$     create /fdl = [.VMS]STREAM_LF.FDL UNZIP.HTX
$     open /append help_temp UNZIP.HTX
$     copy 'help_temp_name'.OUT help_temp
$     close help_temp
$     delete 'help_temp_name'.OUT;*
$!
$ endif
$!
$ if (MAKE_MSG)
$ then
$!
$! Process the message file.
$!
$     message /object = [.'dest']UNZIP_MSG.OBJ /nosymbols -
       [.VMS]UNZIP_MSG.MSG
$     link /shareable = [.'dest']UNZIP_MSG.EXE [.'dest']UNZIP_MSG.OBJ
$!
$ endif
$!
$ if (MAKE_OBJ)
$ then
$!
$! Compile the primary sources.
$!
$     cc 'DEF_UNX' /object = [.'dest']UNZIP.OBJ UNZIP.C
$     cc 'DEF_UNX' /object = [.'dest']CRC32.OBJ CRC32.C
$     cc 'DEF_UNX' /object = [.'dest']CRYPT.OBJ CRYPT.C
$     cc 'DEF_UNX' /object = [.'dest']ENVARGS.OBJ ENVARGS.C
$     cc 'DEF_UNX' /object = [.'dest']EXPLODE.OBJ EXPLODE.C
$     cc 'DEF_UNX' /object = [.'dest']EXTRACT.OBJ EXTRACT.C
$     cc 'DEF_UNX' /object = [.'dest']FILEIO.OBJ FILEIO.C
$     cc 'DEF_UNX' /object = [.'dest']GLOBALS.OBJ GLOBALS.C
$     cc 'DEF_UNX' /object = [.'dest']INFLATE.OBJ INFLATE.C
$     cc 'DEF_UNX' /object = [.'dest']LIST.OBJ LIST.C
$     cc 'DEF_UNX' /object = [.'dest']MATCH.OBJ MATCH.C
$     cc 'DEF_UNX' /object = [.'dest']PROCESS.OBJ PROCESS.C
$     cc 'DEF_UNX' /object = [.'dest']TTYIO.OBJ TTYIO.C
$     cc 'DEF_UNX' /object = [.'dest']UBZ2ERR.OBJ UBZ2ERR.C
$     cc 'DEF_UNX' /object = [.'dest']UNREDUCE.OBJ UNREDUCE.C
$     cc 'DEF_UNX' /object = [.'dest']UNSHRINK.OBJ UNSHRINK.C
$     cc 'DEF_UNX' /object = [.'dest']ZIPINFO.OBJ ZIPINFO.C
$     cc 'DEF_UNX' /object = [.'dest']VMS.OBJ [.VMS]VMS.C
$!
$     if (LIBUNZIP .ne. 0)
$     then
$!
$! Compile the callable object library sources, DLL/REENTRANT-sensitive.
$!
$         cc 'DEF_LIBUNZIP' /object = [.'dest']API_L.OBJ API.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']CRYPT_L.OBJ CRYPT.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']EXPLODE_L.OBJ EXPLODE.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']EXTRACT_L.OBJ EXTRACT.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']FILEIO_L.OBJ FILEIO.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']GLOBALS_L.OBJ GLOBALS.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']INFLATE_L.OBJ INFLATE.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']LIST_L.OBJ LIST.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']PROCESS_L.OBJ PROCESS.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']TTYIO_L.OBJ TTYIO.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']UBZ2ERR_L.OBJ UBZ2ERR.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']UNSHRINK_L.OBJ UNSHRINK.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']UNZIP_L.OBJ UNZIP.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']ZIPINFO_L.OBJ ZIPINFO.C
$         cc 'DEF_LIBUNZIP' /object = [.'dest']VMS_L.OBJ [.VMS]VMS.C
$     endif
$!
$     if (AES_WG .ne. 0)
$     then
$         cc 'DEF_UNX' /object = [.'dest']AESCRYPT.OBJ [.AES_WG]AESCRYPT.C
$         cc 'DEF_UNX' /object = [.'dest']AESKEY.OBJ [.AES_WG]AESKEY.C
$         cc 'DEF_UNX' /object = [.'dest']AESTAB.OBJ [.AES_WG]AESTAB.C
$         cc 'DEF_UNX' /object = [.'dest']FILEENC.OBJ [.AES_WG]FILEENC.C
$         cc 'DEF_UNX' /object = [.'dest']HMAC.OBJ [.AES_WG]HMAC.C
$         cc 'DEF_UNX' /object = [.'dest']PRNG.OBJ [.AES_WG]PRNG.C
$         cc 'DEF_UNX' /object = [.'dest']PWD2KEY.OBJ [.AES_WG]PWD2KEY.C
$         cc 'DEF_UNX' /object = [.'dest']SHA1.OBJ [.AES_WG]SHA1.C
$     endif
$!
$     if (LZMA .ne. 0)
$     then
$         cc 'DEF_UNX' /object = [.'dest']LZFIND.OBJ [.SZIP]LZFIND.C
$         cc 'DEF_UNX' /object = [.'dest']LZMADEC.OBJ [.SZIP]LZMADEC.C
$     endif
$!
$     if (PPMD .ne. 0)
$     then
$         cc 'DEF_UNX' /object = [.'dest']PPMD8.OBJ [.SZIP]PPMD8.C
$         cc 'DEF_UNX' /object = [.'dest']PPMD8DEC.OBJ [.SZIP]PPMD8DEC.C
$     endif
$!
$     if (LIBUNZIP .ne. 0)
$     then
$!
$! Create the callable library link options file.
$!
$         def_dev_dir_orig = f$environment( "default")
$         set default [.'dest']
$         def_dev_dir = f$environment( "default")
$         set default 'def_dev_dir_orig'
$         create /fdl = [.VMS]STREAM_LF.FDL 'libunzip_opt'
$         open /append opt_file_lib 'libunzip_opt'
$         write opt_file_lib "! DEFINE LIB_IZUNZIP ''def_dev_dir'"
$         if (IZ_BZIP2 .nes. "")
$         then
$             write opt_file_lib "! DEFINE LIB_BZIP2 ''f$trnlnm( "lib_bzip2")'"
$         endif
$         if (IZ_ZLIB .nes. "")
$         then
$             write opt_file_lib "! DEFINE LIB_ZLIB ''f$trnlnm( "lib_zlib")'"
$         endif
$         write opt_file_lib "LIB_IZUNZIP:''lib_unzip_name' /library"
$         if (IZ_BZIP2 .nes. "")
$         then
$             write opt_file_lib "''lib_bzip2_opts'" - ", " - ","
$         endif
$         write opt_file_lib "LIB_IZUNZIP:''lib_unzip_name' /library"
$         if (IZ_ZLIB .nes. "")
$         then
$             write opt_file_lib "''libunzip_opt'" - ", " - ","
$         endif
$         close opt_file_lib
$!
$     endif
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the primary object library.
$!
$     if (f$search( lib_unzip) .eqs. "") then -
       libr /object /create 'lib_unzip'
$!
$     libr /object /replace 'lib_unzip' -
       [.'dest']CRC32.OBJ, -
       [.'dest']CRYPT.OBJ, -
       [.'dest']ENVARGS.OBJ, -
       [.'dest']EXPLODE.OBJ, -
       [.'dest']EXTRACT.OBJ, -
       [.'dest']FILEIO.OBJ, -
       [.'dest']GLOBALS.OBJ, -
       [.'dest']INFLATE.OBJ, -
       [.'dest']LIST.OBJ, -
       [.'dest']MATCH.OBJ, -
       [.'dest']PROCESS.OBJ, -
       [.'dest']TTYIO.OBJ, -
       [.'dest']UBZ2ERR.OBJ, -
       [.'dest']UNREDUCE.OBJ, -
       [.'dest']UNSHRINK.OBJ, -
       [.'dest']ZIPINFO.OBJ, -
       [.'dest']VMS.OBJ
$!
$     if (AES_WG .ne. 0)
$     then
$         libr /object /replace 'lib_unzip' -
           [.'dest']AESCRYPT.OBJ, -
           [.'dest']AESKEY.OBJ, -
           [.'dest']AESTAB.OBJ, -
           [.'dest']FILEENC.OBJ, -
           [.'dest']HMAC.OBJ, -
           [.'dest']PRNG.OBJ, -
           [.'dest']PWD2KEY.OBJ, -
           [.'dest']SHA1.OBJ
$     endif
$!
$     if (LZMA .ne. 0)
$     then
$         libr /object /replace 'lib_unzip' -
           [.'dest']LZFIND.OBJ, -
           [.'dest']LZMADEC.OBJ
$     endif
$!
$     if (PPMD .ne. 0)
$     then
$         libr /object /replace 'lib_unzip' -
           [.'dest']PPMD8.OBJ, -
           [.'dest']PPMD8DEC.OBJ
$     endif
$!
$! Create the callable UnZip object library and link options file.
$!
$     if (LIBUNZIP .ne. 0)
$     then
$!
$         if (f$search( lib_libunzip) .eqs. "") then -
           libr /object /create 'lib_libunzip'
$!
$! Modules sensitive to DLL/REENTRANT.
$!
$         libr /object /replace 'lib_libunzip' -
           [.'dest']API_L.OBJ, -
           [.'dest']CRYPT_L.OBJ, -
           [.'dest']EXPLODE_L.OBJ, -
           [.'dest']EXTRACT_L.OBJ, -
           [.'dest']FILEIO_L.OBJ, -
           [.'dest']GLOBALS_L.OBJ, -
           [.'dest']INFLATE_L.OBJ, -
           [.'dest']LIST_L.OBJ, -
           [.'dest']PROCESS_L.OBJ, -
           [.'dest']TTYIO_L.OBJ, -
           [.'dest']UBZ2ERR_L.OBJ, -
           [.'dest']UNSHRINK_L.OBJ, -
           [.'dest']ZIPINFO_L.OBJ, -
           [.'dest']VMS_L.OBJ
$!
$! Modules insensitive to DLL/REENTRANT.
$!
$         libr /object /replace 'lib_libunzip' -
           [.'dest']CRC32.OBJ, -
           [.'dest']ENVARGS.OBJ, -
           [.'dest']MATCH.OBJ, -
           [.'dest']UNREDUCE.OBJ
$!
$         if (AES_WG .ne. 0)
$         then
$             libr /object /replace 'lib_libunzip' -
               [.'dest']AESCRYPT.OBJ, -
               [.'dest']AESKEY.OBJ, -
               [.'dest']AESTAB.OBJ, -
               [.'dest']FILEENC.OBJ, -
               [.'dest']HMAC.OBJ, -
               [.'dest']PRNG.OBJ, -
               [.'dest']PWD2KEY.OBJ, -
               [.'dest']SHA1.OBJ
$         endif
$!
$         if (LZMA .ne. 0)
$         then
$             libr /object /replace 'lib_libunzip' -
               [.'dest']LZFIND.OBJ, -
               [.'dest']LZMADEC.OBJ
$         endif
$!
$         if (PPMD .ne. 0)
$         then
$             libr /object /replace 'lib_libunzip' -
               [.'dest']PPMD8.OBJ, -
               [.'dest']PPMD8DEC.OBJ
$         endif
$!
$     endif
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the module ID options file.
$!
$     optgen_verify = f$verify( 0)
$     @ [.vms]optgen.com UnZip iz_unzip_versn
$     open /write opt_file_ln SYS$DISK:[.'dest']UNZIP.OPT
$     write opt_file_ln "Ident = ""UnZip ''f$trnlnm( "iz_unzip_versn")'"""
$     close opt_file_ln
$     deassign iz_unzip_versn
$     tmp = f$verify( optgen_verify)
$!
$! Link the executable.
$!
$     link /executable = [.'dest']'unzx_unx'.EXE -
       SYS$DISK:[.'dest']UNZIP.OBJ, -
       'lib_unzip' /library, -
       'lib_bzip2_opts' -
       'lib_unzip' /library, -
       'lib_zlib_opts' -
       'opts' -
       SYS$DISK:[.'dest']UNZIP.OPT /options
$!
$ endif
$!
$!----------------------- UnZip (CLI interface) section ----------------------
$!
$! Process the CLI help file, if desired.
$!
$ if (MAKE_HELP)
$ then
$     set default [.VMS]
$     edit /tpu /nosection /nodisplay /command = CVTHELP.TPU -
       UNZIP_CLI.HELP
$     set default [-]
$     runoff /output = UNZIP_CLI.HLP [.VMS]UNZIP_CLI.RNH
$ endif
$!
$! Make the CLI help output text file, if desired.
$!
$ if (MAKE_HELP .and. MAKE_HELP_TEXT)
$ then
$     help_temp_name = "help_temp_"+ f$getjpi( 0, "PID")
$     if (f$search( help_temp_name+ ".HLB") .nes. "") then -
       delete 'help_temp_name'.HLB;*
$     library /create /help 'help_temp_name'.HLB UNZIP_CLI.HLP
$     help /library = sys$disk:[]'help_temp_name'.HLB -
       /output = 'help_temp_name'.OUT unzip...
$     delete 'help_temp_name'.HLB;*
$     create /fdl = [.VMS]STREAM_LF.FDL UNZIP_CLI.HTX
$     open /append help_temp UNZIP_CLI.HTX
$     copy 'help_temp_name'.OUT help_temp
$     close help_temp
$     delete 'help_temp_name'.OUT;*
$ endif
$!
$ if (MAKE_OBJ)
$ then
$!
$! Compile the CLI sources.
$!
$     cc 'DEF_CLI' /object = [.'dest']UNZIPCLI.OBJ UNZIP.C
$     cc 'DEF_CLI' /object = [.'dest']ZIPINFO_C.OBJ ZIPINFO.C
$     cc 'DEF_CLI' /object = [.'dest']CMDLINE.OBJ -
       [.VMS]CMDLINE.C
$!
$! Create the command definition object file.
$!
$     cppcld_verify = f$verify( 0)
$     @ [.vms]cppcld.com "''cc'" [.VMS]UNZ_CLI.CLD -
       [.'dest']UNZ_CLI.CLD "''defs'"
$     tmp = f$verify( cppcld_verify)
$     set command /object = [.'dest']UNZ_CLI.OBJ [.'dest']UNZ_CLI.CLD
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the CLI object library.
$!
$     if (f$search( lib_unzipcli) .eqs. "") then -
       libr /object /create 'lib_unzipcli'
$!
$     libr /object /replace 'lib_unzipcli' -
       [.'dest']CMDLINE.OBJ, -
       [.'dest']ZIPINFO_C.OBJ, -
       [.'dest']UNZ_CLI.OBJ
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Link the CLI executable.
$!
$     link /executable = [.'dest']'unzx_cli'.EXE -
       SYS$DISK:[.'dest']UNZIPCLI.OBJ, -
       'lib_unzipcli' /library, -
       'lib_unzip' /library, -
       'lib_bzip2_opts' -
       'lib_unzip' /library, -
       'lib_zlib_opts' -
       'opts' -
       SYS$DISK:[.'dest']UNZIP.OPT /options
$!
$ endif
$!
$!-------------------------- UnZipSFX section --------------------------------
$!
$ if (MAKE_OBJ)
$ then
$!
$! Compile the variant SFX sources.
$!
$     cc 'DEF_SXUNX' /object = [.'dest']UNZIPSFX.OBJ UNZIP.C
$     cc 'DEF_SXUNX' /object = [.'dest']CRC32_.OBJ CRC32.C
$     cc 'DEF_SXUNX' /object = [.'dest']CRYPT_.OBJ CRYPT.C
$     cc 'DEF_SXUNX' /object = [.'dest']EXTRACT_.OBJ EXTRACT.C
$     cc 'DEF_SXUNX' /object = [.'dest']FILEIO_.OBJ FILEIO.C
$     cc 'DEF_SXUNX' /object = [.'dest']GLOBALS_.OBJ GLOBALS.C
$     cc 'DEF_SXUNX' /object = [.'dest']INFLATE_.OBJ INFLATE.C
$     cc 'DEF_SXUNX' /object = [.'dest']MATCH_.OBJ MATCH.C
$     cc 'DEF_SXUNX' /object = [.'dest']PROCESS_.OBJ PROCESS.C
$     cc 'DEF_SXUNX' /object = [.'dest']TTYIO_.OBJ TTYIO.C
$     cc 'DEF_SXUNX' /object = [.'dest']UBZ2ERR_.OBJ UBZ2ERR.C
$     cc 'DEF_SXUNX' /object = [.'dest']VMS_.OBJ [.VMS]VMS.C
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the SFX object library.
$!
$     if (f$search( lib_unzipsfx) .eqs. "") then -
       libr /object /create 'lib_unzipsfx'
$!
$     libr /object /replace 'lib_unzipsfx' -
       [.'dest']CRC32_.OBJ, -
       [.'dest']CRYPT_.OBJ, -
       [.'dest']EXTRACT_.OBJ, -
       [.'dest']FILEIO_.OBJ, -
       [.'dest']GLOBALS_.OBJ, -
       [.'dest']INFLATE_.OBJ, -
       [.'dest']MATCH_.OBJ, -
       [.'dest']PROCESS_.OBJ, -
       [.'dest']TTYIO_.OBJ, -
       [.'dest']UBZ2ERR_.OBJ, -
       [.'dest']VMS_.OBJ
$!
$     if (AES_WG .ne. 0)
$     then
$         libr /object /replace 'lib_unzipsfx' -
           [.'dest']AESCRYPT.OBJ, -
           [.'dest']AESKEY.OBJ, -
           [.'dest']AESTAB.OBJ, -
           [.'dest']FILEENC.OBJ, -
           [.'dest']HMAC.OBJ, -
           [.'dest']PRNG.OBJ, -
           [.'dest']PWD2KEY.OBJ, -
           [.'dest']SHA1.OBJ
$     endif
$!
$     if (LZMA .ne. 0)
$     then
$         libr /object /replace 'lib_unzipsfx' -
           [.'dest']LZFIND.OBJ, -
           [.'dest']LZMADEC.OBJ
$     endif
$!
$     if (PPMD .ne. 0)
$     then
$         libr /object /replace 'lib_unzipsfx' -
           [.'dest']PPMD8.OBJ, -
           [.'dest']PPMD8DEC.OBJ
$     endif
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the program ID options file.
$!
$     optgen_verify = f$verify( 0)
$     @ [.vms]optgen.com UnZip iz_unzip_versn
$     open /write opt_file_ln SYS$DISK:[.'dest']UNZIPSFX.OPT
$     write opt_file_ln "Ident = ""UnZipSFX ''f$trnlnm( "iz_unzip_versn")'"""
$     close opt_file_ln
$     deassign iz_unzip_versn
$     tmp = f$verify( optgen_verify)
$!
$! Link the SFX executable.
$!
$     link /executable = [.'dest']'unzsfx_unx'.EXE -
       SYS$DISK:[.'dest']UNZIPSFX.OBJ, -
       'lib_unzipsfx' /library, -
       'lib_bzip2_opts' -
       'lib_unzipsfx' /library, -
       'lib_zlib_opts' -
       'opts' -
       SYS$DISK:[.'dest']UNZIPSFX.OPT /options
$!
$ endif
$!
$!--------------------- UnZipSFX (CLI interface) section ---------------------
$!
$ if (MAKE_OBJ)
$ then
$!
$! Compile the SFX CLI sources.
$!
$     cc 'DEF_SXCLI' /object = [.'dest']UNZSXCLI.OBJ UNZIP.C
$     cc 'DEF_SXCLI' /object = [.'dest']CMDLINE_.OBJ -
       [.VMS]CMDLINE.C
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Create the SFX CLI object library.
$!
$     if (f$search( lib_unzipsfxcli) .eqs. "") then -
       libr /object /create 'lib_unzipsfxcli'
$!
$     libr /object /replace 'lib_unzipsfxcli' -
       [.'dest']CMDLINE_.OBJ, -
       [.'dest']UNZ_CLI.OBJ
$!
$ endif
$!
$ if (MAKE_EXE)
$ then
$!
$! Link the SFX CLI executable.
$!
$     link /executable = [.'dest']'unzsfx_cli'.EXE -
       SYS$DISK:[.'dest']UNZSXCLI.OBJ, -
       'lib_unzipsfxcli' /library, -
       'lib_unzipsfx' /library, -
       'lib_bzip2_opts' -
       'lib_unzipsfx' /library, -
       'lib_zlib_opts' -
       'opts' -
       SYS$DISK:[.'dest']UNZIPSFX.OPT /options
$!
$ endif
$!
$!----------------------------- Symbols section ------------------------------
$!
$ if (MAKE_SYM)
$ then
$!
$     there = here- "]"+ ".''dest']"
$!
$!    Define the foreign command symbols.  Similar commands may be useful
$!    in SYS$MANAGER:SYLOGIN.COM and/or users' LOGIN.COM.
$!
$     unzip   == "$''there'''unzexec'.EXE"
$     zipinfo == "$''there'''unzexec'.EXE ""-Z"""
$!
$ endif
$!
$! Deassign the temporary process logical names, restore the original
$! default directory, and restore the DCL verify status.
$!
$ error:
$!
$ if (f$trnlnm( "iz_unzip_versn", "LNM$PROCESS_TABLE") .nes. "")
$ then
$     deassign iz_unzip_versn
$ endif
$!
$ if (IZ_BZIP2 .nes. "")
$ then
$     if (f$trnlnm( "incl_bzip2", "LNM$PROCESS_TABLE") .nes. "")
$     then
$         deassign incl_bzip2
$     endif
$     if (f$trnlnm( "lib_bzip2", "LNM$PROCESS_TABLE") .nes. "")
$     then
$         deassign lib_bzip2
$     endif
$ endif
$!
$ if (IZ_ZLIB .nes. "")
$ then
$     if (f$trnlnm( "incl_zlib", "LNM$PROCESS_TABLE") .nes. "")
$     then
$         deassign incl_zlib
$     endif
$     if (f$trnlnm( "incl_zlib_contrib_infback9", -
       "LNM$PROCESS_TABLE") .nes. "")
$     then
$         deassign incl_zlib_contrib_infback9
$     endif
$     if (f$trnlnm( "lib_zlib", "LNM$PROCESS_TABLE") .nes. "")
$     then
$         deassign lib_zlib
$     endif
$ endif
$!
$ if (f$type( here) .nes. "")
$ then
$     if (here .nes. "")
$     then
$         set default 'here'
$     endif
$ endif
$!
$ if (f$type( OLD_VERIFY) .nes. "")
$ then
$     tmp = f$verify( OLD_VERIFY)
$ endif
$!
$ exit
$!
