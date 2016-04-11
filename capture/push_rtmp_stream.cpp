#include "push_rtmp_stream.h"
#include "util.h"

#define STREAM_CHANNEL_METADATA  0x03  
#define STREAM_CHANNEL_VIDEO     0x04  
#define STREAM_CHANNEL_AUDIO     0x05  


#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"librtmp.lib")
#endif



push_rtmp_stream::push_rtmp_stream()
	: rtmp_(NULL)
	, packet_(NULL)
	, curr_step_(0)
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


int push_rtmp_stream::init(const std::string& url)
{
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

	this->send_thread_ = std::shared_ptr<std::thread>(
		new std::thread(&push_rtmp_stream::send_thread_entry, this));
	
	return 0;
}


void push_rtmp_stream::fini()
{
	this->curr_step_ = 0;		// notify thread exit

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


enum nal_unit_type_e
{
	NAL_UNKNOWN = 0,
	NAL_SLICE = 1,
	NAL_SLICE_DPA = 2,
	NAL_SLICE_DPB = 3,
	NAL_SLICE_DPC = 4,
	NAL_SLICE_IDR = 5,    /* ref_idc != 0 */
	NAL_SEI = 6,    /* ref_idc == 0 */
	NAL_SPS = 7,
	NAL_PPS = 8,
	NAL_AUD = 9,
	NAL_FILLER = 12,
	/* ref_idc == 0 for 6,9,10,11,12 */
};


void push_rtmp_stream::execute(void* h, const char* out_buf, long out_size, int frame_type, void* user_data, unsigned long long time_stamp)
{
	push_rtmp_stream* p = (push_rtmp_stream*)user_data;
	p->execute_impl(h, out_buf, out_size, frame_type, time_stamp);
}


void push_rtmp_stream::send_video_sps_pps()
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

	RTMP_SendPacket(this->rtmp_, this->packet_, TRUE);
}


void push_rtmp_stream::send_rtmp_video(unsigned char * buf, int len, unsigned long long time_stamp)
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

	RTMP_SendPacket(this->rtmp_, this->packet_, TRUE);
}


int push_rtmp_stream::execute_audio(void* h, const char* out_buf, long out_size, void* user_data, bool is_header)
{
	push_rtmp_stream* p = (push_rtmp_stream*)user_data;
	p->execute_audio_i(h, out_buf, out_size, is_header);
	return 0;
}


void push_rtmp_stream::execute_audio_i(void* h, const char* out_buf, long out_size, bool is_header)
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

void push_rtmp_stream::send_rtmp_audio_header()
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

	RTMP_SendPacket(this->rtmp_, this->packet_, FALSE);
}


void push_rtmp_stream::send_rtmp_audio(unsigned char* buf, int len, unsigned long long time_stamp)
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

	RTMP_SendPacket(this->rtmp_, this->packet_, TRUE);
}


void push_rtmp_stream::execute_impl(void* h, const char* out_buf, long out_size, int frame_type, unsigned long long time_stamp)
{
	static bool once = true;

	// may recetive this->sps_ and pps one more times ???
	if (once)
	{
		if (frame_type == NAL_SPS) 
		{
			this->sps_.assign(out_buf + 4, out_size - 4);
		}
		else if (frame_type == NAL_PPS) 
		{
			this->pps_.assign(out_buf + 4, out_size - 4);
			once = false;
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


void push_rtmp_stream::sync_stream_thread_entry()
{
	// wait for received video sps (pps) and audio header
	while (this->curr_step_ < 2)
		Sleep(50);
		
	++this->curr_step_;		// increase curr_step_ to 3 for start push audio and video frame into buff

	// send sps(pps) and audio header first
	this->send_video_sps_pps();
	this->send_rtmp_audio_header();

	unsigned long long first_video_time_stamp = -1;
	unsigned long long curr_video_time_stamp = 0;
	
	for (; 3 == this->curr_step_; )
	{
		std::unique_lock<std::mutex> g(this->cs_video_);
		this->cv_video_.wait(g, [this](){
			return this->video_data_.size() > 1;
		});

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

	for (;;)
	{
		std::unique_lock<std::mutex> g(this->cs_to_send_);
		this->cv_to_send_.wait(g, [this](){
			return this->to_send_list_.size() > 10;
		});

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
				this->send_rtmp_audio((unsigned char*)data.c_str(), data.length(), time_stamp);
			else
				this->send_rtmp_video((unsigned char*)data.c_str(), data.length(), time_stamp);
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



