#include "GetVPSSPSPPS.h"
#include <string.h>
#include "h2645_util.h"


int GetH265VPSandSPSandPPS(uint8_t *p_data, int len, uint8_t *_vps, int *_vpslen, uint8_t *_sps, int *_spslen, uint8_t *_pps, int *_ppslen)
{
	uint8_t nalu_t; int nalu_len;
	uint8_t *r, *end = p_data + len;
	*_vpslen = 0, *_spslen = 0; *_ppslen = 0;

	r = avc_find_startcode(p_data, end);

	while (r < end)
	{
		uint8_t *r1;

		while (!*(r++));
		r1 = avc_find_startcode(r, end);
		nalu_t = (r[0] >> 1) & 0x3F;
		nalu_len = (int)(r1 - r);

		if (nalu_t == 32) {
			memcpy(_vps, r, nalu_len);
			*_vpslen = nalu_len;
		}
		else if (nalu_t == 33) {
			memcpy(_sps, r, nalu_len);
			*_spslen = nalu_len;
		}
		else if (nalu_t == 34) {
			memcpy(_pps, r, nalu_len);
			*_ppslen = nalu_len;
		}

		if (*_vpslen > 0 && *_spslen > 0 && *_ppslen > 0)
			break;

		r = r1;
	}

	return (*_vpslen > 0 && *_spslen > 0 && *_ppslen > 0) ? 0 : -1;
}

int GetH264SPSandPPS(uint8_t *p_data, int len, uint8_t *_sps, int *_spslen, uint8_t *_pps, int *_ppslen)
{
	uint8_t nalu_t; int nalu_len;
	uint8_t *r, *end = p_data + len;
	*_spslen = 0; *_ppslen = 0;

	r = avc_find_startcode(p_data, end);

	while (r < end)
	{
		uint8_t *r1;

		while (!*(r++));
		r1 = avc_find_startcode(r, end);
		nalu_t = r[0] & 0x1F;
		nalu_len = (int)(r1 - r);

		if (nalu_t == 7) {
			memcpy(_sps, r, nalu_len);
			*_spslen = nalu_len;
		}
		else if (nalu_t == 8) {
			memcpy(_pps, r, nalu_len);
			*_ppslen = nalu_len;
		}

		if ((*_spslen > 0 && *_ppslen > 0))
			break;

		r = r1;
	}

	return (*_spslen > 0 && *_ppslen > 0) ? 0 : -1;
}
