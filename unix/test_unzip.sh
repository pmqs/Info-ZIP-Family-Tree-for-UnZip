#!/bin/sh

#    UnZip test script.
#
#    $1 = test archive name.  Default: testmake.zip
#    $2 = program directory (relative).  Default: .
#    $3 = non-null to skip funzip and SFX tests.
#
#    2013-06-03  SMS.  Added exit status value.
#    2012-12-16  SMS.  Added "-mc-" to UnZip "-Z" command to suppress
#                      member counts by dir/file/link, because not all
#                      systems have links, which changes the report.
#    2012-06-22  SMS.  MinGW accommodation.  Changed to try
#                      "unzipsfx.exe" when plain "unzipsfx" is missing.
#                      Use "unzip -a" to avoid line-ending differences.
#    2012-02-24  SMS.  Added $3 check for PPMd test.
#                      Added exit status tests.
#    2011-08-05  SMS.  New.

test_archive=${1:-testmake.zip}
prod=${2:-.}

pass=0
fail=0

# Clean environment.

if test -n "${UNZIP}" ; then
    UNZIP='' ; export UNZIP
fi

if test -n "${UNZIPOPT}" ; then
    UNZIPOPT='' ; export UNZIPOPT
fi

if test -n "${ZIPINFO}" ; then
    ZIPINFO='' ; export ZIPINFO
fi

if test -n "${ZIPINFOOPT}" ; then
    ZIPINFOOPT='' ; export ZIPINFOOPT
fi

# Check for existence of expected programs.

if test -z "$3" ; then
    program_list='funzip unzip unzipsfx'
else
    program_list='unzip'
fi

for program in $program_list ; do
    if test -f "${prod}/${program}" ; then
        echo ">>> Found executable: ${program}"
    else
        echo ">>> CAN'T FIND EXECUTABLE: ${prod}/${program}"
    fi
done

# Check for existence of test archive.

if test -f "${test_archive}" ; then
    echo ">>> Found test archive: ${test_archive}"
else
    echo ">>> CAN'T FIND TEST ARCHIVE: ${test_archive}"
fi

# Error/exit handler.

dir_orig=` pwd `

trap 'cd "${dir_orig}" ;
if test -n "${tmp_dir}" -a -d "${tmp_dir}" ; then
    rm -r "${tmp_dir}"
fi' 0 2 3 6 15

# Create and move into a temporary test directory.

tmp_dir="test_dir_$$"
mkdir "${tmp_dir}"
cd "${tmp_dir}"

# Expected test archive member names.

member_1='notes'
member_2='testmake.zipinfo'

# Test simple UnZip extraction.

echo ''
echo '>>> UnZip extraction test...'

../${prod}/unzip -b "../${test_archive}" "${member_1}"
status=$?

if test ${status} -ne 0 ; then
    echo ">>> Fail: UnZip exit status = ${status}."
    fail=` expr $fail + 1 `
elif test -f "${member_1}" -a ! -f "${member_2}" ; then
    echo ">>> Pass."
    pass=` expr $pass + 1 `
else
    echo ">>> Fail: Can't find (only) expected extracted file: ${member_1}"
    pwd ; ls -l
    fail=` expr $fail + 1 `
fi

# Test UnZip extraction with "-x" option.
# (Use "-a" to get proper local line endings for "-Z" test below.)

echo ''
echo '>>> UnZip "-x" extraction test...'

../${prod}/unzip -ao "../${test_archive}" -x "${member_1}"
status=$?

if test ${status} -ne 0 ; then
    echo ">>> Fail: UnZip exit status = ${status}."
    fail=` expr $fail + 1 `
elif test -f "${member_2}"; then
    echo ">>> Pass."
    pass=` expr $pass + 1 `
else
    echo ">>> Fail: Can't find expected extracted file: ${member_2}"
    fail=` expr $fail + 1 `
fi

# Test UnZip extraction with "-d dest_dir" option.

echo ''
echo '>>> UnZip "-d dest_dir" extraction test...'

if test -f "${member_1}" ; then
    ../${prod}/unzip -bo "../${test_archive}" -d dest_dir "${member_1}"
    status=$?

    if test ${status} -ne 0 ; then
        echo ">>> Fail: UnZip exit status = ${status}."
        fail=` expr $fail + 1 `
    elif test -f "dest_dir/${member_1}"; then
        if diff "${member_1}" "dest_dir/${member_1}" ; then
            echo ">>> Pass."
            pass=` expr $pass + 1 `
        else
            echo ">>> Fail: Extracted file contents mismatch."
            fail=` expr $fail + 1 `
        fi
    else
        echo \
 ">>> Fail: Can't find expected extracted file: dest_dir/${member_1}"
        fail=` expr $fail + 1 `
    fi
else
    echo \
 ">>> Fail: The UnZip \"-d\" test relies on success in the UnZip \"-x\" test."
    fail=` expr $fail + 1 `
fi

# Test UnZip extraction with "-o" option.

echo ''
echo '>>> UnZip "-o" extraction test...'

../${prod}/unzip -bo "../${test_archive}" "${member_1}"
status=$?

if test ${status} -ne 0 ; then
    echo ">>> Fail: UnZip exit status = ${status}."
    fail=` expr $fail + 1 `
elif test -f "${member_1}"; then
    echo ">>> Pass."
    pass=` expr $pass + 1 `
else
    echo ">>> Fail: Can't find expected extracted file: ${member_1}"
    fail=` expr $fail + 1 `
fi

# Test ZipInfo ("unzip -Z").

echo ''
echo '>>> ZipInfo ("unzip -Z") test...'

if test -f "${member_2}"; then
    (
      cd ..
      ${prod}/unzip -Z -mc- "${test_archive}" > "${tmp_dir}/testmake.unzip-Z"
    )
    status=$?

    if test ${status} -ne 0 ; then
        echo ">>> Fail: UnZip exit status = ${status}."
        fail=` expr $fail + 1 `
    elif diff testmake.unzip-Z "${member_2}" ; then
        echo ">>> Pass."
        pass=` expr $pass + 1 `
    else
        echo ">>> Fail: ZipInfo output mismatch."
        cat <<EOD

>>> ###   ZipInfo output does not match the expected output.
>>> ###   (If the only difference is in the date-time values, then this may
>>> ###   be caused by a time-zone difference, which may not be important.)

EOD
        fail=` expr $fail + 1 `
    fi
else
    echo ">>> Fail: The ZipInfo test relies on success in the UnZip test."
    fail=` expr $fail + 1 `
fi

# Test FunZip extraction.

if test -z "$3" ; then
    echo ''
    echo '>>> FunZip extraction test...'
    echo '>>>  (Expect a "zipfile has more than one entry" warning.)'

    if test -f "${member_1}" ; then

        ../${prod}/funzip < "../${test_archive}" > "dest_dir/${member_1}_fun"
        status=$?

        if test ${status} -ne 0 ; then
            echo ">>> Fail: FunZip exit status = ${status}."
            fail=` expr $fail + 1 `
        elif test -f "dest_dir/${member_1}_fun"; then
            if diff "${member_1}" "dest_dir/${member_1}_fun" ; then
                echo ">>> Pass."
                pass=` expr $pass + 1 `
            else
                echo ">>> Fail: Extracted file contents mismatch."
                fail=` expr $fail + 1 `
            fi
        else
            echo \
 ">>> Fail: Can't find expected extracted file: dest_dir/${member_1}_fun"
             fail=` expr $fail + 1 `
         fi
     else
         echo ">>> Fail: The FunZip test relies on success in the UnZip test."
         fail=` expr $fail + 1 `
     fi
fi

# Test UnZipSFX.

if test -z "$3" ; then
    echo ''
    echo '>>> UnZipSFX test...'

    if test -f "dest_dir/${member_1}" ; then

        rm ${member_1} ${member_2}
        # MinGW "test -f fred" detects "fred.exe", but the "cat" below
        # needs the real name, so we use a more realistic test.
        uzs='unzipsfx'
        cat ../${prod}/${uzs} > /dev/null 2> /dev/null
        if [ $? -ne 0 ]; then
            uzs="${uzs}.exe"
        fi
        cat ../${prod}/${uzs} "../${test_archive}" > test_sfx
        chmod 0700 test_sfx
        ./test_sfx -b ${member_1}
        status=$?

        if test ${status} -ne 0 ; then
            echo ">>> Fail: UnZipSFX exit status = ${status}."
            fail=` expr $fail + 1 `
        elif test -f "${member_1}"; then
            if diff "${member_1}" "dest_dir/${member_1}" ; then
                echo ">>> Pass."
                pass=` expr $pass + 1 `
            else
                echo ">>> Fail: Extracted file contents mismatch."
                fail=` expr $fail + 1 `
            fi
        else
            echo ">>> Fail: Can't find expected extracted file: ${member_1}"
            fail=` expr $fail + 1 `
        fi
    else
        echo \
 ">>> Fail: The UnZipSFX test relies on success in the UnZip \"-d\" test."
        fail=` expr $fail + 1 `
    fi
fi

# Expected results.

fail_expected=0
if test -z "$3" ; then
    pass_expected=7
else
    pass_expected=5
fi

# Result summary.

echo ''
echo ">>> Test Results:   Pass: ${pass}, Fail: ${fail}"
exit_status=0
if test $pass -ne $pass_expected -o $fail -ne $fail_expected ; then
    echo ">>> ###   Expected: Pass: ${pass_expected}, Fail: ${fail_expected}"
    exit_status=1
fi
echo ''

exit ${exit_status}
