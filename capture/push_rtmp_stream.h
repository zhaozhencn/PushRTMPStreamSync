#pragma once

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

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


class push_rtmp_stream
{
public:
	push_rtmp_stream();
	~push_rtmp_stream();

public:
	int init(const std::string& url);
	void fini();

public:
	static void execute(void* h, const char* out_buf, long out_size, int frame_type, void* user_data, unsigned long long time_stamp);
	static int execute_audio(void* h, const char* out_buf, long out_size, void* user_data, bool is_header);

public:
	void add_audio_to_send_list(unsigned long long time_stamp, const std::string& data);
	void add_video_to_send_list(unsigned long long time_stamp, const std::string& data);

protected:
	void sync_stream_thread_entry();
	void send_thread_entry();

protected:
	void execute_impl(void* h, const char* out_buf, long out_size, int frame_type, unsigned long long time_stamp);

private:
	void send_video_sps_pps();
	void send_rtmp_audio_header();

private:
	void send_rtmp_video(unsigned char * buf, int len, unsigned long long time_stamp);
	void send_rtmp_audio(unsigned char* buf, int len, unsigned long long time_stamp);
	void execute_audio_i(void* h, const char* out_buf, long out_size, bool is_header);

private:
	RTMP*							rtmp_;
	RTMPPacket*						packet_;
		
	std::string						sps_;
	std::string						pps_;
	std::string						audio_header_;

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
};


