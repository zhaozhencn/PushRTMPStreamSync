#pragma once

#include "push_rtmp_stream.h"
#include "push_rtmp_stream_man.h"
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
	void start(const std::string& rtmp_url, int camera_idx, int audio_idx);

public:
	static void cb(IplImage* img, long user_data, int camera_idx);
	static void cb_audio(unsigned char* raw_data, unsigned long len, long user_data, int audio_idx);

public:
	static unsigned int __stdcall video_thread_proc(void* ctx);

protected:
	void video_thread_entry();

protected:
	void cb_impl(IplImage* img, int camera_idx);
	void cb_audio_impl(unsigned char* raw_data, unsigned long len, int audio_idx);

protected:
	HANDLE					h_;
	x264_encoder			x264_enc_;
	HANDLE					h_audio_;
	aac_encoder				aac_enc_;
	ColorSpaceConversions	conv_;
	bool					force_make_key_frame_;
	push_rtmp_stream_man	push_;
	typedef std::shared_ptr<video_effect> video_effect_ptr;
	video_effect_ptr		video_effect_;

};

