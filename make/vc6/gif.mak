##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=gif_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC)
	
OOPT = /Fo

LIBOBJS = 	$(WORKDIR)\$(OBJDIR)\dgif_lib.obj  \
			$(WORKDIR)\$(OBJDIR)\gif_err.obj  \
			$(WORKDIR)\$(OBJDIR)\gifalloc.obj

LIBS=gif.lib       

default: $(LIBS)

OBJDIR_EXISTS:
	@-mkdir $(WORKDIR)\$(OBJDIR)

$(LIBS): OBJDIR_EXISTS $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	    
$(WORKDIR)\$(OBJDIR)\dgif_lib.obj : $(SFOX_ROOT)\module\gif\dgif_lib.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\gif\dgif_lib.c

$(WORKDIR)\$(OBJDIR)\gif_err.obj : $(SFOX_ROOT)\module\gif\gif_err.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\gif\gif_err.c

$(WORKDIR)\$(OBJDIR)\gifalloc.obj : $(SFOX_ROOT)\module\gif\gifalloc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\gif\gifalloc.c
