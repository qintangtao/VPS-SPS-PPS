#ifndef BS_H
#define BS_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bs_s bs_t;

bs_t* bs_new(uint8_t* buf, size_t size);
void bs_free(bs_t* s);

int bs_bits_left(bs_t *s);

void bs_write_u(bs_t *s, int i_count, uint32_t i_bits);
void bs_write_u1(bs_t *s, uint32_t i_bit);
uint32_t bs_read_u(bs_t *s, int i_count);
uint32_t bs_read_u1(bs_t *s);

// encoder
void bs_write_ue(bs_t *s, uint32_t val);
void bs_write_se(bs_t *s, int32_t val);
void bs_write_te(bs_t *s, int x, int32_t val);

// decoder
uint32_t bs_read_ue(bs_t *s);
int32_t bs_read_se(bs_t *s);
uint32_t bs_read_te(bs_t *s, int x);

#ifdef __cplusplus
}
#endif

#endif /* BS_H */