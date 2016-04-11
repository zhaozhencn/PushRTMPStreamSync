#pragma once
#include <Windows.h>


class swscale_wrapper
{
public:
	swscale_wrapper();
	~swscale_wrapper();

public:
	int exec_scale(unsigned char* src_slice[], const int src_stride[], int src_width, int src_height, int src_fmt,
		unsigned char* dst_slice[], const int dst_stride[], int dst_width, int dst_height, int dst_fmt);

public:

	typedef int (*exec_scale_p)(unsigned char* src_slice[], const int src_stride[], int src_width, int src_height, int src_fmt,
		unsigned char* dst_slice[], const int dst_stride[], int dst_width, int dst_height, int dst_fmt);

private:
	static HMODULE hModule;
	static exec_scale_p exec;
};

