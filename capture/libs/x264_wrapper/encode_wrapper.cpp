extern "C" {
	#include <inttypes.h>
	#include <string.h>
	#include "x264.h"
}

#include "encode_wrapper.h"
#include <map>
#include <utility>


#ifndef NULL
#define NULL 0
#endif

struct x264_enc_info
{
	x264_param_t	param_;
	x264_picture_t	pic_in_;
	void*			user_data_;
};

std::map<unsigned long, x264_enc_info> g_enc_map;


#define ZH264_DEFAULT 0
#define ZH264_ULTRAFAST 1
#define ZH264_SUPERFAST 2
#define ZH264_VERYFAST 3
#define ZH264_FASTER 4
#define ZH264_FAST 5
#define ZH264_MEDIUM 6
#define ZH264_SLOW 7
#define ZH264_SLOWER 8
#define ZH264_VERYSLOW 9


X264_CONFIG_SET(x264_param_t*param, int mode)
{
	switch (mode)
	{
	case ZH264_DEFAULT:
	case ZH264_MEDIUM:
	case ZH264_ULTRAFAST:
	{
		param->i_frame_reference = 1;
		param->i_scenecut_threshold = 0;
		param->b_deblocking_filter = 0;
		param->b_cabac = 0;
		param->i_bframe = 0;
		param->analyse.intra = 0;
		param->analyse.inter = 0;
		param->analyse.b_transform_8x8 = 0;
		param->analyse.i_me_method = X264_ME_DIA;
		param->analyse.i_subpel_refine = 0;
		param->rc.i_aq_mode = 0;
		param->analyse.b_mixed_references = 0;
		param->analyse.i_trellis = 0;
		param->i_bframe_adaptive = X264_B_ADAPT_NONE;
		param->rc.b_mb_tree = 0;
		param->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
		param->analyse.b_weighted_bipred = 0;
		param->rc.i_lookahead = 0;
	}
	break;
	case ZH264_SUPERFAST:
	{
		param->analyse.inter = X264_ANALYSE_I8x8 | X264_ANALYSE_I4x4;
		param->analyse.i_me_method = X264_ME_DIA;
		param->analyse.i_subpel_refine = 1;
		param->i_frame_reference = 1;
		param->analyse.b_mixed_references = 0;
		param->analyse.i_trellis = 0;
		param->rc.b_mb_tree = 0;
		param->analyse.i_weighted_pred = X264_WEIGHTP_SIMPLE;
		param->rc.i_lookahead = 0;
	}
	break;
	case ZH264_VERYFAST:
	{
		param->analyse.i_me_method = X264_ME_HEX;
		param->analyse.i_subpel_refine = 2;
		param->i_frame_reference = 1;
		param->analyse.b_mixed_references = 0;
		param->analyse.i_trellis = 0;
		param->analyse.i_weighted_pred = X264_WEIGHTP_SIMPLE;
		param->rc.i_lookahead = 10;
	}
	break;
	case ZH264_FASTER:
	{
		param->analyse.b_mixed_references = 0;
		param->i_frame_reference = 2;
		param->analyse.i_subpel_refine = 4;
		param->analyse.i_weighted_pred = X264_WEIGHTP_SIMPLE;
		param->rc.i_lookahead = 20;
	}
	break;
	case ZH264_FAST:
	{
		param->i_frame_reference = 2;
		param->analyse.i_subpel_refine = 6;
		param->analyse.i_weighted_pred = X264_WEIGHTP_SIMPLE;
		param->rc.i_lookahead = 30;
	}
	break;
	case ZH264_SLOW:
	{
		param->analyse.i_me_method = X264_ME_UMH;
		param->analyse.i_subpel_refine = 8;
		param->i_frame_reference = 5;
		param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
		param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
		param->rc.i_lookahead = 50;
	}
	break;
	case ZH264_SLOWER:
	{
		param->analyse.i_me_method = X264_ME_UMH;
		param->analyse.i_subpel_refine = 9;
		param->i_frame_reference = 8;
		param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
		param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
		param->analyse.inter |= X264_ANALYSE_PSUB8x8;
		param->analyse.i_trellis = 2;
		param->rc.i_lookahead = 60;
	}
	break;
	case ZH264_VERYSLOW:
	{
		param->analyse.i_me_method = X264_ME_UMH;
		param->analyse.i_subpel_refine = 10;
		param->analyse.i_me_range = 24;
		param->i_frame_reference = 16;
		param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
		param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
		param->analyse.inter |= X264_ANALYSE_PSUB8x8;
		param->analyse.i_trellis = 2;
		param->i_bframe = 8;
		param->rc.i_lookahead = 60;
	}
	break;
	}
}



extern "C" void* open_encoder(int width, int height, int bitrate, int fps, int bitstream_ctl, void* user_data)
{
#define MY_SETTING
#ifdef MY_SETTING
	x264_picture_t pic = {0};
	x264_param_t param = {0};
	x264_nal_t *headers = NULL;
	int i_nal = 0;

	x264_param_default_preset(&param, "veryfast", "zerolatency");
    param.i_threads = 1;
    param.i_width = width;
    param.i_height = height;
    param.i_fps_num = fps;
    param.i_fps_den = 1;

    // param.i_keyint_max = 300;
	// param.i_keyint_min = 10;
	// param.i_scenecut_threshold = 600;
    // param.b_intra_refresh = 1;

	param.rc.i_rc_method = bitstream_ctl;
	param.rc.i_bitrate = bitrate;
	param.rc.i_vbv_max_bitrate = bitrate;
	param.rc.i_vbv_buffer_size = 65536;
	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.f_rf_constant = 0.0f;
//  param.rc.f_rf_constant = 25;
//  param.rc.f_rf_constant_max = 35;

	param.i_keyint_min = fps;
	param.i_keyint_max = fps;
	param.i_fps_num = fps;
	param.i_timebase_num = 1;
	param.i_timebase_den = fps;
	
	param.i_level_idc = 41;
	

    // param.b_annexb = 0;
    param.b_repeat_headers = 1;
	param.i_log_level = X264_LOG_ERROR; //  X264_LOG_DEBUG;

    x264_param_apply_profile(&param, "main");

#else
	x264_nal_t *headers = NULL;
	int i_nal = 0;
	x264_picture_t pic = { 0 };
	x264_param_t param = { 0 };
	int qp = 25;

	x264_param_default(&param);
// 	X264_CONFIG_SET(&param, ZH264_MEDIUM);
	x264_param_default_preset(&param, "veryfast", "film");

	param.i_width = width;
	param.i_height = height;
	param.i_threads = 1;

	param.i_log_level = X264_LOG_NONE;
	param.i_fps_num = fps;
	param.i_fps_den = 1;
	param.i_keyint_min = 10; //  X264_KEYINT_MIN_AUTO;
	param.i_keyint_max = 300; //  keyframe;

	// param.b_annexb = 0;
	param.b_repeat_headers = 1;
//	param.i_timebase_num = param.i_fps_den;
//	param.i_timebase_den = param.i_fps_num;


#if 0
	if (bitstream_ctl == 0)
	{
		if (qp>0 && qp <= 51)
		{
			param.rc.i_rc_method = X264_RC_CQP;//恒定质量	
			param.rc.i_qp_constant = qp;
		}
	}
	else
	{
		if (bitrate>0)
		{
			if (bitstream_ctl == 0)
				param.rc.i_rc_method = X264_RC_ABR;////平均码率//恒定码率
			else if (bitstream_ctl == 2)
				param.rc.i_rc_method = X264_RC_CRF;//码率动态

			param.rc.i_bitrate = bitrate;//单位kbps
			param.rc.i_vbv_max_bitrate = bitrate;
		}
	}

#endif

	param.rc.i_rc_method = bitstream_ctl;
	param.rc.i_bitrate = bitrate;


#if 0
	param.analyse.intra = X264_ANALYSE_I4x4;//| X264_ANALYSE_I8x8 | X264_ANALYSE_PSUB8x8;
	param.analyse.inter = X264_ANALYSE_I4x4;//| X264_ANALYSE_I8x8 | X264_ANALYSE_PSUB8x8;
	//	param.analyse.i_me_range = 24;//越大越好
	param.analyse.b_transform_8x8 = 0;
	param.i_cqm_preset = X264_CQM_FLAT;
	param.analyse.i_me_method = X264_ME_UMH;
	param.b_cabac = 0;
	param.rc.b_mb_tree = 0;//实时编码为0
	param.rc.b_stat_write = 0;   /* Enable stat writing in psz_stat_out */
	param.rc.b_stat_read = 0;    /* Read stat from psz_stat_in and use it */
#endif

	x264_param_apply_profile(&param, "main");

#endif

    x264_t* p = x264_encoder_open(&param);
	if (NULL == p)
		return NULL;

	if (x264_encoder_headers(p, &headers, &i_nal) < 0 
		|| x264_picture_alloc(&pic, X264_CSP_I420, width, height) < 0)
	{
		x264_encoder_close(p);
		return NULL;
	}

	x264_enc_info info = { 0 };
	info.param_ = param;
	info.pic_in_ = pic;
	info.user_data_ = user_data;

	g_enc_map.insert(std::make_pair((unsigned long)p, info));
	return p;	
}


extern "C" int encode(void* h, const char* in_buf, long in_size, enc_cb_t cb, unsigned long long time_stamp, bool force_key_frame)
{
	unsigned long key = (unsigned long)h;
	if (g_enc_map.find(key) == g_enc_map.end())
		return -1;

	x264_enc_info& ref = g_enc_map[key];

	// YV12 check
	if (in_size != ref.param_.i_width * ref.param_.i_height * 3 / 2)
		return -2;

	ref.pic_in_.img.i_csp = X264_CSP_I420;
	ref.pic_in_.img.i_plane = 3;
	ref.pic_in_.img.i_stride[0] = ref.param_.i_width;
	ref.pic_in_.img.i_stride[1] = ref.param_.i_width / 2;
	ref.pic_in_.img.i_stride[2] = ref.param_.i_width / 2;
	memcpy(ref.pic_in_.img.plane[0], in_buf, ref.param_.i_width * ref.param_.i_height);
	memcpy(ref.pic_in_.img.plane[1], in_buf + ref.param_.i_width * ref.param_.i_height,
		ref.param_.i_width * ref.param_.i_height / 4);
	memcpy(ref.pic_in_.img.plane[2], in_buf + ref.param_.i_width * ref.param_.i_height * 5 / 4,
		ref.param_.i_width * ref.param_.i_height / 4);
	
	x264_picture_t* pic_in = &ref.pic_in_;
	x264_picture_t pic_out;
	x264_nal_t *nal;
	int i_nal;
	int i_frame_size = 0;
	
	if (force_key_frame)
		pic_in->i_type = X264_TYPE_IDR;
	else
		pic_in->i_type = X264_TYPE_AUTO;

	i_frame_size = x264_encoder_encode((x264_t*)h, &nal, &i_nal, pic_in, &pic_out);
	if (i_frame_size < 0)
		return -3;
	else if (0 == i_frame_size)
		return -4;
	else if (cb != NULL)
	{
		for (int i = 0; i < i_nal; ++i)
			cb((void*)key, (char*)nal[i].p_payload, nal[i].i_payload, nal[i].i_type, ref.user_data_, time_stamp);
	}

	return 0;

}


extern "C" void close_encoder(void* h)
{
	unsigned long key = (unsigned long)h;
	if (g_enc_map.find(key) == g_enc_map.end())
		return;
	
	x264_enc_info& ref = g_enc_map[key];

	// close handle and cleanup picture 
	x264_encoder_close((x264_t*)h);

	x264_picture_t* pic_in = &ref.pic_in_;
	x264_picture_clean(pic_in);

	g_enc_map.erase(key);
}


