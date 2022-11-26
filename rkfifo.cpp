#include "rkfifo.h"

#define is_power_of_2(x)	((x) != 0 && (((x) & ((x) - 1)) == 0))
 
static uint32_t CLZ_32(uint32_t n)
{
    int ret=0;
    uint32_t tmp = ~n;
    while(tmp&0x80000000)
    {
        tmp <<= 1;
        ret++;
    }
    return ret;
}
uint32_t rounddown_pow_of_two(uint32_t n)
{
    uint32_t ret;
    if(n==0){
        return 0;
    }
 
    if((n & (n-1)) == 0){
        return n;
    }
 
    ret = CLZ_32(n);
    return 1<<(32-ret); 
}

unsigned int inline min(unsigned int a,unsigned int b) 
{
	return	a>b ? b : a;
}



rkfifo::rkfifo(uint32_t size, uint32_t esize)
{
	size /= esize;
	if(size < 2)
	{
		size = 2;
	}
	
	if (!is_power_of_2(size)){
		size = rounddown_pow_of_two(size);	
	}

	c_in = 0;
	c_out = 0;
	c_esize = esize;
 
	c_data = malloc(size * esize);
 
	if (!c_data) {
		c_mask = 0;
	}
	c_mask = size - 1;

}

rkfifo::~rkfifo()
{
	if(c_data)
	{
		free(c_data);
		c_data = NULL;
	}
	c_data = 0;
	c_in = 0;
	c_out = 0;
	c_esize = 0;
	c_data = NULL;
	c_mask = 0;

}


unsigned int rkfifo::kfifo_unused()
{
	return (c_mask + 1) - (c_in - c_out);
}

void rkfifo::kfifo_copy_in(const void *src,		unsigned int len, unsigned int off)
{
	unsigned int size = c_mask + 1;
	unsigned int esize = c_esize;
	unsigned int l;
 
	off &= c_mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);
 
	memcpy((uint8_t*)c_data + off, src, l);
	memcpy(c_data, (uint8_t*)src + l, len - l);
}

unsigned int rkfifo::rkfifo_in(const void *buf, unsigned int len)
{
	unsigned int l;
 
	l = kfifo_unused();
	if (len > l)
		len = l;
 
	kfifo_copy_in(buf, len, c_in);
	c_in += len;
	return len;
}

void rkfifo::kfifo_copy_out(void *dst,unsigned int len, unsigned int off) 
{
	unsigned int size = c_mask + 1;
	unsigned int esize = c_esize;
	unsigned int l;
 
	off &= c_mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);
 
	memcpy(dst, (uint8_t*)c_data + off, l);
	memcpy((uint8_t*)dst + l, c_data, len - l);
}
 
unsigned int rkfifo::rkfifo_out_peek(void *buf, unsigned int len)
{
	unsigned int l;
 
	l = c_in - c_out;
	if (len > l)
		len = l;
	
 	if(buf != NULL && len>0)
		kfifo_copy_out(buf, len, c_out);
	return len;
}
 
 
unsigned int rkfifo::rkfifo_out(void *buf, unsigned int len)
{
	len = rkfifo_out_peek(buf, len);
	c_out += len;
	return len;
}

unsigned int rkfifo::rkfifo_out_peek2(unsigned int len)
{
	c_out += len;
	return len;
}


