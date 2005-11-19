$! BUILD_UNZIP.COM
$!
$!     Build procedure for VMS versions of UnZip/ZipInfo and UnZipSFX
$!
$!     Last revised:  2005-11-17  SMS.
$!
$!     Command args:
$!     - suppress help file processing: "NOHELP"
$!     - select link-only: "LINK"
$!     - select compiler environment: "VAXC", "DECC", "GNUC"
$!     - select BZIP2 support: "IZ_BZIP2=dev:[dir]", where "dev:[dir]"
$!       (or a suitable logical name) tells where to find "bzlib.h".
$!       The BZIP2 object library (LIBBZ2.OLB) is expected to be in a
$!       "[.dest]" directory under that one ("dev:[dir.ALPHAL]", for
$!       example), or in that directory itself.
$!       By default, the SFX programs are built without BZIP2 support.
$!       Add "BZIP2_SFX=1" to the LOCAL_UNZIP C macros to enable it.
$!       (See LOCAL_UNZIP, below.)
$!     - select large-file support: "LARGE"
$!     - select compiler listings: "LIST"  Note that the whole argument
$!       is added to the compiler command, so more elaborate options
$!       like "LIST/SHOW=ALL" (quoted or space-free) may be specified.
$!     - supply additional compiler options: "CCOPTS=xxx"  Allows the
$!       user to add compiler command options like /ARCHITECTURE or
$!       /[NO]OPTIMIZE.  For example, CCOPTS=/ARCH=HOST/OPTI=TUNE=HOST
$!       or CCOPTS=/DEBUG/NOOPTI.  These options must be quoted or
$!       space-free.
$!     - supply additional linker options: "LINKOPTS=xxx"  Allows the
$!       user to add linker command options like /DEBUG or /MAP.  For
$!       example: LINKOPTS=/DEBUG or LINKOPTS=/MAP/CROSS.  These options
$!       must be quoted or space-free.  Default is
$!       LINKOPTS=/NOTRACEBACK, but if the user specifies a LINKOPTS
$!       string, /NOTRACEBACK will not be included unless specified by
$!       the user.
$!     - select installation of CLI interface version of unzip:
$!       "VMSCLI" or "CLI"
$!     - force installation of UNIX interface version of unzip
$!       (override LOCAL_UNZIP environment): "NOVMSCLI" or "NOCLI"
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
$     local_unzip = ""
$ else  ! Trim blanks and append comma if missing
$     local_unzip = f$edit( local_unzip, "TRIM")
$     if (f$extract( (f$length( local_unzip)- 1), 1, local_unzip) .nes. ",")
$     then
$         local_unzip = local_unzip + ", "
$     endif
$ endif
$!
$! Check for the presence of "VMSCLI" in local_unzip.  If yes, we will
$! define the foreign command for "unzip" to use the executable
$! containing the CLI interface.
$!
$ pos_cli = f$locate( "VMSCLI", local_unzip)
$ len_local_unzip = f$length( local_unzip)
$ if (pos_cli .ne. len_local_unzip)
$ then
$     CLI_IS_DEFAULT = 1
$     ! Remove "VMSCLI" macro from local_unzip. The UnZip executable
$     ! including the CLI interface is now created unconditionally.
$     local_unzip = f$extract( 0, pos_cli, local_unzip)+ -
       f$extract( pos_cli+7, len_local_unzip- (pos_cli+ 7), local_unzip)
$ else
$     CLI_IS_DEFAULT = 0
$ endif
$ delete /symbol /local pos_cli
$ delete /symbol /local len_local_unzip
$!
$!##################### Customizing section #############################
$!
$ unzx_unx = "unzip"
$ unzx_cli = "unzip_cli"
$ unzsfx_unx = "unzipsfx"
$ unzsfx_cli = "unzipsfx_cli"
$!
$ CCOPTS = ""
$ IZ_BZIP2 = ""
$ LINKOPTS = "/notraceback"
$ LINK_ONLY = 0
$ LISTING = " /nolist"
$ LARGE_FILE = 0
$ MAKE_HELP = 1
$ MAY_USE_DECC = 1
$ MAY_USE_GNUC = 0
$!
$! Process command line parameters requesting optional features.
$!
$ arg_cnt = 1
$ argloop:
$     current_arg_name = "P''arg_cnt'"
$     curr_arg = f$edit( 'current_arg_name', "UPCASE")
$     if (curr_arg .eqs. "") then goto argloop_out
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
$! Note: LINK test must follow LINKOPTS test.
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "LINK")
$     then
$         LINK_ONLY = 1
$         goto argloop_end
$     endif
$!
$     if (f$extract( 0, 4, curr_arg) .eqs. "LIST")
$     then
$         LISTING = "/''curr_arg'"      ! But see below for mods.
$         goto argloop_end
$     endif
$!
$     if (curr_arg .eqs. "NOHELP")
$     then
$         MAKE_HELP = 0
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
$ dest = arch
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
$     defs = "''local_unzip'MODERN"
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
$         defs = "''local_unzip'MODERN"
$     else
$         ! We use VAXC (or GNU C):
$         USE_DECC_VAX = 0
$         defs = "''local_unzip'VMS"
$         if ((.not. HAVE_VAXC_VAX .and. MAY_HAVE_GNUC) .or. MAY_USE_GNUC)
$         then
$             cc = "gcc"
$             dest = "''dest'G"
$             cmpl = "GNU C"
$             opts = "GNU_CC:[000000]GCCLIB.OLB /LIBRARY,"
$         else
$             if (HAVE_DECC_VAX)
$             then
$                 cc = "cc /vaxc"
$             else
$                 cc = "cc"
$             endif
$             dest = "''dest'V"
$             cmpl = "VAC C"
$         endif
$         opts = "''opts' SYS$DISK:[.''dest']VAXCSHR.OPT /OPTIONS,"
$     endif
$ endif
$!
$ if (IZ_BZIP2 .nes. "")
$ then
$     defs = "USE_BZIP2, ''defs'"
$ endif
$!
$! Reveal the plan.  If compiling, set some compiler options.
$!
$ if (LINK_ONLY)
$ then
$     say "Linking on ''arch' for ''cmpl'."
$ else
$     say "Compiling on ''arch' using ''cmpl'."
$!
$     DEF_UNX = "/define = (''defs')"
$     DEF_CLI = "/define = (''defs', VMSCLI)"
$     DEF_SXUNX = "/define = (''defs', SFX)"
$     DEF_SXCLI = "/define = (''defs', VMSCLI, SFX)"
$ endif
$!
$! Change the destination directory, if the large-file option is enabled.
$!
$ if (LARGE_FILE .ne. 0)
$ then
$     dest = "''dest'L"
$ endif
$!
$! If BZIP2 support was selected, find the header file and object
$! library.  Complain if things fail.
$!
$ lib_bzip2_opts = ""
$ if (IZ_BZIP2 .nes. "")
$ then
$     if (.not. LINK_ONLY)
$     then
$         define incl_bzip2 'IZ_BZIP2'
$     endif
$!
$     @ [.vms]find_bzip2_lib.com 'IZ_BZIP2' 'dest' lib_bzip2
$     if (f$trnlnm( "lib_bzip2") .eqs. "")
$     then
$         say "Can't find BZIP2 object library.  Can't link."
$         goto error
$     else
$         lib_bzip2_opts = "lib_bzip2:libbz2.olb /library,"
$     endif
$ endif
$!
$! If [.'dest'] does not exist, either complain (link-only) or make it.
$!
$ if (f$search( "''dest'.dir;1") .eqs. "")
$ then
$     if (LINK_ONLY)
$     then
$         say "Can't find directory ""[.''dest']"".  Can't link."
$         goto error
$     else
$         create /directory [.'dest']
$     endif
$ endif
$!
$ if (.not. LINK_ONLY)
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
$     cc = cc+ " /include = ([], [.vms])"+ LISTING+ CCOPTS
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
   (f$search( "[.''dest']vaxcshr.opt") .eqs. ""))
$ then
$     open /write opt_file_ln [.'dest']vaxcshr.opt
$     write opt_file_ln "SYS$SHARE:VAXCRTL.EXE /SHARE"
$     close opt_file_ln
$ endif
$!
$! Show interesting facts.
$!
$ say "   architecture = ''arch' (destination = [.''dest'])"
$!
$ if (IZ_BZIP2 .nes. "")
$ then
$     say "   BZIP2 dir = ''f$trnlnm( "lib_bzip2")'"
$ endif
$!
$ if (.not. LINK_ONLY)
$ then
$     say "   cc = ''cc'"
$ endif
$!
$ say "   link = ''link'"
$!
$ if (.not. MAKE_HELP)
$ then
$     say "   Not making new help files."
$ endif
$!
$ say ""
$!
$ tmp = f$verify( 1)    ! Turn echo on to see what's happening.
$!
$!------------------------------- UnZip section ------------------------------
$!
$ if (.not. LINK_ONLY)
$ then
$!
$! Process the help file, if desired.
$!
$     if (MAKE_HELP)
$     then
$         runoff /out = unzip.hlp [.vms]unzip_def.rnh
$     endif
$!
$! Compile the sources.
$!
$     cc 'DEF_UNX' /object = [.'dest']unzip.obj unzip.c
$     cc 'DEF_UNX' /object = [.'dest']crc32.obj crc32.c
$     cc 'DEF_UNX' /object = [.'dest']crctab.obj crctab.c
$     cc 'DEF_UNX' /object = [.'dest']crypt.obj crypt.c
$     cc 'DEF_UNX' /object = [.'dest']envargs.obj envargs.c
$     cc 'DEF_UNX' /object = [.'dest']explode.obj explode.c
$     cc 'DEF_UNX' /object = [.'dest']extract.obj extract.c
$     cc 'DEF_UNX' /object = [.'dest']fileio.obj fileio.c
$     cc 'DEF_UNX' /object = [.'dest']globals.obj globals.c
$     cc 'DEF_UNX' /object = [.'dest']inflate.obj inflate.c
$     cc 'DEF_UNX' /object = [.'dest']list.obj list.c
$     cc 'DEF_UNX' /object = [.'dest']match.obj match.c
$     cc 'DEF_UNX' /object = [.'dest']process.obj process.c
$     cc 'DEF_UNX' /object = [.'dest']ttyio.obj ttyio.c
$     cc 'DEF_UNX' /object = [.'dest']unreduce.obj unreduce.c
$     cc 'DEF_UNX' /object = [.'dest']unshrink.obj unshrink.c
$     cc 'DEF_UNX' /object = [.'dest']zipinfo.obj zipinfo.c
$     cc 'DEF_UNX' /object = [.'dest']vms.obj [.vms]vms.c
$!
$! Create the object library.
$!
$     if (f$search( "[.''dest']unzip.olb") .eqs. "") then -
       libr /object /create [.'dest']unzip.olb
$!
$     libr /object /replace [.'dest']unzip.olb -
       [.'dest']crc32.obj, -
       [.'dest']crctab.obj, -
       [.'dest']crypt.obj, -
       [.'dest']envargs.obj, -
       [.'dest']explode.obj, -
       [.'dest']extract.obj, -
       [.'dest']fileio.obj, -
       [.'dest']globals.obj, -
       [.'dest']inflate.obj, -
       [.'dest']list.obj, -
       [.'dest']match.obj, -
       [.'dest']process.obj, -
       [.'dest']ttyio.obj, -
       [.'dest']unreduce.obj, -
       [.'dest']unshrink.obj, -
       [.'dest']zipinfo.obj, -
       [.'dest']vms.obj
$!
$ endif
$!
$! Link the executable.
$!
$ link /executable = [.'dest']'unzx_unx'.exe -
   [.'dest']unzip.obj, -
   [.'dest']unzip.olb /library, -
   'lib_bzip2_opts' -
   'opts' -
   [.VMS]unzip.opt /options
$!
$!----------------------- UnZip (CLI interface) section ----------------------
$!
$ if (.not. LINK_ONLY)
$ then
$!
$! Process the CLI help file, if desired.
$!
$     if (MAKE_HELP)
$     then
$         set default [.vms]
$         edit /tpu /nosection /nodisplay /command = cvthelp.tpu -
           unzip_cli.help
$         set default [-]
$         runoff /output = unzip_cli.hlp [.vms]unzip_cli.rnh
$     endif
$!
$! Compile the CLI sources.
$!
$     cc 'DEF_CLI' /object = [.'dest']unzipcli.obj unzip.c
$     cc 'DEF_CLI' /object = [.'dest']cmdline.obj -
       [.vms]cmdline.c
$!
$! Create the command definition object file.
$!
$     set command /object = [.'dest']unz_cli.obj [.vms]unz_cli.cld
$!
$! Create the CLI object library.
$!
$     if (f$search( "[.''dest']unzipcli.olb") .eqs. "") then -
       libr /object /create [.'dest']unzipcli.olb
$!
$     libr /object /replace [.'dest']unzipcli.olb -
       [.'dest']cmdline.obj, -
       [.'dest']unz_cli.obj
$!
$ endif
$!
$! Link the CLI executable.
$!
$ link /executable = [.'dest']'unzx_cli'.exe -
   [.'dest']unzipcli.obj, -
   [.'dest']unzipcli.olb /library, -
   [.'dest']unzip.olb /library, -
   'lib_bzip2_opts' -
   'opts' -
   [.VMS]unzip.opt /options
$!
$!-------------------------- UnZipSFX section --------------------------------
$!
$ if (.not. LINK_ONLY)
$ then
$!
$! Compile the variant SFX sources.
$!
$     cc 'DEF_SXUNX' /object = [.'dest']unzipsfx.obj unzip.c
$     cc 'DEF_SXUNX' /object = [.'dest']crc32_.obj crc32.c
$     cc 'DEF_SXUNX' /object = [.'dest']crctab_.obj crctab.c
$     cc 'DEF_SXUNX' /object = [.'dest']crypt_.obj crypt.c
$     cc 'DEF_SXUNX' /object = [.'dest']extract_.obj extract.c
$     cc 'DEF_SXUNX' /object = [.'dest']fileio_.obj fileio.c
$     cc 'DEF_SXUNX' /object = [.'dest']globals_.obj globals.c
$     cc 'DEF_SXUNX' /object = [.'dest']inflate_.obj inflate.c
$     cc 'DEF_SXUNX' /object = [.'dest']match_.obj match.c
$     cc 'DEF_SXUNX' /object = [.'dest']process_.obj process.c
$     cc 'DEF_SXUNX' /object = [.'dest']ttyio_.obj ttyio.c
$     cc 'DEF_SXUNX' /object = [.'dest']vms_.obj [.vms]vms.c
$!
$! Create the SFX object library.
$!
$     if f$search( "[.''dest']unzipsfx.olb") .eqs. "" then -
       libr /object /create [.'dest']unzipsfx.olb
$!
$     libr /object /replace [.'dest']unzipsfx.olb -
       [.'dest']crc32_.obj, -
       [.'dest']crctab_.obj, -
       [.'dest']crypt_.obj, -
       [.'dest']extract_.obj, -
       [.'dest']fileio_.obj, -
       [.'dest']globals_.obj, -
       [.'dest']inflate_.obj, -
       [.'dest']match_.obj, -
       [.'dest']process_.obj, -
       [.'dest']ttyio_.obj, -
       [.'dest']vms_.obj
$!
$ endif
$!
$! Link the SFX executable.
$!
$ link /executable = [.'dest']'unzsfx_unx'.exe -
   [.'dest']unzipsfx.obj, -
   [.'dest']unzipsfx.olb /library, -
   'lib_bzip2_opts' -
   'opts' -
   [.VMS]unzipsfx.opt /options
$!
$!--------------------- UnZipSFX (CLI interface) section ---------------------
$!
$ if (.not. LINK_ONLY)
$ then
$!
$! Compile the SFX CLI sources.
$!
$     cc 'DEF_SXCLI' /object = [.'dest']unzsxcli.obj unzip.c
$     cc 'DEF_SXCLI' /object = [.'dest']cmdline_.obj -
       [.vms]cmdline.c
$!
$! Create the SFX CLI object library.
$!
$     if (f$search( "[.''dest']unzsxcli.olb") .eqs. "") then -
       libr /object /create [.'dest']unzsxcli.olb
$!
$     libr /object /replace [.'dest']unzsxcli.olb -
       [.'dest']cmdline_.obj, -
       [.'dest']unz_cli.obj
$!
$ endif
$!
$! Link the SFX CLI executable.
$!
$ link /executable = [.'dest']'unzsfx_cli'.exe -
   [.'dest']unzsxcli.obj, -
   [.'dest']unzsxcli.olb /library, -
   [.'dest']unzipsfx.olb /library, -
   'lib_bzip2_opts' -
   'opts' -
   [.VMS]unzipsfx.opt /options
$!
$!----------------------------- Symbols section ------------------------------
$!
$ there = here- "]"+ ".''dest']"
$!
$! Define the foreign command symbols.  Similar commands may be useful
$! in SYS$MANAGER:SYLOGIN.COM and/or users' LOGIN.COM.
$!
$ unzip   == "$''there'''UNZEXEC'.exe"
$ zipinfo == "$''there'''UNZEXEC'.exe ""-Z"""
$!
$! Restore the original default directory, deassign the temporary
$! logical names, and restore the DCL verify status.
$!
$ error:
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
$ if (f$trnlnm( "incl_bzip2") .nes. "")
$ then
$     deassign incl_bzip2
$ endif
$!
$ if (f$trnlnm( "lib_bzip2") .nes. "")
$ then
$     deassign lib_bzip2
$ endif
$!
$ exit
$!
