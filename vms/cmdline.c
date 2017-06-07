/*
  Copyright (c) 1990-2017 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2009-Jan-02 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/


/*    Stand-alone test procedure:
 *
 * cc /define = TEST=1 /include = [] /object = [.vms] [.vms]cmdline.c
 * set command /object = [.vms]unz_cli.obj [.vms]unz_cli.cld
 * link /executable = [] [.vms]cmdline.obj, [.vms]unz_cli.obj
 * EXEC*UTE == "$ SYS$DISK:[]'"
 * exec cmdline [ /qualifiers ...] [parameters ...]
 */


/* 2004-12-13 SMS.
 * Disabled the module name macro to accommodate old GNU C which didn't
 * obey the directive, and thus confused MMS/MMK where the object
 * library dependencies need to have the correct module name.
 */
#if 0
#define module_name VMS_UNZIP_CMDLINE
#define module_ident "02-018"
#endif /* 0 */

/*
**
**  Facility:   UNZIP
**
**  Module:     VMS_UNZIP_CMDLINE
**
**  Author:     Hunter Goatley <goathunter@MadGoat.com>
**
**  Date:       25 Apr 97 (orig. Zip version, 30 Jul 93)
**
**  Abstract:   Routines to handle a VMS CLI interface for UnZip.  The CLI
**              command line is parsed and a new argc/argv are built and
**              returned to UnZip.
**
**  Modified by:
**
**      02-018          S. Schweda              18-Apr-2015
**              Added /VERBOSE = BRIEF.
**      02-017          S. Schweda              06-Oct-2012
**              Added /DECIMAL_TIME, /[NO]JAR, /JUNK_DIRS=n,
**              /MATCH (replacing /CASE_MATCH).  Restored and deprecated
**              /[NO]CASE_INSENSITIVE.  Changed /NAMES = [NO]LOWERCASE
**              to /NAMES = [NO]DOWNCASE.  Added Zip-like
**              /VERBOSE={NORMAL|MORE}, deprecating /BRIEF and /FULL.
**
**      02-016          S. Schweda              11-Aug-2011 17:00
**              Added /NAMES = [ [NO]LOWERCASE | [NO]ODS2 | [NO]SPACES ].
**
**      02-015          S. Schweda              31-Jul-2010 22:00
**              Rewrote option conversion code to use a separate argv[]
**              member for each option, eliminating combined short
**              options.  Changed /ZIPINFO /COMMENT to use "-z" instead
**              of (incorrect) "-c".  Removed traces of /EXTRACT.  Added
**              /VERBOSE = COMMAND.  Replaced /[NO]CASE_INSENSITIVE with
**              /CASE_MATCH.  Added (and used) macros DESCRIPTOR_D and
**              ADD_ARG.
**
**      02-014          E. Gordon               08-Jul-2010 16:41
**              Modified to work with get_options() and new command table.
**      02-013          S. Schweda, C. Spieler  29-Dec-2007 03:34
**              Extended /RESTORE qualifier to support timestamp restoration
**              options.
**      02-012          Steven Schweda          07-Jul-2006 19:04
**              Added /TEXT=STMLF qualifier option.
**      02-011          Christian Spieler       21-Apr-2005 01:23
**              Added /FULL=DIAGNOSTICS option modifier.
**      02-010          Steven Schweda          14-FEB-2005 20:04
**              Added /DOT_VERSION (-Y) and /ODS2 (-2) qualifiers.
**      02-009          Steven Schweda          28-JAN-2005 16:16
**              Added /TIMESTAMP (-T) qualifier.
**      02-008          Christian Spieler       08-DEC-2001 23:44
**              Added support for /TRAVERSE_DIRS argument
**      02-007          Christian Spieler       24-SEP-2001 21:12
**              Escape verbatim '%' chars in format strings; version unchanged.
**      02-007          Onno van der Linden     02-Jul-1998 19:07
**              Modified to support GNU CC 2.8 on Alpha; version unchanged.
**      02-007          Johnny Lee              25-Jun-1998 07:38
**              Fixed typo (superfluous ';'); no version num change.
**      02-007          Hunter Goatley          11-NOV-1997 10:38
**              Fixed "zip" vs. "unzip" typo; no version num change.
**      02-007          Christian Spieler       14-SEP-1997 22:43
**              Cosmetic mods to stay in sync with Zip; no version num change.
**      02-007          Christian Spieler       12-JUL-1997 02:05
**              Revised argv vector construction for better handling of quoted
**              arguments (e.g.: embedded white space); no version num change.
**      02-007          Christian Spieler       04-MAR-1997 22:25
**              Made /CASE_INSENSITIVE common to UnZip and ZipInfo mode;
**              added support for /PASSWORD="decryption_key" argument.
**      02-006          Christian Spieler       11-MAY-1996 22:40
**              Added SFX version of VMSCLI_usage().
**      02-005          Patrick Ellis           09-MAY-1996 22:25
**              Show UNIX style usage screen when UNIX style options are used.
**      02-004          Christian Spieler       06-FEB-1996 02:20
**              Added /HELP qualifier.
**      02-003          Christian Spieler       23-DEC-1995 17:20
**              Adapted to UnZip 5.2.
**      02-002          Hunter Goatley          16-JUL-1994 10:20
**              Fixed some typos.
**      02-001          Cave Newt               14-JUL-1994 15:18
**              Removed obsolete /EXTRACT option; fixed /*TEXT options;
**              wrote VMSCLI usage() function
**      02-000          Hunter Goatley          12-JUL-1994 00:00
**              Original UnZip version (v5.11).
**      01-000          Hunter Goatley          30-JUL-1993 07:54
**              Original version (for Zip v1.9p1).
**
*/



/* 2004-12-13 SMS.
 * Disabled the module name macro to accommodate old GNU C which didn't
 * obey the directive, and thus confused MMS/MMK where the object
 * library dependencies need to have the correct module name.
 */
#if 0
#if defined(__DECC) || defined(__GNUC__)
#pragma module module_name module_ident
#else
#module module_name module_ident
#endif
#endif /* 0 */

#define UNZIP_INTERNAL
#include "unzip.h"

/* Workaround for broken header files of older DECC distributions
 * that are incompatible with the /NAMES=AS_IS qualifier. */
/* - lib$routines.h definitions: */
#define lib$establish LIB$ESTABLISH
#define lib$get_foreign LIB$GET_FOREIGN
#define lib$get_input LIB$GET_INPUT
#define lib$sig_to_ret LIB$SIG_TO_RET
/* - str$routines.h definitions: */
#define str$concat STR$CONCAT
#define str$find_first_substring STR$FIND_FIRST_SUBSTRING

#include <ssdef.h>
#include <descrip.h>
#include <climsgdef.h>
#include <clidef.h>
#include <lib$routines.h>
#include <str$routines.h>

#ifndef CLI$_COMMA
globalvalue CLI$_COMMA;
#endif

/*
 * DESCRIPTOR_D macro.  Like $DESCRIPTOR, but with:
 *    dsc$descriptor_s  ->  dsc$descriptor_d
 *       DSC$K_CLASS_S  ->  DSC$K_CLASS_D
 *              string  ->  (NULL)
 */
#define DESCRIPTOR_D( name) struct dsc$descriptor_d name = \
 { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, NULL }

/*
 *  Memory allocation block size for argv string buffer.
 */
#define ARGBSIZE_UNIT 256

/*
 *  Memory reallocation macro for argv string buffer.
 */
#define CHECK_BUFFER_ALLOCATION( buf, reserved, requested) { \
    if ((requested) > (reserved)) { \
        char *save_buf = (buf); \
        (reserved) += ARGBSIZE_UNIT; \
        if (((buf) = (char *)izu_realloc( (buf), (reserved))) == NULL) { \
            if (save_buf != NULL)izu_free( save_buf); \
            return SS$_INSFMEM; \
        } \
    } \
}

/*
 * Macro to add an argument to argv string buffer.
 */
#define ADD_ARG( opt) \
    x = cmdl_len; \
    cmdl_len += strlen( opt)+ 1; \
    CHECK_BUFFER_ALLOCATION( the_cmd_line, cmdl_size, cmdl_len) \
    strcpy( &the_cmd_line[ x], opt);


/*
**  Define descriptors for all of the CLI parameters and qualifiers.
*/
#if 0
$DESCRIPTOR(cli_extract,        "EXTRACT");             /* obsolete */
#endif
$DESCRIPTOR(cli_text,           "TEXT");                /* -a[a] */
$DESCRIPTOR(cli_text_auto,      "TEXT.AUTO");           /* -a */
$DESCRIPTOR(cli_text_all,       "TEXT.ALL");            /* -aa */
$DESCRIPTOR(cli_text_none,      "TEXT.NONE");           /* -a- */
$DESCRIPTOR(cli_text_stmlf,     "TEXT.STMLF");          /* -S */
$DESCRIPTOR(cli_binary,         "BINARY");              /* -b[b] */
$DESCRIPTOR(cli_binary_auto,    "BINARY.AUTO");         /* -b */
$DESCRIPTOR(cli_binary_all,     "BINARY.ALL");          /* -bb */
$DESCRIPTOR(cli_binary_none,    "BINARY.NONE");         /* -b- */
$DESCRIPTOR(cli_case_insensitive, "CASE_INSENSITIVE");  /* -C */
$DESCRIPTOR(cli_match,          "MATCH");               /* -C, -W */
$DESCRIPTOR(cli_match_case,     "MATCH.CASE");          /* -C[-] */
$DESCRIPTOR(cli_match_case_blind, "MATCH.CASE.BLIND");    /* -C */
$DESCRIPTOR(cli_match_case_sens, "MATCH.CASE.SENSITIVE"); /* -C- */
#ifdef WILD_STOP_AT_DIR
$DESCRIPTOR(cli_match_wild,     "MATCH.WILD_MATCH_SLASH"); /* -W[-] */
#endif /* def WILD_STOP_AT_DIR */
$DESCRIPTOR(cli_screen,         "SCREEN");              /* -c */
$DESCRIPTOR(cli_directory,      "DIRECTORY");           /* -d */
$DESCRIPTOR(cli_auto_dir,       "AUTO_DIRECTORY");      /* -da */
$DESCRIPTOR(cli_auto_dir_reuse, "AUTO_DIRECTORY.REUSE"); /* -da=reuse */
$DESCRIPTOR(cli_freshen,        "FRESHEN");             /* -f */
$DESCRIPTOR(cli_help,           "HELP");                /* -h */
$DESCRIPTOR(cli_help_normal,    "HELP.NORMAL");         /* -h */
$DESCRIPTOR(cli_help_extended,  "HELP.EXTENDED");       /* -hh */
$DESCRIPTOR(cli_jar,            "JAR");                 /* --jar */
$DESCRIPTOR(cli_junk_dirs,      "JUNK_DIRS");           /* -j */
$DESCRIPTOR(cli_lowercase,      "LOWERCASE");           /* -L */
$DESCRIPTOR(cli_license,        "LICENSE");             /* --license */
$DESCRIPTOR(cli_list,           "LIST");                /* -l */
$DESCRIPTOR(cli_brief,          "BRIEF");               /* -l */
$DESCRIPTOR(cli_page,           "PAGE");                /* -M , -ZM */
$DESCRIPTOR(cli_full,           "FULL");                /* -v */
$DESCRIPTOR(cli_full_diags,     "FULL.DIAGNOSTICS");    /* -vv */
$DESCRIPTOR(cli_existing,       "EXISTING");            /* -o, -oo, -n */
$DESCRIPTOR(cli_exist_newver,   "EXISTING.NEW_VERSION"); /* -o */
$DESCRIPTOR(cli_exist_over,     "EXISTING.OVERWRITE");  /* -oo */
$DESCRIPTOR(cli_exist_noext,    "EXISTING.NOEXTRACT");  /* -n */
$DESCRIPTOR(cli_overwrite,      "OVERWRITE");           /* -o, -n */
$DESCRIPTOR(cli_quiet,          "QUIET");               /* -q */
$DESCRIPTOR(cli_super_quiet,    "QUIET.SUPER");         /* -qq */
$DESCRIPTOR(cli_test,           "TEST");                /* -t */
$DESCRIPTOR(cli_pipe,           "PIPE");                /* -p */
$DESCRIPTOR(cli_password,       "PASSWORD");            /* -P */
$DESCRIPTOR(cli_names_char_set, "NAMES.CHAR_SET");      /* -0, -0- */
$DESCRIPTOR(cli_names_down,     "NAMES.DOWNCASE");      /* -L, -LL */
$DESCRIPTOR(cli_names_down_all, "NAMES.DOWNCASE.ALL");  /* -LL */
$DESCRIPTOR(cli_names_nodown,   "NAMES.NODOWNCASE");    /* -L-L- */
$DESCRIPTOR(cli_names_ods2,     "NAMES.ODS2");          /* -2 */
$DESCRIPTOR(cli_names_spaces,   "NAMES.SPACES");        /* -s */
$DESCRIPTOR(cli_timestamp,      "TIMESTAMP");           /* -T */
$DESCRIPTOR(cli_uppercase,      "UPPERCASE");           /* -U */
$DESCRIPTOR(cli_update,         "UPDATE");              /* -u */
$DESCRIPTOR(cli_version,        "VERSION");             /* -V */
$DESCRIPTOR(cli_restore,        "RESTORE");             /* -k, -ka, -X */
$DESCRIPTOR(cli_restore_acl,    "RESTORE.ACL");         /* -ka */
$DESCRIPTOR(cli_restore_own,    "RESTORE.OWNER");       /* -X- */
$DESCRIPTOR(cli_restore_prot,   "RESTORE.PROTECTION");  /* -k, -k- */
$DESCRIPTOR(cli_restore_prot_lim, "RESTORE.PROTECTION.LIMITED"); /* -kk -k */
$DESCRIPTOR(cli_restore_prot_orig, "RESTORE.PROTECTION.ORIGINAL"); /* -k */
$DESCRIPTOR(cli_restore_date,   "RESTORE.DATE");        /* -DD */
$DESCRIPTOR(cli_restore_date_all, "RESTORE.DATE.ALL");  /* -D- */
$DESCRIPTOR(cli_restore_date_files, "RESTORE.DATE.FILES"); /* -D */
$DESCRIPTOR(cli_dot_version,    "DOT_VERSION");         /* -Y */
$DESCRIPTOR(cli_comment,        "COMMENT");             /* -z, -Zz */
$DESCRIPTOR(cli_exclude,        "EXCLUDE");             /* -x */
$DESCRIPTOR(cli_ods2,           "ODS2");                /* -2 */
$DESCRIPTOR(cli_traverse,       "TRAVERSE_DIRS");       /* -: */

$DESCRIPTOR(cli_zipinfo,        "ZIPINFO");             /* -Z */
$DESCRIPTOR(cli_header,         "HEADER");              /* -Z -h */
$DESCRIPTOR(cli_long,           "LONG");                /* -Z =l */
$DESCRIPTOR(cli_medium,         "MEDIUM");              /* -Zm */
$DESCRIPTOR(cli_member_counts,  "MEMBER_COUNTS");       /* -Z -mc */
$DESCRIPTOR(cli_one_line,       "ONE_LINE");            /* -Z -2 */
$DESCRIPTOR(cli_short,          "SHORT");               /* -Z -s */
$DESCRIPTOR(cli_decimal_time,   "DECIMAL_TIME");        /* -Z -T */
$DESCRIPTOR(cli_times,          "TIMES");               /* -Z -T */
$DESCRIPTOR(cli_totals,         "TOTALS");              /* -Z -t */
$DESCRIPTOR(cli_noverbose,      "NOVERBOSE");           /* -v-, -Z -v- */
$DESCRIPTOR(cli_verbose,        "VERBOSE");             /* -v, -Z -v, -vq */
$DESCRIPTOR(cli_verbose_brief,  "VERBOSE.BRIEF");       /* -vq */
$DESCRIPTOR(cli_verbose_command, "VERBOSE.COMMAND");    /* (none) */
$DESCRIPTOR(cli_verbose_more,   "VERBOSE.MORE");        /* -vv, -Z -vv */
$DESCRIPTOR(cli_verbose_normal, "VERBOSE.NORMAL");      /* -v, -Z -v */

$DESCRIPTOR(cli_yyz,            "YYZ_UNZIP");

$DESCRIPTOR(cli_zipfile,        "ZIPFILE");
$DESCRIPTOR(cli_infile,         "INFILE");
$DESCRIPTOR(unzip_command,      "unzip ");

static int show_VMSCLI_usage;
static int verbose_command = 0;

#ifndef vms_unzip_cld
#  define vms_unzip_cld VMS_UNZIP_CLD
#endif
#if defined(__DECC) || defined(__GNUC__)
extern void *vms_unzip_cld;
#else
globalref void *vms_unzip_cld;
#endif

/* extern unsigned int LIB$GET_INPUT(void), LIB$SIG_TO_RET(void); */

/*
 * Old systems may lack <cli$routines.h>, so we provide the important
 * stuff.
 */
#ifndef cli$dcl_parse
#  define cli$dcl_parse CLI$DCL_PARSE
#endif
#ifndef cli$present
#  define cli$present CLI$PRESENT
#endif
#ifndef cli$get_value
#  define cli$get_value CLI$GET_VALUE
#endif
extern unsigned int cli$dcl_parse();
extern unsigned int cli$present();
extern unsigned int cli$get_value();

static unsigned int get_list( struct dsc$descriptor_s *,
                              struct dsc$descriptor_d *,
                              int,
                              char **,
                              unsigned int *,
                              unsigned int *);
static unsigned int check_cli( struct dsc$descriptor_s *);


unsigned int
vms_unzip_cmdline (int *argc_p, char ***argv_p)
{
/*
**  Routine:    vms_unzip_cmdline
**
**  Function:
**
**      Parse the DCL command line and create a replacement argv array
**      to be passed to UnZip.
**
**      NOTE: the argv[] is built as we go, so all the parameters are
**      checked in the appropriate order!!
**
**  Formal parameters:
**
**      argc_p          - Address of int to receive the new argc
**      argv_p          - Address of char ** to receive the argv address
**
**  Calling sequence:
**
**      status = vms_unzip_cmdline (&argc, &argv);
**
**  Returns:
**
**      SS$_NORMAL      - Success.
**      SS$_INSFMEM     - A malloc() or realloc() failed
**      SS$_ABORT       - Bad time value
**
**  Modified to work with the get_option() command line parser.  08 July 2010
**
*/
    char *opt;
    char *ptr;
    char *the_cmd_line;                 /* buffer for argv strings */
    unsigned int cmdl_size;             /* allocated size of buffer */
    unsigned int cmdl_len;              /* used size of buffer */
    unsigned int status;
    int exclude_list;
    int x;
    int zipinfo;

    int new_argc;                       /* Arg count for new arg vector. */
    char **new_argv;                    /* New arg vector. */

    DESCRIPTOR_D( work_str);
    DESCRIPTOR_D( foreign_cmdline);
    DESCRIPTOR_D( output_directory);    /* Arg for /DIRECTORY = dir_name. */
    DESCRIPTOR_D( junk_dirs_arg);       /* Arg for /JUNK_DIRS = level. */
    DESCRIPTOR_D( password_arg);        /* Arg for /PASSWORD = passwd. */

    /*
    **  See if the program was invoked by the CLI (SET COMMAND) or by
    **  a foreign command definition.  Check for /YYZ_UNZIP, which is a
    **  valid default qualifier solely for this test.
    */
    show_VMSCLI_usage = TRUE;
    status = check_cli(&cli_yyz);
    if (!(status & 1)) {
        lib$get_foreign(&foreign_cmdline);
        /*
        **  If nothing was returned or the first character is a "-", then
        **  assume it's a UNIX-style command and return.
        */
        if (foreign_cmdline.dsc$w_length == 0)
            return (SS$_NORMAL);
        if ((*(foreign_cmdline.dsc$a_pointer) == '-') ||
            ((foreign_cmdline.dsc$w_length > 1) &&
             (*(foreign_cmdline.dsc$a_pointer) == '"') &&
             (*(foreign_cmdline.dsc$a_pointer + 1) == '-'))) {
            show_VMSCLI_usage = FALSE;
            return SS$_NORMAL;
        }
        str$concat(&work_str, &unzip_command, &foreign_cmdline);
        status = cli$dcl_parse(&work_str, &vms_unzip_cld, lib$get_input,
                        lib$get_input, 0);
        if (!(status & 1)) return status;
    }

    /*
     *  There will always be a new_argv[] because of the image name.
     */
    if ((the_cmd_line = (char *)izu_malloc(cmdl_size = ARGBSIZE_UNIT)) == NULL)
        return SS$_INSFMEM;

#define UNZIP_COMMAND_NAME "unzip"

    strcpy( the_cmd_line, UNZIP_COMMAND_NAME);
    cmdl_len = sizeof( UNZIP_COMMAND_NAME);

    /*
     * UnZip or ZipInfo?
     */
    zipinfo = 0;
    status = cli$present( &cli_zipinfo);
    if (status & 1)
    {
        /* ZipInfo ("unzip -Z"). */
        zipinfo = 1;
        /* Put out "-Z" option first. */
#define IPT__Z   "-Z"           /* "-Z"  ZipInfo. */
        ADD_ARG( IPT__Z);
    }
    else
    {
        /* UnZip (normal). */
        /* Process special qualifiers (those with deferred or no
         * new-command-line activity).
         */

        /*
         * Extract destination directory (-d).
         */
        status = cli$present( &cli_directory);
        if (status == CLI$_PRESENT)
        {
            /* /DIRECTORY = destination_dir */
            status = cli$get_value( &cli_directory, &output_directory);
        }

        /*
         * Decryption password from command line.
         */
        status = cli$present( &cli_password);
        if (status == CLI$_PRESENT)
        {
            /* /PASSWORD = passwd */
            status = cli$get_value( &cli_password, &password_arg);
        }
    } /* ZipInfo [else] */

    /* Process options common to UnZip and ZipInfo. */

    /* Process special qualifiers (those with deferred or no
     * new-command-line activity).
     */

    /*
    **  Check existence of a list of files to exclude, fetch is done later.
    */
    status = cli$present(&cli_exclude);
    exclude_list = ((status & 1) != 0);

    /*
     * Show license text.
     */
#define OPT_LI   "--license"    /* "--license  Show license text. */

    status = cli$present( &cli_license);
    if (status & 1)
    {
        ADD_ARG( OPT_LI);
    }

    /*
     * Verbose command-line translation.
     */
    status = cli$present( &cli_verbose_command);
    if (status & 1)
    {
        /* /VERBOSE = COMMAND */
        verbose_command = 1;
    }

    /*
     * Member name matching:
     * Case-sensitivity (-C), and
     * Wildcards match directory slash (-W-).
     */
#define OPT__C   "-C"           /* "-C"  Case-blind matching. */
#define OPT__CN  "-C-"          /* ""    Case-sensitive matching (default). */
#define OPT__W   "-W"           /* "-W"  Wildcards stop at dir slash. */
#define OPT__WN  "-W-"          /* ""    Wildcards match dir slash (dflt). */

    status = cli$present( &cli_match_case);
    if (status & 1)
    {
        status = cli$present( &cli_match_case_blind);
        if (status & 1)
        {
            /* /MATCH = CASE = BLIND */
            opt = OPT__C;
        }
        else
        {
            /* /MATCH = CASE = SENSITIVE */
            opt = OPT__CN;
        }
        ADD_ARG( opt);
    }

    /* Deprecated.
     * Filename matching case-sensitivity (-C)
     * Clear any existing "-C" option with "-C-", then add
     * the desired "C" value.
     */

    status = cli$present( &cli_case_insensitive);
    if ((status & 1) || (status == CLI$_NEGATED))
    {
        if (status == CLI$_NEGATED)
        {
            /* /NOCASE_INSENSITIVE */
            opt = OPT__CN;
        }
        else
        {
            /* /CASE_INSENSITIVE */
            opt = OPT__C;
        }
        ADD_ARG( opt);
    }

#ifdef WILD_STOP_AT_DIR
    status = cli$present( &cli_match_wild);
    if ((status & 1) || (status == CLI$_NEGATED))
    {
        if (status == CLI$_NEGATED)
        {
            /* /MATCH = NOWILD_MATCH_SLASH */
            opt = OPT__W;
        }
        else
        {
            /* /MATCH = WILD_MATCH_SLASH */
            opt = OPT__WN;
        }
        ADD_ARG( opt);
    }
#endif /* def WILD_STOP_AT_DIR */

    /*
     * Use built-in ("more") pager for all screen output.
     * Clear any existing "-M" option with "-M-", then add
     * the desired "M" value.
     */
#define OPT__M   "-M-M"         /* "-M"  Use built-in "more" pager. */
#define OPT__MN  "-M-"          /* ""    Use no built-in pager (default). */

    status = cli$present( &cli_page);
    if ((status & 1) || (status == CLI$_NEGATED))
    {
        if (status == CLI$_NEGATED)
        {
            /* /NOPAGE */
            opt = OPT__MN;
        }
        else
        {
            /* /PAGE */
            opt = OPT__M;
        }
        ADD_ARG( opt);
    }

    /*
     * Display (only) the archive comment.
     * Clear any existing "-z" option with "-z-", then add
     * the desired "z" value.
     */
#define OPT_Z   "-z-z"          /* "-z"  Display the comment (only). */
#define OPT_ZN  "-z-"           /* ""    Don't display the comment (only). */

    status = cli$present( &cli_comment);
    if ((status & 1) || (status == CLI$_NEGATED))
    {
        if (status == CLI$_NEGATED)
        {
            /* /NOCOMMENT */
            opt = OPT_ZN;
        }
        else
        {
            /* /COMMENT */
            opt = OPT_Z;
        }
        ADD_ARG( opt);
    }

    if (zipinfo == 0)
    {
        /* UnZip (normal) options. */

        /*
         * Convert files as text (CR LF -> LF, etc.)
         * Clear any existing "-a[a]" options with "-a-a-", then add
         * the desired "a" and/or "S" value(s).
         */
#define OPT_A   "-a-a-a"        /* "-a"  auto-convert text files. */
#define OPT_AA  "-a-a-aa"       /* "-aa" convert all files as text. */
#define OPT_AN  "-a-a-"         /* ""    convert no files as text. */

#define OPT__S  "-S"            /* "-S"  Use Stream_LF for text files. */

        status = cli$present( &cli_text);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTEXT */
                opt = OPT_AN;
            }
            else
            {
                /* /TEXT */
                status = cli$present( &cli_text_none);
                if (status & 1)
                {
                    /* /TEXT = NONE */
                    opt = OPT_AN;
                }
                else
                {
                    status = cli$present( &cli_text_all);
                    if (status & 1)
                    {
                         /* /TEXT = ALL */
                         opt = OPT_AA;
                    }
                    else
                    {
                         /* /TEXT or /TEXT = AUTO */
                         opt = OPT_A;
                    }
                }
            }
            ADD_ARG( opt);

            status = cli$present( &cli_text_stmlf);
            if (status & 1)
            {
                /* /TEXT = STMLF */
                ADD_ARG( OPT__S);
            }
        }

        /*
         * Write binary files in VMS binary (fixed-length, 512-byte
         * records, record attributes: none) format.
         * Clear any existing "-b[b]" options with "-b-b-", then add
         * the desired "b" value.
         */
#define OPT_B   "-b-b-b"        /* "-b"  Fixed-512 for binary files. */
#define OPT_BB  "-b-b-bb"       /* "-bb" Fixed-512 for all files. */
#define OPT_BN  "-b-b-"         /* ""    Fixed-512 for no files. */

        status = cli$present( &cli_binary);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOBINARY */
                opt = OPT_BN;
            }
            else
            {
                /* /BINARY */
                status = cli$present( &cli_binary_none);
                if (status & 1)
                {
                    /* /BINARY = NONE */
                    opt = OPT_BN;
                }
                else
                {
                    status = cli$present( &cli_binary_all);
                    if (status & 1)
                    {
                         /* /BINARY = ALL */
                         opt = OPT_BB;
                    }
                    else
                    {
                         /* /BINARY or /BINARY = AUTO */
                         opt = OPT_B;
                    }
                }
            }
            ADD_ARG( opt);
        }

        /*
         * Extract files to screen/stdout.
         * Clear any existing "-c" option with "-c-", then add
         * the desired "c" value.
         */
#define OPT_C   "-c-c"          /* "-c"  Extract to screen/stdout. */
#define OPT_CN  "-c-"           /* ""    Extract to files (default). */

        status = cli$present( &cli_screen);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOSCREEN */
                opt = OPT_CN;
            }
            else
            {
                /* /SCREEN */
                opt = OPT_C;
            }
            ADD_ARG( opt);
        }

        /*
         * Restore file/directory date-times.
         * Clear any existing "-D[D]" options with "-D-D-", then add
         * the desired "D" value.  Note that on VMS, D_flag=1 ("-D") is
         * the default.
         */
#define OPT__D   "-D-D-DD"      /* "-D"  Restore no date-times. */
#define OPT__DD  "-D-D-D"       /* ""    Restore only file date-times. */
#define OPT__DN  "-D-D-"        /* "-D-" Restore file and dir date-times. */

        status = cli$present( &cli_restore_date);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /RESTORE = NODATE */
                opt = OPT__D;
            }
            else
            {
                /* /RESTORE = DATE */
                status = cli$present(&cli_restore_date_all);
                if (status & 1)
                {
                    /* /RESTORE = (DATE = ALL) */
                    opt = OPT__DN;
                }
                else
                {
                    /* /RESTORE = (DATE = FILES) */
                    opt = OPT__DD;
                }
            }
            ADD_ARG( opt);
        }

        /* Automatic destination directory.
         * Add the desired "-da[-]" value.
         */
#define OPT_DA  "-da"           /* "-da" Use auto destination dir. */
#define OPT_DAN "-da-"          /* ""    Normal destination dir (default). */
#define OPT_DAR "-da=reuse"     /* "-da=reuse" Allow existing auto dest dir. */

        status = cli$present( &cli_auto_dir);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOAUTO_DIRECTORY */
                opt = OPT_DAN;
            }
            else
            {
                status = cli$present( &cli_auto_dir_reuse);
                if (status & 1)
                {
                    /* /AUTO_DIRECTORY=REUSE */
                    opt = OPT_DAR;
                }
                else
                {
                    /* /AUTO_DIRECTORY */
                    opt = OPT_DA;
                }
            }
            ADD_ARG( opt);
        }

        /* Freshen existing files, create none.
         * Clear any existing "-f" option with "-f-", then add
         * the desired "f" value.
         */
#define OPT_F   "-f-f"          /* "-f"  Freshen existing files, create none. */
#define OPT_FN  "-f-"           /* ""    Normal extract (default). */

        status = cli$present( &cli_freshen);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOFRESHEN */
                opt = OPT_FN;
            }
            else
            {
                /* /FRESHEN */
                opt = OPT_F;
            }
            ADD_ARG( opt);
        }

        /*
         * Help.
         * "-h" is not negatable, so simply put out the the desired "h" value.
         */
#define OPT_H   "-h"        /* "-h"  Normal help. */
#define OPT_HH  "-hh"       /* "-hh" Extended help. */

        status = cli$present( &cli_help);
        if (status & 1)
        {
            status = cli$present( &cli_help_extended);
            if (status & 1)
            {
                /* /HELP = EXTENDED */
                opt = OPT_HH;
            }
            else
            {
                /* /HELP = NORMAL */
                opt = OPT_H;
            }
            ADD_ARG( opt);
        }

        /*
         * Junk stored directory names when extracting.
         * Specify an explict "-j=N" value to override any existing "j"
         * value.
         */
#define J0      "0"             /* "-j=0"  Junk no directory names (default). */
#define JM1     "-1"            /* "-j=-1" Junk all directory names. */
#define OPT_JE  "-j="           /* "-j=N"  Junk "N" directory names. */

        status = cli$present( &cli_junk_dirs);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            char *jdp;
            int jdl;

            if (status == CLI$_NEGATED)
            {
                /* /NOJUNK_DIRS */
                jdp = J0;
                jdl = sizeof( J0)- 1;
            }
            else
            {
                /* /JUNK_DIRS */
                status = cli$get_value( &cli_junk_dirs, &junk_dirs_arg);
                if (junk_dirs_arg.dsc$w_length == 0)
                {
                    /* If no value was specified, use "-1" (all). */
                    jdp = JM1;
                    jdl = sizeof( JM1)- 1;
                }
                else
                {
                    jdp = junk_dirs_arg.dsc$a_pointer;
                    jdl = junk_dirs_arg.dsc$w_length;
                }
            }
            x = cmdl_len;
            cmdl_len += sizeof( OPT_JE)+ jdl;
            CHECK_BUFFER_ALLOCATION( the_cmd_line, cmdl_size, cmdl_len)
            strcpy( &the_cmd_line[ x], OPT_JE);
            strncpy( &the_cmd_line[ x+ sizeof( OPT_JE)- 1], jdp, jdl);
            the_cmd_line[ cmdl_len- 1] = '\0';
        }

        /*
         * Process archive as Java JAR.
         */
#define OPT_JAR  "--jar"        /* "--jar"   Process as Java JAR. */
#define OPT_JARN "--jar-"       /* "--jar-"  Process normally. */

        status = cli$present( &cli_jar);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOJAR */
                opt = OPT_JARN;
            }
            else
            {
                /* /JAR */
                opt = OPT_JAR;
            }
            ADD_ARG( opt);
        }

        /*
         * Restore protection info.
         * Clear any existing "-k" option with "-k-k-k", then add
         * the desired "k" value.
         */
#define OPT_K   "-k-k-k"        /* ""    Restore (limited) perms (dflt). */
#define OPT_KK  "-k-k-kk"       /* "-k"  Restore archive permissions. */
#define OPT_KN  "-k-k-"         /* "-k-" Restore no permissions. */

        opt = NULL;
        status = cli$present( &cli_restore_prot);
        if (status == CLI$_NEGATED)
        {
            /* /RESTORE = NOPROTECTION */
            opt = OPT_KN;
        }
        else
        {
            status = cli$present( &cli_restore_prot_lim);
            if (status & 1)
            {
                /* /RESTORE = PROTECTION = LIMITED */
                opt = OPT_K;
            }
            status = cli$present( &cli_restore_prot_orig);
            if (status & 1)
            {
                /* /RESTORE = PROTECTION = ORIGINAL */
                opt = OPT_KK;
            }
        }
        if (opt != NULL)
        {
            ADD_ARG( opt);
        }

        /*
         * Restore ACL.
         */
#define OPT_KA  "-ka"           /* "-ka" Restore ACL. */
#define OPT_KAN "-ka-"          /* "-ka-"  Restore no ACL. */

        opt = NULL;
        status = cli$present( &cli_restore_acl);
        if (status & 1)
        {
            /* /RESTORE = ACL */
            opt = OPT_KA;
        }
        else if (status == CLI$_NEGATED)
        {
            /* /RESTORE = NOACL */
            opt = OPT_KAN;
        }
        if (opt != NULL)
        {
            ADD_ARG( opt);
        }

        /*
         * List archive contents (/BRIEF (default) or /FULL).
         * Clear any existing "-l" option with "-l-", then add
         * the desired "l" value.
         */
#define OPT_L    "-l-l"         /* "-l"    List archive contents. */
#define OPT_LV   "-l-lv"        /* "-lv"   List archive contents /FULL. */
#define OPT_LVV  "-l-lvv"       /* "-lvv"  List archive contents /FULL=DIAG. */
#define OPT_LN   "-l-"          /* ""      Normal extract (default). */

        status = cli$present( &cli_list);
        if (status & 1)
        {
            opt = OPT_L;
            if (cli$present( &cli_full) & 1)
            {
                opt = OPT_LV;
                if (cli$present( &cli_full_diags) & 1)
                {
                    opt = OPT_LVV;
                }
            }
            ADD_ARG( opt);
        }

        /*
         * Deprecated.  Use /NAMES = [NO]DOWNCASE.
         * Make (some) names lowercase.
         * Clear any existing "-L" option with "-L-L-", then add
         * the desired "L" value.
         */
#define OPT__L   "-L-L-L"       /* "-L"    Downcase (some) names. */
#define OPT__LL  "-L-L-LL"      /* "-LL"   Downcase all names. */
#define OPT__LN  "-L-L-"        /* ""      Normal extract (default). */

        status = cli$present( &cli_lowercase);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOLOWERCASE */
                opt = OPT__LN;
            }
            else
            {
                /* /LOWERCASE */
                opt = OPT__L;
            }
            ADD_ARG( opt);
        }

        /*
         * Deprecated.  Use /NAMES = [NO]DOWNCASE.
         * Uppercase (don't convert to lower case).
         * Clear any existing "-L" option with "-L-L-", then add
         * the desired "L" value.
         */
        status = cli$present( &cli_uppercase);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOUPPERCASE */
                opt = OPT__L;
            }
            else
            {
                /* /UPPERCASE */
                opt = OPT__LN;
            }
            ADD_ARG( opt);
        }

        /*
         * Existing files: new version, overwrite, no extract?
         * Clear any existing "-n" or "-o" option with "-n-o-o-", then
         * add the desired "-n"/"-o[o]" value.
         */
#define OPT_N   "-n-o-o-n"      /* "-n"    Never overwrite (NOEXTRACT). */
#define OPT_O   "-n-o-o-o"      /* "-o"    Create new version (NEW_VERSION). */
#define OPT_OO  "-n-o-o-oo"     /* "-oo"   Overwrite (OVERWRITE). */
#define OPT_ON  "-n-o-o-"       /* ""      Normal inquiry (default). */

        status = cli$present( &cli_existing);
        if (status & 1)
        {
            status = cli$present( &cli_exist_newver);
            if (status == CLI$_PRESENT)
            {
                opt = OPT_O;
            }
            else
            {
                status = cli$present( &cli_exist_over);
                if (status == CLI$_PRESENT)
                {
                    opt = OPT_OO;
                }
                else
                {
                    status = cli$present( &cli_exist_noext);
                    if (status == CLI$_PRESENT)
                    {
                        opt = OPT_N;
                    }
                }
            }
            ADD_ARG( opt);
        }

        /*
         * Overwrite files (deprecated).
         * Clear any existing "-n" or "-o" option with "-n-o-o-", then
         * add the desired "-n"/"-o[o]" value.
         */
        status = cli$present( &cli_overwrite);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOOVERWRITE */
                opt = OPT_N;
            }
            else
            {
                /* /OVERWRITE */
                opt = OPT_O;
            }
            ADD_ARG( opt);
        }

        /*
         * Transform names.
         * Do not map FAT/NTFS names.
         * Convert names to ODS2 restrictions.
         * Convert spaces in names to underscores.
         * Clear any existing "-2"/"-s" option with "-2-"/"-s-", then
         * add the desired "2"/"s" value.
         */
#define OPT_0   "-0"            /* "-0"  Do not map FAT/NTFS names. */
#define OPT_0N  "-0-"           /* "-0-" Map FAT/NTFS names. */
#define OPT_2   "-2-2"          /* "-2"  Convert names to ODS2. */
#define OPT_2N  "-2-"           /* ""    Normal extract (default). */
#define OPT_S   "-s-s"          /* "-s"  Convert spaces. */
#define OPT_SN  "-s-"           /* ""    Preserve spaces (default). */

        status = cli$present( &cli_names_char_set);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NAMES = NOCHAR_SET */
                opt = OPT_0;
            }
            else
            {
                /* /NAMES = CHAR_SET */
                opt = OPT_0N;
            }
            ADD_ARG( opt);
        }

        status = cli$present( &cli_names_nodown);
        if (status & 1)
        {
            /* /NAMES = NODOWNCASE */
            ADD_ARG( OPT__LN);
        }
        else
        {
            status = cli$present( &cli_names_down);
            if (status & 1)
            {
                status = cli$present( &cli_names_down_all);
                if (status & 1)
                {
                    /* /NAMES = DOWNCASE = ALL */
                    opt = OPT__LL;
                }
                else
                {
                    /* /NAMES = DOWNCASE = SOME */
                    opt = OPT__L;
                }
                ADD_ARG( opt);
            }
        }

        status = cli$present( &cli_names_ods2);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NAMES = NOODS2 */
                opt = OPT_2N;
            }
            else
            {
                /* /NAMES = ODS2 */
                opt = OPT_2;
            }
            ADD_ARG( opt);
        }

        status = cli$present( &cli_names_spaces);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NAMES = NOSPACES */
                opt = OPT_S;
            }
            else
            {
                /* /NAMES = SPACES */
                opt = OPT_SN;
            }
            ADD_ARG( opt);
        }

        /*
         * Force conversion of extracted file names to ODS2 conventions.
         * Clear any existing "-2" option with "-2-", then add
         * the desired "2" value.
         */

        status = cli$present( &cli_ods2);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOODS2 */
                opt = OPT_2N;
            }
            else
            {
                /* /ODS2 */
                opt = OPT_2;
            }
            ADD_ARG( opt);
        }

        /*
         * Pipe files to SYS$OUTPUT (stdout) with no informationals.
         * Clear any existing "-p" option with "-p-", then add
         * the desired "p" value.
         */
#define OPT_P   "-p-p"          /* "-p"    Pipe files to stdout. */
#define OPT_PN  "-p-"           /* ""      Normal extract (default). */

        status = cli$present( &cli_pipe);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /PIPE */
                opt = OPT_P;
            }
            else
            {
                /* /NOPIPE */
                opt = OPT_P;
            }
            ADD_ARG( opt);
        }

        /*
         * Quiet.
         * Clear any existing "-q" option with "-q-q-", then add
         * the desired "q" value.
         */
#define OPT_Q   "-q-q-q-q-q"    /* "-q"  Quiet (NORMAL). */
#define OPT_QQ  "-q-q-q-q-qq"   /* "-qq" Quiet (SUPER). */
#define OPT_QN  "-q-q-q-q-"     /* ""    Normal noisiness (default). */

        status = cli$present( &cli_quiet);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOQUIET */
                opt = OPT_QN;
            }
            else
            {
                status = cli$present( &cli_super_quiet);
                if (status & 1)
                {
                    /* /QUIET = SUPER */
                    opt = OPT_QQ;
                }
                else
                {
                    /* /QUIET = NORMAL */
                    opt = OPT_Q;
                }
            }
            ADD_ARG( opt);
        }

        /*
         * Test archive integrity.
         * Clear any existing "-t" option with "-t-", then add
         * the desired "t" value.
         */
#define OPT_T   "-t-t"          /* "-t"  Test archive integrity. */
#define OPT_TN  "-t-"           /* ""    Normal extract (default). */

        status = cli$present( &cli_test);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTEST */
                opt = OPT_TN;
            }
            else
            {
                /* /TEST */
                opt = OPT_T;
            }
            ADD_ARG( opt);
        }

        /*
         * Set archive timestamp according to its newest file.
         * Clear any existing "-T" option with "-T-", then add
         * the desired "T" value.
         */
#define OPT__T   "-T-T"         /* "-T"  Set archive timestamp. */
#define OPT__TN  "-T-"          /* ""    Normal extract (default). */

        status = cli$present( &cli_timestamp);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTIMESTAMP */
                opt = OPT__TN;
            }
            else
            {
                /* /TIMESTAMP */
                opt = OPT__T;
            }
            ADD_ARG( opt);
        }

        /*
         * Update (extract only new and newer files).
         * Clear any existing "-u" option with "-u-", then add
         * the desired "u" value.
         */
#define OPT_U   "-u-u"          /* "-u"  Update (extract new and newer). */
#define OPT_UN  "-u-"           /* ""    Normal extract (default). */

        status = cli$present( &cli_update);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOUPDATE */
                opt = OPT_UN;
            }
            else
            {
                /* /UPDATE */
                opt = OPT_U;
            }
            ADD_ARG( opt);
        }

        /*
         * Verbose/version ("-v" or "-vq" report).
         * Clear any existing "-v" options with "-v-v-", then add
         * the desired "v" value.
         */
#define OPT_V   "-v-v-v"        /* "-v"  Verbose (normal). */
#define OPT_VN  "-v-v-"         /* ""    Normal extract (default). */
#define OPT_VQ  "-vq"           /* "-vq" Verbose (brief). */
#define OPT_VV  "-v-v-vv"       /* "-vv" Verbose (more). */

        status = cli$present( &cli_noverbose);
        if (status & 1)
        {
            /* /NOVERBOSE */
            ADD_ARG( OPT_VN);
        }

        /* Note that if more than one of the following options is
         * specified, the maximum one is used.  CLD rules may be more
         * restrictive.
         */
        status = cli$present( &cli_verbose);
        if (status & 1)
        {
            int i;
            char *opt = NULL;

            if ((status = cli$present( &cli_verbose_brief)) & 1)
            {
                /* /VERBOSE = BRIEF */
                opt = OPT_VQ;
            }
            if ((status = cli$present( &cli_verbose_normal)) & 1)
            {
                /* /VERBOSE [ = NORMAL ] */
                opt = OPT_V;
            }
            if ((status = cli$present( &cli_verbose_more)) & 1)
            {
                /* /VERBOSE = MORE */
                opt = OPT_VV;
            }

            if (opt != NULL)
            {
            ADD_ARG( opt);
            }
        }

        /*
         * Version (retain VMS/DEC-20 file versions).
         * Clear any existing "-V" option with "-V-", then add
         * the desired "V" value.
         */
#define OPT__V   "-V-V"         /* "-V"  Extract files with versions. */
#define OPT__VN  "-V-"          /* ""    Normal extract (default). */

        status = cli$present( &cli_version);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOVERSION */
                opt = OPT__VN;
            }
            else
            {
                /* /VERSION */
                opt = OPT__V;
            }
            ADD_ARG( opt);
        }

        /*
         * Restore owner (UIC) info.
         */
#define OPT__X   "-X"           /* "-X"  Restore owner (UIC). */
#define OPT__XN  "-X-"          /* "-X-" Restore no owner (UIC). */

        opt = NULL;
        status = cli$present( &cli_restore_own);
        if (status & 1)
        {
            /* /RESTORE = OWNER*/
            opt = OPT__X;
        }
        else if (status == CLI$_NEGATED)
        {
            /* /RESTORE = NOOWNER*/
            opt = OPT__XN;
        }
        if (opt != NULL)
        {
            ADD_ARG( opt);
        }

        /*
         * Treat trailing ".###" as version number.
         * (Extract "name.type.###" as "nane.type;###".)
         * Clear any existing "-Y" option with "-Y-", then add
         * the desired "Y" value.
         */
#define OPT__Y   "-Y-Y"         /* "-Y"  Treat ".###" as version number. */
#define OPT__YN  "-Y-"          /* ""    Normal extract (default). */

        status = cli$present( &cli_dot_version);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NODOT_VERSION */
                opt = OPT__YN;
            }
            else
            {
                /* /DOT_VERSION */
                opt = OPT__Y;
            }
            ADD_ARG( opt);
        }

        /*
         * Traverse directories (don't skip "../" path components).
         * Clear any existing "-:" option with "-:-", then add
         * the desired ":" value.
         */
#define OPT_COL   "-:-:"        /* "-:"  Follow "../" path components. */
#define OPT_COLN  "-:-"         /* ""    Ignore "../" components. (default). */

        status = cli$present( &cli_traverse);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTRAVERSE_DIRS */
                opt = OPT_COLN;
            }
            else
            {
                /* /TRAVERSE_DIRS */
                opt = OPT_COL;
            }
            ADD_ARG( opt);
        }
    }
    else /* if (zipinfo == 0) */
    {
        /* ZipInfo options. */

        /*
         * Show only file names, one per line (but allow /HEADER (-h),
         * /TOTALS (-t), or /COMMENT (-z)).
         * Clear any existing "-2" option with "-2-", then add
         * the desired "2" value.
         */
#define IPT_2   "-2-2"          /* "-2"  Names-only format. */
#define IPT_2N  "-2-"           /* ""    Normal format (default). */

        status = cli$present( &cli_one_line);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOONE_LINE */
                opt = IPT_2N;
            }
            else
            {
                /* /ONE_LINE */
                opt = IPT_2;
            }
            ADD_ARG( opt);
        }

        /*
         * Put out header line.
         * Clear any existing "-h" option with "-h-", then add
         * the desired "h" value.
         */
#define IPT_H   "-h-h"          /* "-h"  Put out header line. */
#define IPT_HN  "-h-"           /* ""    Omit header line (default). */

        status = cli$present( &cli_header);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOHEADER */
                opt = IPT_HN;
            }
            else
            {
                /* /HEADER */
                opt = IPT_H;
            }
            ADD_ARG( opt);
        }

        /*
         * Long UNIX "ls -l" format.
         * Clear any existing "-l" option with "-l-", then add
         * the desired "l" value.
         */
#define IPT_L   "-l-l"          /* "-l"  Long UNIX "ls -l" format. */
#define IPT_LN  "-l-"           /* ""    Normal format (default). */

        status = cli$present( &cli_long);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOLONG */
                opt = IPT_LN;
            }
            else
            {
                /* /LONG */
                opt = IPT_L;
            }
            ADD_ARG( opt);
        }

        /*
         * Medium UNIX "ls -l" format.
         * Clear any existing "-m" option with "-m-", then add
         * the desired "m" value.
         */
#define IPT_M   "-m-m"          /* "-m"  Medium UNIX "ls -l" format. */
#define IPT_MN  "-m-"           /* ""    Normal format (default). */

        status = cli$present( &cli_medium);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOMEDIUM */
                opt = IPT_MN;
            }
            else
            {
                /* /MEDIUM */
                opt = IPT_M;
            }
            ADD_ARG( opt);
        }

        /*
         * Short UNIX "ls -l" format (default).
         * Clear any existing "-s" option with "-s-", then add
         * the desired "s" value.
         */
#define IPT_S   "-s-s"          /* "-s"  Short UNIX "ls -l" format (deflt). */
#define IPT_SN  "-s-"           /* ""    Normal format (default). */

        status = cli$present( &cli_short);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOSHORT */
                opt = IPT_SN;
            }
            else
            {
                /* /SHORT */
                opt = IPT_S;
            }
            ADD_ARG( opt);
        }

        /*
         * Put out member counts, directories/files/links.
         */
#define IPT_MC  "-mc"           /* "-mc"   Put out member counts (default). */
#define IPT_MCN "-mc-"          /* "-mc-"  Omit member counts. */

        status = cli$present( &cli_member_counts);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOMEMBER_COUNTS */
                opt = IPT_MCN;
            }
            else
            {
                /* /MEMBER_COUNTS */
                opt = IPT_MC;
            }
            ADD_ARG( opt);
        }

        /*
         * Put out totals summary.
         * Clear any existing "-t" option with "-t-", then add
         * the desired "t" value.
         */
#define IPT_T   "-t-t"          /* "-t"  Put out totals summary. */
#define IPT_TN  "-t-"           /* ""    Omit totals summary (default). */

        status = cli$present( &cli_totals);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTOTALS */
                opt = IPT_TN;
            }
            else
            {
                /* /TOTALS */
                opt = IPT_T;
            }
            ADD_ARG( opt);
        }

        /*
         * Put out date-times as sortable decimal values.
         * Clear any existing "-T" option with "-T-", then add
         * the desired "T" value.
         */
#define IPT__T   "-T-T"         /* "-T"  Put out sortable date-times. */
#define IPT__TN  "-T-"          /* ""    Put out normal date-times (deflt). */

        status = cli$present( &cli_decimal_time);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NODECIMAL_TIME */
                opt = IPT__TN;
            }
            else
            {
                /* /DECIMAL_TIME */
                opt = IPT__T;
            }
            ADD_ARG( opt);
        }

        status = cli$present( &cli_times);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOTIMES (deprecated) */
                opt = IPT__TN;
            }
            else
            {
                /* /TIMES (deprecated) */
                opt = IPT__T;
            }
            ADD_ARG( opt);
        }

        /*
         * Verbose, multi-page format.
         * Clear any existing "-v" option with "-v-", then add
         * the desired "v" value.
         */
#define IPT_V   "-v-v"          /* "-v"  Verbose, multi-page format. */
#define IPT_VN  "-v-"           /* ""    Normal format (default). */

        status = cli$present( &cli_noverbose);
        if (status & 1)
        {
            /* /NOVERBOSE */
            ADD_ARG( IPT_VN);
        }

        status = cli$present( &cli_verbose_normal);
        if ((status & 1) || (status == CLI$_NEGATED))
        {
            if (status == CLI$_NEGATED)
            {
                /* /NOVERBOSE (?) */
                opt = IPT_VN;
            }
            else
            {
                /* /VERBOSE */
                opt = IPT_V;
            }
            ADD_ARG( opt);
        }

    } /* if (zipinfo == 0) [else] */

    /*
    **  If specified, add the decryption password argument.
    **/
    if (password_arg.dsc$w_length != 0) {
        x = cmdl_len;
        cmdl_len += password_arg.dsc$w_length + 4;
        CHECK_BUFFER_ALLOCATION(the_cmd_line, cmdl_size, cmdl_len)
        strcpy(&the_cmd_line[x], "-P");
        strncpy(&the_cmd_line[x+3], password_arg.dsc$a_pointer,
                password_arg.dsc$w_length);
        the_cmd_line[cmdl_len-1] = '\0';
    }

    /*
    **  Now get the specified zip file name.
    */
    status = cli$present(&cli_zipfile);
    if (status & 1) {
        status = cli$get_value(&cli_zipfile, &work_str);

        x = cmdl_len;
        cmdl_len += work_str.dsc$w_length + 1;
        CHECK_BUFFER_ALLOCATION(the_cmd_line, cmdl_size, cmdl_len)
        strncpy(&the_cmd_line[x], work_str.dsc$a_pointer,
                work_str.dsc$w_length);
        the_cmd_line[cmdl_len-1] = '\0';
    }

    /*
    **  Get the output directory (-d), for UnZip.
    **/
    if (output_directory.dsc$w_length != 0) {
        x = cmdl_len;
        cmdl_len += output_directory.dsc$w_length + 4;
        CHECK_BUFFER_ALLOCATION(the_cmd_line, cmdl_size, cmdl_len)
        strcpy(&the_cmd_line[x], "-d");
        strncpy(&the_cmd_line[x+3], output_directory.dsc$a_pointer,
                output_directory.dsc$w_length);
        the_cmd_line[cmdl_len-1] = '\0';
    }

    /*
    **  Run through the list of files to unzip.
    */
    status = cli$present(&cli_infile);
    if (status & 1) {
        status = get_list(&cli_infile, &foreign_cmdline, '\0',
                          &the_cmd_line, &cmdl_size, &cmdl_len);
        if (!(status & 1)) return status;
    }

    /*
    **  Get the list of files to exclude, if there are any.
    */
    if (exclude_list) {
        x = cmdl_len;
        cmdl_len += 3;
        CHECK_BUFFER_ALLOCATION(the_cmd_line, cmdl_size, cmdl_len)
        strcpy(&the_cmd_line[x], "-x");

        status = get_list(&cli_exclude, &foreign_cmdline, '\0',
                          &the_cmd_line, &cmdl_size, &cmdl_len);
        if (!(status & 1)) return status;
    }

    /*
    **  We have finished collecting the strings for the argv vector,
    **  release unused space.
    */
    if ((the_cmd_line = (char *)izu_realloc(the_cmd_line, cmdl_len)) == NULL)
        return SS$_INSFMEM;

    /*
    **  Now that we've built our new UNIX-like command line, count the
    **  number of args and build an argv array.
    */
    for (new_argc = 0, x = 0; x < cmdl_len; x++)
        if (the_cmd_line[x] == '\0')
            new_argc++;

    /*
    **  Allocate memory for the new argv[].  The last element of argv[]
    **  is supposed to be NULL, so allocate enough for new_argc+1.
    */
    if ((new_argv = (char **) calloc(new_argc+1, sizeof(char *))) == NULL)
        return SS$_INSFMEM;

    /*
    **  For each option, store the address in new_argv[] and convert the
    **  separating blanks to nulls so each argv[] string is terminated.
    */
    for (ptr = the_cmd_line, x = 0; x < new_argc; x++) {
        new_argv[x] = ptr;
        ptr += strlen(ptr) + 1;
    }
    new_argv[new_argc] = NULL;

    /* Show the complete UNIX command line, if requested. */
    if (verbose_command != 0)
    {
        printf( "   UNIX command line args (argc = %d):\n", new_argc);
        for (x = 0; x < new_argc; x++)
            printf( "%s\n", new_argv[ x]);
        printf( "\n");
    }

    /*
    **  All finished.  Return the new argc and argv[] addresses to Zip.
    */
    *argc_p = new_argc;
    *argv_p = new_argv;

    return SS$_NORMAL;
}



static unsigned int
get_list (struct dsc$descriptor_s *qual, struct dsc$descriptor_d *rawtail,
          int delim, char **p_str, unsigned int *p_size, unsigned int *p_end)
{
/*
**  Routine:    get_list
**
**  Function:   This routine runs through a comma-separated CLI list
**              and copies the strings to the argv buffer.  The
**              specified separation character is used to separate
**              the strings in the argv buffer.
**
**              All unquoted strings are converted to lower-case.
**
**  Formal parameters:
**
**      qual    - Address of descriptor for the qualifier name
**      rawtail - Address of descriptor for the full command line tail
**      delim   - Character to use to separate the list items
**      p_str   - Address of pointer pointing to output buffer (argv strings)
**      p_size  - Address of number containing allocated size for output string
**      p_end   - Address of number containing used length in output buf
**
*/

    unsigned int status;
    DESCRIPTOR_D( work_str);

    status = cli$present(qual);
    if (status & 1) {

        unsigned int len;
        unsigned int old_len;
        int ind;
        int sind;
        int keep_case;
        char *src, *dst; int x;

        /*
        **  Just in case the string doesn't exist yet, though it does.
        */
        if (*p_str == NULL) {
            *p_size = ARGBSIZE_UNIT;
            if ((*p_str = (char *)izu_malloc(*p_size)) == NULL)
                return SS$_INSFMEM;
            len = 0;
        } else {
            len = *p_end;
        }

        while ((status = cli$get_value(qual, &work_str)) & 1) {
            old_len = len;
            len += work_str.dsc$w_length + 1;
            CHECK_BUFFER_ALLOCATION(*p_str, *p_size, len)

            /*
            **  Look for the filename in the original foreign command
            **  line to see if it was originally quoted.  If so, then
            **  don't convert it to lowercase.
            */
            keep_case = FALSE;
            str$find_first_substring(rawtail, &ind, &sind, &work_str);
            if ((ind > 1 && *(rawtail->dsc$a_pointer + ind - 2) == '"') ||
                (ind == 0))
                keep_case = TRUE;

            /*
            **  Copy the string to the buffer, converting to lowercase.
            */
            src = work_str.dsc$a_pointer;
            dst = *p_str+old_len;
            for (x = 0; x < work_str.dsc$w_length; x++) {
                if (!keep_case && ((*src >= 'A') && (*src <= 'Z')))
                    *dst++ = *src++ + 32;
                else
                    *dst++ = *src++;
            }
            if (status == CLI$_COMMA)
                (*p_str)[len-1] = (char)delim;
            else
                (*p_str)[len-1] = '\0';
        }
        *p_end = len;
    }
    return SS$_NORMAL;
}


static unsigned int
check_cli (struct dsc$descriptor_s *qual)
{
/*
**  Routine:    check_cli
**
**  Function:   Check to see if a CLD was used to invoke the program.
**
**  Formal parameters:
**
**      qual    - Address of descriptor for qualifier name to check.
**
*/
    lib$establish(lib$sig_to_ret);      /* Establish condition handler */
    return cli$present(qual);           /* Just see if something was given */
}


#ifndef TEST

int VMSCLI_usage(__GPRO__ int error)    /* returns PK-type error code */
{
# ifdef SFX

    /* UnZipSFX Usage Guide. */

    int flag;

    if (!show_VMSCLI_usage)
       return usage(__G__ error);

    flag = (error? 1 : 0);

    Info(slide, flag, ((char *)slide, UnzipBanner,
     "UnZip", UzpVersionStr(), UZ_VERSION_DATE, UZ_VERSION_DATE));
#  ifdef BETA_MSG
    Info( slide, flag, ((char *)slide, LoadFarString( BetaVersion),
     "\n", ""));
#  endif
    Info(slide, flag, ((char *)slide, "\
\n\
Usage: MCR %s -\n\
        [/unzip_qualifiers] [member [,...]]\n\
", G.zipfn));

/* 2012-12-28 SMS.
 * See same-date note below.  ("/DIRECTORY=dir, ")
 */
#  ifdef SFX_EXDIR
    Info(slide, flag, ((char *)slide, "\
\n\
Primary mode SFX qualifiers:\n\
  /COMMENT, /FRESHEN, /LICENSE, /PIPE, /SCREEN, /TEST, /UPDATE\n\
General SFX qualifiers:\n\
  /[NO]BINARY[=ALL|AUTO|NONE], /DIRECTORY=dir, /DOT_VERSION,\n\
  /EXCLUDE=(member [,...]), /EXISTING={NEW_VERSION|OVERWRITE|NOEXTRACT},\n\
"
));
#  else /* def SFX_EXDIR */
    Info(slide, flag, ((char *)slide, "\
\n\
Primary mode SFX qualifiers:\n\
  /COMMENT, /FRESHEN, /LICENSE, /PIPE, /SCREEN, /TEST, /UPDATE\n\
General SFX qualifiers:\n\
  /[NO]BINARY[=ALL|AUTO|NONE], /DOT_VERSION,\n\
  /EXCLUDE=(member [,...]), /EXISTING={NEW_VERSION|OVERWRITE|NOEXTRACT},\n\
"
));
#  endif /* def SFX_EXDIR [else] */

    Info(slide, flag, ((char *)slide,
#  ifdef WILD_STOP_AT_DIR
"\
  /JAR, /MATCH=(CASE={BLIND|SENSITIVE}, [NO]WILD_MATCH_SLASH),\n\
%s\n",
#  else /* def WILD_STOP_AT_DIR */
"\
  /JAR, /MATCH=(CASE={BLIND|SENSITIVE}),\n\
%s\n",
#  endif /* def WILD_STOP_AT_DIR [else] */
"\
  /[NO]JUNK_DIRS[=level], /NAMES=[[NO]DOWNCASE]|[[NO]ODS2]|[NO]SPACES],\n\
"));

/* 2012-12-28 SMS.
 * See same-date note below.
 */
#  ifdef MORE
    Info(slide, flag, ((char *)slide, "\
  /[NO]PAGE, /PASSWORD=passwd, /QUIET[=SUPER],\n\
  /RESTORE=([NO]ACL, [NO]DATE=[ALL|FILES], [NO]OWNER,\n\
   [NO]PROTECTION=[LIMITED|ORIGINAL]),\n\
  /[NO]TEXT[=([ALL|AUTO|NONE], STMLF)], /[NO]TRAVERSE_DIRS, /VERSION\n\
\n\
Quote member names if /MATCH=CASE=SENSITIVE (default).  For details, see\n\
UnZip documentation.  For more options, use an external (full-featured)\n\
UnZip program instead of this built-in (limited) UnZipSFX self-extractor.\n\
"
));
#  else /* def MORE */
    Info(slide, flag, ((char *)slide, "\
  %s/PASSWORD=passwd, /QUIET[=SUPER],\n\
  /RESTORE=([NO]ACL, [NO]DATE=[ALL|FILES], [NO]OWNER,\n\
   [NO]PROTECTION=[LIMITED|ORIGINAL]),\n\
  /[NO]TEXT[=([ALL|AUTO|NONE], STMLF)], /[NO]TRAVERSE_DIRS, /VERSION\n\
\n\
Quote member names if /MATCH=CASE=SENSITIVE (default).  For details, see\n\
UnZip documentation.  For more options, use an external (full-featured)\n\
UnZip program instead of this built-in (limited) UnZipSFX self-extractor.\n\
"
));
#  endif /* def MORE [else] */

    if (error)
        return PK_PARAM;
    else
        return PK_COOL;     /* just wanted usage screen: no error */

# else /* def SFX */

    /* Normal UnZip or ZipInfo Usage Guide. */

    int flag;

    if (!show_VMSCLI_usage)
       return usage(__G__ error);

/*---------------------------------------------------------------------------
    If user requested usage, send it to stdout; else send to stderr.
  ---------------------------------------------------------------------------*/

    flag = (error? 1 : 0);

/*---------------------------------------------------------------------------
    Print either ZipInfo usage or UnZip usage, depending on incantation.
  ---------------------------------------------------------------------------*/

    if (uO.zipinfo_mode)
    {
#  ifndef NO_ZIPINFO

        /* ZipInfo Usage guide.
         *    Compare: unzip.c: ZipInfoUsageLine1, et al.
         */
        Info( slide, flag, ((char *)slide, ZipInfoUsageLine1,
          UzpVersionStr(), UZ_VERSION_DATE, ZiDclStr()));

        Info(slide, flag, ((char *)slide, "\
Usage:  ZIPINFO [/zipinfo_qualifiers] [file[.zip]] [member [,...]]\n\
   or:  UNZIP /ZIPINFO [/zipinfo_qualifiers] [file[.zip]] [member [,...]]\n\
\n\
  Report archive (\"file.zip\") and member properties (name, date-time,\n\
  compression and/or encryption method, and so on).\n\
\n\
Primary listing-format qualifiers:\n\
  /LONG    long Unix \"ls -l\" format    /ONE_LINE  filenames only, one/line\n\
  /MEDIUM  medium Unix \"ls -l\" format  /SHORT   short \"ls -l\" format (default.)\n\
  /VERBOSE verbose, very detailed format\n\n\
"));

        Info(slide, flag, ((char *)slide, "\
General qualifiers:\n\
  /COMMENT  include archive comment   /DECIMAL_TIME sortable dec'l time format\n\
  /EXCLUDE=(member [,...])  exclude members from listing\n\
  /HEADER  include header            /PAGE  page output through built-in \"more\"\n\
%s\
%s",
#   ifdef WILD_STOP_AT_DIR
 "\
  /MATCH=(CASE={BLIND|SENSITIVE}, [NO]WILD_MATCH_SLASH)  member name matching\n\
",
#   else /* def WILD_STOP_AT_DIR */
 "\
  /MATCH=(CASE={BLIND|SENSITIVE})  member name matching\n\
",
#   endif /* def WILD_STOP_AT_DIR [else] */
 "\
  /TOTALS  include totals trailer for listed files\n\
"));

        Info(slide, flag, ((char *)slide, "\n\
Unix-style usage guide: unzip \"-Z\"\n\
Quote archive member names if /MATCH=CASE=SENSITIVE (default).\n\
"));

#  endif /* ndef NO_ZIPINFO */
    }
    else
    {   /* UnZip mode */

        /* Normal UnZip Usage Guide. */

        Info(slide, flag, ((char *)slide, UnzipUsageLine1,
          UzpVersionStr(), UZ_VERSION_DATE, UzpDclStr()));

#  ifdef BETA_MSG
        Info(slide, flag, ((char *)slide, BetaVersion, "", ""));
#  endif

        Info(slide, flag, ((char *)slide, "\
Usage: UNZIP [/unzip_qualifiers] [file[.zip]] [member [,...]]\n\
  Default action: Extract specified (or all) members from file.zip\n\
%s\n",
#  ifdef NO_ZIPINFO
          "  (ZipInfo mode is disabled in this build.)"
#  else
          "  ZipInfo-mode usage: \"UNZIP /ZIPINFO\""
#  endif
));

        Info(slide, flag, ((char *)slide, "\
Primary mode qualifiers (For Unix-style options: \"unzip -h\"):\n\
  /COMMENT, /FRESHEN, /HELP[=EXTENDED], /LICENSE, /LIST, /PIPE, /SCREEN,\n\
  /TEST, /TIMESTAMP, /UPDATE, /VERBOSE\n\
General qualifiers:\n\
  /[NO]AUTO_DIRECTORY, /[NO]BINARY[=ALL|AUTO|NONE], /DIRECTORY=dir,\n\
  /DOT_VERSION, /[NO]BINARY[=ALL|AUTO|NONE], /DIRECTORY=dir, /DOT_VERSION,\n\
  /EXCLUDE=(member [,...]), /EXISTING={NEW_VERSION|OVERWRITE|NOEXTRACT},\n\
"));

        Info(slide, flag, ((char *)slide,
#  ifdef WILD_STOP_AT_DIR
 "\
  /JAR, /MATCH=(CASE={BLIND|SENSITIVE}, [NO]WILD_MATCH_SLASH),\n\
%s\n",
#  else /* def WILD_STOP_AT_DIR */
 "\
  /JAR, /MATCH=(CASE={BLIND|SENSITIVE}),\n\
%s",
#  endif /* def WILD_STOP_AT_DIR [else] */
 "\
  /[NO]JUNK_DIRS[=level], /NAMES=[[NO]DOWNCASE]|[[NO]ODS2]|[NO]SPACES],\n\
"));

/* 2012-12-28 SMS.
 * Had attempted to use a "%s" for "/[NO]PAGE, " (or ""), conditional on
 * MORE, to combine the following into one statement, but DEC C
 * V4.0-000 was confused by the #ifdef within the argument list for
 * Info(), which is already a macro (unzpriv.h), causing a (spurious)
 * %CC-E-BADIFDEF.
 */
#  ifdef MORE
        Info(slide, flag, ((char *)slide, "\
  /[NO]PAGE, /PASSWORD=passwd, /QUIET[=SUPER],\n\
  /RESTORE=([NO]ACL, [NO]DATE=[ALL|FILES], [NO]OWNER,\n\
   [NO]PROTECTION=[LIMITED|ORIGINAL]),\n\
  /[NO]TEXT[=([ALL|AUTO|NONE], STMLF)], /[NO]TRAVERSE_DIRS, /VERBOSE, /VERSION\n\
"));
#  else /* def MORE */
        Info(slide, flag, ((char *)slide, "\
  /PASSWORD=passwd, /QUIET[=SUPER],\n\
  /RESTORE=([NO]ACL, [NO]DATE=[ALL|FILES], [NO]OWNER,\n\
   [NO]PROTECTION=[LIMITED|ORIGINAL]),\n\
  /[NO]TEXT[=([ALL|AUTO|NONE], STMLF)], /[NO]TRAVERSE_DIRS, /VERBOSE, /VERSION\n\
"));
#  endif /* def MORE [else] */

        Info(slide, flag, ((char *)slide, "\
Examples (see unzip.txt or \"HELP UNZIP\" for more info):\n\
   unzip edit1 /EXCL=joe.jou /MATCH = CASE=BLIND  => Extract all files except\
\n\
      joe.jou (or JOE.JOU, or any combination of case) from zipfile edit1.zip.\
\n  \
 unzip zip201 \"Makefile.VMS\" vms/*.[ch]         => extract VMS Makefile and\
\n\
      *.c and *.h files.  Quote member names if /MATCH=CASE=SENS (default).\
\n\
   unzip foo /DIR=tmp:[.test] /JUNK /TEXT /EXIS=NEW  => extract all files to\
\n\
      tmp. dir., flatten hierarchy, auto-conv. text files, create new versions.\
\n"));

    } /* end if (zipinfo_mode) */

    if (error)
    {
        show_env( __G__ 1);
        return PK_PARAM;
    }
    else
        return PK_COOL;     /* just wanted usage screen: no error */

# endif /* def SFX [else] */

} /* end function VMSCLI_usage() */

#endif /* ndef TEST */


#ifdef TEST
int
main( int argc, char **argv)
{
    return vms_unzip_cmdline( &argc, &argv);
}
#endif /* def TEST */

