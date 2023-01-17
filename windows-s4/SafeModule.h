#pragma once

#include<windows.h>
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
	BYTE  kn_e;
	BYTE  kn_d;
	bool enable;
};

