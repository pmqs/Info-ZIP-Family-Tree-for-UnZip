/* 2005-11-17 SMS.
 * VMS-specific BZLIB.H jacket header file to ensure compatibility with
 * BZIP2 code compiled using /NAMES = AS_IS.
 *
 * The logical name INCL_BZIP2 must point to the BZIP2 source directory.
 */

#pragma names save
#pragma names as_is

#include "INCL_BZIP2:BZLIB.H"

#pragma names restore
