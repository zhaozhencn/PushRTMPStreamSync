#include "sche_timer.h"
#include "util.h"
#include <algorithm>

sche_timer::sche_timer()
{
}


sche_timer::~sche_timer()
{
}


unsigned long long 	sche_timer::register_timer(int interval, int repeat_cnt, timer_handler* h)
{
	std::unique_lock<std::recursive_mutex> g(this->mutex_);

	unsigned long long id = get_qpc_time_ms::exec();

	node n;
	n.handler_ = node::timer_handler_ptr(h);
	n.interval_ = interval;
	n.repeat_cnt_ = repeat_cnt;
	n.absloute_tick_ = id;
	h->id(id);

	this->node_map_.insert(std::make_pair(id, n));
	return id;
}


void sche_timer::unregister_timer(unsigned long long id)
{
	std::unique_lock<std::recursive_mutex> g(this->mutex_);
	auto it = this->node_map_.find(id);
	if (it != this->node_map_.end())
		it->second.repeat_cnt_ = DELETE_TAG;			// set delete tag
}


void sche_timer::run_loop(volatile bool& interrupted)
{
	for (; !interrupted; )
	{
		this->run();
		Sleep(MIN_SCHE_INTERVAL);
	}
}


void sche_timer::run()
{
	std::unique_lock<std::recursive_mutex> g(this->mutex_);

	unsigned long long curr_tick = get_qpc_time_ms::exec();
	std::for_each(this->node_map_.begin(), this->node_map_.end(), [&](node_map::reference ref)
	{
		//   |------------|------------|------------|------------|------------
		// prev  t1  t2  curr
		// handle the timer t1¡¢t2 between prev and curr   

		auto& node = ref.second;
		if (ref.second.absloute_tick_ > curr_tick - MIN_SCHE_INTERVAL && ref.second.absloute_tick_ <= curr_tick)
		{
			if (node.repeat_cnt_ == -1)
			{
				node.absloute_tick_ += node.interval_;
				if (node.handler_.get() != NULL)
					node.handler_->exec();
			}
			else if (node.repeat_cnt_ > 0)
			{
				--node.repeat_cnt_;
				node.absloute_tick_ += node.interval_;
				if (node.handler_.get() != NULL)
					node.handler_->exec();

				if (0 == node.repeat_cnt_)
					node.repeat_cnt_ = DELETE_TAG;
			}
		}
		else if (ref.second.absloute_tick_ < curr_tick - MIN_SCHE_INTERVAL)
			node.repeat_cnt_ = DELETE_TAG;
	});

	// delete all element that repeat_cnt_ == DELETE_TAG
	node_map tmp;
	std::for_each(this->node_map_.begin(), this->node_map_.end(), [&](node_map::const_reference ref)
	{
		if (DELETE_TAG != ref.second.repeat_cnt_)
			tmp.insert(ref);
	});

	tmp.swap(this->node_map_);
}

