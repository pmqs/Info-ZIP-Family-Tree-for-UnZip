$! MAKESFX.COM
$!
$!     Convert a Zip archive into a self-extracting (SFX) archive.
$!
$! Usage:
$!     @ MAKESFX.COM archive[.zip] [U=unzipsfx_program] [Z=zip_symbol]
$!     (Defaults:                    'UNZIPSFX'            ZIP)
$!
$! This procedure copies an UNZIPSFX.EXE program and an existing Zip
$! archive ("archive.zip") to form an SFX archive ("archive.exe").  It
$! then uses a "ZIP -A" command to adjust the offsets in the resulting
$! executable to eliminate a warning message which would otherwise be
$! emitted at run time.
$!
$! This procedure expects to use two DCL symbols:
$!
$!     UNZIPSFX  to locate the UNZIPSFX.EXE executable
$!     ZIP       a foreign command symbol to run a Zip program
$!
$! Either or both may be defined on the command line to override an
$! existing definition, or to supply a missing definition.  For example,
$! if UNZIPSFX is defined, and ZIP points to the right Zip program, so:
$!
$!     UNZIPSFX :== u_dev:[u_dir]UNZIPSFX.EXE
$!     ZIP :== $ z_dev:[z_dir]ZIP.EXE
$!
$! then a simple command like the following should work:
$!
$!     @ MAKESFX.COM archive
$!
$! Note that the ZIP symbol is a normal foreign command symbol (with the
$! leading "$"), but the UNZIPSFX symbol is not.  It simply specifies
$! the location of the UNZIPSFX.EXE executable.
$!
$! The following command line shows how to specify an UNZIPSFX.EXE
$! explicitly, and a different Zip command symbol.
$!
$!     @ MAKESFX.COM archive U=u2_dev:[u2_dir]UNZIPSFX_CLI.EXE Z=ZIP3
$!
$! With this command line, the procedure will use the SFX program
$! u2_dev:[u2_dir]UNZIPSFX_CLI.EXE, and the command "ZIP3 -A ...". 
$! This would depend on "ZIP3" having been defined appropriately, for
$! example:
$!
$!     ZIP3 :== $ z3_dev:[z3_dir]ZIP.EXE
$!
$! Specifying a null ZIP symbol will inhibit the offset adjustment:
$!
$!     @ MAKESFX.COM archive U=u2_dev:[u2_dir]UNZIPSFX_CLI.EXE Z=
$!
$!
$!     Last revised:  2007-11-28  SMS.
$!
$!-----------------------------------------------------------------------------
$!
$! Preparation.
$!
$ say = "write sys$output"
$ proc = f$environment( "PROCEDURE")
$ procedure = f$parse( proc, , , "NAME")+ f$parse( proc, , , "TYPE")
$ procblank = f$extract( 0, f$length( procedure), -
   "                                ")
$!
$! Set initial values.
$!
$ err = 0
$ archive = ""
$ unzipsfx = ""
$ zip_loc = ""
$!
$! Loop through arguments.
$!
$ a = 1
$ arg_loop:
$    arg = p'a'
$    if (arg .eqs. "") then goto have_args
$    argc = f$edit( arg, "COLLAPSE")
$    argcu = f$edit( argc, "UPCASE")
$!
$! U=unzipsfx_program?
$!
$    if (f$extract( 0, 2, argcu) .eqs. "U=")
$    then
$       if (unzipsfx .eqs. "")
$       then
$          unzipsfx = f$extract( 2, 10000, argc)
$          goto arg_next
$       else
$          err = 1
$          goto have_args
$       endif
$    endif
$!
$! Z=zip_symbol?
$!
$    if (f$extract( 0, 2, argcu) .eqs. "Z=")
$    then
$       if (zip_loc .eqs. "")
$       then
$          zip_loc = f$extract( 2, 10000, argc)
$          if (zip_loc .eqs. "") then zip_loc = "-"
$          goto arg_next
$       else
$          err = 2
$          goto have_args
$       endif
$    endif
$!
$! Archive name.
$!
$    if (archive .eqs. "")
$    then
$       archive = argc
$    else
$       err = 3
$       goto have_args
$    endif
$!
$ arg_next:
$ a = a+ 1
$ goto arg_loop
$!
$ have_args:
$!
$! If the user specified a bad (non-null) ZIP symbol, complain.
$!
$ if ((zip_loc .nes. "") .and. (zip_loc .nes. "-"))
$ then
$    if (f$type( 'zip_loc') .nes. "STRING")
$    then
$       say "ZIP symbol (foreign command) not defined: ''zip_loc'"
$       say ""
$       err = 4
$    endif
$ endif
$!
$! Fill in default values.
$!
$ if (unzipsfx .eqs. "") then unzipsfx = "UNZIPSFX"
$ if (zip_loc .eqs. "")
$ then
$    if (f$type( zip) .eqs. "STRING")
$    then
$       zip_loc = "ZIP"
$    endif
$ endif
$!
$! Check UnZipSFX program.
$!
$ unzipsfx_exe = f$parse( unzipsfx, ".exe")
$ if (f$search( unzipsfx_exe) .eqs. "")
$ then
$    say "Can't find UnZipSFX program: ''unzipsfx_exe'"
$    say ""
$    err = 5
$ endif
$!
$ if ((err .ne. 0) .or. (archive .eqs. ""))
$ then
$    say -
 "Usage:  @ ''procedure' archive[.zip] [U=unzipsfx_program] [Z=zip_symbol]"
$    say -
 "        (Defaults: ''procblank'        'UNZIPSFX'            ZIP)"
$    exit %x07A280A4       ! IZ_UNZIP-F-PARAM.
$ endif
$!
$! Check archive.
$!
$ archive_zip = f$parse( archive, ".zip")
$ if (f$search( archive_zip) .eqs. "")
$ then
$    say "Can't find archive: ''archive_zip'"
$    exit %x07A28094    ! IZ_UNZIP-F-NOZIP.
$ endif
$!
$! Copy the UnZipSFX executable and the original archive to the
$! destination executable.
$!
$ say "   Copying ..."
$ archive_exe = f$parse( ".exe", archive)
$ copy 'unzipsfx_exe', 'archive_zip' 'archive_exe'
$!
$! If ZIP is available, adjust the UnZipSFX executable.
$!
$ if ((zip_loc .nes. "") .and. (zip_loc .nes. "-"))
$ then
$    say "   Adjusting offsets ..."
$    'zip_loc' "-A" 'archive_exe'
$ endif
$!
