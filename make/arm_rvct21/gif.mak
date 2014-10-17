ROOTDIR=E:\F_Soft
SYSINCDIR=E:\F_Soft\tools\arm_rvct20\include
TOOLDIR=E:\F_Soft\tools\arm_rvct21\bin
WORKDIR=$(ROOTDIR)\make\arm_rvct21
CC=$(TOOLDIR)\tcc
LIB=$(TOOLDIR)\armar -create
CCOPT=--apcs=/interwork/noswst \
	--diag_suppress 177,186 \
	--enum_is_int \
	--littleend \
	-O3 \
	-Otime \
	-c \
	-ec \
	-g- \
	-I$(ROOTDIR) \
	-I$(SYSINCDIR) \

LIBOBJS = 	$(WORKDIR)\dgif_lib.obj  \
			$(WORKDIR)\gif_err.obj  \
			$(WORKDIR)\gifalloc.obj
	
LIBS=gif.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB) $(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	    
$(WORKDIR)\dgif_lib.obj : $(ROOTDIR)\module\gif\dgif_lib.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\gif\dgif_lib.c

$(WORKDIR)\gif_err.obj : $(ROOTDIR)\module\gif\gif_err.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\gif\gif_err.c

$(WORKDIR)\gifalloc.obj : $(ROOTDIR)\module\gif\gifalloc.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\gif\gifalloc.c
