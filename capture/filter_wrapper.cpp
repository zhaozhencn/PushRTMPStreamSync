#include "filter_wrapper.h"

bool filter_wrapper::once = true;

filter_wrapper::filter_wrapper(int width, int height, int width_out, int height_out, int fps, const std::string& filter_descr)
{
	if (once)
	{
		once = false;
		avfilter_register_all();
	}

	this->width_ = width;
	this->height_ = height;
	this->width_out = width_out;
	this->height_out = height_out;
	this->fps_ = fps;
	this->filter_descr_ = filter_descr;

	outputs = avfilter_inout_alloc();
	inputs = avfilter_inout_alloc();


	char args[512] = { 0 };
	AVFilter *buffersrc = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("ffbuffersink");
	AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
	AVBufferSinkParams *buffersink_params;

	filter_graph = avfilter_graph_alloc();

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		width, height, AV_PIX_FMT_YUV420P, 1, fps, 1, 1);

	int ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		args, NULL, filter_graph);
	if (ret < 0) {
		printf("Cannot create buffer source\n");
		return;
	}

	/* buffer video sink: to terminate the filter chain. */
	buffersink_params = av_buffersink_params_alloc();
	buffersink_params->pixel_fmts = pix_fmts;
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		NULL, buffersink_params, filter_graph);
	av_free(buffersink_params);
	if (ret < 0) {
		printf("Cannot create buffer sink\n");
		return;
	}

	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr.c_str(), &inputs, &outputs, NULL)) < 0)
		return;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		return;

	frame_in = av_frame_alloc();
	frame_buffer_in = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
	av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
		AV_PIX_FMT_YUV420P, width, height, 1);

	frame_out = av_frame_alloc();
	frame_buffer_out = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width_out, height_out, 1));
	av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
		AV_PIX_FMT_YUV420P, width_out, height_out, 1);

	frame_in->width = width;
	frame_in->height = height;
	frame_in->format = AV_PIX_FMT_YUV420P;
}


filter_wrapper::~filter_wrapper()
{
	if (frame_in != NULL)
		av_frame_free(&frame_in);

	if (frame_out != NULL)
		av_frame_free(&frame_out);

	if (frame_buffer_out != NULL)
		av_free(frame_buffer_out);

	if (frame_buffer_in != NULL)
		av_free(frame_buffer_in);

	if (outputs != NULL)
		avfilter_inout_free(&outputs);

	if (inputs != NULL)
		avfilter_inout_free(&inputs);

	if (filter_graph != NULL)
		avfilter_graph_free(&filter_graph);
}


filter_wrapper::filter_wrapper(filter_wrapper&& t)
{
	this->frame_in = t.frame_in;
	t.frame_in = NULL;
	this->frame_out = t.frame_out;
	t.frame_out = NULL;
	this->frame_buffer_in = t.frame_buffer_in;
	t.frame_buffer_in = NULL;
	this->frame_buffer_out = t.frame_buffer_out;
	t.frame_buffer_out = NULL;
	this->filter_graph = t.filter_graph;
	t.filter_graph = NULL;
	this->outputs = t.outputs;
	t.outputs = NULL;
	this->inputs = t.inputs;
	t.inputs = NULL;
	this->buffersink_ctx = t.buffersink_ctx;
	t.buffersink_ctx = NULL;
	this->buffersrc_ctx = t.buffersrc_ctx;
	t.buffersrc_ctx = NULL;
	this->width_ = t.width_;
	this->height_ = t.height_;
	this->width_out = t.width_out;
	this->height_out = t.height_out;
	this->fps_ = t.fps_;
	this->filter_descr_ = t.filter_descr_;
	t.filter_descr_.clear();
}


filter_wrapper& filter_wrapper::operator = (filter_wrapper&& t)
{
	if (this == &t)
		return *this;

	if (frame_in != NULL)
		av_frame_free(&frame_in);

	if (frame_out != NULL)
		av_frame_free(&frame_out);

	if (frame_buffer_out != NULL)
		av_free(frame_buffer_out);

	if (frame_buffer_in != NULL)
		av_free(frame_buffer_in);

	if (outputs != NULL)
		avfilter_inout_free(&outputs);

	if (inputs != NULL)
		avfilter_inout_free(&inputs);

	if (filter_graph != NULL)
		avfilter_graph_free(&filter_graph);

	this->frame_in = t.frame_in;
	t.frame_in = NULL;
	this->frame_out = t.frame_out;
	t.frame_out = NULL;
	this->frame_buffer_in = t.frame_buffer_in;
	t.frame_buffer_in = NULL;
	this->frame_buffer_out = t.frame_buffer_out;
	t.frame_buffer_out = NULL;
	this->filter_graph = t.filter_graph;
	t.filter_graph = NULL;
	this->outputs = t.outputs;
	t.outputs = NULL;
	this->inputs = t.inputs;
	t.inputs = NULL;
	this->buffersink_ctx = t.buffersink_ctx;
	t.buffersink_ctx = NULL;
	this->buffersrc_ctx = t.buffersrc_ctx;
	t.buffersrc_ctx = NULL;
	this->width_ = t.width_;
	this->height_ = t.height_;
	this->width_out = t.width_out;
	this->height_out = t.height_out;
	this->fps_ = t.fps_;
	this->filter_descr_ = t.filter_descr_;
	t.filter_descr_.clear();

	return *this;
}


void filter_wrapper::add_filter(filter_wrapper* p)
{
	this->chain_.push_back(filter_wrapper_ptr(p));
}
	

int filter_wrapper::handle(const std::string& input, std::string& output)
{
	frame_in->data[0] = (uint8_t*)input.c_str();
	frame_in->data[1] = (uint8_t*)input.c_str() + this->width_ * this->height_;
	frame_in->data[2] = (uint8_t*)input.c_str() + this->width_ * this->height_ * 5 / 4;

	if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
		return -1;

	/* pull filtered pictures from the filtergraph */
	int ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
	if (ret < 0)
		return -1;

	//output Y,U,V
	if (frame_out->format == AV_PIX_FMT_YUV420P){
		output = std::string((char*)frame_out->data[0], frame_out->linesize[0] * frame_out->height);
		output += std::string((char*)frame_out->data[1], frame_out->linesize[1] * frame_out->height / 2);
		output += std::string((char*)frame_out->data[2], frame_out->linesize[2] * frame_out->height / 2);
	}

	av_frame_unref(frame_out);

	std::for_each(this->chain_.begin(), this->chain_.end(), [&](filter_wrapper_chain::reference ref)
	{
		std::string in = output;
		ref->handle(in, output);
	});

	return 0;
}


scale_filter_wrapper::scale_filter_wrapper(int width, int height, int width_out, int height_out, int fps)
	: filter_wrapper(width, height, width_out, height_out, fps, "scale=in_w*1.0:in_h*1.2")
{

}


int scale_filter_wrapper::handle(const std::string& input, std::string& output)
{
	frame_in->data[0] = (uint8_t*)input.c_str();
	frame_in->data[1] = (uint8_t*)input.c_str() + this->width_ * this->height_;
	frame_in->data[2] = (uint8_t*)input.c_str() + this->width_ * this->height_ * 5 / 4;

	if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
		return -1;

	/* pull filtered pictures from the filtergraph */
	int ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
	if (ret < 0)
		return -1;

	//output Y,U,V
	if (frame_out->format == AV_PIX_FMT_YUV420P){
		output = std::string((char*)frame_out->data[0], frame_out->linesize[0] * frame_out->height);
		output += std::string((char*)frame_out->data[1], frame_out->linesize[1] * frame_out->height / 2);
		output += std::string((char*)frame_out->data[2], frame_out->linesize[2] * frame_out->height / 2);
	}

	av_frame_unref(frame_out);

	return 0;
}


crop_filter_wrapper::crop_filter_wrapper(int width, int height, int width_out, int height_out, int fps)
	: filter_wrapper(width, height, width_out, height_out, fps, "crop=in_w:in_h-96:0:48")
{

}


int crop_filter_wrapper::handle(const std::string& input, std::string& output)
{
	frame_in->data[0] = (uint8_t*)input.c_str();
	frame_in->data[1] = (uint8_t*)input.c_str() + this->width_ * this->height_;
	frame_in->data[2] = (uint8_t*)input.c_str() + this->width_ * this->height_ * 5 / 4;

	if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
		return -1;

	/* pull filtered pictures from the filtergraph */
	int ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
	if (ret < 0)
		return -1;

	//output Y,U,V
	if (frame_out->format == AV_PIX_FMT_YUV420P){
		output = std::string((char*)frame_out->data[0], frame_out->linesize[0] * frame_out->height);
		output += std::string((char*)frame_out->data[1], frame_out->linesize[1] * frame_out->height / 2);
		output += std::string((char*)frame_out->data[2], frame_out->linesize[2] * frame_out->height / 2);
	}

	av_frame_unref(frame_out);

	return 0;
}





