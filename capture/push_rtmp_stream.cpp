#include "push_rtmp_stream.h"
#include "util.h"
#include "push_rtmp_stream_man.h"
#include "comm.h"

#define STREAM_CHANNEL_METADATA  0x03  
#define STREAM_CHANNEL_VIDEO     0x04  
#define STREAM_CHANNEL_AUDIO     0x05  


#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"librtmp.lib")
#endif



push_rtmp_stream::push_rtmp_stream(push_rtmp_stream_man* man)
	: rtmp_(NULL)
	, packet_(NULL)
	, once_(true)
	, curr_step_(0)
	, man_(man)
{
}


push_rtmp_stream::~push_rtmp_stream()
{
	try
	{
		fini();
	}
	catch (...)
	{

	}
}


int push_rtmp_stream::init(const std::string& url, int camera_idx, int audio_idx)
{
	this->url_ = url;
	this->camera_idx_ = camera_idx;
	this->audio_idx_ = audio_idx;

	RTMP_LogLevel lvl = RTMP_LOGINFO;
	RTMP_LogSetLevel(lvl);
	rtmp_ = RTMP_Alloc();
	RTMP_Init(rtmp_);
	rtmp_->Link.timeout = 5;

	RTMP_SetupURL(rtmp_, (char*)url.c_str());
	RTMP_EnableWrite(rtmp_);

	if (!RTMP_Connect(rtmp_, NULL))
		return -1;

	if (!RTMP_ConnectStream(rtmp_, 0))
		return -1;

	this->packet_ = (RTMPPacket*)malloc(sizeof(RTMPPacket));
	memset(packet_, 0, sizeof(RTMPPacket));
	RTMPPacket_Alloc(packet_, 1024 * 640);
	RTMPPacket_Reset(packet_);
	packet_->m_hasAbsTimestamp = TRUE;
	packet_->m_nChannel = 0x04;
	packet_->m_nInfoField2 = this->rtmp_->m_stream_id;

	this->sync_stream_thread_ = std::shared_ptr<std::thread>(
		new std::thread(&push_rtmp_stream::sync_stream_thread_entry, this));
	
	return 0;
}


void push_rtmp_stream::fini()
{
	this->curr_step_ = 0;			// notify thread exit
	this->cv_to_send_.notify_all();
	this->cv_video_.notify_all();

	if (this->sync_stream_thread_.get())
	{
		this->sync_stream_thread_->join();
		this->sync_stream_thread_.reset();
	}

	if (this->send_thread_.get())
	{
		this->send_thread_->join();
		this->send_thread_.reset();
	}

	this->to_send_list_.clear();
	this->video_data_.clear();
	this->audio_data_.clear();

	if (this->rtmp_ != NULL){
		RTMP_Close(this->rtmp_);
		RTMP_Free(this->rtmp_);
		this->rtmp_ = NULL;
	}

	if (packet_ != NULL){
		RTMPPacket_Free(packet_);
		free(packet_);
		packet_ = NULL;
	}
}


int push_rtmp_stream::send_video_sps_pps()
{
	unsigned char *body = (unsigned char *)this->packet_->m_body;
	int i = 0;
	int sps_len = this->sps_.length();
	int pps_len = this->pps_.length();

	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = this->sps_[1];
	body[i++] = this->sps_[2];
	body[i++] = this->sps_[3];
	body[i++] = 0xff;

	/*this->sps_*/
	body[i++] = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], this->sps_.c_str(), sps_len);
	i += sps_len;

	/*pps*/
	body[i++] = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len)& 0xff;
	memcpy(&body[i], this->pps_.c_str(), pps_len);
	i += pps_len;

	this->packet_->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	this->packet_->m_nBodySize = i;
	this->packet_->m_nChannel = 0x04;
	this->packet_->m_nTimeStamp = 0;
	this->packet_->m_hasAbsTimestamp = 0;
	this->packet_->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	this->packet_->m_nInfoField2 = this->rtmp_->m_stream_id;

	return RTMP_SendPacket(this->rtmp_, this->packet_, TRUE) ? 0 : -1;
}


int push_rtmp_stream::send_rtmp_video(unsigned char* buf, int len, unsigned long long time_stamp)
{
	unsigned char *body = (unsigned char *)this->packet_->m_body;

	if (buf[2] == 0x00) { /*00 00 00 01*/
		buf += 4;
		len -= 4;
	}
	else if (buf[2] == 0x01){ /*00 00 01*/
		buf += 3;
		len -= 3;
	}

	/*send video packet*/
	this->packet_->m_nBodySize = len + 9;
	
	/*key frame*/
	body[0] = 0x27;
	int type = buf[0] & 0x1f;
	if (type == NAL_SLICE_IDR) {
		body[0] = 0x17;
	}

	body[1] = 0x01;   /*nal unit*/
	body[2] = 0x00;
	body[3] = 0x00;
	body[4] = 0x00;

	body[5] = (len >> 24) & 0xff;
	body[6] = (len >> 16) & 0xff;
	body[7] = (len >> 8) & 0xff;
	body[8] = (len)& 0xff;

	/*copy data*/
	memcpy(body + 9, buf, len);

	this->packet_->m_hasAbsTimestamp = TRUE;
	this->packet_->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	this->packet_->m_nInfoField2 = this->rtmp_->m_stream_id;
	this->packet_->m_nChannel = 0x04;
	this->packet_->m_headerType = RTMP_PACKET_SIZE_LARGE;
	this->packet_->m_nTimeStamp = time_stamp;

	return RTMP_SendPacket(this->rtmp_, this->packet_, TRUE) ? 0 : -1;
}


void push_rtmp_stream::video_cb(void* h, const char* out_buf, long out_size, int frame_type, unsigned long long time_stamp)
{
	// may recetive this->sps_ and pps one more times ???
	if (once_)
	{
		if (frame_type == NAL_SPS)
		{
			this->sps_.assign(out_buf + 4, out_size - 4);
		}
		else if (frame_type == NAL_PPS)
		{
			this->pps_.assign(out_buf + 4, out_size - 4);
			once_ = false;
			++this->curr_step_;
		}
	}
	else if (NAL_SLICE_IDR == frame_type || NAL_SLICE == frame_type)
	{
		if (this->curr_step_ < 3)
			return;

		std::unique_lock<std::mutex> g(this->cs_video_);
		this->video_data_.push_back(std::make_tuple(time_stamp, frame_type, std::string(out_buf, out_size)));
		this->cv_video_.notify_one();
	}
}


void push_rtmp_stream::audio_cb(void* h, const char* out_buf, long out_size, bool is_header)
{
	if (is_header)
	{
		this->audio_header_ = std::string(out_buf, out_buf + out_size);
		++this->curr_step_;
	}
	else
	{
		if (this->curr_step_ < 3)
			return;

		std::unique_lock<std::mutex> g(this->cs_audio_);
		unsigned long long tick = get_qpc_time_ms::exec();
		this->audio_data_.push_back(std::make_pair(tick, std::string(out_buf, out_buf + out_size)));
	}
}


int push_rtmp_stream::send_rtmp_audio_header()
{
	unsigned char* body = (unsigned char *)this->packet_->m_body;
	this->packet_->m_nBodySize = audio_header_.length() + 2;

	/*send audio this->packet_*/
	memset(body, 0, audio_header_.length() + 2);

	/*AF 00 + AAC RAW data*/
	body[0] = 0xAF;
	body[1] = 0x00;
	
	/*copy data*/
	memcpy(&body[2], this->audio_header_.c_str(), this->audio_header_.length());

	this->packet_->m_hasAbsTimestamp = TRUE;
	this->packet_->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	this->packet_->m_nInfoField2 = this->rtmp_->m_stream_id;
	this->packet_->m_nChannel = STREAM_CHANNEL_AUDIO;
	this->packet_->m_headerType = RTMP_PACKET_SIZE_LARGE;
	this->packet_->m_nTimeStamp = 0; 

	return RTMP_SendPacket(this->rtmp_, this->packet_, FALSE) ? 0 : -1;
}


int push_rtmp_stream::send_rtmp_audio(unsigned char* buf, int len, unsigned long long time_stamp)
{
	unsigned char* body = (unsigned char *)this->packet_->m_body;
	this->packet_->m_nBodySize = len + 2;

	/*send audio this->packet_*/
	memset(body, 0, len + 2);
	
	/*AF 01 + AAC RAW data*/
	body[0] = 0xAF;
	body[1] = 0x01;

	/*copy data*/
	memcpy(body + 2, buf, len);

	this->packet_->m_hasAbsTimestamp = TRUE;
	this->packet_->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	this->packet_->m_nInfoField2 = this->rtmp_->m_stream_id;
	this->packet_->m_nChannel = STREAM_CHANNEL_AUDIO;
	this->packet_->m_headerType = RTMP_PACKET_SIZE_LARGE;
	this->packet_->m_nTimeStamp = time_stamp;

	return RTMP_SendPacket(this->rtmp_, this->packet_, TRUE) ? 0 : -1;
}


void push_rtmp_stream::sync_stream_thread_entry()
{
	// wait for received video sps (pps) and audio header
	int max_waiting_time_threshold = 0;
	while (this->curr_step_ < 2)
	{
		if (++max_waiting_time_threshold > MAX_WAIT_TIMES_THRESHOLD)
			return;

		Sleep(50);
	}

		
	++this->curr_step_;		// increase curr_step_ to 3 for start push audio and video frame into buff

	// send sps(pps) and audio header first
	if (-1 == this->send_video_sps_pps() || -1 == send_rtmp_audio_header())
	{
		if (this->man_)
			this->man_->notify_disconnect(this->url_, this->camera_idx_, this->audio_idx_);

		return;
	}

	// start send thread when it already.
	this->send_thread_ = std::shared_ptr<std::thread>(
		new std::thread(&push_rtmp_stream::send_thread_entry, this));

	unsigned long long first_video_time_stamp = -1;
	unsigned long long curr_video_time_stamp = 0;
	
	for (;;)
	{
		std::unique_lock<std::mutex> g(this->cs_video_);
		this->cv_video_.wait(g, [&](){
			return this->video_data_.size() > 1 || 3 != this->curr_step_;
		});

		if (3 != this->curr_step_)
			break;

		video_data_list::const_reference d = this->video_data_.front();

		if (-1 == first_video_time_stamp)
		{
			int frame_type = std::get<1>(d);

			// make sure the first video is I frame
			if (frame_type != NAL_SLICE_IDR)
			{
				this->video_data_.pop_front();
				continue;
			}

			first_video_time_stamp = std::get<0>(d);
		}
		
		curr_video_time_stamp = std::get<0>(d) - first_video_time_stamp;

		// handle relation audio data
		{
			std::unique_lock<std::mutex> g(this->cs_audio_);
			this->audio_data_.remove_if([&](data_list::const_reference ref) -> bool
			{
				// if audio timestamp is earlier than first_video_time_stamp, drop it immediately
				if (ref.first < first_video_time_stamp)
					return true;

				// if audio timestamp is later than current video frame, handler it later.
				unsigned long long t = ref.first - first_video_time_stamp;
				if (t > curr_video_time_stamp)
					return false;

				// push the correct audio frame into send_list
				this->add_audio_to_send_list(t, ref.second);
				return true;
			});
		}

		// handle current video frame 
		add_video_to_send_list(curr_video_time_stamp, std::get<2>(d));
		this->video_data_.pop_front();
	}

}


void push_rtmp_stream::send_thread_entry()
{
	bool need_send_first_video = true;
	int ret = 0;

	for (;;)
	{
		std::unique_lock<std::mutex> g(this->cs_to_send_);
		this->cv_to_send_.wait(g, [&](){
			return this->to_send_list_.size() > 10 || 3 != this->curr_step_;
		});

		// notify exit thread
		if (3 != this->curr_step_)
			break;

		mix_data_list::const_reference ref = this->to_send_list_.front();
		auto time_stamp = std::get<0>(ref);
		auto type = std::get<1>(ref);
		auto data = std::get<2>(ref);

		// make sure send video frame first time
		if (VIDEO == type && need_send_first_video)
			need_send_first_video = false;

		if (!need_send_first_video)
		{
			if (AUDIO == type)
				ret = this->send_rtmp_audio((unsigned char*)data.c_str(), data.length(), time_stamp);
			else
				ret = this->send_rtmp_video((unsigned char*)data.c_str(), data.length(), time_stamp);
			
			if (-1 == ret && this->man_)
			{
				this->man_->notify_disconnect(this->url_, this->camera_idx_, this->audio_idx_);
				break;
			}
		}

		to_send_list_.pop_front();
	}
}


void push_rtmp_stream::add_audio_to_send_list(unsigned long long time_stamp, const std::string& data)
{
	std::unique_lock<std::mutex> g(this->cs_to_send_);
	to_send_list_.push_back(std::make_tuple(time_stamp, AUDIO, data));
	this->cv_to_send_.notify_one();
}


void push_rtmp_stream::add_video_to_send_list(unsigned long long time_stamp, const std::string& data)
{
	std::unique_lock<std::mutex> g(this->cs_to_send_);
	to_send_list_.push_back(std::make_tuple(time_stamp, VIDEO, data));
	this->cv_to_send_.notify_one();
}



