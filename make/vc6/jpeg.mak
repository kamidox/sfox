##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=jpeg_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC)
	
OOPT = /Fo

LIBOBJS = 	$(WORKDIR)\$(OBJDIR)\jcomapi.obj  \
			$(WORKDIR)\$(OBJDIR)\jdtrans.obj  \
			$(WORKDIR)\$(OBJDIR)\jdapistd.obj \
			$(WORKDIR)\$(OBJDIR)\jdatasrc.obj \
			$(WORKDIR)\$(OBJDIR)\jdcoefct.obj \
			$(WORKDIR)\$(OBJDIR)\jdhuff.obj   \
			$(WORKDIR)\$(OBJDIR)\jdinput.obj  \
			$(WORKDIR)\$(OBJDIR)\jdmainct.obj \
			$(WORKDIR)\$(OBJDIR)\jdmarker.obj \
			$(WORKDIR)\$(OBJDIR)\jdmaster.obj \
			$(WORKDIR)\$(OBJDIR)\jdmerge.obj  \
			$(WORKDIR)\$(OBJDIR)\jdphuff.obj  \
			$(WORKDIR)\$(OBJDIR)\jdpostct.obj \
			$(WORKDIR)\$(OBJDIR)\jdsample.obj \
			$(WORKDIR)\$(OBJDIR)\jdapimin.obj \
			$(WORKDIR)\$(OBJDIR)\jerror.obj   \
			$(WORKDIR)\$(OBJDIR)\jidctred.obj \
			$(WORKDIR)\$(OBJDIR)\jidctfst.obj  \
			$(WORKDIR)\$(OBJDIR)\jidctint.obj \
			$(WORKDIR)\$(OBJDIR)\jidctflt.obj \
			$(WORKDIR)\$(OBJDIR)\jmemmgr.obj   \
			$(WORKDIR)\$(OBJDIR)\jmemnobs.obj  \
			$(WORKDIR)\$(OBJDIR)\jutils.obj    \
			$(WORKDIR)\$(OBJDIR)\jquant2.obj   \
			$(WORKDIR)\$(OBJDIR)\jddctmgr.obj  \
			$(WORKDIR)\$(OBJDIR)\jquant1.obj   \
			$(WORKDIR)\$(OBJDIR)\jdcolor.obj
	
LIBS=jpeg.lib       
        
default: $(LIBS)
        
OBJDIR_EXISTS:
	@-mkdir $(WORKDIR)\$(OBJDIR)

$(LIBS): OBJDIR_EXISTS $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	    
$(WORKDIR)\$(OBJDIR)\jcomapi.obj : $(SFOX_ROOT)\module\jpeg\jcomapi.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jcomapi.c

$(WORKDIR)\$(OBJDIR)\jdtrans.obj : $(SFOX_ROOT)\module\jpeg\jdtrans.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdtrans.c

$(WORKDIR)\$(OBJDIR)\jdapistd.obj : $(SFOX_ROOT)\module\jpeg\jdapistd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdapistd.c

$(WORKDIR)\$(OBJDIR)\jdatasrc.obj : $(SFOX_ROOT)\module\jpeg\jdatasrc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdatasrc.c

$(WORKDIR)\$(OBJDIR)\jdcoefct.obj : $(SFOX_ROOT)\module\jpeg\jdcoefct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdcoefct.c

$(WORKDIR)\$(OBJDIR)\jdhuff.obj : $(SFOX_ROOT)\module\jpeg\jdhuff.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdhuff.c

$(WORKDIR)\$(OBJDIR)\jdinput.obj : $(SFOX_ROOT)\module\jpeg\jdinput.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdinput.c

$(WORKDIR)\$(OBJDIR)\jdmainct.obj : $(SFOX_ROOT)\module\jpeg\jdmainct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdmainct.c

$(WORKDIR)\$(OBJDIR)\jdmarker.obj : $(SFOX_ROOT)\module\jpeg\jdmarker.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdmarker.c

$(WORKDIR)\$(OBJDIR)\jdmaster.obj : $(SFOX_ROOT)\module\jpeg\jdmaster.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdmaster.c

$(WORKDIR)\$(OBJDIR)\jdmerge.obj : $(SFOX_ROOT)\module\jpeg\jdmerge.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdmerge.c

$(WORKDIR)\$(OBJDIR)\jdphuff.obj : $(SFOX_ROOT)\module\jpeg\jdphuff.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdphuff.c

$(WORKDIR)\$(OBJDIR)\jdpostct.obj : $(SFOX_ROOT)\module\jpeg\jdpostct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdpostct.c

$(WORKDIR)\$(OBJDIR)\jdsample.obj : $(SFOX_ROOT)\module\jpeg\jdsample.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdsample.c

$(WORKDIR)\$(OBJDIR)\jdapimin.obj : $(SFOX_ROOT)\module\jpeg\jdapimin.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdapimin.c

$(WORKDIR)\$(OBJDIR)\jerror.obj : $(SFOX_ROOT)\module\jpeg\jerror.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jerror.c

$(WORKDIR)\$(OBJDIR)\jidctred.obj : $(SFOX_ROOT)\module\jpeg\jidctred.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jidctred.c

$(WORKDIR)\$(OBJDIR)\jidctfst.obj : $(SFOX_ROOT)\module\jpeg\jidctfst.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jidctfst.c

$(WORKDIR)\$(OBJDIR)\jidctint.obj : $(SFOX_ROOT)\module\jpeg\jidctint.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jidctint.c

$(WORKDIR)\$(OBJDIR)\jidctflt.obj : $(SFOX_ROOT)\module\jpeg\jidctflt.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jidctflt.c

$(WORKDIR)\$(OBJDIR)\jmemmgr.obj : $(SFOX_ROOT)\module\jpeg\jmemmgr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jmemmgr.c

$(WORKDIR)\$(OBJDIR)\jmemnobs.obj : $(SFOX_ROOT)\module\jpeg\jmemnobs.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jmemnobs.c

$(WORKDIR)\$(OBJDIR)\jutils.obj : $(SFOX_ROOT)\module\jpeg\jutils.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jutils.c

$(WORKDIR)\$(OBJDIR)\jquant2.obj : $(SFOX_ROOT)\module\jpeg\jquant2.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jquant2.c

$(WORKDIR)\$(OBJDIR)\jddctmgr.obj : $(SFOX_ROOT)\module\jpeg\jddctmgr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jddctmgr.c

$(WORKDIR)\$(OBJDIR)\jquant1.obj : $(SFOX_ROOT)\module\jpeg\jquant1.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jquant1.c

$(WORKDIR)\$(OBJDIR)\jdcolor.obj : $(SFOX_ROOT)\module\jpeg\jdcolor.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\jpeg\jdcolor.c
