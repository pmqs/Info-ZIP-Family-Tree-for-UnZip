                        README_AES_WG.txt
                        -----------------

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Description
      -----------

   The Info-ZIP programs UnZip (version 6.1 and later) and Zip (version
3.1 and later) offer optional support for Advanced Encryption Standard
(AES) encryption, a relatively strong encryption method.  Traditional
zip encryption, in contrast, is now considered weak and relatively easy
to crack.  Our IZ_AES_WG (Info-ZIP AES WinZip/Gladman) source kit adds
support for WinZip-compatible AES encryption, using AES encryption code
supplied by Brian Gladman.  The WinZip AES scheme is described in:

      http://www.winzip.com/aes_info.htm

Briefly, the WinZip AES scheme uses compression method 99 to flag the
AES-encrypted entries.  (In contrast, PKZIP AES encryption uses an
incompatible scheme with different archive data structures.  However,
current versions of PKZIP may also be able to process WinZip AES
encrypted archive entries.)

   Our AES implementation supports 128-, 192-, and 256-bit keys.  See
the various discussions of WinZip AES encryption on the Internet for
more on the security of the WinZip AES encryption implementation.  We
may add additional encryption methods in the future.

   The IZ_AES_WG source kit is based on an AES source kit provided by
Brian Gladman:

      http://gladman.plushost.co.uk/oldsite/cryptography_technology/
      fileencrypt/files.zip

with one header file ("brg_endian.h") from a newer kit:

      http://gladman.plushost.co.uk/oldsite/AES/aes-src-11-01-11.zip

(Non-Windows users should use "unzip -a" when unpacking those kits.)

   There are two main reasons we are providing essentially a repackaging
of the Gladman AES code.  First, some minor changes were needed to
improve its portability to different hardware and operating systems.
Second, the version of Gladman code used, though recommended by WinZip,
is relatively old and so not fully supported by Dr. Gladman.  We felt it
necessary to capture this version of the Gladman AES code with our
changes and make it available.  We are currently looking at newer
versions of the Gladman AES code and may update our implementation in
the future.

   The portability-related changes to the original Gladman code include:

      Use of <string.h> instead of <memory.h>.

      Use of "brg_endian.h" for endian determination.

      Changing "brg_endian.h" to work with GNU C on non-Linux systems,
      and on SunOS 4.x systems.

      #include <limits.h> instead of "limits.h" in aes.h.

Comments in the code identify the changes.  (Look for "Info-ZIP".)  The
original files are preserved in an "original_files" subdirectory, for
reference.

   The name "IZ_AES_WG" (Info-ZIP AES WinZip/Gladman) is used by
Info-ZIP to identify our implementation of WinZip AES encryption of Zip
entries, using encryption code supplied by Dr. Gladman.  WinZip is a
registered trademark of WinZip International LLC.  PKZIP is a registered
trademark of PKWARE, Inc.

   The source code files from Dr. Gladman are subject to the LICENSE
TERMS at the top of each source file.  The entire IZ_AES_WG kit is
provided under the Info-ZIP license, a copy of which is included in
LICENSE.txt.  The latest version of the Info-ZIP license should be
available at:

      http://www.info-zip.org/license.html

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Building UnZip and Zip with AES_WG Encryption Support
      -----------------------------------------------------

   First, obtain the IZ_AES_WG source kit, which is packaged separately
from the basic UnZip and Zip source kits.  For example, on the Info-ZIP
FTP server, the IZ_AES_WG source kit should be found in:

      ftp://ftp.info-zip.org/pub/infozip/crypt/

The latest kit should be:

      ftp://ftp.info-zip.org/pub/infozip/crypt/iz_aes_wg.zip

however, different UnZip and Zip versions may need particular IZ_AES_WG
kit versions, so, before downloading a particular IZ_AES_WG source kit,
it would be best to consult the INSTALL files in the UnZip and Zip
source kits, and the the Version History section of the current version
of this document: 

      ftp://ftp.info-zip.org/pub/infozip/crypt/README_AES_WG.txt

   The build instructions (INSTALL) in the UnZip and Zip source kits
describe how to unpack the IZ_AES_WG source kit, and how to build UnZip
and Zip with AES_WG encryption.

   The UnZip and Zip README files have additional general information on
AES encryption, and the UnZip and Zip manual pages provide the details
on how to use AES encryption.

   Be aware that some countries or jurisdictions may restrict who may
download and use strong encryption source code and binaries.
Prospective users are responsible for determining whether they are
legally allowed to download and use this encryption software.

   Note that many of the servers that distribute Info-ZIP software are
situated in the United States.  See the latest version of file
USexport_AES_WG.msg for information regarding export from the US. 
Downloads of Info-ZIP encryption software are subject to the limitations
noted.

      ftp://ftp.info-zip.org/pub/infozip/crypt/USexport_AES_WG.msg

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Acknowledgements
      ----------------

   We're grateful to Dr. Gladman for providing the AES encryption code.
Any problems involving AES_WG encryption in Info-ZIP programs should be
reported to the Info-ZIP team, not to Dr. Gladman.  However, any questions
on AES encryption or decryption algorithms, or regarding Gladman's code
(except as we modified and use it) should be addressed to Dr. Gladman.

   We're grateful to WinZip for making the WinZip AES specification
available, and providing the detailed information needed to implement
it.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      IZ_AES_WG Version History
      -------------------------

      1.3  2013-11-18  Renamed USexport.msg to USexport_AES_WG.msg to
                       distinguish it from the Traditional encryption
                       notice, USexport.msg.
      1.2  2013-04-12  Avoid <sys/isa_defs.h> on __sun systems with
                       __sparc defined (for SunOS 4.x).
      1.1  2012-12-31  #include <limits.h> instead of "limits.h" in
                       aes.h (for VAX C).  (SMS)
      1.0  2011-07-07  Minor documentation changes.  (SMS, EG)
                       Compatible with UnZip 6.10 and Zip 3.1.
                       US Department of Commerce BIS notified.
      0.5  2011-07-07  Minor documentation changes.  (SMS, EG)
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.4  2011-06-25  Minor documentation changes.  (SMS, EG)
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.3  2011-06-22  Initial beta version.  (SMS, EG)
      0.2  2011-06-20  Minor documentation updates.  (EG)
      0.1  2011-06-17  Initial alpha version.  (SMS)

