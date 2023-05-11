#ifndef H2645_VUI_H
#define H2645_VUI_H

#include <stdint.h>
#include "bs.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct PSRational {
		int num; ///< Numerator
		int den; ///< Denominator
	} PSRational;

	typedef struct H2645VUI {
		PSRational sar;

		int overscan_info_present_flag;
		int overscan_appropriate_flag;

		int video_signal_type_present_flag;
		int video_format;
		int video_full_range_flag;
		int colour_description_present_flag;
		int colour_primaries;
		int transfer_characteristics;
		int matrix_coeffs;

		int chroma_loc_info_present_flag;
		int chroma_sample_loc_type_top_field;
		int chroma_sample_loc_type_bottom_field;
		int chroma_location;
	} H2645VUI;

	void h2645_decode_common_vui_params(bs_t* s, H2645VUI *vui);

#ifdef __cplusplus
}
#endif

#endif  /* H2645_VUI_H */



