
        /*
         * just about to extract file:  if extracting to disk, check if
         * already exists, and if so, take appropriate action according to
         * fflag/uflag/overwrite_all/etc. (we couldn't do this in upper
         * loop because we don't store the possibly renamed filename[] in
         * info[])
         */
#ifdef DLL
        if (!uO.tflag && !uO.cflag && !G.redirect_data)
#else
        if (!uO.tflag && !uO.cflag)
#endif
        {
            renamed = FALSE;   /* user hasn't renamed output file yet */

startover:
            query = FALSE;
            skip_entry = SKIP_NO;
            /* for files from DOS FAT, check for use of backslash instead
             *  of slash as directory separator (bug in some zipper(s); so
             *  far, not a problem in HPFS, NTFS or VFAT systems)
             */
#ifndef SFX
            if (G.pInfo->hostnum == FS_FAT_ && !MBSCHR(G.filename, '/')) {
                char *p=G.filename;

                if (*p) do {
                    if (*p == '\\') {
                        if (!G.reported_backslash) {
                            Info(slide, 0x21, ((char *)slide,
                              LoadFarString(BackslashPathSep), G.zipfn));
                            G.reported_backslash = TRUE;
                            if (!error_in_archive)
                                error_in_archive = PK_WARN;
                        }
                        *p = '/';
                    }
                } while (*PREINCSTR(p));
            }
#endif /* !SFX */

            if (!renamed)
            {
              /* Transform absolute path to relative path, and warn user. */
              error = name_abs_rel( __G);
              if (error != PK_OK)
                error_in_archive = error;
            }

            name_junk( __G);

            /* mapname() can create dirs if not freshening or if renamed. */
            error = mapname_dir_vollab( __G__ renamed,
#ifdef SET_DIR_ATTRIB
             pnum_dirs, pdirlist,
#endif
             &error_in_archive);

            if (error != 0)
                continue;       /* No more to do with this one.  Go to next. */

#ifdef QDOS
            QFilename(__G__ G.filename);
#endif

#if defined( UNIX) && defined( __APPLE__)
            /* If we are doing special AppleDouble file processing,
             * and this is an AppleDouble file,
             * then we should ignore a file-exists test, which may be
             * expected to succeed.
             */

            if (G.apple_double && (!uO.J_flag))
            {
                /* Fake a does-not-exist value for this AppleDouble file. */
                cfn = DOES_NOT_EXIST;
            }
            else
            {
                /* Do the real test. */
                cfn = check_for_newer(__G__ G.filename);
            }

            /* Use "cfn" on Mac, plain check_for_newer() elsewhere. */
            switch (cfn)
#else /* defined( UNIX) && defined( __APPLE__) */
            switch (check_for_newer(__G__ G.filename))
#endif /* defined( UNIX) && defined( __APPLE__) */
            {
                case DOES_NOT_EXIST:
#ifdef NOVELL_BUG_FAILSAFE
                    G.dne = TRUE;   /* stat() says file DOES NOT EXIST */
#endif
                    /* freshen (no new files): skip unless just renamed */
                    if (uO.fflag && !renamed)
                        skip_entry = SKIP_Y_NONEXIST;
                    break;
                case EXISTS_AND_OLDER:
#ifdef UNIXBACKUP
                    if (!uO.B_flag)
#endif
                    {
                        if (IS_OVERWRT_NONE)
                            /* never overwrite:  skip file */
                            skip_entry = SKIP_Y_EXISTING;
                        else if (!IS_OVERWRT_ALL)
                            query = TRUE;
                    }
                    break;
                case EXISTS_AND_NEWER:             /* (or equal) */
#ifdef UNIXBACKUP
                    if ((!uO.B_flag && IS_OVERWRT_NONE) ||
#else
                    if (IS_OVERWRT_NONE ||
#endif
                        (uO.uflag && !renamed)) {
                        /* skip if update/freshen & orig name */
                        skip_entry = SKIP_Y_EXISTING;
                    } else {
#ifdef UNIXBACKUP
                        if (!IS_OVERWRT_ALL && !uO.B_flag)
#else
                        if (!IS_OVERWRT_ALL)
#endif
                            query = TRUE;
                    }
                    break;
                }
#ifdef VMS
            /* 2008-07-24 SMS.
             * On VMS, if the file name includes a version number,
             * and "-V" ("retain VMS version numbers", V_flag) is in
             * effect, then the VMS-specific code will handle any
             * conflicts with an existing file, making this query
             * redundant.  (Implicit "y" response here.)
             */
            if (query && (uO.V_flag > 0)) {
                /* Not discarding file versions.  Look for one. */
                int cndx = strlen(G.filename) - 1;

                while ((cndx > 0) && (isdigit(G.filename[cndx])))
                    cndx--;
                if (G.filename[cndx] == ';')
                    /* File version found; skip the generic query,
                     * proceeding with its default response "y".
                     */
                    query = FALSE;
            }
#endif /* VMS */
            if (query) {
#ifdef WINDLL
                switch (G.lpUserFunctions->replace != NULL ?
                        (*G.lpUserFunctions->replace)(G.filename, FILNAMSIZ) :
                        IDM_REPLACE_NONE) {
                    case IDM_REPLACE_RENAME:
                        _ISO_INTERN(G.filename);
                        renamed = TRUE;
                        goto startover;
                    case IDM_REPLACE_ALL:
                        G.overwrite_mode = OVERWRT_ALWAYS;
                        /* FALL THROUGH, extract */
                    case IDM_REPLACE_YES:
                        break;
                    case IDM_REPLACE_NONE:
                        G.overwrite_mode = OVERWRT_NEVER;
                        /* FALL THROUGH, skip */
                    case IDM_REPLACE_NO:
                        skip_entry = SKIP_Y_EXISTING;
                        break;
                }
#else /* !WINDLL */
                extent fnlen;
reprompt:
                Info(slide, 0x81, ((char *)slide,
                  LoadFarString(ReplaceQuery),
                  FnFilter1(G.filename)));
                if (fgets_ans( __G) < 0)
                {
                    Info(slide, 1, ((char *)slide,
                      LoadFarString(AssumeNone)));
                    *G.answerbuf = 'N';
                    if (!error_in_archive)
                        error_in_archive = 1;  /* not extracted:  warning */
                }
                switch (*G.answerbuf) {
                    case 'r':
                    case 'R':
                        do
                        {
                            Info(slide, 0x81, ((char *)slide,
                              LoadFarString(NewNameQuery)));
                            if (fgets( G.filename, FILNAMSIZ, stdin) == NULL)
                            {   /* read() error.  Try again. */
                                goto reprompt;
                            }
                            else
                            {
                                /* Usually get \n here.  Check for it. */
                                fnlen = strlen(G.filename);
                                if (lastchar(G.filename, fnlen) == '\n')
                                    G.filename[--fnlen] = '\0';
                            }
                        } while (fnlen == 0);
# ifdef WIN32  /* WIN32 fgets( ... , stdin) returns OEM coded strings */
                        _OEM_INTERN(G.filename);
# endif
                        renamed = TRUE;
                        goto startover;   /* sorry for a goto */
                    case 'A':   /* dangerous option:  force caps */
                        G.overwrite_mode = OVERWRT_ALWAYS;
                        /* FALL THROUGH, extract */
                    case 'y':
                    case 'Y':
                        break;
                    case 'N':
                        G.overwrite_mode = OVERWRT_NEVER;
                        /* FALL THROUGH, skip */
                    case 'n':
                        /* skip file */
                        skip_entry = SKIP_Y_EXISTING;
                        break;
                    case '\n':
                    case '\r':
                        /* Improve echo of '\n' and/or '\r'
                           (sizeof(G.answerbuf) == 10 (see globals.h), so
                           there is enough space for the provided text...) */
                        strcpy(G.answerbuf, "{ENTER}");
                        /* fall through ... */
                    default:
                        /* usually get \n here:  remove it for nice display
                           (fnlen can be re-used here, we are outside the
                           "enter new filename" loop) */
                        fnlen = strlen(G.answerbuf);
                        if (lastchar(G.answerbuf, fnlen) == '\n')
                            G.answerbuf[--fnlen] = '\0';
                        Info(slide, 1, ((char *)slide,
                          LoadFarString(InvalidResponse), G.answerbuf));
                        goto reprompt;   /* yet another goto? */
                } /* end switch (*answerbuf) */
#endif /* ?WINDLL */
            } /* end if (query) */
            if (skip_entry != SKIP_NO) {
#ifdef WINDLL
                if (skip_entry == SKIP_Y_EXISTING) {
                    /* report skipping of an existing entry */
                    Info(slide, 0, ((char *)slide,
                      ((IS_OVERWRT_NONE || !uO.uflag || renamed) ?
                       "Target file exists.\nSkipping %s\n" :
                       "Target file newer.\nSkipping %s\n"),
                      FnFilter1(G.filename)));
                }
#endif /* WINDLL */
                continue;
            }
        } /* end if (extracting to disk) */

