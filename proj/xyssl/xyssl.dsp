# Microsoft Developer Studio Project File - Name="xyssl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xyssl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xyssl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xyssl.mak" CFG="xyssl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xyssl - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xyssl - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xyssl - Win32 Release"

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
# ADD LIB32 /nologo /out:"xyssl.lib"

!ELSEIF  "$(CFG)" == "xyssl - Win32 Debug"

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
# ADD LIB32 /nologo /out:"xyssl.lib"

!ENDIF 

# Begin Target

# Name "xyssl - Win32 Release"
# Name "xyssl - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\module\xyssl\aes.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\arc4.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\base64.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\bignum.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\des.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\dhm.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\havege.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md2.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md4.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\rsa.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\sha1.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\sha2.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\ssl_cli.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\ssl_srv.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\ssl_tls.c
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\x509_read.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\module\xyssl\aes.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\arc4.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\base64.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\bignum.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\des.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\dhm.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\havege.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md2.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md4.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\muladdc.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\net.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\rsa.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\sha2.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\ssl.h
# End Source File
# Begin Source File

SOURCE=..\..\module\xyssl\x509.h
# End Source File
# End Group
# End Target
# End Project
