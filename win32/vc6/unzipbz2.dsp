# Microsoft Developer Studio Project File - Name="unzipbz2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=unzipbz2 - Win32 ASM Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "unzipbz2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "unzipbz2.mak" CFG="unzipbz2 - Win32 ASM Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "unzipbz2 - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unzipbz2 - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "unzipbz2 - Win32 ASM Release" (based on "Win32 (x86) Console Application")
!MESSAGE "unzipbz2 - Win32 ASM Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "unzipbz2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "unzipbz2__Win32_Release"
# PROP BASE Intermediate_Dir "unzipbz2__Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "unzipbz2__Win32_Release"
# PROP Intermediate_Dir "unzipbz2__Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../bzip2" /D "NDEBUG" /D "WIN32" /D "USE_BZIP2" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /machine:I386 /out:"unzipbz2__Win32_Release/unzip.exe"

!ELSEIF  "$(CFG)" == "unzipbz2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "unzipbz2__Win32_Debug"
# PROP BASE Intermediate_Dir "unzipbz2__Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "unzipbz2__Win32_Debug"
# PROP Intermediate_Dir "unzipbz2__Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../bzip2" /D "USE_BZIP2" /D "_CONSOLE" /D "_DEBUG" /D "WIN32" /D "_MBCS" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /out:"unzipbz2__Win32_Debug/unzip.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "unzipbz2 - Win32 ASM Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "unzipbz2__Win32_ASM_Release"
# PROP BASE Intermediate_Dir "unzipbz2__Win32_ASM_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "unzipbz2__Win32_ASM_Release"
# PROP Intermediate_Dir "unzipbz2__Win32_ASM_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../bzip2" /D "NDEBUG" /D "WIN32" /D "ASM_CRC" /D "USE_BZIP2" /D "_CONSOLE" /D "_MBCS" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /machine:I386 /out:"unzipbz2__Win32_ASM_Release/unzip.exe"

!ELSEIF  "$(CFG)" == "unzipbz2 - Win32 ASM Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "unzipbz2__Win32_ASM_Debug"
# PROP BASE Intermediate_Dir "unzipbz2__Win32_ASM_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "unzipbz2__Win32_ASM_Debug"
# PROP Intermediate_Dir "unzipbz2__Win32_ASM_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../../bzip2" /D "_DEBUG" /D "WIN32" /D "ASM_CRC" /D "USE_BZIP2" /D "_CONSOLE" /D "_MBCS" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /out:"unzipbz2__Win32_ASM_Debug/unzip.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "unzipbz2 - Win32 Release"
# Name "unzipbz2 - Win32 Debug"
# Name "unzipbz2 - Win32 ASM Release"
# Name "unzipbz2 - Win32 ASM Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\crc32.c
# End Source File
# Begin Source File

SOURCE=..\crc_i386.c
# End Source File
# Begin Source File

SOURCE=..\..\crypt.c
# End Source File
# Begin Source File

SOURCE=..\..\envargs.c
# End Source File
# Begin Source File

SOURCE=..\..\explode.c
# End Source File
# Begin Source File

SOURCE=..\..\extract.c
# End Source File
# Begin Source File

SOURCE=..\..\fileio.c
# End Source File
# Begin Source File

SOURCE=..\..\globals.c
# End Source File
# Begin Source File

SOURCE=..\..\inflate.c
# End Source File
# Begin Source File

SOURCE=..\..\list.c
# End Source File
# Begin Source File

SOURCE=..\..\match.c
# End Source File
# Begin Source File

SOURCE=..\nt.c
# End Source File
# Begin Source File

SOURCE=..\..\process.c
# End Source File
# Begin Source File

SOURCE=..\..\ttyio.c
# End Source File
# Begin Source File

SOURCE=..\..\ubz2err.c
# End Source File
# Begin Source File

SOURCE=..\..\unreduce.c
# End Source File
# Begin Source File

SOURCE=..\..\unshrink.c
# End Source File
# Begin Source File

SOURCE=..\..\unzip.c
# End Source File
# Begin Source File

SOURCE=..\win32.c
# End Source File
# Begin Source File

SOURCE=..\win32i64.c
# End Source File
# Begin Source File

SOURCE=..\..\zipinfo.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\consts.h
# End Source File
# Begin Source File

SOURCE=..\..\crc32.h
# End Source File
# Begin Source File

SOURCE=..\..\crypt.h
# End Source File
# Begin Source File

SOURCE=..\..\ebcdic.h
# End Source File
# Begin Source File

SOURCE=..\..\globals.h
# End Source File
# Begin Source File

SOURCE=..\..\inflate.h
# End Source File
# Begin Source File

SOURCE=..\nt.h
# End Source File
# Begin Source File

SOURCE=..\..\ttyio.h
# End Source File
# Begin Source File

SOURCE=..\..\unzip.h
# End Source File
# Begin Source File

SOURCE=..\..\unzpriv.h
# End Source File
# Begin Source File

SOURCE=..\..\unzvers.h
# End Source File
# Begin Source File

SOURCE=..\w32cfg.h
# End Source File
# Begin Source File

SOURCE=..\..\zip.h
# End Source File
# End Group
# End Target
# End Project
