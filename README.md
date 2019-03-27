# PushRTMPStreamSync
push audio and video stream on rtmp protocol synchronously

usage step:

1 building x264 using mingw

 cd libs\x264
 ./configure && make
 
2 building x264_wrapper using mingw

 cd libs\x264_wrapper 
 make -f Makefile.encode_wrapper
 
2 building capture.sln using Visual Studio 2015 (select Release Mode)




