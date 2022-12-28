#ifndef __SAFEMODULE__H
#define __SAFEMODULE__H
#include <stdint.h>
#define BYTE unsigned char
class SafeModule
{
public:
	SafeModule();
	~SafeModule();


	int encryp(BYTE* data, int len);
	int decrypt(BYTE* data, int len);

	int setKey(BYTE keyValue[16]);
	int getKey(BYTE keyValue[16]);

private:
	BYTE  key[16];
	bool enable;
};

#endif

