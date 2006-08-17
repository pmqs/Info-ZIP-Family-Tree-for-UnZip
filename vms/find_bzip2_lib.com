$!                                              17 November 2005.  SMS.
$!
$! Info-ZIP VMS accessory procedure.
$!
$! Find the BZIP2 object library under P1, starting in the [.'P2']
$! destination directory.  (We assume, initially, that the BZIP2
$! directory has a destination directory structure like ours.)
$!
$! Set the P3 logical name to the directory where it was found.
$! P4 and P5 may be used for qualifiers on the DEFINE command.
$!
$ bz_orig = p1
$ dest = p2
$ libbz2 = "libbz2.olb"
$!
$! Remove any trailing colon, to allow logical name translation.
$!
$ bz_dev_dir = ""
$ bz_base = bz_orig
$ if (f$extract( (f$length( bz_base)- 1), 1, bz_base) .eqs. ":")
$ then
$     bz_base = bz_base- ":"
$ endif
$!
$ bz_base_eqv = f$trnlnm( bz_base)
$ if (bz_base_eqv .nes. "")
$ then
$     bz_orig = bz_base_eqv
$     bz_base = bz_base_eqv
$ endif
$ bz_base = bz_base- "]"
$!
$! Candidate 1 = the actual analogue destination directory.
$!
$ bz_dev_dir_cand = bz_base+ "."+ dest+ "]"
$ lib_cand = bz_dev_dir_cand+ libbz2
$ if (f$search( lib_cand) .nes. "")
$ then
$     bz_dev_dir = bz_dev_dir_cand
$ else
$!
$!     Candidate 2 = the actual analogue destination directory + "L".
$!
$     bz_dev_dir_cand = bz_base+ "."+ dest+ "L]"
$     lib_cand = bz_dev_dir_cand+ libbz2
$     if (f$search( lib_cand) .nes. "")
$     then
$         bz_dev_dir = bz_dev_dir_cand
$     else
$!
$!         Candidate 3 = the actual user-specified directory.
$!
$         bz_dev_dir_cand = bz_orig
$         lib_cand = bz_dev_dir_cand+ libbz2
$         if (f$search( lib_cand) .nes. "")
$         then
$             bz_dev_dir = bz_dev_dir_cand
$         endif
$     endif
$ endif
$!
$ if (bz_dev_dir .nes. "")
$ then
$     if (p3 .eqs. "")
$     then
$         write sys$output bz_dev_dir
$     else
$         define 'p4' 'p3' 'bz_dev_dir' 'p5'
$     endif
$ endif
$!
