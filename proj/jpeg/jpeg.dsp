# Microsoft Developer Studio Project File - Name="jpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=jpeg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "jpeg.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "jpeg.mak" CFG="jpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jpeg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "E:\work\sfox" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "FS_PLT_WIN" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"jpeg.lib"

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "E:\work\sfox" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "FS_PLT_WIN" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"jpeg.lib"

!ENDIF 

# Begin Target

# Name "jpeg - Win32 Release"
# Name "jpeg - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\module\jpeg\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdphuff.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jerror.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jidctred.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\module\jpeg\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdct.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jerror.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\..\module\jpeg\jversion.h
# End Source File
# End Group
# End Target
# End Project
