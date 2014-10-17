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

LIBOBJS = $(WORKDIR)\aes.obj \
		$(WORKDIR)\arc4.obj \
		$(WORKDIR)\base64.obj \
		$(WORKDIR)\bignum.obj \
		$(WORKDIR)\des.obj \
		$(WORKDIR)\dhm.obj \
		$(WORKDIR)\havege.obj \
		$(WORKDIR)\md2.obj \
		$(WORKDIR)\md4.obj \
		$(WORKDIR)\md5.obj \
		$(WORKDIR)\rsa.obj \
		$(WORKDIR)\sha1.obj \
		$(WORKDIR)\sha2.obj \
		$(WORKDIR)\ssl_cli.obj \
		$(WORKDIR)\ssl_tls.obj \
		$(WORKDIR)\ssl_srv.obj \
		$(WORKDIR)\x509_read.obj
		
LIBS=xyssl.lib       

default: $(LIBS)

$(LIBS): $(LIBOBJS)
	$(LIB) $(WORKDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\*.obj /F /Q
	
$(WORKDIR)\aes.obj : $(ROOTDIR)\module\xyssl\aes.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\aes.c

$(WORKDIR)\arc4.obj : $(ROOTDIR)\module\xyssl\arc4.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\arc4.c

$(WORKDIR)\base64.obj : $(ROOTDIR)\module\xyssl\base64.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\base64.c
	
$(WORKDIR)\bignum.obj : $(ROOTDIR)\module\xyssl\bignum.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\bignum.c

$(WORKDIR)\des.obj : $(ROOTDIR)\module\xyssl\des.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\des.c

$(WORKDIR)\dhm.obj : $(ROOTDIR)\module\xyssl\dhm.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\dhm.c

$(WORKDIR)\havege.obj : $(ROOTDIR)\module\xyssl\havege.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\havege.c

$(WORKDIR)\md2.obj : $(ROOTDIR)\module\xyssl\md2.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\md2.c
	
$(WORKDIR)\md4.obj : $(ROOTDIR)\module\xyssl\md4.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\md4.c
	
$(WORKDIR)\md5.obj : $(ROOTDIR)\module\xyssl\md5.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\md5.c
	
$(WORKDIR)\rsa.obj : $(ROOTDIR)\module\xyssl\rsa.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\rsa.c
	
$(WORKDIR)\sha1.obj : $(ROOTDIR)\module\xyssl\sha1.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\sha1.c

$(WORKDIR)\sha2.obj : $(ROOTDIR)\module\xyssl\sha2.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\sha2.c

$(WORKDIR)\ssl_cli.obj : $(ROOTDIR)\module\xyssl\ssl_cli.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\ssl_cli.c
	
$(WORKDIR)\ssl_tls.obj : $(ROOTDIR)\module\xyssl\ssl_tls.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\ssl_tls.c

$(WORKDIR)\ssl_srv.obj : $(ROOTDIR)\module\xyssl\ssl_srv.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\ssl_srv.c

$(WORKDIR)\x509_read.obj : $(ROOTDIR)\module\xyssl\x509_read.c
	$(CC) $(CCOPT) -o$@ $(ROOTDIR)\module\xyssl\x509_read.c

