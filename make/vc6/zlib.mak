##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=zlib_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC)
	
OOPT = /Fo

LIBOBJS = $(WORKDIR)\$(OBJDIR)\adler32.obj \
		$(WORKDIR)\$(OBJDIR)\compress.obj \
		$(WORKDIR)\$(OBJDIR)\crc32.obj \
		$(WORKDIR)\$(OBJDIR)\deflate.obj \
		$(WORKDIR)\$(OBJDIR)\gzio.obj \
		$(WORKDIR)\$(OBJDIR)\infback.obj \
		$(WORKDIR)\$(OBJDIR)\inffast.obj \
		$(WORKDIR)\$(OBJDIR)\inflate.obj \
		$(WORKDIR)\$(OBJDIR)\inftrees.obj \
		$(WORKDIR)\$(OBJDIR)\trees.obj \
		$(WORKDIR)\$(OBJDIR)\uncompr.obj \
		$(WORKDIR)\$(OBJDIR)\zutil.obj 
		
LIBS=zlib.lib       

default: $(LIBS)

OBJDIR_EXISTS:
	@-mkdir $(WORKDIR)\$(OBJDIR)

$(LIBS): OBJDIR_EXISTS $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	
$(WORKDIR)\$(OBJDIR)\adler32.obj : $(SFOX_ROOT)\module\zlib\adler32.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\adler32.c

$(WORKDIR)\$(OBJDIR)\compress.obj : $(SFOX_ROOT)\module\zlib\compress.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\compress.c

$(WORKDIR)\$(OBJDIR)\crc32.obj : $(SFOX_ROOT)\module\zlib\crc32.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\crc32.c

$(WORKDIR)\$(OBJDIR)\deflate.obj : $(SFOX_ROOT)\module\zlib\deflate.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\deflate.c

$(WORKDIR)\$(OBJDIR)\gzio.obj : $(SFOX_ROOT)\module\zlib\gzio.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\gzio.c

$(WORKDIR)\$(OBJDIR)\infback.obj : $(SFOX_ROOT)\module\zlib\infback.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\infback.c

$(WORKDIR)\$(OBJDIR)\inffast.obj : $(SFOX_ROOT)\module\zlib\inffast.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\inffast.c

$(WORKDIR)\$(OBJDIR)\inflate.obj : $(SFOX_ROOT)\module\zlib\inflate.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\inflate.c

$(WORKDIR)\$(OBJDIR)\inftrees.obj : $(SFOX_ROOT)\module\zlib\inftrees.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\inftrees.c

$(WORKDIR)\$(OBJDIR)\trees.obj : $(SFOX_ROOT)\module\zlib\trees.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\trees.c
	
$(WORKDIR)\$(OBJDIR)\uncompr.obj : $(SFOX_ROOT)\module\zlib\uncompr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\uncompr.c
	
$(WORKDIR)\$(OBJDIR)\zutil.obj : $(SFOX_ROOT)\module\zlib\zutil.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\zlib\zutil.c
