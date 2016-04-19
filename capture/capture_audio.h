#pragma once

#include "collect_sound.h"
#include "sound_handler.h"



typedef void(*raw_audio_callback)(unsigned char* raw_data, unsigned long len, long user_data, int audio_idx);


class capture_audio : public sound_handler
{
public:
	void start(int samples_per_sec, int channel, int bits_per_sample, raw_audio_callback cb, long user_data, int audio_idx)
	{
		this->cb_ = cb;
		this->user_data_ = user_data;
		this->audio_idx_ = audio_idx;
		this->collector_ = new collect_sound;

		WAVEFORMATEX wf;
		wf.cbSize = 0;
		wf.nBlockAlign = channel * bits_per_sample / 8;
		wf.nChannels = channel;
		wf.nSamplesPerSec = samples_per_sec;
		wf.wBitsPerSample = bits_per_sample;
		wf.nAvgBytesPerSec = samples_per_sec * wf.nBlockAlign;
		wf.wFormatTag = WAVE_FORMAT_PCM;


		if (!this->collector_->is_running())
			this->collector_->start(&GUID_NULL, &wf, this);
	}

	~capture_audio()
	{
		stop();
	}

	void stop()
	{
		if (this->collector_->is_running())
			this->collector_->stop();
	}

	void handle(unsigned char* data, unsigned int len);


private:
	collect_sound*			collector_;
	raw_audio_callback		cb_;
	long					user_data_;
	int						audio_idx_;
};


