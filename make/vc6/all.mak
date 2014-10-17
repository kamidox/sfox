default: smartfox.lib

smartfox.lib: sfox.lib xyssl.lib jpeg.lib gif.lib zlib.lib png.lib
	@lib /OUT:sfox_vc6.lib sfox_objs/sfox.lib xyssl_objs/xyssl.lib jpeg_objs/jpeg.lib gif_objs/gif.lib zlib_objs/zlib.lib png_objs/png.lib
	
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
