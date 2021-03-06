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

LIBOBJS = 	$(WORKDIR)\jcomapi.obj  \
			$(WORKDIR)\jdtrans.obj  \
			$(WORKDIR)\jdapistd.obj \
			$(WORKDIR)\jdatasrc.obj \
			$(WORKDIR)\jdcoefct.obj \
			$(WORKDIR)\jdhuff.obj   \
			$(WORKDIR)\jdinput.obj  \
			$(WORKDIR)\jdmainct.obj \
			$(WORKDIR)\jdmarker.obj \
			$(WORKDIR)\jdmaster.obj \
			$(WORKDIR)\jdmerge.obj  \
			$(WORKDIR)\jdphuff.obj  \
			$(WORKDIR)\jdpostct.obj \
			$(WORKDIR)\jdsample.obj \
			$(WORKDIR)\jdapimin.obj \
			$(WORKDIR)\jerror.obj   \
			$(WORKDIR)\jidctred.obj \
			$(WORKDIR)\jidctfst.obj  \
			$(WORKDIR)\jidctint.obj \
			$(WORKDIR)\jidctflt.obj \
			$(WORKDIR)\jmemmgr.obj   \
			$(WORKDIR)\jmemnobs.obj  \
			$(WORKDIR)\jutils.obj    \
			$(WORKDIR)\jquant2.obj   \
			$(WORKDIR)\jddctmgr.obj  \
			$(WORKDIR)\jquant1.obj   \
			$(WORKDIR)\jdcolor.obj
	
LIBS=jpeg.lib       
        
default: $(LIBS)
        
$(LIBS): $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	    
$(WORKDIR)\jcomapi.obj : $(ROOTDIR)\module\jpeg\jcomapi.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jcomapi.c

$(WORKDIR)\jdtrans.obj : $(ROOTDIR)\module\jpeg\jdtrans.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdtrans.c

$(WORKDIR)\jdapistd.obj : $(ROOTDIR)\module\jpeg\jdapistd.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdapistd.c

$(WORKDIR)\jdatasrc.obj : $(ROOTDIR)\module\jpeg\jdatasrc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdatasrc.c

$(WORKDIR)\jdcoefct.obj : $(ROOTDIR)\module\jpeg\jdcoefct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdcoefct.c

$(WORKDIR)\jdhuff.obj : $(ROOTDIR)\module\jpeg\jdhuff.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdhuff.c

$(WORKDIR)\jdinput.obj : $(ROOTDIR)\module\jpeg\jdinput.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdinput.c

$(WORKDIR)\jdmainct.obj : $(ROOTDIR)\module\jpeg\jdmainct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdmainct.c

$(WORKDIR)\jdmarker.obj : $(ROOTDIR)\module\jpeg\jdmarker.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdmarker.c

$(WORKDIR)\jdmaster.obj : $(ROOTDIR)\module\jpeg\jdmaster.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdmaster.c

$(WORKDIR)\jdmerge.obj : $(ROOTDIR)\module\jpeg\jdmerge.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdmerge.c

$(WORKDIR)\jdphuff.obj : $(ROOTDIR)\module\jpeg\jdphuff.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdphuff.c

$(WORKDIR)\jdpostct.obj : $(ROOTDIR)\module\jpeg\jdpostct.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdpostct.c

$(WORKDIR)\jdsample.obj : $(ROOTDIR)\module\jpeg\jdsample.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdsample.c

$(WORKDIR)\jdapimin.obj : $(ROOTDIR)\module\jpeg\jdapimin.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdapimin.c

$(WORKDIR)\jerror.obj : $(ROOTDIR)\module\jpeg\jerror.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jerror.c

$(WORKDIR)\jidctred.obj : $(ROOTDIR)\module\jpeg\jidctred.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jidctred.c

$(WORKDIR)\jidctfst.obj : $(ROOTDIR)\module\jpeg\jidctfst.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jidctfst.c

$(WORKDIR)\jidctint.obj : $(ROOTDIR)\module\jpeg\jidctint.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jidctint.c

$(WORKDIR)\jidctflt.obj : $(ROOTDIR)\module\jpeg\jidctflt.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jidctflt.c

$(WORKDIR)\jmemmgr.obj : $(ROOTDIR)\module\jpeg\jmemmgr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jmemmgr.c

$(WORKDIR)\jmemnobs.obj : $(ROOTDIR)\module\jpeg\jmemnobs.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jmemnobs.c

$(WORKDIR)\jutils.obj : $(ROOTDIR)\module\jpeg\jutils.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jutils.c

$(WORKDIR)\jquant2.obj : $(ROOTDIR)\module\jpeg\jquant2.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jquant2.c

$(WORKDIR)\jddctmgr.obj : $(ROOTDIR)\module\jpeg\jddctmgr.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jddctmgr.c

$(WORKDIR)\jquant1.obj : $(ROOTDIR)\module\jpeg\jquant1.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jquant1.c

$(WORKDIR)\jdcolor.obj : $(ROOTDIR)\module\jpeg\jdcolor.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\jpeg\jdcolor.c
