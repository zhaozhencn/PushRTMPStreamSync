#include "business.h"
#include <fstream>
#include "opencv/cv.h" 
#include "opencv/highgui.h" 
#include "util.h"


business::business()
{
}


business::~business()
{
}




#define SAMPLES_PER_SECOND  44100
#define	CHANNEL				2 
#define BITS_PER_SAMPLE		16
#define VIDEO_FPS			15
#define CAMERA_IDX			0
#define AUDIO_IDX			0

void business::start()
{
	this->h_ = INVALID_HANDLE_VALUE;
	this->h_audio_ = INVALID_HANDLE_VALUE;
	this->force_make_key_frame_ = true;

	int idx = capture_video::enum_devs();

	capture_audio* p_audio = new capture_audio();
	p_audio->start(SAMPLES_PER_SECOND, CHANNEL, BITS_PER_SAMPLE, business::cb_audio, (long)this, AUDIO_IDX);

	capture_video* p = new capture_video();
	p->start(CAMERA_IDX, VIDEO_FPS, business::cb, (long)this);

	printf("Sleep 2 secs..........................\n");
	Sleep(2000);

	this->force_make_key_frame_ = true;
	std::string url = "rtmp://192.168.1.159/live/test";
	this->push_.start(url, CAMERA_IDX, 0);

	printf("Running 350 secs for exiting..........................\n");
	Sleep(350000);

	p_audio->stop();
	p->stop();
	this->push_.stop(url);
}


void business::cb(IplImage* img, long user_data, int camera_idx)
{
	business* p = (business*)user_data;
	p->cb_impl(img, camera_idx);
}


#define CURR_FPS 15

void business::cb_impl(IplImage* img, int camera_idx)
{
#define CQP	0
#define CRF 1
#define ABR	2

	if (INVALID_HANDLE_VALUE == this->h_)
	{
		this->h_ = this->x264_enc_.open_encoder(
			img->width,
			img->height,
			500,
			CURR_FPS,
			CRF,
			(void*)&this->push_, camera_idx);
	}

	int yuv_size = img->width * img->height * 3 / 2;
	static BYTE* yuv_buf = NULL;
	if (NULL == yuv_buf)
		yuv_buf = new BYTE[yuv_size];

	conv_.RGB24_to_YV12((BYTE*)img->imageData, yuv_buf, img->width, img->height);


	{
		int tmp_size = img->width * img->height;
		char* tmp_buf = new char[tmp_size / 4];
		memcpy(tmp_buf, yuv_buf + tmp_size, tmp_size / 4);
		memcpy(yuv_buf + tmp_size, yuv_buf + tmp_size + tmp_size / 4, tmp_size / 4);
		memcpy(yuv_buf + tmp_size + tmp_size / 4, tmp_buf, tmp_size / 4);
		delete[] tmp_buf;
	}


	if (NULL == video_effect_.get())
		this->video_effect_ = video_effect_ptr(new video_effect(img->width, img->height, CURR_FPS));

	std::string out_frame;
	if (0 == this->video_effect_->handle(yuv_buf, yuv_size, out_frame))
	{
		unsigned long long t = get_qpc_time_ms::exec();
		this->x264_enc_.encode(this->h_, (const char*)out_frame.c_str(), out_frame.length(),
			push_rtmp_stream_man::video_cb, t, this->force_make_key_frame_);

		if (this->force_make_key_frame_)
			this->force_make_key_frame_ = false;
	}
}


void business::cb_audio(unsigned char* raw_data, unsigned long len, long user_data, int audio_idx)
{
	business* p = (business*)user_data;
	p->cb_audio_impl(raw_data, len, audio_idx);
}


void business::cb_audio_impl(unsigned char* raw_data, unsigned long len, int audio_idx)
{
	if (INVALID_HANDLE_VALUE == this->h_audio_)
		this->h_audio_ = aac_enc_.open_audio_encoder(SAMPLES_PER_SECOND, CHANNEL, (void*)&this->push_, 
		push_rtmp_stream_man::audio_cb, audio_idx);

	aac_enc_.audio_encode(this->h_audio_, (const char*)raw_data, len, push_rtmp_stream_man::audio_cb);
}




