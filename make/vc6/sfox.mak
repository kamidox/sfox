##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=sfox_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC) \
	/I$(SFOX_ROOT)\module\xyssl \
	/I$(SFOX_ROOT)\module\png \
	/I$(SFOX_ROOT)\module\jpeg \
	/I$(SFOX_ROOT)\module\gif
	
OOPT = /Fo
	
LIBOBJS = $(WORKDIR)\$(OBJDIR)\FS_Rfc822.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_Smtp.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_EmlAct.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_Pop3.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_EmlFile.obj   \
		$(WORKDIR)\$(OBJDIR)\FS_EmlMain.obj   \
		$(WORKDIR)\$(OBJDIR)\FS_Res.obj		\
		$(WORKDIR)\$(OBJDIR)\FS_Font.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_GrpLib.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_Gui.obj       \
		$(WORKDIR)\$(OBJDIR)\FS_WebGui.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_Base64.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_Charset.obj   \
		$(WORKDIR)\$(OBJDIR)\FS_File.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_List.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_Mime.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_Sax.obj       \
		$(WORKDIR)\$(OBJDIR)\FS_Util.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_MemDebug.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_Cookie.obj    \
		$(WORKDIR)\$(OBJDIR)\FS_History.obj   \
		$(WORKDIR)\$(OBJDIR)\FS_Html.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_Http.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_Push.obj      \
		$(WORKDIR)\$(OBJDIR)\FS_ServiceInd.obj \
		$(WORKDIR)\$(OBJDIR)\FS_ServiceLoad.obj \
		$(WORKDIR)\$(OBJDIR)\FS_Wbxml.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_WebConfig.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_WebDoc.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_WebMain.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_WebUtil.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_Wml.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_BinWml.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_Wsp.obj  \
		$(WORKDIR)\$(OBJDIR)\FS_Wtp.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsMain.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsList.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsFile.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsCodec.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsNet.obj \
		$(WORKDIR)\$(OBJDIR)\FS_NetConn.obj \
		$(WORKDIR)\$(OBJDIR)\FS_Smil.obj \
		$(WORKDIR)\$(OBJDIR)\FS_MmsConfig.obj \
		$(WORKDIR)\$(OBJDIR)\FS_ImDecAdapter.obj \
		$(WORKDIR)\$(OBJDIR)\FS_SslAdapter.obj \
		$(WORKDIR)\$(OBJDIR)\FS_Image.obj \
		$(WORKDIR)\$(OBJDIR)\FS_SnsMain.obj \
		$(WORKDIR)\$(OBJDIR)\FS_SnsLib.obj
		

LIBS=sfox.lib       

default: $(LIBS)

OBJDIR_EXIST:
	@-mkdir $(WORKDIR)\$(OBJDIR)
	
$(LIBS): OBJDIR_EXIST $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	
$(WORKDIR)\$(OBJDIR)\FS_Rfc822.obj : $(SFOX_ROOT)\src\eml\FS_Rfc822.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_Rfc822.c
$(WORKDIR)\$(OBJDIR)\FS_Smtp.obj : $(SFOX_ROOT)\src\eml\FS_Smtp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_Smtp.c
$(WORKDIR)\$(OBJDIR)\FS_EmlAct.obj : $(SFOX_ROOT)\src\eml\FS_EmlAct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_EmlAct.c
$(WORKDIR)\$(OBJDIR)\FS_EmlFile.obj : $(SFOX_ROOT)\src\eml\FS_EmlFile.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_EmlFile.c
$(WORKDIR)\$(OBJDIR)\FS_EmlMain.obj : $(SFOX_ROOT)\src\eml\FS_EmlMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_EmlMain.c
$(WORKDIR)\$(OBJDIR)\FS_Pop3.obj : $(SFOX_ROOT)\src\eml\FS_Pop3.c
    $(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\eml\FS_Pop3.c


$(WORKDIR)\$(OBJDIR)\FS_Font.obj : $(SFOX_ROOT)\src\gui\FS_Font.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\gui\FS_Font.c
$(WORKDIR)\$(OBJDIR)\FS_GrpLib.obj : $(SFOX_ROOT)\src\gui\FS_GrpLib.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\gui\FS_GrpLib.c
$(WORKDIR)\$(OBJDIR)\FS_Gui.obj : $(SFOX_ROOT)\src\gui\FS_Gui.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\gui\FS_Gui.c
$(WORKDIR)\$(OBJDIR)\FS_WebGui.obj : $(SFOX_ROOT)\src\gui\FS_WebGui.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\gui\FS_WebGui.c


$(WORKDIR)\$(OBJDIR)\FS_Base64.obj : $(SFOX_ROOT)\src\util\FS_Base64.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Base64.c
$(WORKDIR)\$(OBJDIR)\FS_Sax.obj : $(SFOX_ROOT)\src\util\FS_Sax.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Sax.c
$(WORKDIR)\$(OBJDIR)\FS_File.obj : $(SFOX_ROOT)\src\util\FS_File.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_File.c
$(WORKDIR)\$(OBJDIR)\FS_List.obj : $(SFOX_ROOT)\src\util\FS_List.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_List.c
$(WORKDIR)\$(OBJDIR)\FS_Mime.obj : $(SFOX_ROOT)\src\util\FS_Mime.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Mime.c
$(WORKDIR)\$(OBJDIR)\FS_Util.obj : $(SFOX_ROOT)\src\util\FS_Util.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Util.c
$(WORKDIR)\$(OBJDIR)\FS_Charset.obj : $(SFOX_ROOT)\src\util\FS_Charset.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Charset.c
$(WORKDIR)\$(OBJDIR)\FS_MemDebug.obj : $(SFOX_ROOT)\src\util\FS_MemDebug.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_MemDebug.c
$(WORKDIR)\$(OBJDIR)\FS_NetConn.obj : $(SFOX_ROOT)\src\util\FS_NetConn.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_NetConn.c
$(WORKDIR)\$(OBJDIR)\FS_ImDecAdapter.obj : $(SFOX_ROOT)\src\util\FS_ImDecAdapter.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_ImDecAdapter.c
$(WORKDIR)\$(OBJDIR)\FS_Image.obj : $(SFOX_ROOT)\src\util\FS_Image.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_Image.c
$(WORKDIR)\$(OBJDIR)\FS_SslAdapter.obj : $(SFOX_ROOT)\src\util\FS_SslAdapter.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\util\FS_SslAdapter.c
	

$(WORKDIR)\$(OBJDIR)\FS_Res.obj : $(SFOX_ROOT)\src\res\FS_Res.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\res\FS_Res.c


$(WORKDIR)\$(OBJDIR)\FS_Cookie.obj : $(SFOX_ROOT)\src\web\FS_Cookie.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Cookie.c
$(WORKDIR)\$(OBJDIR)\FS_History.obj : $(SFOX_ROOT)\src\web\FS_History.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_History.c
$(WORKDIR)\$(OBJDIR)\FS_Html.obj : $(SFOX_ROOT)\src\web\FS_Html.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Html.c
$(WORKDIR)\$(OBJDIR)\FS_Http.obj : $(SFOX_ROOT)\src\web\FS_Http.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Http.c
$(WORKDIR)\$(OBJDIR)\FS_Push.obj : $(SFOX_ROOT)\src\web\FS_Push.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Push.c
$(WORKDIR)\$(OBJDIR)\FS_ServiceInd.obj : $(SFOX_ROOT)\src\web\FS_ServiceInd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_ServiceInd.c
$(WORKDIR)\$(OBJDIR)\FS_ServiceLoad.obj : $(SFOX_ROOT)\src\web\FS_ServiceLoad.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_ServiceLoad.c
$(WORKDIR)\$(OBJDIR)\FS_Wbxml.obj : $(SFOX_ROOT)\src\web\FS_Wbxml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Wbxml.c
$(WORKDIR)\$(OBJDIR)\FS_WebConfig.obj : $(SFOX_ROOT)\src\web\FS_WebConfig.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_WebConfig.c
$(WORKDIR)\$(OBJDIR)\FS_WebDoc.obj : $(SFOX_ROOT)\src\web\FS_WebDoc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_WebDoc.c
$(WORKDIR)\$(OBJDIR)\FS_WebMain.obj : $(SFOX_ROOT)\src\web\FS_WebMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_WebMain.c
$(WORKDIR)\$(OBJDIR)\FS_WebUtil.obj : $(SFOX_ROOT)\src\web\FS_WebUtil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_WebUtil.c
$(WORKDIR)\$(OBJDIR)\FS_Wml.obj : $(SFOX_ROOT)\src\web\FS_Wml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Wml.c
$(WORKDIR)\$(OBJDIR)\FS_BinWml.obj : $(SFOX_ROOT)\src\web\FS_BinWml.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_BinWml.c
$(WORKDIR)\$(OBJDIR)\FS_Wsp.obj : $(SFOX_ROOT)\src\web\FS_Wsp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Wsp.c
$(WORKDIR)\$(OBJDIR)\FS_Wtp.obj : $(SFOX_ROOT)\src\web\FS_Wtp.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\web\FS_Wtp.c

$(WORKDIR)\$(OBJDIR)\FS_MmsMain.obj : $(SFOX_ROOT)\src\mms\FS_MmsMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsMain.c
$(WORKDIR)\$(OBJDIR)\FS_MmsList.obj : $(SFOX_ROOT)\src\mms\FS_MmsList.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsList.c
$(WORKDIR)\$(OBJDIR)\FS_MmsFile.obj : $(SFOX_ROOT)\src\mms\FS_MmsFile.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsFile.c
$(WORKDIR)\$(OBJDIR)\FS_MmsCodec.obj : $(SFOX_ROOT)\src\mms\FS_MmsCodec.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsCodec.c
$(WORKDIR)\$(OBJDIR)\FS_MmsNet.obj : $(SFOX_ROOT)\src\mms\FS_MmsNet.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsNet.c
$(WORKDIR)\$(OBJDIR)\FS_Smil.obj : $(SFOX_ROOT)\src\mms\FS_Smil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_Smil.c
$(WORKDIR)\$(OBJDIR)\FS_MmsConfig.obj : $(SFOX_ROOT)\src\mms\FS_MmsConfig.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\mms\FS_MmsConfig.c

$(WORKDIR)\$(OBJDIR)\FS_SnsMain.obj : $(SFOX_ROOT)\src\sns\FS_SnsMain.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\sns\FS_SnsMain.c
$(WORKDIR)\$(OBJDIR)\FS_SnsLib.obj : $(SFOX_ROOT)\src\sns\FS_SnsLib.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\src\sns\FS_SnsLib.c
