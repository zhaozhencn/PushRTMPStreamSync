#ifndef push_rtmp_stream_man_H__
#define push_rtmp_stream_man_H__

#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <thread>
#include "sche_timer.h"

class push_rtmp_stream;
class push_rtmp_stream_man
{
public:
	push_rtmp_stream_man();
	~push_rtmp_stream_man();

public:
	push_rtmp_stream_man(const push_rtmp_stream_man&) = delete;
	push_rtmp_stream_man& operator =(const push_rtmp_stream_man&) = delete;
	push_rtmp_stream_man(push_rtmp_stream_man&&) = delete;
	push_rtmp_stream_man& operator =(push_rtmp_stream_man&&) = delete;

public:
	int start(std::string url, int camera_idx, int audio_idx);
	void stop(std::string url);

public:
	static void video_cb(void* h, const char* out_buf, long out_size, int frame_type, void* user_data, 
		unsigned long long time_stamp, int camera_idx);
	static void audio_cb(void* h, const char* out_buf, long out_size, void* user_data, bool is_header, int audio_idx);

public:
	void notify_disconnect(const std::string& url, int camera_idx, int audio_idx);
	void register_timer(int interval, int repeat_cnt, timer_handler* h);
	void unregister_timer(timer_handler* h);

protected:
	void video_cb_impl(void* h, const char* out_buf, long out_size, int frame_type, 
		unsigned long long time_stamp, int camera_idx);
	void audio_cb_impl(void* h, const char* out_buf, long out_size, bool is_header, int audio_idx);

private:
	void daemon_route();

private:
	typedef std::shared_ptr<push_rtmp_stream> push_rtmp_stream_ptr;
	typedef std::map<std::string, push_rtmp_stream_ptr> task_map;
	task_map						task_map_;
	std::mutex						mutex_;

private:
	typedef std::map<int, std::pair<std::string, std::string>> sps_pps_map;
	sps_pps_map						sps_pps_map_;

	typedef std::map<int, std::string> audio_header_map;
	audio_header_map				audio_header_map_;

	sche_timer						sche_timer_;
	std::thread						thr_;
	volatile bool					interrupted_;
};



#endif

