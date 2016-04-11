#pragma once

#include "push_rtmp_stream.h"
#include "convert.h"
#include "x264_encoder.h"
#include "aac_encoder.h"
#include "capture_video.h"
#include "capture_audio.h"
#include "video_effect.h"

class business
{
public:
	business();
	~business();

public:
	void start();

public:
	static void cb(IplImage* img, long user_data);
	static void cb_audio(unsigned char* raw_data, unsigned long len, long user_data);

public:
	static unsigned int __stdcall video_thread_proc(void* ctx);

protected:
	void video_thread_entry();

protected:
	void cb_impl(IplImage* img);
	void cb_audio_impl(unsigned char* raw_data, unsigned long len);

protected:
	HANDLE					h_;
	x264_encoder			x264_enc_;
	HANDLE					h_audio_;
	aac_encoder				aac_enc_;
	ColorSpaceConversions	conv_;
	bool					force_make_key_frame_;
	push_rtmp_stream		push_;
	typedef std::shared_ptr<video_effect> video_effect_ptr;
	video_effect_ptr		video_effect_;

};

