#ifndef ENC_WRAPPER_H__
#define ENC_WRAPPER_H__

#ifdef ENCODE_WRAPPER_EXPORT_API
#define ENCODE_WRAPPER_API __declspec(dllexport)
#else
#define ENCODE_WRAPPER_API __declspec(dllimport)
#endif

typedef void(*enc_cb_t)(void* h, const char* out_buf, long out_size, int frame_type, void* user_data, unsigned long long time_stamp);

extern "C" ENCODE_WRAPPER_API void* open_encoder(int width, int height, int bitrate, int fps, int bitstream_ctl, void* user_data);

extern "C" ENCODE_WRAPPER_API int encode(void* h, const char* in_buf, long in_size, enc_cb_t cb, unsigned long long time_stamp, bool force_key_frame);

extern "C" ENCODE_WRAPPER_API void close_encoder(void* h);

#endif



