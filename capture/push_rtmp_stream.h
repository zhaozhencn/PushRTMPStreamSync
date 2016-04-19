#pragma once

#include <cstdint>
#include <mutex>
#include <queue>
#include <tuple>
#include <memory>
#include <hash_map>
#include <map>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <algorithm>
#include <fstream>

#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"

class push_rtmp_stream_man;
class push_rtmp_stream
{
public:
	push_rtmp_stream(push_rtmp_stream_man* man);
	~push_rtmp_stream();

public:
	push_rtmp_stream(const push_rtmp_stream&) = delete;
	push_rtmp_stream& operator = (const push_rtmp_stream&) = delete;
	push_rtmp_stream(push_rtmp_stream&&) = delete;
	push_rtmp_stream& operator = (push_rtmp_stream&&) = delete;

public:
	int init(const std::string& url, int camera_idx, int audio_idx);
	void fini();

public:
	void video_cb(void* h, const char* out_buf, long out_size, int frame_type, unsigned long long time_stamp);
	void audio_cb(void* h, const char* out_buf, long out_size, bool is_header);

public:
	void add_audio_to_send_list(unsigned long long time_stamp, const std::string& data);
	void add_video_to_send_list(unsigned long long time_stamp, const std::string& data);

protected:
	void start_thread();
	void sync_stream_thread_entry();
	void send_thread_entry();

protected:
	void execute_impl(void* h, const char* out_buf, long out_size, int frame_type, unsigned long long time_stamp);

private:
	enum { MAX_WAIT_TIMES_THRESHOLD = 100 };

private:
	int send_video_sps_pps();
	int send_rtmp_audio_header();

private:
	int send_rtmp_video(unsigned char * buf, int len, unsigned long long time_stamp);
	int send_rtmp_audio(unsigned char* buf, int len, unsigned long long time_stamp);
	void execute_audio_i(void* h, const char* out_buf, long out_size, bool is_header);

private:
	std::string						url_;
	int								camera_idx_;
	int								audio_idx_;
	RTMP*							rtmp_;
	RTMPPacket*						packet_;
		
	std::string						sps_;
	std::string						pps_;
	std::string						audio_header_;
	bool							once_;

private:
	std::shared_ptr<std::thread>	sync_stream_thread_;

	typedef std::list<std::pair<unsigned long long, std::string>> data_list;
	data_list						audio_data_;
	std::mutex						cs_audio_;

	typedef std::list<std::tuple<unsigned long long, int, std::string>> video_data_list;
	video_data_list					video_data_;
	std::mutex						cs_video_;
	std::condition_variable			cv_video_;

	typedef enum { AUDIO = 100, VIDEO = 200 } FRAME_TYPE;
	typedef std::list<std::tuple<unsigned long long, FRAME_TYPE, std::string>> mix_data_list;
	mix_data_list					to_send_list_;

	std::shared_ptr<std::thread>	send_thread_;
	std::mutex						cs_to_send_;
	std::condition_variable			cv_to_send_;

private:
	std::atomic<int>				curr_step_;
	
private:
	push_rtmp_stream_man*			man_;
};


