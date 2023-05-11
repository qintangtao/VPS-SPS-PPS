#ifndef H264_PS_H
#define H264_PS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QP_MAX_NUM (51 + 6*6)           // The maximum supported qp

#define PROFILE_H264_CONSTRAINED  (1<<9)  // 8+1; constraint_set1_flag
#define PROFILE_H264_INTRA        (1<<11) // 8+3; constraint_set3_flag

#define PROFILE_H264_BASELINE             66
#define PROFILE_H264_CONSTRAINED_BASELINE (66|PROFILE_H264_CONSTRAINED)
#define PROFILE_H264_MAIN                 77
#define PROFILE_H264_EXTENDED             88
#define PROFILE_H264_HIGH                 100
#define PROFILE_H264_HIGH_10              110
#define PROFILE_H264_HIGH_10_INTRA        (110|PROFILE_H264_INTRA)
#define PROFILE_H264_MULTIVIEW_HIGH       118
#define PROFILE_H264_HIGH_422             122
#define PROFILE_H264_HIGH_422_INTRA       (122|PROFILE_H264_INTRA)
#define PROFILE_H264_STEREO_HIGH          128
#define PROFILE_H264_HIGH_444             144
#define PROFILE_H264_HIGH_444_PREDICTIVE  244
#define PROFILE_H264_HIGH_444_INTRA       (244|PROFILE_H264_INTRA)
#define PROFILE_H264_CAVLC_444            44

#define PSMKTAG(a,b,c,d)   ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define PSERRTAG(a, b, c, d) (-(int)PSMKTAG(a, b, c, d))

#define ERROR_BSF_NOT_FOUND      PSERRTAG(0xF8,'B','S','F') ///< Bitstream filter not found
#define ERROR_BUG                PSERRTAG( 'B','U','G','!') ///< Internal bug, also see ERROR_BUG2
#define ERROR_BUFFER_TOO_SMALL   PSERRTAG( 'B','U','F','S') ///< Buffer too small
#define ERROR_DECODER_NOT_FOUND  PSERRTAG(0xF8,'D','E','C') ///< Decoder not found
#define ERROR_DEMUXER_NOT_FOUND  PSERRTAG(0xF8,'D','E','M') ///< Demuxer not found
#define ERROR_ENCODER_NOT_FOUND  PSERRTAG(0xF8,'E','N','C') ///< Encoder not found
#define ERROR_EOF                PSERRTAG( 'E','O','F',' ') ///< End of file
#define ERROR_EXIT               PSERRTAG( 'E','X','I','T') ///< Immediate exit was requested; the called function should not be restarted
#define ERROR_EXTERNAL           PSERRTAG( 'E','X','T',' ') ///< Generic error in an external library
#define ERROR_FILTER_NOT_FOUND   PSERRTAG(0xF8,'F','I','L') ///< Filter not found
#define ERROR_INVALIDDATA        PSERRTAG( 'I','N','D','A') ///< Invalid data found when processing input
#define ERROR_MUXER_NOT_FOUND    PSERRTAG(0xF8,'M','U','X') ///< Muxer not found
#define ERROR_OPTION_NOT_FOUND   PSERRTAG(0xF8,'O','P','T') ///< Option not found
#define ERROR_PATCHWELCOME       PSERRTAG( 'P','A','W','E') ///< Not yet implemented in FFmpeg, patches welcome
#define ERROR_PROTOCOL_NOT_FOUND PSERRTAG(0xF8,'P','R','O') ///< Protocol not found
#define ERROR_STREAM_NOT_FOUND   PSERRTAG(0xF8,'S','T','R') ///< Stream not found


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

	/**
	 * Sequence parameter set
	 */
	typedef struct SPS {
		unsigned int sps_id;
		int profile_idc;
		int level_idc;
		int chroma_format_idc;
		int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
		int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
		int poc_type;                      ///< pic_order_cnt_type
		int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
		int delta_pic_order_always_zero_flag;
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
		int ref_frame_count;               ///< num_ref_frames
		int gaps_in_frame_num_allowed_flag;
		int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
		///< (pic_height_in_map_units_minus1 + 1) * (2 - frame_mbs_only_flag)
		int mb_height;
		int frame_mbs_only_flag;
		int mb_aff;                        ///< mb_adaptive_frame_field_flag
		int direct_8x8_inference_flag;
		int crop;                          ///< frame_cropping_flag

		/* those 4 are already in luma samples */
		unsigned int crop_left;            ///< frame_cropping_rect_left_offset
		unsigned int crop_right;           ///< frame_cropping_rect_right_offset
		unsigned int crop_top;             ///< frame_cropping_rect_top_offset
		unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
		int vui_parameters_present_flag;
		H2645VUI vui;

		int timing_info_present_flag;
		uint32_t num_units_in_tick;
		uint32_t time_scale;
		int fixed_frame_rate_flag;
		int32_t offset_for_ref_frame[256];
		int bitstream_restriction_flag;
		int num_reorder_frames;
		int scaling_matrix_present;
		uint8_t scaling_matrix4[6][16];
		uint8_t scaling_matrix8[6][64];
		int nal_hrd_parameters_present_flag;
		int vcl_hrd_parameters_present_flag;
		int pic_struct_present_flag;
		int time_offset_length;
		int cpb_cnt;                          ///< See H.264 E.1.2
		int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
		int cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
		int dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
		int bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
		int bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
		int residual_color_transform_flag;    ///< residual_colour_transform_flag
		int constraint_set_flags;             ///< constraint_set[0-3]_flag
	} SPS;


	/**
	 * Picture parameter set
	 */
	typedef struct PPS {
		unsigned int pps_id;
		unsigned int sps_id;
		int cabac;                  ///< entropy_coding_mode_flag
		int pic_order_present;      ///< pic_order_present_flag
		int slice_group_count;      ///< num_slice_groups_minus1 + 1
		int mb_slice_group_map_type;
		unsigned int ref_count[2];  ///< num_ref_idx_l0/1_active_minus1 + 1
		int weighted_pred;          ///< weighted_pred_flag
		int weighted_bipred_idc;
		int init_qp;                ///< pic_init_qp_minus26 + 26
		int init_qs;                ///< pic_init_qs_minus26 + 26
		int chroma_qp_index_offset[2];
		int deblocking_filter_parameters_present; ///< deblocking_filter_parameters_present_flag
		int constrained_intra_pred;     ///< constrained_intra_pred_flag
		int redundant_pic_cnt_present;  ///< redundant_pic_cnt_present_flag
		int transform_8x8_mode;         ///< transform_8x8_mode_flag
		uint8_t scaling_matrix4[6][16];
		uint8_t scaling_matrix8[6][64];
		uint8_t chroma_qp_table[2][QP_MAX_NUM + 1];  ///< pre-scaled (with chroma_qp_index_offset) version of qp_table
		int chroma_qp_diff;

		uint32_t dequant4_buffer[6][QP_MAX_NUM + 1][16];
		uint32_t dequant8_buffer[6][QP_MAX_NUM + 1][64];
		uint32_t(*dequant4_coeff[6])[16];
		uint32_t(*dequant8_coeff[6])[64];
	} PPS;


	/**
	 * Decode SPS
	 */
	int h264_decode_sps(const uint8_t *data, uint32_t size, SPS *sps);

	/**
	 * Decode PPS
	 */
	int h264_decode_pps(const uint8_t *data, uint32_t size, const SPS *sps, PPS *pps);

	/**
	 * compute profile from sps
	 */
	int h264_get_profile(const SPS *sps);

	/**
	 * compute width from sps
	 */
	int h264_get_width(const SPS *sps);

	/**
	 * compute height from sps
	 */
	int h264_get_height(const SPS *sps);

	/**
	 * compute framerate from sps
	 */
	int h264_get_framerate(const SPS *sps);

#ifdef __cplusplus
}
#endif

#endif /* H264_PS_H */