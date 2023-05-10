#include "H264ParseSPS.h"
#include <stdlib.h>
#include <math.h>
#include "media_util.h"


typedef struct
{
	const uint8_t *data;   //sps数据
	uint32_t size;          //sps数据大小
	uint32_t index;         //当前计算位所在的位置标记
} sps_bit_stream;

static void sps_bs_init(sps_bit_stream *bs, const uint8_t *data, uint32_t size)
{
	if (bs) {
		bs->data = data;
		bs->size = size;
		bs->index = 0;
	}
}

/**
 是否已经到数据流最后

 @param bs sps_bit_stream数据
 @return 1：yes，0：no
 */
static int32_t eof(sps_bit_stream *bs)
{
	return (bs->index >= bs->size * 8);    //位偏移已经超出数据
}

/**
 读取从起始位开始的BitCount个位所表示的值

 @param bs sps_bit_stream数据
 @param bitCount bit位个数(从低到高)
 @return value
 */
static uint32_t u(sps_bit_stream *bs, uint8_t bitCount)
{
	uint32_t val = 0;
	for (uint8_t i = 0; i < bitCount; i++) {
		val <<= 1;
		if (eof(bs)) {
			val = 0;
			break;
		}
		else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) {     //计算index所在的位是否为1
			val |= 1;
		}
		bs->index++;  //递增当前起始位(表示该位已经被计算，在后面的计算过程中不需要再次去计算所在的起始位索引，缺点：后面每个bit位都需要去位移)
	}

	return val;
}

/**
 读取无符号哥伦布编码值(UE)
 #2^LeadingZeroBits - 1 + (xxx)
 @param bs sps_bit_stream数据
 @return value
 */
static uint32_t ue(sps_bit_stream *bs)
{
	uint32_t zeroNum = 0;
	while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) {
		zeroNum++;
	}

	return (uint32_t)((1 << zeroNum) - 1 + u(bs, zeroNum));
}

/**
 读取有符号哥伦布编码值(SE)
 #(-1)^(k+1) * Ceil(k/2)

 @param bs sps_bit_stream数据
 @return value
 */
int32_t se(sps_bit_stream *bs)
{
	int32_t ueVal = (int32_t)ue(bs);
	double k = ueVal;

	int32_t seVal = (int32_t)ceil(k / 2);     //ceil:返回大于或者等于指定表达式的最小整数
	if (ueVal % 2 == 0) {       //偶数取反，即(-1)^(k+1)
		seVal = -seVal;
	}

	return seVal;
}

/**
 视频可用性信息(Video usability information)解析
 @param bs sps_bit_stream数据
 @param info sps解析之后的信息数据及结构体
 @see E.1.1 VUI parameters syntax
 */
void vui_para_parse(sps_bit_stream *bs, sps_info_struct *info)
{
	uint32_t aspect_ratio_info_present_flag = u(bs, 1);
	if (aspect_ratio_info_present_flag) {
		uint32_t aspect_ratio_idc = u(bs, 8);
		if (aspect_ratio_idc == 255) {      //Extended_SAR
			u(bs, 16);      //sar_width
			u(bs, 16);      //sar_height
		}
	}

	uint32_t overscan_info_present_flag = u(bs, 1);
	if (overscan_info_present_flag) {
		u(bs, 1);       //overscan_appropriate_flag
	}

	uint32_t video_signal_type_present_flag = u(bs, 1);
	if (video_signal_type_present_flag) {
		u(bs, 3);       //video_format
		u(bs, 1);       //video_full_range_flag
		uint32_t colour_description_present_flag = u(bs, 1);
		if (colour_description_present_flag) {
			u(bs, 8);       //colour_primaries
			u(bs, 8);       //transfer_characteristics
			u(bs, 8);       //matrix_coefficients
		}
	}

	uint32_t chroma_loc_info_present_flag = u(bs, 1);
	if (chroma_loc_info_present_flag) {
		ue(bs);     //chroma_sample_loc_type_top_field
		ue(bs);     //chroma_sample_loc_type_bottom_field
	}

	uint32_t timing_info_present_flag = u(bs, 1);
	if (timing_info_present_flag) {
		uint32_t num_units_in_tick = u(bs, 32);
		uint32_t time_scale = u(bs, 32);
		uint32_t fixed_frame_rate_flag = u(bs, 1);

		info->fps = (uint32_t)((float)time_scale / (float)num_units_in_tick);
		if (fixed_frame_rate_flag) {
			info->fps = info->fps / 2;
		}
	}

	uint32_t nal_hrd_parameters_present_flag = u(bs, 1);
	if (nal_hrd_parameters_present_flag) {
		//hrd_parameters()  //see E.1.2 HRD parameters syntax
	}

	//后面代码需要hrd_parameters()函数接口实现才有用
	uint32_t vcl_hrd_parameters_present_flag = u(bs, 1);
	if (vcl_hrd_parameters_present_flag) {
		//hrd_parameters()
	}
	if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
		u(bs, 1);   //low_delay_hrd_flag
	}

	u(bs, 1);       //pic_struct_present_flag
	uint32_t bitstream_restriction_flag = u(bs, 1);
	if (bitstream_restriction_flag) {
		u(bs, 1);   //motion_vectors_over_pic_boundaries_flag
		ue(bs);     //max_uint8s_per_pic_denom
		ue(bs);     //max_bits_per_mb_denom
		ue(bs);     //log2_max_mv_length_horizontal
		ue(bs);     //log2_max_mv_length_vertical
		ue(bs);     //max_num_reorder_frames
		ue(bs);     //max_dec_frame_buffering
	}
}

//See FFmpeg h264_ps.c ff_h264_decode_seq_parameter_set
//See 7.3.1 NAL unit syntax
//See 7.3.2.1.1 Sequence parameter set data syntax
int32_t h264_parse_sps(const uint8_t *data, uint32_t dataSize, sps_info_struct *info)
{
	if (!data || dataSize <= 0 || !info) return 0;
	int32_t ret = 0;

	uint8_t *dataBuf = malloc(dataSize);

	remove_emulation_bytes(dataBuf, dataSize, data, dataSize);

	sps_bit_stream bs = { 0 };
	sps_bs_init(&bs, dataBuf, dataSize);

	u(&bs, 1);      //forbidden_zero_bit
	u(&bs, 2);      //nal_ref_idc
	uint32_t nal_unit_type = u(&bs, 5);

	if (nal_unit_type == 0x7) {     //Nal SPS Flag
		info->profile_idc = u(&bs, 8);
		u(&bs, 1);      //constraint_set0_flag
		u(&bs, 1);      //constraint_set1_flag
		u(&bs, 1);      //constraint_set2_flag
		u(&bs, 1);      //constraint_set3_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 2);      //reserved_zero_2bits
		info->level_idc = u(&bs, 8);

		ue(&bs);    //seq_parameter_set_id

		uint32_t chroma_format_idc = 1;
		if (info->profile_idc == 100 || info->profile_idc == 110 || info->profile_idc == 122 ||
			info->profile_idc == 244 || info->profile_idc == 44 || info->profile_idc == 83 ||
			info->profile_idc == 86 || info->profile_idc == 118 || info->profile_idc == 128 ||
			info->profile_idc == 138 || info->profile_idc == 139 || info->profile_idc == 134 || info->profile_idc == 135) {
			chroma_format_idc = ue(&bs);
			if (chroma_format_idc == 3) {
				u(&bs, 1);      //separate_colour_plane_flag
			}

			ue(&bs);        //bit_depth_luma_minus8
			ue(&bs);        //bit_depth_chroma_minus8
			u(&bs, 1);      //qpprime_y_zero_transform_bypass_flag
			uint32_t seq_scaling_matrix_present_flag = u(&bs, 1);
			if (seq_scaling_matrix_present_flag) {
				uint32_t seq_scaling_list_present_flag[8] = { 0 };
				for (int32_t i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++) {
					seq_scaling_list_present_flag[i] = u(&bs, 1);
					if (seq_scaling_list_present_flag[i]) {
						if (i < 6) {    //scaling_list(ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[i])
						}
						else {}
					}
				}
			}
		}

		ue(&bs);        //log2_max_frame_num_minus4
		uint32_t pic_order_cnt_type = ue(&bs);
		if (pic_order_cnt_type == 0) {
			ue(&bs);        //log2_max_pic_order_cnt_lsb_minus4
		}
		else if (pic_order_cnt_type == 1) {
			u(&bs, 1);      //delta_pic_order_always_zero_flag
			se(&bs);        //offset_for_non_ref_pic
			se(&bs);        //offset_for_top_to_bottom_field

			uint32_t num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
			int32_t *offset_for_ref_frame = (int32_t *)malloc((uint32_t)num_ref_frames_in_pic_order_cnt_cycle * sizeof(int32_t));
			for (uint32_t i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
				offset_for_ref_frame[i] = se(&bs);
			}
			free(offset_for_ref_frame);
		}

		ue(&bs);      //max_num_ref_frames
		u(&bs, 1);      //gaps_in_frame_num_value_allowed_flag

		uint32_t pic_width_in_mbs_minus1 = ue(&bs);
		uint32_t pic_height_in_map_units_minus1 = ue(&bs);      //47
		uint32_t frame_mbs_only_flag = u(&bs, 1);

		info->width = (int32_t)(pic_width_in_mbs_minus1 + 1) * 16;
		info->height = (int32_t)(2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;

		if (!frame_mbs_only_flag) {
			u(&bs, 1);      //mb_adaptive_frame_field_flag
		}

		u(&bs, 1);     //direct_8x8_inference_flag
		uint32_t frame_cropping_flag = u(&bs, 1);
		if (frame_cropping_flag) {
			uint32_t frame_crop_left_offset = ue(&bs);
			uint32_t frame_crop_right_offset = ue(&bs);
			uint32_t frame_crop_top_offset = ue(&bs);
			uint32_t frame_crop_bottom_offset = ue(&bs);

			//See 6.2 Source, decoded, and output picture formats
			int32_t crop_unit_x = 1;
			int32_t crop_unit_y = 2 - frame_mbs_only_flag;      //monochrome or 4:4:4
			if (chroma_format_idc == 1) {   //4:2:0
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (chroma_format_idc == 2) {    //4:2:2
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

#if 0
			int vsub = (chroma_format_idc == 1) ? 1 : 0;
			int hsub = (chroma_format_idc == 1 || chroma_format_idc == 2) ? 1 : 0;
			int step_x = 1 << hsub;
			int step_y = (2 - frame_mbs_only_flag) << vsub;
			crop_left = frame_crop_left_offset * step_x;
			crop_right = frame_crop_right_offset * step_x;
			crop_top = frame_crop_top_offset * step_y;
			crop_bottom = frame_crop_bottom_offset * step_y;
#endif

			info->width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
			info->height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}

		uint32_t vui_parameters_present_flag = u(&bs, 1);
		if (vui_parameters_present_flag) {
			vui_para_parse(&bs, info);
		}

		ret = 1;
	}

	free(dataBuf);

	return ret;
}