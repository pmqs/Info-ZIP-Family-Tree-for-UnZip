                        README_AES_WG.txt
                        -----------------

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Introduction
      ------------

   The Info-ZIP programs UnZip (version 6.1 and later) and Zip (version
3.1 and later) include optional support for Advanced Encryption Standard
(AES) encryption, a relatively strong encryption method.  This document
describes the Info-ZIP AES implementation, and provides its terms of
use.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

         WARNING
         -------

      The Info-ZIP AES_WG source kit (iz_aes_wg.zip), and the UnZip and
      Zip source kits which include it, are subject to US export control
      laws.  BEFORE downloading or using any version of any of these
      kits, read the following Encryption Notice.  You agree to follow
      these terms (as well as the terms of the Info-ZIP license) when
      you download and/or use any of these source kits.

         Encryption Notice
         -----------------

      This software kit includes encryption software.  The country or
      other jurisdiction where you are may restrict the import,
      possession, use, and/or re-export to another country, of
      encryption software.  BEFORE using any encryption software, please
      check all applicable laws, regulations, and policies concerning
      the import, possession, use, and re-export of encryption software,
      to see if these are permitted.  Some helpful information may be
      found at: http://www.wassenaar.org/

      Export and re-export of this software from the US are governed by
      the US Department of Commerce, Bureau of Industry and Security
      (BIS).  This is open-source ("publicly available") software.
      Info-ZIP has submitted the required notification to the BIS.  The
      details are:
         Export Commodity Control Number (ECCN) 5D002
         License Exception: Technology Software Unrestricted (TSU)
         (Export Administration Regulations (EAR) Section 740.13)

      A copy of the required BIS notification is available in the file
      aes_wg/USexport_aes_wg.msg in the source kits, and at:
         ftp://ftp.info-zip.org/pub/infozip/crypt/USexport_AES_WG.msg

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Description
      -----------

   The Info-ZIP programs UnZip (version 6.1 and later) and Zip (version
3.1 and later) include optional support for Advanced Encryption Standard
(AES) encryption, a relatively strong encryption method.  Traditional
zip encryption, in contrast, is now considered relatively weak and easy
to crack.

   The Info-ZIP AES implementation is based on the WinZip AES
specification, and uses AES encryption code supplied by Brian Gladman.
We refer to it as IZ_AES_WG (Info-ZIP AES WinZip/Gladman) or simply
AES_WG.  The WinZip AES scheme is described in:

      http://www.winzip.com/aes_info.htm

   Briefly, the WinZip AES scheme uses compression method 99 to flag an
AES-encrypted archive member.  (In contrast, PKZIP AES encryption uses
an incompatible scheme with different archive data structures.  However,
current versions of PKZIP may also be able to process WinZip AES
encrypted archive entries.)

   The IZ_AES_WG implementation supports 128-, 192-, and 256-bit keys.
See the various discussions of WinZip AES encryption on the Internet for
more on the security of the WinZip AES encryption implementation.

   The IZ_AES_WG source kit is based on an AES source kit provided by
Brian Gladman, which we obtained at:

      http://gladman.plushost.co.uk/oldsite/cryptography_technology/
      fileencrypt/files.zip

with one header file ("brg_endian.h") from a newer kit:

      http://gladman.plushost.co.uk/oldsite/AES/aes-src-11-01-11.zip

(Non-Windows users should use "unzip -a" when unpacking those kits.)

   That site is no longer active, so the only independent sources for
those kits may be Internet archive providers.  Dr. Gladman's current AES
material may be found at:

      http://www.gladman.me.uk/AES
      https://github.com/BrianGladman/AES

   We chose the code from the older Gladman code kits primarily to
ensure compatibility with WinZip.  We made some changes to it to improve
its portability to different hardware and operating systems.

   The portability-related changes to the original Gladman code include:

      Use of <string.h> instead of <memory.h>.

      Use of "brg_endian.h" for endian determination.

      Changing "brg_endian.h" to work with GNU C on non-Linux systems,
      and on SunOS 4.x systems.

      #include <limits.h> instead of "limits.h" in aes.h.

      Changing some "long" types to "int" or "sha1_32t" in hmac.c and
      hmac.h to accommodate systems (like Mac OS X on Intel) where a
      64-bit "long" type caused bad results.

   Comments in the code identify the changes.  (Look for "Info-ZIP".)
The IZ_AES_WG kit includes the original files from those kits, as well
as files which we have modified.  The original forms of modified files
are preserved in an "orig" subdirectory, for reference.

   The name "IZ_AES_WG" (Info-ZIP AES WinZip/Gladman) is used by
Info-ZIP to identify our implementation of WinZip AES encryption of Zip
archive members, using encryption code supplied by Dr. Gladman.  WinZip
is a registered trademark of WinZip International LLC.  PKZIP is a
registered trademark of PKWARE, Inc.

   The source code files from Dr. Gladman are subject to the LICENSE
TERMS at the top of each source file.  The entire IZ_AES_WG kit is
provided under the Info-ZIP license, a copy of which is included in the
file LICENSE in the source kits.  The latest version of the Info-ZIP
license should be available at:

      http://www.info-zip.org/license.html

   NOTE: The IZ_AES_WG source kit is intended for use with the Info-ZIP
         UnZip and Zip programs ONLY.  Any other use is unsupported and
         not recommended.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      Building UnZip and Zip with AES_WG Encryption Support
      -----------------------------------------------------

   The IZ_AES_WG source kit is normally included in the basic UnZip and
Zip source kits, and its files should be found in an "aes_wg"
subdirectory.

   The build instructions (INSTALL) in the UnZip and Zip source kits
describe how to build UnZip and Zip with (or without) support for AES_WG
encryption.

   The UnZip and Zip README files have additional general information on
AES encryption, and the UnZip and Zip manual pages provide the details
on how to use AES encryption in these programs.

   Be aware that some countries or jurisdictions may restrict who may
download or use strong encryption source code and binaries. Prospective
users are responsible for determining whether they are legally allowed
to download or use this encryption software.

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

      1.6  2017-01-23  Revised to reflect the inclusion of the IZ_AES_WG
                       sources in the UnZip and Zip source kits.  [SMS]
      1.5  2016-07-15  Updated USexport_AES_WG.msg to current version.
                       Updated zip-comment.txt.  Updated this file
                       (README_AES_WG.txt) to include legal notices
                       and updates.  [EG]
      1.4  2015-04-03  Changed "long" types to "int" for counters, and
                       to "sha1_32t" for apparent 32-bit byte groups,
                       where a 64-bit "long" type caused bad results (on
                       Mac OS X, Intel).  (hmac.c, hmac.h) [SMS]
      1.3  2013-11-18  Renamed USexport.msg to USexport_AES_WG.msg to
                       distinguish it from the Traditional encryption
                       notice, USexport.msg.  [SMS]
      1.2  2013-04-12  Avoid <sys/isa_defs.h> on __sun systems with
                       __sparc defined (for SunOS 4.x).  (brg_endian.h)
                       [SMS]
      1.1  2012-12-31  #include <limits.h> instead of "limits.h" in
                       aes.h (for VAX C).  [SMS]
      1.0  2011-07-07  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
                       US Department of Commerce BIS notified.
      0.5  2011-07-07  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.4  2011-06-25  Minor documentation changes.  [SMS, EG]
                       Compatible with UnZip 6.10 and Zip 3.1.
      0.3  2011-06-22  Initial beta version.  [SMS, EG]
      0.2  2011-06-20  Minor documentation updates.  [EG]
      0.1  2011-06-17  Initial alpha version.  [SMS]

