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
	$(LIB) $(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	
$(WORKDIR)\png.obj : $(ROOTDIR)\module\png\png.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\png.c

$(WORKDIR)\pngerror.obj : $(ROOTDIR)\module\png\pngerror.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngerror.c

$(WORKDIR)\pngget.obj : $(ROOTDIR)\module\png\pngget.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngget.c

$(WORKDIR)\pngmem.obj : $(ROOTDIR)\module\png\pngmem.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngmem.c

$(WORKDIR)\pngpread.obj : $(ROOTDIR)\module\png\pngpread.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngpread.c

$(WORKDIR)\pngread.obj : $(ROOTDIR)\module\png\pngread.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngread.c

$(WORKDIR)\pngrio.obj : $(ROOTDIR)\module\png\pngrio.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngrio.c

$(WORKDIR)\pngrtran.obj : $(ROOTDIR)\module\png\pngrtran.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngrtran.c
	
$(WORKDIR)\pngrutil.obj : $(ROOTDIR)\module\png\pngrutil.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngrutil.c
	
$(WORKDIR)\pngset.obj : $(ROOTDIR)\module\png\pngset.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngset.c
	
$(WORKDIR)\pngtrans.obj : $(ROOTDIR)\module\png\pngtrans.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngtrans.c

$(WORKDIR)\pngvcrd.obj : $(ROOTDIR)\module\png\pngvcrd.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngvcrd.c

$(WORKDIR)\pngwio.obj : $(ROOTDIR)\module\png\pngwio.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngwio.c
	
$(WORKDIR)\pngwrite.obj : $(ROOTDIR)\module\png\pngwrite.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngwrite.c
	
$(WORKDIR)\pngwtran.obj : $(ROOTDIR)\module\png\pngwtran.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngwtran.c

$(WORKDIR)\pngwutil.obj : $(ROOTDIR)\module\png\pngwutil.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\png\pngwutil.c
