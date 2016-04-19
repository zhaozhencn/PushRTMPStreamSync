#pragma once

#include "opencv/cv.h" 
#include "opencv/highgui.h" 
#include <atlbase.h>
#include <dshow.h>

#include <thread>

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "Quartz.lib") 

#ifdef DEBUG
#pragma comment(lib, "opencv_core2410d.lib")
#pragma comment(lib, "opencv_highgui2410d.lib")
#pragma comment(lib, "opencv_imgproc2410d.lib")
#else
#pragma comment(lib, "opencv_core2410.lib")
#pragma comment(lib, "opencv_highgui2410.lib")
#pragma comment(lib, "opencv_imgproc2410.lib")

#endif


#pragma comment(lib, "cscc.lib")


typedef void(*yuv_callback)(IplImage* img, long user_data, int camera_idx);

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( x ) \
	if ( NULL != x ) \
		{ \
	x->Release( ); \
	x = NULL; \
		}
#endif


class capture_video
{
public:
	capture_video();
	virtual ~capture_video();

	int		start(int camera_idx, int rate, yuv_callback cb, long user_data);
	void	stop();

	static int enum_devs();

protected:
	int		svc();

private:
	int						idx_;
	int						interval_;
	yuv_callback			yuv_;
	long					user_data_;
	volatile bool			flag_run_;

private:
	std::thread				thr_;
};