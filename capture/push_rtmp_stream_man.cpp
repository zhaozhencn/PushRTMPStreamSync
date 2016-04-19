#include "push_rtmp_stream_man.h"
#include "push_rtmp_stream.h"
#include "comm.h"
#include <future>
#include <iostream>

push_rtmp_stream_man::push_rtmp_stream_man()
	: interrupted_(false)
{

	this->thr_ = std::thread(&push_rtmp_stream_man::daemon_route, this);
}


push_rtmp_stream_man::~push_rtmp_stream_man()
{
	if (this->thr_.get_id() != std::thread::id())
	{
		this->interrupted_ = true;
		this->thr_.join();
	}
}


int push_rtmp_stream_man::start(std::string url, int camera_idx, int audio_idx)
{
	push_rtmp_stream_ptr ptr = push_rtmp_stream_ptr(new push_rtmp_stream(this));
	if (-1 == ptr->init(url, camera_idx, audio_idx))
		return -1;

	// set h.264 sps¡¢pps¡¢aac header
	const std::string& sps = this->sps_pps_map_[camera_idx].first;
	if (!sps.empty())
		ptr->video_cb(NULL, sps.c_str(), sps.length(), NAL_SPS, camera_idx);

	const std::string& pps = this->sps_pps_map_[camera_idx].second;
	if (!pps.empty())
		ptr->video_cb(NULL, pps.c_str(), pps.length(), NAL_PPS, camera_idx);

	const std::string& audio_header = this->audio_header_map_[audio_idx];
	if (!audio_header.empty())
		ptr->audio_cb(NULL, audio_header.c_str(), audio_header.length(), true);

	{
		std::unique_lock<std::mutex> g(this->mutex_);
		task_map_.insert(std::make_pair(url, ptr));
	}

	return 0;
}


void push_rtmp_stream_man::stop(std::string url)
{
	std::unique_lock<std::mutex> g(this->mutex_);
	task_map::iterator it = task_map_.find(url);
	if (it != this->task_map_.end())
	{
		it->second->fini();
		this->task_map_.erase(it);
	}
}


class conn_rtmp_handler : public timer_handler
{
public:
	conn_rtmp_handler(push_rtmp_stream_man* p, const std::string& url, int camera_idx, int audio_idx)
		: p_(p)
		, url_(url)
		, camera_idx_(camera_idx)
		, audio_idx_(audio_idx)
	{

	}
	
	virtual void exec()
	{
		p_->stop(this->url_);
		int ret = p_->start(this->url_, this->camera_idx_, this->audio_idx_);
		if (0 == ret)
			this->p_->unregister_timer(this);

		std::cout << "conn_rtmp_handler::exec start ret: " << ret << std::endl;
	}

public:
	push_rtmp_stream_man*	p_;
	std::string				url_;
	int						camera_idx_;
	int						audio_idx_;
};


void push_rtmp_stream_man::register_timer(int interval, int repeat_cnt, timer_handler* h)
{
	this->sche_timer_.register_timer(interval, repeat_cnt, h);
}


void push_rtmp_stream_man::unregister_timer(timer_handler* h)
{
	this->sche_timer_.unregister_timer(h->id());
}


void push_rtmp_stream_man::notify_disconnect(const std::string& url, int camera_idx, int audio_idx)
{
	std::cout << "notify_disconnect " << std::endl;
	conn_rtmp_handler* p = new conn_rtmp_handler(this, url, camera_idx, audio_idx);
	this->register_timer(5000, 10, p);
}


void push_rtmp_stream_man::video_cb(void* h, const char* out_buf, long out_size, int frame_type, 
	void* user_data, unsigned long long time_stamp, int camera_idx)
{
	push_rtmp_stream_man* p = (push_rtmp_stream_man*)user_data;
	p->video_cb_impl(h, out_buf, out_size, frame_type, time_stamp, camera_idx);
}


void push_rtmp_stream_man::video_cb_impl(void* h, const char* out_buf, long out_size, int frame_type,
	unsigned long long time_stamp, int camera_idx)
{
	std::unique_lock<std::mutex> g(this->mutex_);

	if (frame_type == NAL_SPS)
		this->sps_pps_map_[camera_idx].first = std::string(out_buf, out_size);
	else if (frame_type == NAL_PPS)
		this->sps_pps_map_[camera_idx].second = std::string(out_buf, out_size);

	std::for_each(this->task_map_.begin(), this->task_map_.end(), [&](task_map::const_reference ref)
	{
		ref.second->video_cb(h, out_buf, out_size, frame_type, time_stamp);
	});
}


void push_rtmp_stream_man::audio_cb(void* h, const char* out_buf, long out_size, void* user_data, 
	bool is_header, int audio_idx)
{
	push_rtmp_stream_man* p = (push_rtmp_stream_man*)user_data;
	p->audio_cb_impl(h, out_buf, out_size, is_header, audio_idx);
}


void push_rtmp_stream_man::audio_cb_impl(void* h, const char* out_buf, long out_size, bool is_header, int audio_idx)
{
	std::unique_lock<std::mutex> g(this->mutex_);

	if (is_header)
		this->audio_header_map_[audio_idx] = std::string(out_buf, out_size);

	std::for_each(this->task_map_.begin(), this->task_map_.end(), [&](task_map::const_reference ref)
	{
		ref.second->audio_cb(h, out_buf, out_size, is_header);
	});
}


void push_rtmp_stream_man::daemon_route()
{
	this->sche_timer_.run_loop(this->interrupted_);
}



