##################################################################################
# define SFOX_ROOT in environment variable, e.g. SFOX_ROOT = "E:\work\sfox"
# define SFOX_INC in environment variable, 
# e.g. SFOX_INC="C:\Program Files\Microsoft Visual Studio\VC98\Include"
##################################################################################

WORKDIR=$(SFOX_ROOT)\make\vc6
OBJDIR=xyssl_objs

CC=cl
LIB=lib /OUT:
CCOPT=/FD /MT /W3 /nologo /c /Zi /TC /Od \
	/I$(SFOX_ROOT) \
	/I$(SFOX_INC)
	
OOPT = /Fo

LIBOBJS = $(WORKDIR)\$(OBJDIR)\aes.obj \
		$(WORKDIR)\$(OBJDIR)\arc4.obj \
		$(WORKDIR)\$(OBJDIR)\base64.obj \
		$(WORKDIR)\$(OBJDIR)\bignum.obj \
		$(WORKDIR)\$(OBJDIR)\des.obj \
		$(WORKDIR)\$(OBJDIR)\dhm.obj \
		$(WORKDIR)\$(OBJDIR)\havege.obj \
		$(WORKDIR)\$(OBJDIR)\md2.obj \
		$(WORKDIR)\$(OBJDIR)\md4.obj \
		$(WORKDIR)\$(OBJDIR)\md5.obj \
		$(WORKDIR)\$(OBJDIR)\rsa.obj \
		$(WORKDIR)\$(OBJDIR)\sha1.obj \
		$(WORKDIR)\$(OBJDIR)\sha2.obj \
		$(WORKDIR)\$(OBJDIR)\ssl_cli.obj \
		$(WORKDIR)\$(OBJDIR)\ssl_tls.obj \
		$(WORKDIR)\$(OBJDIR)\ssl_srv.obj \
		$(WORKDIR)\$(OBJDIR)\x509_read.obj
		
LIBS=xyssl.lib       

default: $(LIBS)

OBJDIR_EXISTS:
	@-mkdir $(WORKDIR)\$(OBJDIR)

$(LIBS): OBJDIR_EXISTS $(LIBOBJS)
	$(LIB)$(WORKDIR)\$(OBJDIR)\$(LIBS) $(LIBOBJS)
	del $(WORKDIR)\$(OBJDIR)\*.obj /F /Q
	
$(WORKDIR)\$(OBJDIR)\aes.obj : $(SFOX_ROOT)\module\xyssl\aes.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\aes.c

$(WORKDIR)\$(OBJDIR)\arc4.obj : $(SFOX_ROOT)\module\xyssl\arc4.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\arc4.c

$(WORKDIR)\$(OBJDIR)\base64.obj : $(SFOX_ROOT)\module\xyssl\base64.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\base64.c
	
$(WORKDIR)\$(OBJDIR)\bignum.obj : $(SFOX_ROOT)\module\xyssl\bignum.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\bignum.c

$(WORKDIR)\$(OBJDIR)\des.obj : $(SFOX_ROOT)\module\xyssl\des.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\des.c

$(WORKDIR)\$(OBJDIR)\dhm.obj : $(SFOX_ROOT)\module\xyssl\dhm.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\dhm.c

$(WORKDIR)\$(OBJDIR)\havege.obj : $(SFOX_ROOT)\module\xyssl\havege.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\havege.c

$(WORKDIR)\$(OBJDIR)\md2.obj : $(SFOX_ROOT)\module\xyssl\md2.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\md2.c
	
$(WORKDIR)\$(OBJDIR)\md4.obj : $(SFOX_ROOT)\module\xyssl\md4.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\md4.c
	
$(WORKDIR)\$(OBJDIR)\md5.obj : $(SFOX_ROOT)\module\xyssl\md5.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\md5.c
	
$(WORKDIR)\$(OBJDIR)\rsa.obj : $(SFOX_ROOT)\module\xyssl\rsa.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\rsa.c
	
$(WORKDIR)\$(OBJDIR)\sha1.obj : $(SFOX_ROOT)\module\xyssl\sha1.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\sha1.c

$(WORKDIR)\$(OBJDIR)\sha2.obj : $(SFOX_ROOT)\module\xyssl\sha2.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\sha2.c

$(WORKDIR)\$(OBJDIR)\ssl_cli.obj : $(SFOX_ROOT)\module\xyssl\ssl_cli.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\ssl_cli.c
	
$(WORKDIR)\$(OBJDIR)\ssl_tls.obj : $(SFOX_ROOT)\module\xyssl\ssl_tls.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\ssl_tls.c

$(WORKDIR)\$(OBJDIR)\ssl_srv.obj : $(SFOX_ROOT)\module\xyssl\ssl_srv.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\ssl_srv.c

$(WORKDIR)\$(OBJDIR)\x509_read.obj : $(SFOX_ROOT)\module\xyssl\x509_read.c
	$(CC) $(CCOPT) $(OOPT)$@ $(SFOX_ROOT)\module\xyssl\x509_read.c

