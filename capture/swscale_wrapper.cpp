#include "swscale_wrapper.h"


HMODULE swscale_wrapper::hModule = NULL;
swscale_wrapper::exec_scale_p swscale_wrapper::exec = NULL;

swscale_wrapper::swscale_wrapper()
{
	if (NULL == swscale_wrapper::hModule)
	{
		swscale_wrapper::hModule =
			::LoadLibraryEx("E:\\work-zzl\\lib\\swscale_wrapper\\bin\\libswscale_wrapper.so", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

		swscale_wrapper::exec = (exec_scale_p)::GetProcAddress(hModule, "exec_scale");
		// printf("\n LoadLibrary libswscale_wrapper.so ret: [ %X ] \n", hModule);
	}
}


swscale_wrapper::~swscale_wrapper()
{
}


int swscale_wrapper::exec_scale(unsigned char* src_slice[], const int src_stride[], int src_width, int src_height, int src_fmt,
	unsigned char* dst_slice[], const int dst_stride[], int dst_width, int dst_height, int dst_fmt)
{
	if (NULL == swscale_wrapper::exec)
		return -2;

	return swscale_wrapper::exec(src_slice, src_stride, src_width, src_height, src_fmt,
		dst_slice, dst_stride, dst_width, dst_height, dst_fmt);
}





