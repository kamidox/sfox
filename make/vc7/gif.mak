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

LIBOBJS = 	$(WORKDIR)\dgif_lib.obj  \
			$(WORKDIR)\gif_err.obj  \
			$(WORKDIR)\gifalloc.obj

LIBS=gif.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	    
$(WORKDIR)\dgif_lib.obj : $(ROOTDIR)\module\gif\dgif_lib.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\gif\dgif_lib.c

$(WORKDIR)\gif_err.obj : $(ROOTDIR)\module\gif\gif_err.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\gif\gif_err.c

$(WORKDIR)\gifalloc.obj : $(ROOTDIR)\module\gif\gifalloc.c
	$(CC) $(CCOPT) $(OOPT)$@ $(ROOTDIR)\module\gif\gifalloc.c
