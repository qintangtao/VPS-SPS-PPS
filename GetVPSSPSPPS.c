#include "GetVPSSPSPPS.h"
#include <string.h>

static uint8 * avc_find_startcode_internal(uint8 *p, uint8 *end)
{
	uint8 *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
		{
			return p;
		}
	}

	for (end -= 3; p < end; p += 4)
	{
		uint32 x = *(const uint32*)p;

		if ((x - 0x01010101) & (~x) & 0x80808080)
		{ // generic
			if (p[1] == 0)
			{
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0)
			{
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
		{
			return p;
		}
	}

	return end + 3;
}

static uint8 * avc_find_startcode(uint8 *p, uint8 *end)
{
	uint8 *out = avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
	{
		out--;
	}

	return out;
}

int GetH265VPSandSPSandPPS(uint8 *p_data, int len, uint8 *_vps, int *_vpslen, uint8 *_sps, int *_spslen, uint8 *_pps, int *_ppslen)
{
	uint8 nalu_t; int nalu_len;
	uint8 *r, *end = p_data + len;
	*_vpslen = 0, *_spslen = 0; *_ppslen = 0;

	r = avc_find_startcode(p_data, end);

	while (r < end)
	{
		uint8 *r1;

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

int GetH264SPSandPPS(uint8 *p_data, int len, uint8 *_sps, int *_spslen, uint8 *_pps, int *_ppslen)
{
	uint8 nalu_t; int nalu_len;
	uint8 *r, *end = p_data + len;
	*_spslen = 0; *_ppslen = 0;

	r = avc_find_startcode(p_data, end);

	while (r < end)
	{
		uint8 *r1;

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
