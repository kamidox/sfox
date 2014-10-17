sfox
====

Network applications implement by C. Including web browser, mms client, email client, dcd client, sns client etc.

## What's sfox

sfox come from smart fox. It is the name of the company I tried to setup.

This repository contain a web browser and mms client which full support [OMA][1] 1.2 spec. Also, it contain a Email client support pop3/smtp and some funny tools like DCD which defined by CMCC.

## What's sfox really

1. It's implement by C
2. It's portable, it's successfully running on Windows, [Spreadtrum][2] feature phone platform like 6600/6800/6531 etc, [MTK][3] feature phone platform like 6235/6250 etc.
3. It's very old. It tends to run on feature phone. Most of the code is writing in 2006-2007 when smart phone is not that popular, iOS/Android not born yet.

So, you could see a geek in 2007 who use feature phone develop by the company he works, in additional, it run some cool internet apps develop by himself, it can browser the web, retrieve emails, read news by DCD, get stock infomation by Stock apps ... Yes, it's me :)

## What's in sfox really!

1. An adapter layer which in src/inte, all the platform dependence code is implement here
2. A mini gui system and graphic system which in src/gui
3. An utilities code reside in src/util, which including sax xml parser, memory manager, list component, image decoder interface etc
4. Web core code reside in src/web, which including wtp/wsp protocol implement, http protocol implement, html/wml/wbxml decoder, service indicator/service loading, browser history etc.
5. Mms core code reside in src/mms, which contains mms document decoder and render
6. Email core code reside in src/eml, which contains rfc822/rfc2822 implement and smtp/pop3 implement
7. Dcd core code reside in src/dcd, which including dcd implement defined by CMCC
8. Other funning tools like src/sns which can read weibo/twitter information from third-party and src/stk can get stock information from sina

## Any open source project used in sfox?

sfox use following open source project:

1. [giflib][4]
2. [libjpeg][5]
3. [libpng][6]
4. [xyssl][7], which rename to [polarssl][8] now
5. [zlib][9]

## How to build

1. Install Visual C++ 6.0 in windows. Yes, as state above, it's very old. If you cannot find one, mail me.
2. Open `proj/F_Soft.dsw` in VC6 and build the project

It also support build arm architecture librarys, please refer to `make` directory. Please note that it use `nmake` which come from VC6. It's easy to porting to GNU make system.

## License

Just use it if it's useful for you. I used to pround of myself to write this code. I used to excited to work to midnight to work on this project. It's all gone now. Enjoy geeking ...

[1]: http://technical.openmobilealliance.org/Technical/technical-information/specifications-for-public-comment-archive
[2]: www.spreadtrum.com.cn
[3]: http://www.mediatek.com
[4]: http://sourceforge.net/projects/giflib/
[5]: http://sourceforge.net/projects/libjpeg/
[6]: http://www.libpng.org/pub/png/libpng.html
[7]: http://xyssl.sourcearchive.com/documentation/0.9/files.html
[8]: https://polarssl.org/
[9]: http://www.zlib.net/

