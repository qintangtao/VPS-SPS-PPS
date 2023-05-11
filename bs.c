#include "bs.h"
#include <stdlib.h>

typedef struct bs_s
{
	uint8_t *p_start;
	uint8_t *p;
	uint8_t *p_end;
	int     i_left;    /* i_count number of available bits */
} bs_t;

static inline void bs_init(bs_t *s, void *p_data, size_t i_data)
{
	s->p = s->p_start = (uint8_t*)p_data;
	s->p_end = (uint8_t*)p_data + i_data;
	s->i_left = 8;
}

bs_t* bs_new(uint8_t* buf, size_t size)
{
	bs_t* s = (bs_t*)malloc(sizeof(bs_t));
	if (s)
		bs_init(s, buf, size);
	return s;
}
void bs_free(bs_t* s)
{
	free(s);
}

int bs_bits_left(bs_t *s)
{
	return (s->p_end - s->p);
	//return gb->size_in_bits - get_bits_count(gb);
}

void bs_write_u(bs_t *s, int i_count, uint32_t i_bits)
{
	if (s->p >= s->p_end - 4)
		return;
	while (i_count > 0)
	{
		if (i_count < 32)
			i_bits &= (1 << i_count) - 1;
		if (i_count < s->i_left)
		{
			*s->p = (*s->p << i_count) | i_bits;
			s->i_left -= i_count;
			break;
		}
		else
		{
			*s->p = (*s->p << s->i_left) | (i_bits >> (i_count - s->i_left));
			i_count -= s->i_left;
			s->p++;
			s->i_left = 8;
		}
	}
}

void bs_write_u1(bs_t *s, uint32_t i_bit)
{
	if (s->p < s->p_end)
	{
		*s->p <<= 1;
		*s->p |= i_bit;
		s->i_left--;
		if (s->i_left == 0)
		{
			s->p++;
			s->i_left = 8;
		}
	}
}

void bs_write_ue(bs_t *s, uint32_t val)
{
	int i_size = 0;
	static const int i_size0_255[256] =
	{
		1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
	};

	if (val == 0)
	{
		bs_write_u1(s, 1);
	}
	else
	{
		unsigned int tmp = ++val;

		if (tmp >= 0x00010000)//2^16=256*256
		{
			i_size += 16;
			tmp >>= 16;
		}
		if (tmp >= 0x100)//2^8=256
		{
			i_size += 8;
			tmp >>= 8;
		}
		i_size += i_size0_255[tmp];

		bs_write_u(s, 2 * i_size - 1, val);
	}
}

void bs_write_se(bs_t *s, int32_t val)
{
	bs_write_ue(s, val <= 0 ? -val * 2 : val * 2 - 1);
}


void bs_write_te(bs_t *s, int x, int32_t val)
{
	if (x == 1)
	{
		bs_write_u1(s, 1 & ~val);
	}
	else if (x > 1)
	{
		bs_write_ue(s, val);
	}
}


uint32_t bs_read_u(bs_t *s, int i_count)
{
	static uint32_t i_mask[33] = { 0x00,
								 0x01,      0x03,      0x07,      0x0f,
								 0x1f,      0x3f,      0x7f,      0xff,
								 0x1ff,     0x3ff,     0x7ff,     0xfff,
								 0x1fff,    0x3fff,    0x7fff,    0xffff,
								 0x1ffff,   0x3ffff,   0x7ffff,   0xfffff,
								 0x1fffff,  0x3fffff,  0x7fffff,  0xffffff,
								 0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
								 0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff };
	int      i_shr;
	uint32_t i_result = 0;

	while (i_count > 0)
	{
		if (s->p >= s->p_end)
		{
			break;
		}

		if ((i_shr = s->i_left - i_count) >= 0)
		{
			/* more in the buffer than requested */
			i_result |= (*s->p >> i_shr)&i_mask[i_count];
			s->i_left -= i_count;
			if (s->i_left == 0)
			{
				s->p++;
				s->i_left = 8;
			}
			return(i_result);
		}
		else
		{
			/* less in the buffer than requested */
			i_result |= (*s->p&i_mask[s->i_left]) << -i_shr;
			i_count -= s->i_left;
			s->p++;
			s->i_left = 8;
		}
	}
	return(i_result);
}

uint32_t bs_read_u1(bs_t *s)
{
	if (s->p < s->p_end)
	{
		unsigned int i_result;

		s->i_left--;
		i_result = (*s->p >> s->i_left) & 0x01;
		if (s->i_left == 0)
		{
			s->p++;
			s->i_left = 8;
		}
		return i_result;
	}

	return 0;
}

uint32_t bs_read_ue(bs_t *s)
{
	int i = 0;

	while (bs_read_u1(s) == 0 && s->p < s->p_end && i < 32)
	{
		i++;
	}
	
	return((1 << i) - 1 + bs_read_u(s, i));
}

int32_t bs_read_se(bs_t *s)
{
	int32_t val = bs_read_ue(s);

	return val & 0x01 ? (val + 1) / 2 : -(val / 2);
}

uint32_t bs_read_te(bs_t *s, int x)
{
	if (x == 1)
	{
		return 1 - bs_read_u1(s);
	}
	else if (x > 1)
	{
		return bs_read_ue(s);
	}
	return 0;
}