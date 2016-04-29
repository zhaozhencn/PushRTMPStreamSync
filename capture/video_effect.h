#pragma once

#include "filter_wrapper.h"

class video_effect
{
public:
	video_effect(int width, int height, int fps);
	~video_effect();

public:
	int handle(unsigned char* yuv420, int yuv420_len, std::string& out_frame);

private:
	std::shared_ptr<filter_wrapper> filter_ptr_;
};


