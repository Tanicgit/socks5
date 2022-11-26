#ifndef __RKFIFO_H
#define __RKFIFO_H
 

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>



#define ESIZE 1

 
//#define	ENOMEM		12	/* Out of Memory */
//#define	EINVAL		22	/* Invalid argument */
//#define ENOSPC		28	/* No space left on device */
 
class rkfifo{
public:
	rkfifo(uint32_t size,uint32_t esize);
	~rkfifo();
	unsigned int rkfifo_in(const void *buf, unsigned int len);
	unsigned int rkfifo_out_peek(void *buf, unsigned int len);
	unsigned int rkfifo_out_peek2(unsigned int len);
	unsigned int rkfifo_out(void *buf, unsigned int len);
	unsigned int kfifo_unused();
	
private:
	void kfifo_copy_in(const void *src,	unsigned int len, unsigned int off);
	void kfifo_copy_out(void *dst,unsigned int len, unsigned int off);
	
	/*
	unsigned int min(unsigned int a,unsigned int b) 
	{
		return	a>b ? b : a;
	}
	*/
	unsigned int	c_in;
	unsigned int	c_out;
	unsigned int	c_mask;
	unsigned int	c_esize;
	void		*c_data;

	
};
 
#endif
 
