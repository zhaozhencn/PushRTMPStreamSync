#pragma once

#include <functional>
#include <hash_map>
#include <mutex>
#include <memory>

class timer_handler
{
public:
	virtual ~timer_handler()
	{
		;
	}

public:
	unsigned long long id() const
	{
		return id_;
	}

	void id(unsigned long long id)
	{
		this->id_ = id;
	}
	
public:
	virtual void exec() = 0;

protected:
	unsigned long long	id_;
};


class sche_timer
{
public:
	sche_timer();
	~sche_timer();

public:
	unsigned long long register_timer(int interval, int repeat_cnt, timer_handler* h);
	void unregister_timer(unsigned long long id);

public:
	void run_loop(volatile bool& interrupted);
	void run();

private:
	class node
	{
	public:
		typedef std::shared_ptr<timer_handler> timer_handler_ptr;
		timer_handler_ptr		handler_;
		int						interval_;				// ms
		int						repeat_cnt_;
		unsigned long long		absloute_tick_;			// ms
	};

	enum { MIN_SCHE_INTERVAL = 40 };					// ms
	enum { DELETE_TAG = -10 };

private:
	typedef std::hash_map<unsigned long long, node> node_map;
	node_map						node_map_;
	std::recursive_mutex			mutex_;

};

