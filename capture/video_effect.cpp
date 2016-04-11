#include "video_effect.h"


video_effect::video_effect(int width, int height, int fps)
{
	this->width_ = width;
	this->height_ = height;
	this->fps_ = fps;

	//const char *filter_descr = "lutyuv='u=128:v=128'";
	// const char *filter_descr = "boxblur";
	//const char *filter_descr = "hflip";
	//const char *filter_descr = "hue='h=60:s=-3'";
	//const char *filter_descr = "crop=2/3*in_w:2/3*in_h";
	//const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
	//const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='Lei Xiaohua'";
	const char* filter_descr = "lutyuv='y=1.2*val'"; // "curves=preset=lighter";

	outputs = avfilter_inout_alloc();
	inputs = avfilter_inout_alloc();

	avfilter_register_all();

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

	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL)) < 0)
		return;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		return;

	frame_in = av_frame_alloc();
	frame_buffer_in = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
	av_image_fill_arrays(frame_in->data, frame_in->linesize, frame_buffer_in,
		AV_PIX_FMT_YUV420P, width, height, 1);

	frame_out = av_frame_alloc();
	frame_buffer_out = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1));
	av_image_fill_arrays(frame_out->data, frame_out->linesize, frame_buffer_out,
		AV_PIX_FMT_YUV420P, width, height, 1);

	frame_in->width = width;
	frame_in->height = height;
	frame_in->format = AV_PIX_FMT_YUV420P;
}


video_effect::~video_effect()
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

int video_effect::handle(unsigned char* yuv420, std::string& out_frame)
{
	frame_in->data[0] = yuv420;
	frame_in->data[1] = yuv420 + this->width_ * this->height_;
	frame_in->data[2] = yuv420 + this->width_ * this->height_ * 5 / 4;

	if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0)
		return -1;

	/* pull filtered pictures from the filtergraph */
	int ret = av_buffersink_get_frame(buffersink_ctx, frame_out);
	if (ret < 0)
		return -1;

	//output Y,U,V
	if (frame_out->format == AV_PIX_FMT_YUV420P){
		out_frame += std::string((char*)frame_out->data[0], frame_out->linesize[0] * frame_out->height);
		out_frame += std::string((char*)frame_out->data[1], frame_out->linesize[1] * frame_out->height / 2);
		out_frame += std::string((char*)frame_out->data[2], frame_out->linesize[2] * frame_out->height / 2);
	}

	av_frame_unref(frame_out);

	return 0;
}


