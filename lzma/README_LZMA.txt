                        README_LZMA.txt
                        ---------------

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Description
      -----------

   The Info-ZIP programs UnZip (version 6.1 and later) and Zip (version
3.1 and later) offer optional support for LZMA compression (method 14).
The PKWARE, Inc. APPNOTE describes the LZMA method as an Early Feature
Specification, hence subject to change.

      http://www.pkware.com/documents/casestudies/APPNOTE.TXT

   Our LZMA implementation uses public-domain code from the LZMA
Software Development Kit (SDK) provided by Igor Pavlov:

      http://www.7-zip.org/sdk.html

Only a small subset of the LZMA SDK is used by (and provided with) the
Info-ZIP programs, in an "lzma/" subdirectory.

   Minor changes were made to various files to improve portability. 
Indentation was changed to keep "#" in column one.  Some global names
longer than 31 characters were shortened for VMS.  Files named "7z*.h"
were renamed to "Sz*.h", to accommodate some IBM operating systems. 
Originals of the changed files may be found in the "lzma/orig/"
directory.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Building UnZip and Zip with LZMA Compression Support
      ----------------------------------------------------

   The build instructions (in the file INSTALL or an OS-specific
INSTALL supplement, in the UnZip and Zip source kits) describe how to
build UnZip and Zip with support for LZMA compression.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Acknowledgement
      ---------------

   We're grateful to Igor Pavlov for providing the LZMA and PPMd
compression code, and to Dmitry Shkarin for the p7zip material.  Any
problems involving LZMA or PPMd compression in Info-ZIP programs should
be reported to the Info-ZIP team, not to Mr. Pavlov or Mr. Shkarin. 
However, any questions on LZMA or PPMd compression algorithms, or
regarding the original LZMA SDK or PPMd code (except as we modified and
use it) should be addressed to the original authors.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Document Revision History
      -------------------------

      2011-08-14  New.  LZMA SDK version 9.20.  (SMS)

      2011-12-24  Added PPMd from p7zip version 9.20.1.  (SMS)

      2011-12-24  Removed references to 7zFile.[ch], SzFile.[ch].  (SMS)

      2012-04-21  Renamed "lzma/" directory to "szip/".  (SMS)

      2018-11-27  Restructured the kit to replace the common szip/
                  directory with separate lzma/ and ppmd/ directories,
                  to cope with inconsistencies in their current header
                  files.  LZMA SDK version 18.05.  (SMS)

