ROOTDIR=E:\F_Soft
SYSINCDIR="C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\include"
TOOLDIR="C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin"
WORKDIR=$(ROOTDIR)\make\vc7
CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(ROOTDIR) \
	/I$(SYSINCDIR) \
	/I$(ROOTDIR)\module\xyssl \
	/I$(ROOTDIR)\module\png \
	/I$(ROOTDIR)\module\jpeg \
	/I$(ROOTDIR)\module\gif
	
OOPT = /Fo
	
LIBOBJS = $(WORKDIR)\FS_Rfc822.obj    \
		$(WORKDIR)\FS_Smtp.obj      \
		$(WORKDIR)\FS_EmlAct.obj    \
		$(WORKDIR)\FS_Pop3.obj      \
		$(WORKDIR)\FS_EmlFile.obj   \
		$(WORKDIR)\FS_EmlMain.obj   \
		$(WORKDIR)\FS_Res.obj		\
		$(WORKDIR)\FS_Font.obj      \
		$(WORKDIR)\FS_GrpLib.obj    \
		$(WORKDIR)\FS_Gui.obj       \
		$(WORKDIR)\FS_WebGui.obj    \
		$(WORKDIR)\FS_Base64.obj    \
		$(WORKDIR)\FS_Charset.obj   \
		$(WORKDIR)\FS_File.obj      \
		$(WORKDIR)\FS_List.obj      \
		$(WORKDIR)\FS_Mime.obj      \
		$(WORKDIR)\FS_Sax.obj       \
		$(WORKDIR)\FS_Util.obj      \
		$(WORKDIR)\FS_MemDebug.obj  \
		$(WORKDIR)\FS_Cookie.obj    \
		$(WORKDIR)\FS_History.obj   \
		$(WORKDIR)\FS_Html.obj      \
		$(WORKDIR)\FS_Http.obj      \
		$(WORKDIR)\FS_Push.obj      \
		$(WORKDIR)\FS_ServiceInd.obj \
		$(WORKDIR)\FS_ServiceLoad.obj \
		$(WORKDIR)\FS_Wbxml.obj  \
		$(WORKDIR)\FS_WebConfig.obj  \
		$(WORKDIR)\FS_WebDoc.obj  \
		$(WORKDIR)\FS_WebMain.obj  \
		$(WORKDIR)\FS_WebUtil.obj  \
		$(WORKDIR)\FS_Wml.obj  \
		$(WORKDIR)\FS_BinWml.obj  \
		$(WORKDIR)\FS_Wsp.obj  \
		$(WORKDIR)\FS_Wtp.obj \
		$(WORKDIR)\FS_MmsMain.obj \
		$(WORKDIR)\FS_MmsList.obj \
		$(WORKDIR)\FS_MmsFile.obj \
		$(WORKDIR)\FS_MmsCodec.obj \
		$(WORKDIR)\FS_MmsNet.obj \
		$(WORKDIR)\FS_NetConn.obj \
		$(WORKDIR)\FS_Smil.obj \
		$(WORKDIR)\FS_MmsConfig.obj \
		$(WORKDIR)\FS_ImDecAdapter.obj \
		$(WORKDIR)\FS_Image.obj \
		$(WORKDIR)\FS_SslAdapter.obj

LIBS=sfox.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	
$(WORKDIR)\FS_Rfc822.obj : $(ROOTDIR)\src\eml\FS_Rfc822.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_Rfc822.c
$(WORKDIR)\FS_Smtp.obj : $(ROOTDIR)\src\eml\FS_Smtp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_Smtp.c
$(WORKDIR)\FS_EmlAct.obj : $(ROOTDIR)\src\eml\FS_EmlAct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_EmlAct.c
$(WORKDIR)\FS_EmlFile.obj : $(ROOTDIR)\src\eml\FS_EmlFile.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_EmlFile.c
$(WORKDIR)\FS_EmlMain.obj : $(ROOTDIR)\src\eml\FS_EmlMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_EmlMain.c
$(WORKDIR)\FS_Pop3.obj : $(ROOTDIR)\src\eml\FS_Pop3.c
    $(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\eml\FS_Pop3.c


$(WORKDIR)\FS_Font.obj : $(ROOTDIR)\src\gui\FS_Font.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\gui\FS_Font.c
$(WORKDIR)\FS_GrpLib.obj : $(ROOTDIR)\src\gui\FS_GrpLib.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\gui\FS_GrpLib.c
$(WORKDIR)\FS_Gui.obj : $(ROOTDIR)\src\gui\FS_Gui.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\gui\FS_Gui.c
$(WORKDIR)\FS_WebGui.obj : $(ROOTDIR)\src\gui\FS_WebGui.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\gui\FS_WebGui.c


$(WORKDIR)\FS_Base64.obj : $(ROOTDIR)\src\util\FS_Base64.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Base64.c
$(WORKDIR)\FS_Sax.obj : $(ROOTDIR)\src\util\FS_Sax.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Sax.c
$(WORKDIR)\FS_File.obj : $(ROOTDIR)\src\util\FS_File.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_File.c
$(WORKDIR)\FS_List.obj : $(ROOTDIR)\src\util\FS_List.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_List.c
$(WORKDIR)\FS_Mime.obj : $(ROOTDIR)\src\util\FS_Mime.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Mime.c
$(WORKDIR)\FS_Util.obj : $(ROOTDIR)\src\util\FS_Util.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Util.c
$(WORKDIR)\FS_Charset.obj : $(ROOTDIR)\src\util\FS_Charset.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Charset.c
$(WORKDIR)\FS_MemDebug.obj : $(ROOTDIR)\src\util\FS_MemDebug.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_MemDebug.c
$(WORKDIR)\FS_NetConn.obj : $(ROOTDIR)\src\util\FS_NetConn.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_NetConn.c
$(WORKDIR)\FS_ImDecAdapter.obj : $(ROOTDIR)\src\util\FS_ImDecAdapter.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_ImDecAdapter.c
$(WORKDIR)\FS_Image.obj : $(ROOTDIR)\src\util\FS_Image.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_Image.c
$(WORKDIR)\FS_SslAdapter.obj : $(ROOTDIR)\src\util\FS_SslAdapter.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\util\FS_SslAdapter.c
	

$(WORKDIR)\FS_Res.obj : $(ROOTDIR)\src\res\FS_Res.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\res\FS_Res.c


$(WORKDIR)\FS_Cookie.obj : $(ROOTDIR)\src\web\FS_Cookie.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Cookie.c
$(WORKDIR)\FS_History.obj : $(ROOTDIR)\src\web\FS_History.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_History.c
$(WORKDIR)\FS_Html.obj : $(ROOTDIR)\src\web\FS_Html.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Html.c
$(WORKDIR)\FS_Http.obj : $(ROOTDIR)\src\web\FS_Http.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Http.c
$(WORKDIR)\FS_Push.obj : $(ROOTDIR)\src\web\FS_Push.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Push.c
$(WORKDIR)\FS_ServiceInd.obj : $(ROOTDIR)\src\web\FS_ServiceInd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_ServiceInd.c
$(WORKDIR)\FS_ServiceLoad.obj : $(ROOTDIR)\src\web\FS_ServiceLoad.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_ServiceLoad.c
$(WORKDIR)\FS_Wbxml.obj : $(ROOTDIR)\src\web\FS_Wbxml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Wbxml.c
$(WORKDIR)\FS_WebConfig.obj : $(ROOTDIR)\src\web\FS_WebConfig.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_WebConfig.c
$(WORKDIR)\FS_WebDoc.obj : $(ROOTDIR)\src\web\FS_WebDoc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_WebDoc.c
$(WORKDIR)\FS_WebMain.obj : $(ROOTDIR)\src\web\FS_WebMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_WebMain.c
$(WORKDIR)\FS_WebUtil.obj : $(ROOTDIR)\src\web\FS_WebUtil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_WebUtil.c
$(WORKDIR)\FS_Wml.obj : $(ROOTDIR)\src\web\FS_Wml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Wml.c
$(WORKDIR)\FS_BinWml.obj : $(ROOTDIR)\src\web\FS_BinWml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_BinWml.c
$(WORKDIR)\FS_Wsp.obj : $(ROOTDIR)\src\web\FS_Wsp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Wsp.c
$(WORKDIR)\FS_Wtp.obj : $(ROOTDIR)\src\web\FS_Wtp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\web\FS_Wtp.c

$(WORKDIR)\FS_MmsMain.obj : $(ROOTDIR)\src\mms\FS_MmsMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsMain.c
$(WORKDIR)\FS_MmsList.obj : $(ROOTDIR)\src\mms\FS_MmsList.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsList.c
$(WORKDIR)\FS_MmsFile.obj : $(ROOTDIR)\src\mms\FS_MmsFile.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsFile.c
$(WORKDIR)\FS_MmsCodec.obj : $(ROOTDIR)\src\mms\FS_MmsCodec.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsCodec.c
$(WORKDIR)\FS_MmsNet.obj : $(ROOTDIR)\src\mms\FS_MmsNet.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsNet.c
$(WORKDIR)\FS_Smil.obj : $(ROOTDIR)\src\mms\FS_Smil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_Smil.c
$(WORKDIR)\FS_MmsConfig.obj : $(ROOTDIR)\src\mms\FS_MmsConfig.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\src\mms\FS_MmsConfig.c
	