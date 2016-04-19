#pragma once

#include <string>
#include <map>
#include <faac.h>
#include "util.h"

typedef void (*audio_enc_cb_t)(void* h, const char* out_buf, long out_size, void* user_data, bool is_header, int audio_idx);

class aac_encoder
{
public:
	aac_encoder();
	~aac_encoder();

public:
	void* open_audio_encoder(int samples_per_sec, int channel, void* user_data, audio_enc_cb_t cb, int audio_idx);
	void  audio_encode(void* h, const char* in_buf, long in_size, audio_enc_cb_t cb);
	void  audio_close_encoder(void* h);

public:
	void*			handler_;
	unsigned long	input_samples_;
	unsigned long	max_output_bytes_;
	void*			user_data_;
	std::string		buf_;
	int				audio_idx_;
};




