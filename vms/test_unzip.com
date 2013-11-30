$! TEST_UNZIP.COM
$!
$!    Info-ZIP UnZip (basic) test DCL script.
$!
$!     Last revised:  2013-11-29  SMS.
$!
$!----------------------------------------------------------------------
$! Copyright (c) 2011-2013 Info-ZIP.  All rights reserved.
$!
$! See the accompanying file LICENSE, version 2009-Jan-2 or later (the
$! contents of which are also included in zip.h) for terms of use.  If,
$! for some reason, all these files are missing, the Info-ZIP license
$! may also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
$!----------------------------------------------------------------------
$!
$!    P1 = test archive name.  Default: testmake.zip
$!    P2 = program directory.  Default: "" (current directory)
$!    P3 = non-null to skip SFX tests.
$!
$!    2013-11-29  SMS.  Copyright, documentation, license.
$!    2013-06-03  SMS.  Added exit status value.
$!    2012-12-16  SMS.  Added "-mc-" to UnZip "-Z" command to suppress
$!                      member counts by dir/file/link, because not all
$!                      systems have links, which changes the report.
$!    2012-02-24  SMS.  Added P3 check for PPMd test.
$!                      Added lrl and mrs attributes to set on SFX EXE.
$!                      Added exit status tests.
$!    2011-06-10  SMS.  New.
$!
$ test_archive = f$edit( p1, "trim")
$ if (test_archive .eqs. "")
$ then
$     test_archive = "testmake.zip"
$ endif
$!
$ pass = 0
$ fail = 0
$ exit_status = %x100002A4      ! SS$_BUGCHECK (message suppressed).
$ echo = "write sys$output"
$!
$! Clean environment.
$!
$ vars = "UNZIP_OPTS, UNZIPOPT, ZIPINFO_OPTS, ZIPINFOOPT"
$!
$ elt = 0
$ loop_var_top:
$     var = f$element( elt, ",", vars)
$     if (var .eqs. ",") then goto loop_var_bot
$     var = f$edit( var, "trim")
$     if (f$trnlnm( var) .nes. "")
$     then
$         echo ">>> Warning: Defining process logical name ''var'.  Was:"
$         show logical 'var'
$         define /process 'var' " "
$     endif
$ elt = elt+ 1
$ goto loop_var_top
$ loop_var_bot:
$!
$! Check for existence of expected programs.  Define appropriate symbols.
$!
$ if (p3 .eqs. "")
$ then
$     program_list = "unzip, unzipsfx, unzip_cli, unzipsfx_cli"
$ else
$     program_list = "unzip, unzip_cli"
$ endif
$!
$ elt = 0
$ loop_program_top:
$     program = f$element( elt, ",", program_list)
$     if (program .eqs. ",") then goto loop_program_bot
$     program = f$edit( program, "trim")
$     exe = p2+ program+ ".exe"
$     prog = f$search( exe)
$     if (prog .nes. "")
$     then
$         echo ">>> Found executable: ''program'"
$         if (f$locate( "sfx", f$edit( program, "lowercase")) .lt. -
           f$length( program))
$         then
$             'program' = prog
$         else
$             'program' = "$ ''prog'"
$         endif
$     else
$         echo ">>> CAN'T FIND EXECUTABLE: ''program'"
$         echo ">>> (''exe')"
$         'program' = ""
$     endif
$ elt = elt+ 1
$ goto loop_program_top
$ loop_program_bot:
$!
$! Check for existence of test archive.
$!
$ if (f$search( test_archive) .nes. "")
$ then
$     echo ">>> Found test archive: ''test_archive'"
$ else
$     echo ">>> CAN'T FIND TEST ARCHIVE: ''test_archive'"
$ endif
$!
$! Error/exit handler.
$!
$ pwd = f$environment( "default")
$ on control_y then goto clean_up
$ on error then goto clean_up
$ goto post_handler
$!
$ clean_up:
$ if (f$trnlnm( "unzip_z_in", "LNM$PROCESS") .nes. "")
$ then
$     close unzip_z_in
$ endif
$ if (f$trnlnm( "unzip_z_out", "LNM$PROCESS") .nes. "")
$ then
$     close unzip_z_out
$ endif
$ set default 'pwd'
$ exit 'exit_status'
$!
$ post_handler:
$!
$! Create, clean out, and move into a temporary test directory.
$!
$ tmp_dir = "test_dir_"+ f$getjpi( 0, "pid")
$ if (f$search( "''tmp_dir'.DIR;1") .eqs. "")
$ then
$     create /directory [.'tmp_dir']
$ else
$     if (f$search( "[.''tmp_dir'...]*.*;*") .nes. "")
$     then
$         set protection = w:d [.'tmp_dir'...]*.*;*
$     endif
$     if (f$search( "[.''tmp_dir'.*]*.*;*") .nes. "")
$     then
$         delete [.'tmp_dir'.*]*.*;*
$     endif
$     if (f$search( "[.''tmp_dir']*.*;*") .nes. "")
$     then
$         delete [.'tmp_dir']*.*;*
$     endif
$ endif
$!
$ set default [.'tmp_dir']
$!
$! Expected test archive member names.
$!
$ member_1 = "notes"
$ member_2 = "testmake.zipinfo"
$!
$! Run tests using UNIX-style command line.
$!
$ echo ""
$ echo ">>>    UNIX-style command line tests..."
$ cli = 0
$ gosub run_tests
$!
$! Clean out working directory.  Repeat tests for CLI programs.
$!
$ if (f$search( "[...]*.*;*") .nes. "")
$ then
$     set protection = w:d [...]*.*;*
$ endif
$ if (f$search( "[.*]*.*;*") .nes. "")
$ then
$     delete [.*]*.*;*
$ endif
$ if (f$search( "[]*.*;*") .nes. "")
$ then
$     delete []*.*;*
$ endif
$!
$! Run tests using VMS-style command line.
$!
$ echo ""
$ echo ">>>    VMS-style command line tests..."
$ cli = 1
$ gosub run_tests
$!
$!
$! Expected results.
$!
fail_expected = 0
$ if (p3 .eqs. "")
$ then
$     pass_expected = 12
$ else
$     pass_expected = 10
$ endif
$!
$! Result summary.
$!
$ echo ""
$ echo ">>> Test Results:   Pass: ''pass', Fail: ''fail'"
$ if ((pass .ne. pass_expected) .or. (fail .ne. fail_expected))
$ then
$     echo ">>> ###   Expected: Pass: ''pass_expected', Fail: ''fail_expected'"
$ else
$     exit_status = 1           ! SS$_NORMAL
$ endif
$ echo ""
$!
$ goto clean_up
$!
$!
$! Subroutine to run tests.
$!
$ run_tests:
$!
$! Test simple UnZip extraction.
$!
$ echo ""
$ echo ">>> UnZip extraction test..."
$!
$ on error then continue
$ if (cli)
$ then
$     unzip_cli /text = stmlf [-]'test_archive' "''member_1'"
$     status = $status
$ else
$     unzip "-S" [-]'test_archive' 'member_1'
$     status = $status
$ endif
$ on error then goto clean_up
$!
$ if (status)
$ then
$     if ((f$search( member_1) .nes. "") .and. (f$search( member_2) .eqs. ""))
$     then
$         echo ">>> Pass."
$         pass = pass+ 1
$     else
$         echo ">>> Fail: Can't find (only) expected extracted file: ''member_1'"
$         show default
$         dire /date /prot /size
$         fail = fail+ 1
$     endif
$ else
$     echo ">>> Fail: UnZip exit status = ''status'"
$     fail = fail+ 1
$ endif
$!
$! Test UnZip extraction with "-x" (/exclude) option.
$!
$ echo ""
$ if (cli)
$ then
$     opt = "/exclude"
$ else
$     opt = "-x"
$ endif
$ echo ">>> UnZip ""''opt'"" extraction test..."
$!
$ on error then continue
$ if (cli)
$ then
$     unzip_cli /text = stmlf /existing = new_version [-]'test_archive' -
       /exclude = "''member_1'"
$     status = $status
$ else
$     unzip "-So" [-]'test_archive' -x "''member_1'"
$     status = $status
$ endif
$ on error then goto clean_up
$!
$ if (status)
$ then
$     if (f$search( member_2) .nes. "")
$     then
$         echo ">>> Pass."
$         pass = pass+ 1
$     else
$         echo ">>> Fail: Can't find expected extracted file: ''member_2'"
$         fail = fail+ 1
$     endif
$ else
$     echo ">>> Fail: UnZip exit status = ''status'"
$     fail = fail+ 1
$ endif
$!
$! Test UnZip extraction with "-d dest_dir" option.
$!
$ echo ""
$ if (cli)
$ then
$     opt = "/directory = [.dest_dir]"
$ else
$     opt = "-d [.dest_dir]"
$ endif
$ echo ">>> UnZip ""''opt'"" extraction test..."
$!
$ on error then continue
$ if (cli)
$ then
$     unzip_cli /text = stmlf /existing = new_vers /direct = [.dest_dir] -
       [-]'test_archive' "''member_1'"
$     status = $status
$ else
$     unzip "-So" [-]'test_archive' -d [.dest_dir] "''member_1'"
$     status = $status
$ endif
$ on error then goto clean_up
$!
$ if (status)
$ then
$     if (f$search( member_1) .nes. "")
$     then
$         if (f$search( "[.dest_dir]''member_1'") .nes. "")
$         then
$             differences 'member_1' [.dest_dir]'member_1'
$             if ($status .eq. %X006C8009)
$             then
$                 echo ">>> Pass."
$                 pass = pass+ 1
$             else
$                 echo ">>> Fail: Extracted file contents mismatch."
$                 fail = fail+ 1
$             endif
$         else
$             echo -
 ">>> Fail: Can't find expected extracted file: [.dest_dir]''member_1'"
$             fail = fail+ 1
$         endif
$     else
$         echo -
 ">>> Fail: The UnZip ""-d"" test relies on success in the UnZip ""-x"" test."
$         fail = fail+ 1
$     endif
$ else
$     echo ">>> Fail: UnZip exit status = ''status'"
$     fail = fail+ 1
$ endif
$!
$! Test UnZip extraction with "-o" option.
$!
$ echo ""
$ if (cli)
$ then
$     opt = "/existing = new_version"
$ else
$     opt = "-o"
$ endif
$ echo ">>> UnZip ""''opt'"" extraction test..."
$!
$ on error then continue
$ if (cli)
$ then
$     unzip_cli /text = stmlf /existing = new_vers [-]'test_archive' -
       "''member_1'"
$     status = $status
$ else
$     unzip "-So" [-]'test_archive' "''member_1'"
$     status = $status
$ endif
$ on error then goto clean_up
$!
$ if (status)
$ then
$     if (f$search( member_1) .nes. "")
$     then
$         echo ">>> Pass."
$         pass = pass+ 1
$     else
$         echo ">>> Fail: Can't find expected extracted file: ''member_1'"
$         fail = fail+ 1
$     endif
$ else
$     echo ">>> Fail: UnZip exit status = ''status'"
$     fail = fail+ 1
$ endif
$!
$! Test ZipInfo ("unzip -Z").
$!
$ echo ""
$ if (cli)
$ then
$     opt = "/zipinfo"
$     opt2 = "/nomember_counts"
$ else
$     opt = "-Z"
$     opt2 = "-mc-"
$ endif
$ echo ">>> ZipInfo (""unzip ''opt'"") test..."
$!
$ if (f$search( member_2) .nes. "")
$ then
$!    Arrange for a reliable archive name in the report.
$     pwd_ct = pwd- "]"+ ".]"
$     define /user_mode /translation_attributes = concealed -
       test_dir 'pwd_ct'
$     define /user_mode sys$output testmake.unzip-Z
$!
$     on error then continue
$     if (cli)
$     then
$         unzip_cli 'opt' 'opt2' test_dir:[000000]'test_archive'
$         status = $status
$     else
$         unzip "''opt'" "''opt2'" test_dir:[000000]'test_archive'
$         status = $status
$     endif
$     on error then goto clean_up
$!
$     if (status)
$     then
$!        Simplify the full VMS archive name in the report heading.
$         open /read /error = error_r unzip_z_in testmake.unzip-Z
$         open /write /error = error_w unzip_z_out testmake.unzip-Z2
$         line_nr = 0
$         loop_read_top:
$             read /error = error_c unzip_z_in line
$             if (line_nr .eq. 0)
$             then
$                 line_nr = 1
$                 if (f$element( 0, " ", line) .eqs. "Archive:")
$                 then
$                     archv = f$element( 1, " ", -
                       f$edit( line, "compress, trim"))
$                     archv = f$parse( archv, , , "name", "syntax_only")+ -
                       f$parse( archv, , , "type", "syntax_only")
$                     archv = f$edit( archv, "lowercase")
$                     loc_archv = f$locate( "test_dir", -
                       f$edit( line,"lowercase"))
$                     line = f$extract( 0, loc_archv, line)+ archv
$                 endif
$             endif
$             write /error = error_c unzip_z_out line
$         goto loop_read_top
$         error_c:
$         close unzip_z_out
$         error_w:
$         close unzip_z_in
$         error_r:
$!
$         differences testmake.unzip-Z2 'member_2'
$         if ($status .eq. %X006C8009)
$         then
$             echo ">>> Pass."
$             pass = pass+ 1
$         else
$             echo ">>> Fail: ZipInfo output mismatch."
$             type sys$input

>>> ###   ZipInfo output does not match the expected output.
>>> ###   (If the only difference is in the date-time values, then this may
>>> ###   be caused by a time-zone difference, which may not be important.)

$             eod
$             fail = fail+ 1
$         endif
$     else
$         echo ">>> Fail: UnZip exit status = ''status'"
$         fail = fail+ 1
$     endif
$ else
$     echo ">>> Fail: The ZipInfo test relies on success in the UnZip test."
$     fail = fail+ 1
$ endif
$!
$! Test UnZipSFX.
$!
$ if (p3 .eqs. "")
$ then
$     echo ""
$     echo ">>> UnZipSFX test..."
$!
$     if (f$search( "[.dest_dir]''member_1'") .nes. "")
$     then
$         delete 'member_1';*, 'member_2';*
$!        Adjust attributes of ".exe" for compatibilty with stored test archive.
$         set noon
$         if (cli)
$         then
$             copy 'unzipsfx_cli' sys$disk:[]unzipsfx.exe
$         else
$             copy 'unzipsfx' sys$disk:[]unzipsfx.exe
$         endif
$         rfm_exe = f$file_attributes( "unzipsfx.exe", "rfm")
$         rfm_arc = f$file_attributes( "[-]''test_archive'", "rfm")
$         if ((rfm_arc .eqs. "STMLF") .and. (rfm_exe .nes. rfm_arc))
$         then
$             set file /attributes = (rfm:stmlf, mrs:0, lrl:32767) unzipsfx.exe
$         endif
$         copy unzipsfx.exe, [-]'test_archive' []test_sfx.exe
$         set protection = o:re test_sfx.exe
$!
$         if (cli)
$         then
$             mcr sys$disk:[]test_sfx /text = stmlf "''member_1'"
$             status = $status
$         else
$             mcr sys$disk:[]test_sfx -b "''member_1'"
$             status = $status
$         endif
$!
$         on control_y then goto clean_up
$         on error then goto clean_up
$!
$         if (status)
$         then
$             if (f$search( member_1) .nes. "")
$             then
$                 differences 'member_1' [.dest_dir]'member_1'
$                 if ($status .eq. %X006C8009)
$                 then
$                     echo ">>> Pass."
$                     pass = pass+ 1
$             else
$                 echo ">>> Fail: Extracted file contents mismatch."
$                 fail = fail+ 1
$             endif
$             else
$                 echo -
 ">>> Fail: Can't find expected extracted file: [.dest_dir]''member_1'"
$                 fail = fail+ 1
$             endif
$         else
$             echo ">>> Fail: UnZip exit status = ''status'"
$             fail = fail+ 1
$         endif
$     else
$         echo -
 ">>> Fail: The UnZipSFX test relies on success in the UnZip ""-d"" test."
$         fail = fail+ 1
$     endif
$ endif
$!
$ return
$!
