##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=png_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC)
	
OOPT = /Fo

LIBOBJS = $(WORKDIR)\$(OBJDIR)\png.obj \
		$(WORKDIR)\$(OBJDIR)\pngerror.obj \
		$(WORKDIR)\$(OBJDIR)\pngget.obj \
		$(WORKDIR)\$(OBJDIR)\pngmem.obj \
		$(WORKDIR)\$(OBJDIR)\pngpread.obj \
		$(WORKDIR)\$(OBJDIR)\pngread.obj \
		$(WORKDIR)\$(OBJDIR)\pngrio.obj \
		$(WORKDIR)\$(OBJDIR)\pngrtran.obj \
		$(WORKDIR)\$(OBJDIR)\pngrutil.obj \
		$(WORKDIR)\$(OBJDIR)\pngset.obj \
		$(WORKDIR)\$(OBJDIR)\pngtrans.obj \
		$(WORKDIR)\$(OBJDIR)\pngvcrd.obj \
		$(WORKDIR)\$(OBJDIR)\pngwio.obj \
		$(WORKDIR)\$(OBJDIR)\pngwrite.obj \
		$(WORKDIR)\$(OBJDIR)\pngwtran.obj \
		$(WORKDIR)\$(OBJDIR)\pngwutil.obj 
		
LIBS=png.lib       

default: $(LIBS)

OBJDIR_EXISTS:
	@-mkdir $(WORKDIR)\$(OBJDIR)

$(LIBS): OBJDIR_EXISTS $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\$(OBJDIR)\*.obj /F /Q
	
$(WORKDIR)\$(OBJDIR)\png.obj : $(SFOX_ROOT)\module\png\png.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\png.c

$(WORKDIR)\$(OBJDIR)\pngerror.obj : $(SFOX_ROOT)\module\png\pngerror.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngerror.c

$(WORKDIR)\$(OBJDIR)\pngget.obj : $(SFOX_ROOT)\module\png\pngget.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngget.c

$(WORKDIR)\$(OBJDIR)\pngmem.obj : $(SFOX_ROOT)\module\png\pngmem.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngmem.c

$(WORKDIR)\$(OBJDIR)\pngpread.obj : $(SFOX_ROOT)\module\png\pngpread.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngpread.c

$(WORKDIR)\$(OBJDIR)\pngread.obj : $(SFOX_ROOT)\module\png\pngread.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngread.c

$(WORKDIR)\$(OBJDIR)\pngrio.obj : $(SFOX_ROOT)\module\png\pngrio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngrio.c

$(WORKDIR)\$(OBJDIR)\pngrtran.obj : $(SFOX_ROOT)\module\png\pngrtran.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngrtran.c
	
$(WORKDIR)\$(OBJDIR)\pngrutil.obj : $(SFOX_ROOT)\module\png\pngrutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngrutil.c
	
$(WORKDIR)\$(OBJDIR)\pngset.obj : $(SFOX_ROOT)\module\png\pngset.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngset.c
	
$(WORKDIR)\$(OBJDIR)\pngtrans.obj : $(SFOX_ROOT)\module\png\pngtrans.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngtrans.c

$(WORKDIR)\$(OBJDIR)\pngvcrd.obj : $(SFOX_ROOT)\module\png\pngvcrd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngvcrd.c

$(WORKDIR)\$(OBJDIR)\pngwio.obj : $(SFOX_ROOT)\module\png\pngwio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngwio.c
	
$(WORKDIR)\$(OBJDIR)\pngwrite.obj : $(SFOX_ROOT)\module\png\pngwrite.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngwrite.c
	
$(WORKDIR)\$(OBJDIR)\pngwtran.obj : $(SFOX_ROOT)\module\png\pngwtran.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngwtran.c

$(WORKDIR)\$(OBJDIR)\pngwutil.obj : $(SFOX_ROOT)\module\png\pngwutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\png\pngwutil.c
