#include "aac_encoder.h"


aac_encoder::aac_encoder()
	: handler_(NULL)
	, input_samples_(0)
	, max_output_bytes_(0)
{
}


aac_encoder::~aac_encoder()
{
}


void* aac_encoder::open_audio_encoder(int samples_per_sec, int channel, void* user_data, audio_enc_cb_t cb)
{
	this->handler_ = faacEncOpen(samples_per_sec, channel, &this->input_samples_, &this->max_output_bytes_);
	this->buf_.assign(this->max_output_bytes_, 0);
	this->user_data_ = user_data;

	faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(handler_);
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->bitRate = (32 * 1000) / 2;
	pConfiguration->quantqual = 100;
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->mpegVersion = MPEG4;
	pConfiguration->aacObjectType = LOW;
	pConfiguration->useLfe = 0;
	pConfiguration->outputFormat = 0;

	int nRet = faacEncSetConfiguration(handler_, pConfiguration);

	unsigned char* buf = NULL;
	unsigned long pSizeOfDecoderSpecificInfo = 0;
	faacEncGetDecoderSpecificInfo(handler_, &buf, &pSizeOfDecoderSpecificInfo);

	if (cb)
		cb((void*)handler_, (const char*)buf, pSizeOfDecoderSpecificInfo, this->user_data_, true);

	free(buf);
	return handler_;
}


void aac_encoder::audio_encode(void* h, const char* in_buf, long in_size, audio_enc_cb_t cb)
{
	if (in_size >= this->input_samples_* 2)
	{
		char* p = (char*)this->buf_.c_str();
		int len = faacEncEncode(handler_, (int32_t*)in_buf, this->input_samples_, (unsigned char*)p, this->max_output_bytes_);
		if (len > 0)
			cb((void*)h, p, len, this->user_data_, false);
	}
}


void aac_encoder::audio_close_encoder(void* h)
{
	faacEncClose(handler_);
}






