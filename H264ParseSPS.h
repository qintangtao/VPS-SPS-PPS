#ifndef H264ParseSPS_H
#define H264ParseSPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		uint32_t profile_idc;
		uint32_t level_idc;
		uint32_t width;
		uint32_t height;
		uint32_t fps;       //SPS中可能不包含FPS信息
	} sps_info_struct;


	/**
	 解析SPS数据信息

	 @param data SPS数据内容，需要Nal类型为0x7数据的开始(比如：67 42 00 28 ab 40 22 01 e3 cb cd c0 80 80 a9 02)
	 @param dataSize SPS数据的长度
	 @param info SPS解析之后的信息数据结构体
	 @return success:1，fail:0

	 */
	int h264_parse_sps(const uint8_t *data, uint32_t dataSize, sps_info_struct *info);


#ifdef __cplusplus
}
#endif

#endif /* H264ParseSPS_H */