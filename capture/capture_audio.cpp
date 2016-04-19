#include "capture_audio.h"



void capture_audio::handle(unsigned char* data, unsigned int len)
{
	if (this->cb_)
		this->cb_(data, len, this->user_data_, this->audio_idx_);
}



