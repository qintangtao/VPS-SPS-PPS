#include "h264_ps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media_util.h"
#include "bs.h"

#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define MAX_SPS_COUNT          32
#define MAX_PPS_COUNT         256

#define MIN_LOG2_MAX_FRAME_NUM    4
#define MAX_LOG2_MAX_FRAME_NUM    (12 + 4)

#define EXTENDED_SAR       255

static const uint8_t default_scaling4[2][16] = {
	{  6, 13, 20, 28, 13, 20, 28, 32,
	  20, 28, 32, 37, 28, 32, 37, 42 },
	{ 10, 14, 20, 24, 14, 20, 24, 27,
	  20, 24, 27, 30, 24, 27, 30, 34 }
};
static const uint8_t default_scaling8[2][64] = {
	{  6, 10, 13, 16, 18, 23, 25, 27,
	  10, 11, 16, 18, 23, 25, 27, 29,
	  13, 16, 18, 23, 25, 27, 29, 31,
	  16, 18, 23, 25, 27, 29, 31, 33,
	  18, 23, 25, 27, 29, 31, 33, 36,
	  23, 25, 27, 29, 31, 33, 36, 38,
	  25, 27, 29, 31, 33, 36, 38, 40,
	  27, 29, 31, 33, 36, 38, 40, 42 },
	{  9, 13, 15, 17, 19, 21, 22, 24,
	  13, 13, 17, 19, 21, 22, 24, 25,
	  15, 17, 19, 21, 22, 24, 25, 27,
	  17, 19, 21, 22, 24, 25, 27, 28,
	  19, 21, 22, 24, 25, 27, 28, 30,
	  21, 22, 24, 25, 27, 28, 30, 32,
	  22, 24, 25, 27, 28, 30, 32, 33,
	  24, 25, 27, 28, 30, 32, 33, 35 }
};

static const uint8_t ff_zigzag_scan[16 + 1] = {
	0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
	1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
	1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
	3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};

static const uint8_t ff_zigzag_direct[64] = {
	0,   1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};


static const PSRational ff_h2645_pixel_aspect[] = {
	{   0,  1 },
	{   1,  1 },
	{  12, 11 },
	{  10, 11 },
	{  16, 11 },
	{  40, 33 },
	{  24, 11 },
	{  20, 11 },
	{  32, 11 },
	{  80, 33 },
	{  18, 11 },
	{  15, 11 },
	{  64, 33 },
	{ 160, 99 },
	{   4,  3 },
	{   3,  2 },
	{   2,  1 },
};

static inline void h2645_decode_common_vui_params(bs_t* s, H2645VUI *vui)
{
	int aspect_ratio_info_present_flag;

	aspect_ratio_info_present_flag = bs_read_u1(s);
	if (aspect_ratio_info_present_flag) {
		uint8_t aspect_ratio_idc = bs_read_u(s, 8);
		if (aspect_ratio_idc < FF_ARRAY_ELEMS(ff_h2645_pixel_aspect))
			vui->sar = ff_h2645_pixel_aspect[aspect_ratio_idc];
		else if (aspect_ratio_idc == EXTENDED_SAR) {
			vui->sar.num = bs_read_u(s, 16);
			vui->sar.den = bs_read_u(s, 16);
		}
		else
			fprintf(stderr, "Unknown SAR index: %u.\n", aspect_ratio_idc);
	}
	else
		vui->sar = (PSRational) { 0, 1 };

	vui->overscan_info_present_flag = bs_read_u1(s);
	if (vui->overscan_info_present_flag)
		vui->overscan_appropriate_flag = bs_read_u1(s);

	vui->video_signal_type_present_flag = bs_read_u1(s);
	if (vui->video_signal_type_present_flag) {
		vui->video_format = bs_read_u(s, 3);
		vui->video_full_range_flag = bs_read_u1(s);
		vui->colour_description_present_flag = bs_read_u1(s);
		if (vui->colour_description_present_flag) {
			vui->colour_primaries = bs_read_u(s, 8);
			vui->transfer_characteristics = bs_read_u(s, 8);
			vui->matrix_coeffs = bs_read_u(s, 8);
#if 0
			// Set invalid values to "unspecified"
			if (!av_color_primaries_name(vui->colour_primaries))
				vui->colour_primaries = AVCOL_PRI_UNSPECIFIED;
			if (!av_color_transfer_name(vui->transfer_characteristics))
				vui->transfer_characteristics = AVCOL_TRC_UNSPECIFIED;
			if (!av_color_space_name(vui->matrix_coeffs))
				vui->matrix_coeffs = AVCOL_SPC_UNSPECIFIED;
#endif
		}
	}

	vui->chroma_loc_info_present_flag = bs_read_u1(s);
	if (vui->chroma_loc_info_present_flag) {
		vui->chroma_sample_loc_type_top_field = bs_read_ue(s);
		vui->chroma_sample_loc_type_bottom_field = bs_read_ue(s);
		if (vui->chroma_sample_loc_type_top_field <= 5U)
			vui->chroma_location = vui->chroma_sample_loc_type_top_field + 1;
		else
			vui->chroma_location = 0;
	}
	else
		vui->chroma_location = 1;
}

static inline int decode_hrd_parameters(bs_t* s, SPS *sps)
{
	int cpb_count, i;
	cpb_count = bs_read_ue(s) + 1;

	if (cpb_count > 32U) {
		fprintf(stderr, "cpb_count %d invalid\n", cpb_count);
		return PSERROR_INVALIDDATA;
	}

	bs_read_u(s, 4); /* bit_rate_scale */
	bs_read_u(s, 4); /* cpb_size_scale */
	for (i = 0; i < cpb_count; i++) {
		bs_read_ue(s); /* bit_rate_value_minus1 */
		bs_read_ue(s); /* cpb_size_value_minus1 */
		bs_read_u1(s);          /* cbr_flag */
	}
	sps->initial_cpb_removal_delay_length = bs_read_u(s, 5) + 1;
	sps->cpb_removal_delay_length = bs_read_u(s, 5) + 1;
	sps->dpb_output_delay_length = bs_read_u(s, 5) + 1;
	sps->time_offset_length = bs_read_u(s, 5);
	sps->cpb_cnt = cpb_count;
	return 0;
}

static inline int decode_vui_parameters(bs_t* s, SPS *sps)
{
	h2645_decode_common_vui_params(s, &sps->vui);

	//if (show_bits1(gb) && get_bits_left(gb) < 10) {
	if (bs_bits_left(s) < 10) {
		fprintf(stderr, "Truncated VUI (%d)\n", bs_bits_left(s));
		return 0;
	}

	sps->timing_info_present_flag = bs_read_u1(s);
	if (sps->timing_info_present_flag) {
		unsigned num_units_in_tick = bs_read_u(s, 32);
		unsigned time_scale = bs_read_u(s, 32);
		if (!num_units_in_tick || !time_scale) {
			fprintf(stderr,
				"time_scale/num_units_in_tick invalid or unsupported (%u/%u)\n",
				time_scale, num_units_in_tick);
			sps->timing_info_present_flag = 0;
		}
		else {
			sps->num_units_in_tick = num_units_in_tick;
			sps->time_scale = time_scale;
		}
		sps->fixed_frame_rate_flag = bs_read_u1(s);
	}

	sps->nal_hrd_parameters_present_flag = bs_read_u1(s);
	if (sps->nal_hrd_parameters_present_flag)
		if (decode_hrd_parameters(s, sps) < 0)
			return PSERROR_INVALIDDATA;
	sps->vcl_hrd_parameters_present_flag = bs_read_u1(s);
	if (sps->vcl_hrd_parameters_present_flag)
		if (decode_hrd_parameters(s, sps) < 0)
			return PSERROR_INVALIDDATA;
	if (sps->nal_hrd_parameters_present_flag ||
		sps->vcl_hrd_parameters_present_flag)
		bs_read_u1(s);     /* low_delay_hrd_flag */
	sps->pic_struct_present_flag = bs_read_u1(s);
	if (!bs_bits_left(s))
		return 0;
	sps->bitstream_restriction_flag = bs_read_u1(s);
	if (sps->bitstream_restriction_flag) {
		bs_read_u1(s);     /* motion_vectors_over_pic_boundaries_flag */
		bs_read_ue(s); /* max_bytes_per_pic_denom */
		bs_read_ue(s); /* max_bits_per_mb_denom */
		bs_read_ue(s); /* log2_max_mv_length_horizontal */
		bs_read_ue(s); /* log2_max_mv_length_vertical */
		sps->num_reorder_frames = bs_read_ue(s);
		bs_read_ue(s); /*max_dec_frame_buffering*/

		if (bs_bits_left(s) < 0) {
			sps->num_reorder_frames = 0;
			sps->bitstream_restriction_flag = 0;
		}

		if (sps->num_reorder_frames > 16U
			/* max_dec_frame_buffering || max_dec_frame_buffering > 16 */) {
			fprintf(stderr,
				"Clipping illegal num_reorder_frames %d\n",
				sps->num_reorder_frames);
			sps->num_reorder_frames = 16;
			return PSERROR_INVALIDDATA;
		}
	}

	return 0;
}

static int decode_scaling_list(bs_t* s, uint8_t *factors, int size,
	const uint8_t *jvt_list,
	const uint8_t *fallback_list)
{
	int i, last = 8, next = 8;
	const uint8_t *scan = size == 16 ? ff_zigzag_scan : ff_zigzag_direct;
	if (!bs_read_u1(s)) /* matrix not written, we use the predicted one */
		memcpy(factors, fallback_list, size * sizeof(uint8_t));
	else
		for (i = 0; i < size; i++) {
			if (next) {
				int v = bs_read_se(s);
				if (v < -128 || v > 127) {
					fprintf(stderr, "delta scale %d is invalid\n", v);
					return -1;
				}
				next = (last + v) & 0xff;
			}
			if (!i && !next) { /* matrix not written, we use the preset one */
				memcpy(factors, jvt_list, size * sizeof(uint8_t));
				break;
			}
			last = factors[scan[i]] = next ? next : last;
		}
	return 0;
}

/* returns non zero if the provided SPS scaling matrix has been filled */
static int decode_scaling_matrices(bs_t* s, const SPS *sps,
	const PPS *pps, int is_sps,
	uint8_t(*scaling_matrix4)[16],
	uint8_t(*scaling_matrix8)[64])
{
	int ret = 0;
	int fallback_sps = !is_sps && sps->scaling_matrix_present;
	const uint8_t *fallback[4] = {
		fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
		fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
		fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
		fallback_sps ? sps->scaling_matrix8[3] : default_scaling8[1]
	};
	if (bs_read_u1(s)) {
		ret |= decode_scaling_list(s, scaling_matrix4[0], 16, default_scaling4[0], fallback[0]);        // Intra, Y
		ret |= decode_scaling_list(s, scaling_matrix4[1], 16, default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
		ret |= decode_scaling_list(s, scaling_matrix4[2], 16, default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
		ret |= decode_scaling_list(s, scaling_matrix4[3], 16, default_scaling4[1], fallback[1]);        // Inter, Y
		ret |= decode_scaling_list(s, scaling_matrix4[4], 16, default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
		ret |= decode_scaling_list(s, scaling_matrix4[5], 16, default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
		if (is_sps || pps->transform_8x8_mode) {
			ret |= decode_scaling_list(s, scaling_matrix8[0], 64, default_scaling8[0], fallback[2]); // Intra, Y
			ret |= decode_scaling_list(s, scaling_matrix8[3], 64, default_scaling8[1], fallback[3]); // Inter, Y
			if (sps->chroma_format_idc == 3) {
				ret |= decode_scaling_list(s, scaling_matrix8[1], 64, default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
				ret |= decode_scaling_list(s, scaling_matrix8[4], 64, default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
				ret |= decode_scaling_list(s, scaling_matrix8[2], 64, default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
				ret |= decode_scaling_list(s, scaling_matrix8[5], 64, default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
			}
		}
		if (!ret)
			ret = is_sps;
	}
	return ret;
}

int h264_decode_sps(const uint8_t *data, uint32_t size, SPS *sps)
{
	bs_t* s = NULL;
	uint8_t* web = NULL;
	uint32_t webSize;
	uint32_t nal_t;
	int i; int ret = -1;

	web = malloc(size); // "WEB" means "Without Emulation Bytes"
	if (!web)
		goto fail;

	webSize = remove_emulation_bytes(web, size, data, size);
	if (webSize < 1)
		goto fail;

	s = bs_new(web, webSize);
	if (!s)
		goto fail;

	bs_read_u1(s);
	bs_read_u(s, 2);
	nal_t = bs_read_u(s, 5);
	if (0x07 != nal_t) {
		fprintf(stderr, "nal type %u is not sps\n", nal_t);
		goto fail;
	}
	
	memset(sps, 0, sizeof(sps));

	sps->profile_idc = bs_read_u(s, 8);
	sps->constraint_set_flags |= bs_read_u1(s) << 0;   // constraint_set0_flag
	sps->constraint_set_flags |= bs_read_u1(s) << 1;   // constraint_set1_flag
	sps->constraint_set_flags |= bs_read_u1(s) << 2;   // constraint_set2_flag
	sps->constraint_set_flags |= bs_read_u1(s) << 3;   // constraint_set3_flag
	sps->constraint_set_flags |= bs_read_u1(s) << 4;   // constraint_set4_flag
	sps->constraint_set_flags |= bs_read_u1(s) << 5;   // constraint_set5_flag

	bs_read_u(s, 2);	// reserved_zero_2bits

	sps->level_idc = bs_read_u(s, 8);
	sps->sps_id = bs_read_ue(s);

	if (sps->sps_id >= MAX_SPS_COUNT) {
		fprintf(stderr, "sps_id %u out of range\n", sps->sps_id);
		goto fail;
	}

	if (sps->profile_idc == 100 ||  // High profile
		sps->profile_idc == 110 ||  // High10 profile
		sps->profile_idc == 122 ||  // High422 profile
		sps->profile_idc == 244 ||  // High444 Predictive profile
		sps->profile_idc == 44 ||  // Cavlc444 profile
		sps->profile_idc == 83 ||  // Scalable Constrained High profile (SVC)
		sps->profile_idc == 86 ||  // Scalable High Intra profile (SVC)
		sps->profile_idc == 118 ||  // Stereo High profile (MVC)
		sps->profile_idc == 128 ||  // Multiview High profile (MVC)
		sps->profile_idc == 138 ||  // Multiview Depth High profile (MVCD)
		sps->profile_idc == 144) {  // old High444 profile
		sps->chroma_format_idc = bs_read_ue(s);
		if (sps->chroma_format_idc > 3U) {
			fprintf(stderr, "chroma_format_idc %u", sps->chroma_format_idc);
			goto fail;
		}
		else if (sps->chroma_format_idc == 3) {
			sps->residual_color_transform_flag = bs_read_u1(s);
			if (sps->residual_color_transform_flag) {
				fprintf(stderr, "separate color planes are not supported\n");
				goto fail;
			}
		}

		sps->bit_depth_luma = bs_read_ue(s) + 8;
		sps->bit_depth_chroma = bs_read_ue(s) + 8;
		if (sps->bit_depth_chroma != sps->bit_depth_luma) {
			fprintf(stderr, "Different chroma and luma bit depth");
			goto fail;
		}
		if (sps->bit_depth_luma < 8 || sps->bit_depth_luma   > 14 ||
			sps->bit_depth_chroma < 8 || sps->bit_depth_chroma > 14) {
			fprintf(stderr, "illegal bit depth value (%d, %d)\n", sps->bit_depth_luma, sps->bit_depth_chroma);
			goto fail;
		}

		sps->transform_bypass = bs_read_u1(s);
		ret = decode_scaling_matrices(s, sps, NULL, 1, sps->scaling_matrix4, sps->scaling_matrix8);
		if (ret < 0)
			goto fail;
	} else {
		sps->chroma_format_idc = 1;
		sps->bit_depth_luma = 8;
		sps->bit_depth_chroma = 8;
	}

	sps->log2_max_frame_num = bs_read_ue(s);
	if (sps->log2_max_frame_num < MIN_LOG2_MAX_FRAME_NUM - 4 ||
		sps->log2_max_frame_num > MAX_LOG2_MAX_FRAME_NUM - 4) {
		fprintf(stderr,
			"log2_max_frame_num_minus4 out of range (0-12): %d\n",
			sps->log2_max_frame_num);
		goto fail;
	}
	sps->log2_max_frame_num += 4;

	sps->poc_type = bs_read_ue(s);
	if (sps->poc_type == 0) { // FIXME #define
		unsigned t = bs_read_ue(s);
		if (t > 12) {
			fprintf(stderr, "log2_max_poc_lsb (%d) is out of range\n", t);
			goto fail;
		}
		sps->log2_max_poc_lsb = t + 4;
	} else if (sps->poc_type == 1) { // FIXME #define
		sps->delta_pic_order_always_zero_flag = bs_read_u1(s);
		sps->offset_for_non_ref_pic = bs_read_se(s);
		sps->offset_for_top_to_bottom_field = bs_read_se(s);

		if (sps->offset_for_non_ref_pic == INT32_MIN
			|| sps->offset_for_top_to_bottom_field == INT32_MIN
			) {
			fprintf(stderr, "offset_for_non_ref_pic or offset_for_top_to_bottom_field is out of range\n");
			goto fail;
		}

		sps->poc_cycle_length = bs_read_ue(s);

		if ((unsigned)sps->poc_cycle_length >=
			FF_ARRAY_ELEMS(sps->offset_for_ref_frame)) {
			fprintf(stderr, "poc_cycle_length overflow %d\n", sps->poc_cycle_length);
			goto fail;
		}

		for (i = 0; i < sps->poc_cycle_length; i++) {
			sps->offset_for_ref_frame[i] = bs_read_se(s);
			if (sps->offset_for_ref_frame[i] == INT32_MIN) {
				fprintf(stderr, "offset_for_ref_frame is out of range\n");
				goto fail;
			}
		}
	} else if (sps->poc_type != 2) {
		fprintf(stderr, "illegal POC type %d\n", sps->poc_type);
		goto fail;
	}

	sps->ref_frame_count = bs_read_ue(s);
	//if (avctx->codec_tag == MKTAG('S', 'M', 'V', '2'))
	//	sps->ref_frame_count = FFMAX(2, sps->ref_frame_count);
	if (sps->ref_frame_count > 16) {
		fprintf(stderr, "too many reference frames %d\n", sps->ref_frame_count);
		goto fail;
	}
	sps->gaps_in_frame_num_allowed_flag = bs_read_u1(s);
	sps->mb_width = bs_read_ue(s) + 1;
	sps->mb_height = bs_read_ue(s) + 1;

	sps->frame_mbs_only_flag = bs_read_u1(s);

	if (sps->mb_height >= INT_MAX / 2U) {
		fprintf(stderr, "height overflow\n");
		goto fail;
	}
	sps->mb_height *= 2 - sps->frame_mbs_only_flag;

	if (!sps->frame_mbs_only_flag)
		sps->mb_aff = bs_read_u1(s);
	else
		sps->mb_aff = 0;

	if ((unsigned)sps->mb_width >= INT_MAX / 16 ||
		(unsigned)sps->mb_height >= INT_MAX / 16) {
		fprintf(stderr, "mb_width/height overflow\n");
		goto fail;
	}

	sps->direct_8x8_inference_flag = bs_read_u1(s);

	sps->crop = bs_read_u1(s);
	if (sps->crop) {
		unsigned int crop_left = bs_read_ue(s);
		unsigned int crop_right = bs_read_ue(s);
		unsigned int crop_top = bs_read_ue(s);
		unsigned int crop_bottom = bs_read_ue(s);
		int width = 16 * sps->mb_width;
		int height = 16 * sps->mb_height;

		int vsub = (sps->chroma_format_idc == 1) ? 1 : 0;
		int hsub = (sps->chroma_format_idc == 1 ||
			sps->chroma_format_idc == 2) ? 1 : 0;
		int step_x = 1 << hsub;
		int step_y = (2 - sps->frame_mbs_only_flag) << vsub;

		if (crop_left > (unsigned)INT_MAX / 4 / step_x ||
			crop_right > (unsigned)INT_MAX / 4 / step_x ||
			crop_top > (unsigned)INT_MAX / 4 / step_y ||
			crop_bottom > (unsigned)INT_MAX / 4 / step_y ||
			(crop_left + crop_right) * step_x >= width ||
			(crop_top + crop_bottom) * step_y >= height
			) {
			fprintf(stderr, "crop values invalid %d %d %d %d / %d %d\n", crop_left, crop_right, crop_top, crop_bottom, width, height);
			goto fail;
		}

		sps->crop_left = crop_left * step_x;
		sps->crop_right = crop_right * step_x;
		sps->crop_top = crop_top * step_y;
		sps->crop_bottom = crop_bottom * step_y;
	} else {
		sps->crop_left =
			sps->crop_right =
			sps->crop_top =
			sps->crop_bottom =
			sps->crop = 0;
	}

	sps->vui_parameters_present_flag = bs_read_u1(s);
	if (sps->vui_parameters_present_flag) {
		int ret = decode_vui_parameters(s, sps);
		if (ret < 0)
			goto fail;
	}

	ret = 0;

fail:
	if (s)
		bs_free(s);

	if (web)
		free(web);

	return ret;
}

int h264_decode_pps(const uint8_t *data, uint32_t size, PPS *pps)
{
	return 0;
}

/**
 * Compute profile from profile_idc and constraint_set?_flags.
 *
 * @param sps SPS
 *
 * @return profile as defined by FF_PROFILE_H264_*
 */
int h264_get_profile(const SPS *sps)
{
	int profile = sps->profile_idc;

	switch (sps->profile_idc) {
	case FF_PROFILE_H264_BASELINE:
		// constraint_set1_flag set to 1
		profile |= (sps->constraint_set_flags & 1 << 1) ? FF_PROFILE_H264_CONSTRAINED : 0;
		break;
	case FF_PROFILE_H264_HIGH_10:
	case FF_PROFILE_H264_HIGH_422:
	case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
		// constraint_set3_flag set to 1
		profile |= (sps->constraint_set_flags & 1 << 3) ? FF_PROFILE_H264_INTRA : 0;
		break;
	}

	return profile;
}

/**
 * compute width from sps
 */
int h264_get_width(const SPS *sps)
{
	int width = 16 * sps->mb_width;
	if (sps->crop)
		width -= (sps->crop_left + sps->crop_right);
	return width;
}

/**
 * compute height from sps
 */
int h264_get_height(const SPS *sps)
{
	int height = 16 * sps->mb_height;
	if (sps->crop)
		height -= (sps->crop_top + sps->crop_bottom);
	return height;
}

/**
 * compute framerate from sps
 */
int h264_get_framerate(const SPS *sps)
{
	int framerate = 0;
	if (sps->timing_info_present_flag && sps->time_scale > 0 && sps->num_units_in_tick > 0) {
		framerate = (uint32_t)((float)sps->time_scale / (float)sps->num_units_in_tick);
		if (sps->fixed_frame_rate_flag)
			framerate = framerate / 2;
	}
	return framerate;
}