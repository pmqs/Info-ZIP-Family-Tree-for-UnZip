@echo off

rem #    UnZip test script (Windows command).
rem #
rem #-----------------------------------------------------------------------
rem # Copyright (c) 2012-2013 Info-ZIP.  All rights reserved.
rem #
rem # See the accompanying file LICENSE, version 2009-Jan-2 or later (the
rem # contents of which are also included in zip.h) for terms of use.  If,
rem # for some reason, all these files are missing, the Info-ZIP license
rem # may also be found at: ftp://ftp.info-zip.org/pub/infozip/license.html
rem #-----------------------------------------------------------------------
rem #
rem #    %1 = test archive name.  Default: testmake.zip
rem #    %2 = program directory (relative).  Default: .
rem #    %3 = non-null to skip funzip and SFX tests.
rem #
rem #    2013-06-12  SMS.  New.  Adapted from unix/test_unzip.sh
rem #
rem # Typical usage:
rem #    win32\test_unzip.cmd testmake.zip win32\vc10\Debug
rem #    win32\test_unzip.cmd testmake_ppmd.zip win32\vc10\Debug NOFUNSFX

setlocal enabledelayedexpansion

set t_a=testmake.zip
set prd=.

if "%1" == "" (
    set test_archive=%t_a%
) else (
    if "%1" == """" (
        set test_archive=%t_a%
    ) else (
        set test_archive=%1
    )
)

if "%2" == "" (
    set prod=%prd%
) else (
    if "%2" == """" (
        set prod=%prd%
    ) else (
        set prod=%2
    )
)

set /a pass=0
set /a fail=0

rem # Clean environment.

set UNZIP=
set UNZIPOPT=
set ZIPINFO=
set ZIPINFOOPT=

rem # Check for existence of expected programs.

if "%3" == "" (
    set program_list=funzip unzip unzipsfx
) else (
    set program_list=unzip
)

for %%p in (%program_list%) do (
    set prog=%%p
    set prog_pth=!prod!\%%p.exe
    if exist !prog_pth! (
        echo ^>^>^> Found executable: !prog!
    ) else (
        echo ^>^>^> CAN'T FIND EXECUTABLE: !prog_pth!
    )
)

if exist %test_archive% (
    echo ^>^>^> Found test archive: %test_archive%
) else (
    echo ^>^>^> CAN'T FIND TEST ARCHIVE: %test_archive%
)

rem # Create and move into a temporary test directory.

set dir_orig=%CD%
set tmp_dir=test_dir_%RANDOM%
mkdir %tmp_dir%
cd %tmp_dir%

rem # Expected test archive member names.

set member_1=notes
set member_2=testmake.zipinfo

rem # Test simple UnZip extraction.

echo.
echo ^>^>^> UnZip extraction test...'

..\%prod%\unzip -b ..\%test_archive% %member_1%
set status=!ERRORLEVEL!

if not !status! == 0 (
    echo ^>^>^> Fail: UnZip exit status = !status!.
    set /a fail=%fail% + 1
) else (
    set bad=0
    if not exist %member_1% set bad=1
    if exist %member_2% set bad=2
    if !bad! == 0 (
        echo ^>^>^> Pass.
        set /a pass=%pass% + 1
    ) else (
        echo ^>^>^> Fail: Can't find ^(only^) expected extracted file: %member_1%
        set /a fail=%fail% + 1
    )
)

rem # Test UnZip extraction with "-x" option.
rem # (Use "-a" to get proper local line endings for "-Z" test below.)

echo.
echo ^>^>^> UnZip "-x" extraction test...

..\%prod%\unzip -ao ..\%test_archive% -x %member_1%
set status=!ERRORLEVEL!

if not !status! == 0 (
    echo ^>^>^> Fail: UnZip exit status = !status!.
    set /a fail=%fail% + 1
) else (
    set bad=0
    if not exist %member_2% set bad=1
    if !bad! == 0 (
        echo ^>^>^> Pass.
        set /a pass=%pass% + 1
    ) else (
        echo ^>^>^> Fail: Can't find expected extracted file: %member_2%
        set /a fail=%fail% + 1
    )
)

rem # Test UnZip extraction with "-d dest_dir" option.

echo.
echo ^>^>^> UnZip "-d dest_dir" extraction test...

if exist %member_1% (

    ..\%prod%\unzip -bo ..\%test_archive% -d dest_dir %member_1%
    set status=!ERRORLEVEL!

    if not !status! == 0 (
        echo ^>^>^> Fail: UnZip exit status = !status!.
        set /a fail=%fail% + 1
    ) else (
        if exist dest_dir\%member_1% (
            echo N > no
            comp /a /l %member_1% dest_dir\%member_1% < no > NUL 2>&1
            set status=!ERRORLEVEL!
            del /f no

            if !status! == 0 (
                echo ^>^>^> Pass.
                set /a pass=%pass% + 1
            ) else (
                echo ^>^>^> Fail: Extracted file contents mismatch.
                set /a fail=%fail% + 1
            )
        ) else (
            echo ^>^>^> Fail: Can't find expected extracted file: dest_dir/%member_1%
            set /a fail=%fail% + 1
        )
    )
) else (
    echo ^>^>^> Fail: The UnZip "-d" test relies on success in the UnZip "-x" test."
    set /a fail=%fail% + 1
)

rem # Test UnZip extraction with "-o" option.

echo.
echo ^>^>^> UnZip "-o" extraction test...

..\%prod%\unzip -bo ..\%test_archive% %member_1%
set status=!ERRORLEVEL!

if not !status! == 0 (
    echo ^>^>^> Fail: UnZip exit status = !status!.
    set /a fail=%fail% + 1
) else (
    if exist %member_1% (
        echo ^>^>^> Pass.
        set /a pass=%pass% + 1
    ) else (
        echo ^>^>^> Fail: Can't find expected extracted file: %member_1%
        set /a fail=%fail% + 1
    )
)

rem # Test ZipInfo ("unzip -Z").

echo.
echo ^>^>^> ZipInfo ("unzip -Z") test...

if exist %member_2% (

    cd ..
    %prod%\unzip -Z -mc- %test_archive% > %tmp_dir%\testmake.unzip-Z
    set status=!ERRORLEVEL!
    cd %tmp_dir%

    if not !status! == 0 (
        echo ^>^>^> Fail: UnZip exit status = !status!.
        set /a fail=%fail% + 1
    ) else (
        echo N > no
        comp /a /l testmake.unzip-Z %member_2% < no > NUL 2>&1
        set status=!ERRORLEVEL!
        del /f no

        if !status! == 0 (
            echo ^>^>^> Pass.
            set /a pass=%pass% + 1
        ) else (
            echo ^>^>^> Fail: ZipInfo output mismatch."

echo ^>^>^> ###   ZipInfo output does not match the expected output.
echo ^>^>^> ###   ^(If the only difference is in the date-time values, then this may
echo ^>^>^> ###   be caused by a time-zone difference, which may not be important.^)

            set /a fail=%fail% + 1
        )
    )
) else (
    echo ^>^>^> Fail: The ZipInfo test relies on success in the UnZip test."
    set /a fail=%fail% + 1
)

rem echo on

rem # Test FunZip extraction.

if "%3" == "" (
    echo.
    echo ^>^>^> FunZip extraction test...
    echo ^>^>^>  ^(Expect a "zipfile has more than one entry" warning.^)

    if exist %member_1% (

        ..\%prod%\funzip < ..\%test_archive% > dest_dir\%member_1%_fun
        set status=!ERRORLEVEL!

        if not !status! == 0 (
            echo ^>^>^> Fail: FunZip exit status = !status!.
            set /a fail=%fail% + 1
        ) else (
            if exist dest_dir\%member_1%_fun (
                echo N > no
                comp /a /l %member_1% dest_dir\%member_1%_fun < no > NUL 2>&1
                set status=!ERRORLEVEL!
                del /f no

                if !status! == 0 (
                    echo ^>^>^> Pass.
                    set /a pass=%pass% + 1
                ) else (
                    echo ^>^>^> Fail: Extracted file contents mismatch.
                    set /a fail=%fail% + 1
                )

            ) else (
                echo ^>^>^> Fail: Can't find expected extracted file: dest_dir\%member_1%_fun
                set /a fail=%fail% + 1
            )
        )
    ) else (
         echo ^>^>^> Fail: The FunZip test relies on success in the UnZip test.
         set /a fail=%fail% + 1
    )
)


rem # Test UnZipSFX.

if "%3" == "" (
    echo.
    echo ^>^>^> UnZipSFX test...

    if exist dest_dir\%member_1% (

        if exist %member_1% del %member_1%
        if exist %member_2% del %member_2%

        copy /b ..\%prod%\unzipsfx.exe + ..\%test_archive% test_sfx.exe > NUL
        .\test_sfx -b %member_1%
        set status=!ERRORLEVEL!

        if not !status! == 0 (
            echo ^>^>^> Fail: UnZipSFX exit status = !status!.
            set /a fail=%fail% + 1
        ) else (
            if exist %member_1% (
                echo N > no
                comp /a /l %member_1% dest_dir\%member_1% < no > NUL 2>&1
                set status=!ERRORLEVEL!
                del /f no

                if !status! == 0 (
                    echo ^>^>^> Pass.
                    set /a pass=%pass% + 1
                ) else (
                    echo ^>^>^> Fail: Extracted file contents mismatch."
                    set /a fail=%fail% + 1
                )
            ) else (
                echo ^>^>^> Fail: Can't find expected extracted file: %member_1%
                set /a fail=%fail% + 1
            )
        )
    ) else (
        echo ^>^>^> Fail: The UnZipSFX test relies on success in the UnZip "-d" test.
        set /a fail=%fail% + 1
    )
)

rem Expected results.

set fail_expected=0
if "%3" == "" (
    set pass_expected=7
) else (
    set pass_expected=5
)

rem Result summary.

echo.
echo ^>^>^> Test Results:   Pass: %pass%, Fail: %fail%
set exit_status=0
if not %pass% == %pass_expected% set exit_status=1
if not %fail% == %fail_expected% set exit_status=1
if not %exit_status% == 0 (
    echo ^>^>^> ###   Expected: Pass: %pass_expected%, Fail: %fail_expected%
)
echo.

rem # Clean up.

cd %dir_orig%
if exist %tmp_dir% rmdir /q /s %tmp_dir%

rem # Exit with an appropriate status value.

exit /b %exit_status%
