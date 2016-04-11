#pragma once

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#define snprintf _snprintf

//Windows
extern "C"
{
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

#include <string>

class video_effect
{
public:
	video_effect(int width, int height, int fps);
	~video_effect();

public:
	int handle(unsigned char* yuv420, std::string& out_frame);

private:
	AVFrame *frame_in = NULL;
	AVFrame *frame_out = NULL;
	unsigned char *frame_buffer_in = NULL;
	unsigned char *frame_buffer_out = NULL;
	AVFilterGraph *filter_graph = NULL;
    AVFilterInOut *outputs = NULL;
	AVFilterInOut *inputs = NULL;
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterContext *buffersrc_ctx = NULL;

	int width_;
	int height_;
	int fps_;
};


