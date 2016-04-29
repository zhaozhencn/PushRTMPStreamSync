#include "video_effect.h"


video_effect::video_effect(int width, int height, int fps)
{
	//const char *filter_descr = "lutyuv='u=128:v=128'";
	const char *filter_descr4 = "boxblur=luma_radius=5:luma_power=1";
	//const char *filter_descr = "hflip";
	//const char *filter_descr = "hue='h=60:s=-3'";
	//const char *filter_descr = "crop=2/3*in_w:2/3*in_h";
	//const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
	//const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='Lei Xiaohua'";
	const char* filter_descr = "lutyuv='y=1.2*val'"; // "curves=preset=lighter";
	const char* filter_descr2 = "scale=in_w*1.0:in_h*1.2";
	const char* filter_descr3 = "crop=in_w:in_h-96:0:48";
	
	this->filter_ptr_ = std::shared_ptr<filter_wrapper>(new filter_wrapper(width, height, width, height, fps, filter_descr));
	this->filter_ptr_->add_filter(new scale_filter_wrapper(width, height, width, height * 1.2, fps));
	this->filter_ptr_->add_filter(new crop_filter_wrapper(width, height * 1.2, width, height, fps));
	this->filter_ptr_->add_filter(new filter_wrapper(width, height, width, height, fps, filter_descr4));

}


video_effect::~video_effect()
{
}


int video_effect::handle(unsigned char* yuv420, int yuv420_len, std::string& out_frame)
{
	this->filter_ptr_->handle(std::string((char*)yuv420, (char*)yuv420 + yuv420_len), out_frame);
	return 0;
}


