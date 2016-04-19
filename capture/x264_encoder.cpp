#include "x264_encoder.h"

#include <Windows.h>


typedef void* (*open_encoder_t)(int width, int height, int bitrate, int fps, int bitstream_ctl, void* user_data, int camera_idx);
typedef int(*encode_t)(void* h, const char* in_buf, long in_size, enc_cb_t cb, unsigned long long time_stamp, bool force_key_frame);
typedef void(*close_encoder_t)(void* h);


open_encoder_t open_encoder_fun;
encode_t encode_fun;
close_encoder_t close_encoder_fun;

HMODULE g_hModule = NULL;

x264_encoder::x264_encoder(void)
{
	if (NULL == g_hModule)
	{
		g_hModule = ::LoadLibraryEx("libX264EncoderWrapper.so", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		open_encoder_fun = (open_encoder_t)::GetProcAddress(g_hModule, "open_encoder");
		encode_fun = (encode_t)::GetProcAddress(g_hModule, "encode");
		close_encoder_fun = (close_encoder_t)::GetProcAddress(g_hModule, "close_encoder");
		printf("\n LoadLibrary libx264_encoderWrapper.dll ret: [ %X ] \n", g_hModule);
	}
}


x264_encoder::~x264_encoder(void)
{
}


HANDLE x264_encoder::open_encoder(int width, int height, int bitrate, int fps, int bitstream_ctl, void* user_data, int camera_idx)
{
	return open_encoder_fun(width, height, bitrate, fps, bitstream_ctl, user_data, camera_idx);
}


int x264_encoder::encode(HANDLE h, const char* in_buf, long in_size, enc_cb_t cb, unsigned long long time_stamp, bool force_key_frame)
{
	return encode_fun(h, in_buf, in_size, cb, time_stamp, force_key_frame);
}


void x264_encoder::close_encoder(HANDLE h)
{
	close_encoder_fun(h);
}

