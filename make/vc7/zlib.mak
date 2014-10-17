ROOTDIR=E:\F_Soft
SYSINCDIR="C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\include"
TOOLDIR="C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin"
WORKDIR=$(ROOTDIR)\make\vc7
CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(ROOTDIR) \
	/I$(SYSINCDIR)
	
OOPT = /Fo

LIBOBJS = $(WORKDIR)\adler32.obj \
		$(WORKDIR)\compress.obj \
		$(WORKDIR)\crc32.obj \
		$(WORKDIR)\deflate.obj \
		$(WORKDIR)\gzio.obj \
		$(WORKDIR)\infback.obj \
		$(WORKDIR)\inffast.obj \
		$(WORKDIR)\inflate.obj \
		$(WORKDIR)\inftrees.obj \
		$(WORKDIR)\trees.obj \
		$(WORKDIR)\uncompr.obj \
		$(WORKDIR)\zutil.obj 
		
LIBS=zlib.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	
$(WORKDIR)\adler32.obj : $(ROOTDIR)\module\zlib\adler32.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\adler32.c

$(WORKDIR)\compress.obj : $(ROOTDIR)\module\zlib\compress.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\compress.c

$(WORKDIR)\crc32.obj : $(ROOTDIR)\module\zlib\crc32.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\crc32.c

$(WORKDIR)\deflate.obj : $(ROOTDIR)\module\zlib\deflate.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\deflate.c

$(WORKDIR)\gzio.obj : $(ROOTDIR)\module\zlib\gzio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\gzio.c

$(WORKDIR)\infback.obj : $(ROOTDIR)\module\zlib\infback.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\infback.c

$(WORKDIR)\inffast.obj : $(ROOTDIR)\module\zlib\inffast.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\inffast.c

$(WORKDIR)\inflate.obj : $(ROOTDIR)\module\zlib\inflate.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\inflate.c

$(WORKDIR)\inftrees.obj : $(ROOTDIR)\module\zlib\inftrees.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\inftrees.c

$(WORKDIR)\trees.obj : $(ROOTDIR)\module\zlib\trees.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\trees.c
	
$(WORKDIR)\uncompr.obj : $(ROOTDIR)\module\zlib\uncompr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\uncompr.c
	
$(WORKDIR)\zutil.obj : $(ROOTDIR)\module\zlib\zutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\zlib\zutil.c
