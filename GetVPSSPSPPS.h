#ifndef GETVPSSPSPPS_H
#define GETVPSSPSPPS_H


//输入的pbuf必须包含start code(00 00 00 01)

typedef unsigned char uint8;
typedef unsigned int uint32;

#ifdef __cplusplus
extern "C" {
#endif


int GetH265VPSandSPSandPPS(uint8 *pbuf, int bufsize, uint8 *_vps, int *_vpslen, uint8 *_sps, int *_spslen, uint8 *_pps, int *_ppslen);
int GetH264SPSandPPS(uint8 *pbuf, int bufsize, uint8 *_sps, int *_spslen, uint8 *_pps, int *_ppslen);

#ifdef __cplusplus
}
#endif

#endif /* GETVPSSPSPPS_H */