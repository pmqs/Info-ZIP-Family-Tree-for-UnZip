/**** REXX  ********   ZIP2EXE.CMD  **************  11/06/2020  ******\
|**  Updated (c) 2020 by bww bitwise works GmbH                     **|
|**                                                                 **|
|**  This exec will prepend the Info Zip unzipsfx.exe file to an    **|
|**  existing ZIP file to create a self extracting zip.             **|
|**                                                                 **|
|**  The exec requires 1 argument, the name of the zip file to be   **|
|**  acted upon.                                                    **|
|**                                                                 **|
|**  Put this exec into the path that contains your Info Zip        **|
|**  executables.                                                   **|
|**                                                                 **|
\*********************************************************************/
rc = 0
debug = 0
/**  Start Argument processing  ** end Initialization               **/
parse ARG zip_file
if debug <> 0 then say 'ZIP #'zip_file'#'
zip_file = strip(zip_file)
zip_file = strip(zip_file,,'"')
if debug <> 0 then say 'ZIP #'zip_file'#'

if zip_file = '' then do
    say 'Please, specify the name of the file to be processed.'
    rc = 9
    SIGNAL FINI
end
if pos('.ZIP',translate(zip_file)) = 0 then do
    sfx_file = zip_file||'.exe'
    zip_file = zip_file||'.zip'
end
else sfx_file = substr(zip_file,1,lastpos('.',zip_file))||'exe'
if debug <> 0 then say 'ZIP #'zip_file'#'
if debug <> 0 then say 'SFX #'sfx_file'#'
fileexists = stream(zip_file,'C','QUERY EXISTS')
if debug <> 0 then say 'Exists? #'fileexists'#'
if fileexists = '' then DO
    say 'The file "'||zip_file||'" does not exist - cannot proceed.'
    rc = 9
    SIGNAL FINI
  end
/**  Find  zip.exe location    ** end Argument processing          **/
parse source . . command_file
zipexe = substr(command_file,1,lastpos('\',command_file))||'zip.exe'
if stream(zipexe,'c','query exists') = '' then do
    say 'We are unable to locate the zip.exe command.'
    say 'Eensure that the zip2exe command is in the directory',
        'which contains zip.exe'
    rc = 9
    signal fini
end

/**  Start unzipsfx location    ** end Argument processing          **/
parse SOURCE . . command_file
unzipsfx = SUBSTR(command_file,1,LASTPOS('\',command_file))||,
          'UNZIPSFX.EXE'
if stream(unzipsfx,'C','QUERY EXISTS') = '' then DO
    say 'We are unable to locate the UNZIPSFX.EXE source.'
    say 'Ensure that the ZIP2EXE command is in the directory',
        'which contains UNZIPSFX.EXE'
    rc = 9
    SIGNAL FINI
  end
/**  Execute the command        ** end Argument processing          **/
ADDRESS CMD '@COPY /b "'||unzipsfx||'"+"'||zip_file||'" "'sfx_file||'" >NUL'
if rc = 0 then do
  address cmd '@ZIP.EXE -A "'||sfx_file||'" >NUL'
  say '"'||sfx_file||'" created successfully.'
end 
else say '"'||sfx_file||'" creation failed.'
FINI:
  EXIT  rc
