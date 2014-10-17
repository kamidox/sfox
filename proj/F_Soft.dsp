# Microsoft Developer Studio Project File - Name="F_Soft" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=F_Soft - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "F_Soft.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "F_Soft.mak" CFG="F_Soft - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "F_Soft - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "F_Soft - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "F_Soft - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "E:\work\sfox" /I "E:\work\sfox\proj" /I "E:\work\sfox\module\jpeg" /I "E:\work\sfox\module\gif" /I "E:\work\sfox\module\xyssl" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "FS_PLT_WIN" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/sfox.exe"

!ELSEIF  "$(CFG)" == "F_Soft - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "E:\work\sfox" /I "E:\work\sfox\proj" /I "E:\work\sfox\module\jpeg" /I "E:\work\sfox\module\gif" /I "E:\work\sfox\module\xyssl" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "FS_PLT_WIN" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/sfox.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "F_Soft - Win32 Release"
# Name "F_Soft - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "eml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\eml\FS_EmlAct.c
# End Source File
# Begin Source File

SOURCE=..\src\eml\FS_EmlFile.c
# End Source File
# Begin Source File

SOURCE=..\src\eml\FS_EmlMain.c
# End Source File
# Begin Source File

SOURCE=..\src\eml\FS_Pop3.c
# End Source File
# Begin Source File

SOURCE=..\src\eml\FS_Rfc822.c
# End Source File
# Begin Source File

SOURCE=..\src\eml\FS_Smtp.c
# End Source File
# End Group
# Begin Group "gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\gui\FS_Font.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\FS_GrpLib.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\FS_Gui.c
# End Source File
# Begin Source File

SOURCE=..\src\gui\FS_WebGui.c
# End Source File
# End Group
# Begin Group "inte"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\inte\FS_IFile.c
# End Source File
# Begin Source File

SOURCE=..\src\inte\FS_IMain.c
# End Source File
# Begin Source File

SOURCE=..\src\inte\FS_ISocket.c
# End Source File
# Begin Source File

SOURCE=..\src\inte\FS_IStdLib.c
# End Source File
# Begin Source File

SOURCE=..\src\inte\FS_ISystem.c
# End Source File
# End Group
# Begin Group "util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\util\FS_Base64.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_Charset.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_File.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_Image.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_ImDecAdapter.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_List.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_MemDebug.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_Mime.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_NetConn.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_Sax.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_SslAdapter.c
# End Source File
# Begin Source File

SOURCE=..\src\util\FS_Util.c
# End Source File
# End Group
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\res\FS_Res.c
# End Source File
# End Group
# Begin Group "web"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\web\FS_BinWml.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Cookie.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_History.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Html.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Http.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Push.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_ServiceInd.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_ServiceLoad.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Wbxml.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_WebConfig.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_WebDoc.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_WebMain.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_WebUtil.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Wml.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Wsp.c
# End Source File
# Begin Source File

SOURCE=..\src\web\FS_Wtp.c
# End Source File
# End Group
# Begin Group "mms"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\mms\FS_MmsCodec.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_MmsConfig.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_MmsFile.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_MmsList.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_MmsMain.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_MmsNet.c
# End Source File
# Begin Source File

SOURCE=..\src\mms\FS_Smil.c
# End Source File
# End Group
# Begin Group "dcd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\dcd\FS_DcdLib.c
# End Source File
# Begin Source File

SOURCE=..\src\dcd\FS_DcdMain.c
# End Source File
# Begin Source File

SOURCE=..\src\dcd\FS_DcdPkg.c
# End Source File
# End Group
# Begin Group "stk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\stk\FS_Stock.c
# End Source File
# Begin Source File

SOURCE=..\src\stk\FS_StockMain.c
# End Source File
# End Group
# Begin Group "sns"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\sns\FS_SnsLib.c
# End Source File
# Begin Source File

SOURCE=..\src\sns\FS_SnsMain.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\F_Soft.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "inc_eml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\eml\FS_EmlAct.h
# End Source File
# Begin Source File

SOURCE=..\inc\eml\FS_EmlFile.h
# End Source File
# Begin Source File

SOURCE=..\inc\eml\FS_EmlMain.h
# End Source File
# Begin Source File

SOURCE=..\inc\eml\FS_Pop3.h
# End Source File
# Begin Source File

SOURCE=..\inc\eml\FS_Rfc822.h
# End Source File
# Begin Source File

SOURCE=..\inc\eml\FS_Smtp.h
# End Source File
# End Group
# Begin Group "inc_gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\gui\FS_Font.h
# End Source File
# Begin Source File

SOURCE=..\inc\gui\FS_GrpLib.h
# End Source File
# Begin Source File

SOURCE=..\inc\gui\FS_Gui.h
# End Source File
# Begin Source File

SOURCE=..\inc\gui\FS_WebGui.h
# End Source File
# End Group
# Begin Group "inc_inte"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\inte\FS_ExInte.h
# End Source File
# Begin Source File

SOURCE=..\inc\inte\FS_IFile.h
# End Source File
# Begin Source File

SOURCE=..\inc\inte\FS_Inte.h
# End Source File
# Begin Source File

SOURCE=..\inc\inte\FS_ISocket.h
# End Source File
# Begin Source File

SOURCE=..\inc\inte\FS_ISystem.h
# End Source File
# End Group
# Begin Group "inc_util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\util\FS_Base64.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_Charset.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_File.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_List.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_MemDebug.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_Mime.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_NetConn.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_Sax.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_SslAdapter.h
# End Source File
# Begin Source File

SOURCE=..\inc\util\FS_Util.h
# End Source File
# End Group
# Begin Group "inc_res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\res\FS_Res.h
# End Source File
# End Group
# Begin Group "inc_web"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\web\FS_Cookie.h
# End Source File
# Begin Source File

SOURCE=..\inc\res\FS_GuiId.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_History.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_Http.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_Push.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_Wbxml.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_WebConfig.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_WebDoc.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_WebUtil.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_Wsp.h
# End Source File
# Begin Source File

SOURCE=..\inc\web\FS_Wtp.h
# End Source File
# End Group
# Begin Group "inc_mms"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\mms\FS_MmsCodec.h
# End Source File
# Begin Source File

SOURCE=..\inc\mms\FS_MmsConfig.h
# End Source File
# Begin Source File

SOURCE=..\inc\mms\FS_MmsFile.h
# End Source File
# Begin Source File

SOURCE=..\inc\mms\FS_MmsList.h
# End Source File
# Begin Source File

SOURCE=..\inc\mms\FS_MmsNet.h
# End Source File
# Begin Source File

SOURCE=..\inc\mms\FS_Smil.h
# End Source File
# End Group
# Begin Group "inc_dcd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\dcd\FS_DcdLib.h
# End Source File
# Begin Source File

SOURCE=..\inc\dcd\FS_DcdPkg.h
# End Source File
# End Group
# Begin Group "inc_stk"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\stk\FS_Stock.h
# End Source File
# End Group
# Begin Group "inc_sns"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\inc\sns\FS_SnsLib.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\inc\FS_Config.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\keyboard.bmp
# End Source File
# End Group
# Begin Group "Libs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xyssl\xyssl.lib
# End Source File
# Begin Source File

SOURCE=.\zlib\zlib.lib
# End Source File
# Begin Source File

SOURCE=.\png\png.lib
# End Source File
# Begin Source File

SOURCE=.\jpeg\jpeg.lib
# End Source File
# Begin Source File

SOURCE=.\gif\gif.lib
# End Source File
# End Group
# End Target
# End Project
