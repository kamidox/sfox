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

LIBOBJS = $(WORKDIR)\png.obj \
		$(WORKDIR)\pngerror.obj \
		$(WORKDIR)\pngget.obj \
		$(WORKDIR)\pngmem.obj \
		$(WORKDIR)\pngpread.obj \
		$(WORKDIR)\pngread.obj \
		$(WORKDIR)\pngrio.obj \
		$(WORKDIR)\pngrtran.obj \
		$(WORKDIR)\pngrutil.obj \
		$(WORKDIR)\pngset.obj \
		$(WORKDIR)\pngtrans.obj \
		$(WORKDIR)\pngvcrd.obj \
		$(WORKDIR)\pngwio.obj \
		$(WORKDIR)\pngwrite.obj \
		$(WORKDIR)\pngwtran.obj \
		$(WORKDIR)\pngwutil.obj 
		
LIBS=png.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	
$(WORKDIR)\png.obj : $(ROOTDIR)\module\png\png.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\png.c

$(WORKDIR)\pngerror.obj : $(ROOTDIR)\module\png\pngerror.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngerror.c

$(WORKDIR)\pngget.obj : $(ROOTDIR)\module\png\pngget.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngget.c

$(WORKDIR)\pngmem.obj : $(ROOTDIR)\module\png\pngmem.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngmem.c

$(WORKDIR)\pngpread.obj : $(ROOTDIR)\module\png\pngpread.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngpread.c

$(WORKDIR)\pngread.obj : $(ROOTDIR)\module\png\pngread.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngread.c

$(WORKDIR)\pngrio.obj : $(ROOTDIR)\module\png\pngrio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngrio.c

$(WORKDIR)\pngrtran.obj : $(ROOTDIR)\module\png\pngrtran.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngrtran.c
	
$(WORKDIR)\pngrutil.obj : $(ROOTDIR)\module\png\pngrutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngrutil.c
	
$(WORKDIR)\pngset.obj : $(ROOTDIR)\module\png\pngset.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngset.c
	
$(WORKDIR)\pngtrans.obj : $(ROOTDIR)\module\png\pngtrans.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngtrans.c

$(WORKDIR)\pngvcrd.obj : $(ROOTDIR)\module\png\pngvcrd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngvcrd.c

$(WORKDIR)\pngwio.obj : $(ROOTDIR)\module\png\pngwio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngwio.c
	
$(WORKDIR)\pngwrite.obj : $(ROOTDIR)\module\png\pngwrite.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngwrite.c
	
$(WORKDIR)\pngwtran.obj : $(ROOTDIR)\module\png\pngwtran.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngwtran.c

$(WORKDIR)\pngwutil.obj : $(ROOTDIR)\module\png\pngwutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\png\pngwutil.c
