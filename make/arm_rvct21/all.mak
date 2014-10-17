default: smartfox.lib

smartfox.lib: sfox.lib xyssl.lib jpeg.lib gif.lib zlib.lib png.lib
	@armar -create sfox_arm_rvct21.lib sfox.lib xyssl.lib jpeg.lib gif.lib zlib.lib png.lib
	
sfox.lib:
	@nmake /f sfox.mak

xyssl.lib:
	@nmake /f xyssl.mak
	
jpeg.lib:
	@nmake /f jpeg.mak

gif.lib:
	@nmake /f gif.mak

zlib.lib:
	@nmake /f zlib.mak
	
png.lib:
	@nmake /f png.mak
