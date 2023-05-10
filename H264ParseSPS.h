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
		uint32_t fps;       //SPS�п��ܲ�����FPS��Ϣ
	} sps_info_struct;


	/**
	 ����SPS������Ϣ

	 @param data SPS�������ݣ���ҪNal����Ϊ0x7���ݵĿ�ʼ(���磺67 42 00 28 ab 40 22 01 e3 cb cd c0 80 80 a9 02)
	 @param dataSize SPS���ݵĳ���
	 @param info SPS����֮�����Ϣ���ݽṹ��
	 @return success:1��fail:0

	 */
	int h264_parse_sps(const uint8_t *data, uint32_t dataSize, sps_info_struct *info);


#ifdef __cplusplus
}
#endif

#endif /* H264ParseSPS_H */