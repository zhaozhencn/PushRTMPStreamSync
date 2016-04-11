#pragma once

#include <Windows.h>

class socket_lib_helper
{
public:
	socket_lib_helper()
	{
		WORD version;
		WSADATA wsaData;
		version = MAKEWORD(2, 2);
		WSAStartup(version, &wsaData);
	}

	~socket_lib_helper()
	{
		WSACleanup();
	}
};


class get_qpc_time_ms
{
public:
	static unsigned long long exec()
	{
		if (!inited_)
		{
			inited_ = true;
			QueryPerformanceFrequency(&clock_freq);
		}			

		LARGE_INTEGER curr;
		QueryPerformanceCounter(&curr);

		double t = double(curr.QuadPart);
		t *= 1000000000.0;
		t /= double(clock_freq.QuadPart);

		return unsigned long long(t) / 1000000;
	}

private:
	static bool				inited_; 
	static LARGE_INTEGER	clock_freq;
};



template<typename T>
class array_buf_ptr
{
public:
	array_buf_ptr(const array_buf_ptr&) = delete;
	array_buf_ptr& operator = (const array_buf_ptr&) = delete;

public:
	array_buf_ptr(T* p = NULL)
		: p_buf_ptr_(p)
	{

	}

	~array_buf_ptr()
	{
		if (this->p_buf_ptr_ != NULL)
			delete [] this->p_buf_ptr_;
	}

public:
	array_buf_ptr(array_buf_ptr&& ptr)
	{
		this->p_buf_ptr_ = ptr.p_buf_ptr_;
		ptr->p_buf_ptr_ = NULL;
	}

	array_buf_ptr& operator = (array_buf_ptr&& ptr)
	{
		if (this == ptr)
			return *this;

		if (this->p_buf_ptr_ != NULL)
			delete [] this->p_buf_ptr_;

		this->p_buf_ptr_ = ptr.p_buf_ptr_;
		ptr.p_buf_ptr_ = NULL;
		return *this;
	}

public:
	T* get_pointer()
	{
		return this->p_buf_ptr_;
	}
	
private:
	T* p_buf_ptr_;	
};


