#include "h2645_vui.h"
#include <stdio.h>

#define EXTENDED_SAR       255

#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

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

void h2645_decode_common_vui_params(bs_t* s, H2645VUI *vui)
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