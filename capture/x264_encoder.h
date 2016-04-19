#pragma once
#include <Windows.h>
#include <string>
#include <map>

typedef void(*enc_cb_t)(void* h, const char* out_buf, long out_size, int frame_type, void* user_data, 
	unsigned long long time_stamp, int camera_idx);


class x264_encoder
{
public:
	x264_encoder(void);
public:
	~x264_encoder(void);

public:
	HANDLE open_encoder(int width, int height, int bitrate, int fps, int bitstream_ctl, void* user_data, int camera_idx);
	int encode(HANDLE h, const char* in_buf, long in_size, enc_cb_t cb, unsigned long long time_stamp, bool force_key_frame);
	void close_encoder(HANDLE h);
};
