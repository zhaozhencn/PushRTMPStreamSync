#pragma once

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
#include <memory>
#include <list>
#include <algorithm>

class filter_wrapper
{
public:
	filter_wrapper(int width, int height, int width_out, int height_out, int fps, const std::string& filter_descr);
	~filter_wrapper();

public:
	filter_wrapper(const filter_wrapper&) = delete;
	filter_wrapper& operator = (const filter_wrapper&) = delete;

public:
	filter_wrapper(filter_wrapper&& t);
	filter_wrapper& operator = (filter_wrapper&& t);

public:
	virtual void add_filter(filter_wrapper* p);
	virtual int handle(const std::string& input, std::string& output);
	
protected:
	AVFrame *frame_in = NULL;
	AVFrame *frame_out = NULL;
	unsigned char *frame_buffer_in = NULL;
	unsigned char *frame_buffer_out = NULL;
	AVFilterGraph *filter_graph = NULL;
	AVFilterInOut *outputs = NULL;
	AVFilterInOut *inputs = NULL;
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterContext *buffersrc_ctx = NULL;
	int width_ = 0;
	int height_ = 0;
	int width_out = 0;
	int height_out = 0;
	int fps_ = 0;

	std::string filter_descr_;

private:
	typedef std::shared_ptr<filter_wrapper> filter_wrapper_ptr;
	typedef std::list<filter_wrapper_ptr> filter_wrapper_chain;
	filter_wrapper_chain	chain_;

private:
	static bool once;
};


class scale_filter_wrapper : public filter_wrapper
{
public:
	scale_filter_wrapper(int width, int height, int width_out, int height_out, int fps);
	virtual int handle(const std::string& input, std::string& output);
};


class crop_filter_wrapper : public filter_wrapper
{
public:
	crop_filter_wrapper(int width, int height, int width_out, int height_out, int fps);
	virtual int handle(const std::string& input, std::string& output);
};


