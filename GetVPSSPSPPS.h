#ifndef GETVPSSPSPPS_H
#define GETVPSSPSPPS_H
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

//输入的pbuf必须包含start code(00 00 00 01)

int GetH265VPSandSPSandPPS(uint8_t *pbuf, int bufsize, uint8_t *_vps, int *_vpslen, uint8_t *_sps, int *_spslen, uint8_t *_pps, int *_ppslen);
int GetH264SPSandPPS(uint8_t *pbuf, int bufsize, uint8_t *_sps, int *_spslen, uint8_t *_pps, int *_ppslen);

#ifdef __cplusplus
}
#endif

#endif /* GETVPSSPSPPS_H */