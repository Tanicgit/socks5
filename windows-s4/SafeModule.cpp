#include "SafeModule.h"
#include <string.h>

#define U8_LEFT(a,n) ((a<<n)|(a>>(8-n)))
#define U8_RIGHT(a,n) ((a>>n)|(a<<(8-n)))


SafeModule::SafeModule()
{
	enable = false;
}

SafeModule::~SafeModule()
{
}

int SafeModule::encryp(BYTE* data, int len)
{
	BYTE n = 0;
	if (!enable)
		return 0;

	for (int i = 0;i < len;i++)
	{
		for (int j = 0;j < 16;j++)
		{
			n = key[j] & 7;
			data[i] ^= key[j];
			if (n & 1)
			{
				U8_LEFT(data[i], n);
			}
			else
			{
				U8_RIGHT(data[i], n);
			}
		}
	}
	return 0;
}

int SafeModule::decrypt(BYTE* data, int len)
{
	BYTE n = 0;

	if (!enable)
		return 0;

	for (int i = 0;i < len;i++)
	{
		for (int j = 0;j < 16;j++)
		{
			n = key[j] & 7;
			if (n & 1)
			{
				U8_RIGHT(data[i], n);
			}
			else
			{
				U8_LEFT(data[i], n);
			}
			data[i] ^= key[j];
		}
	}
	return 0;
}

int SafeModule::setKey(BYTE keyValue[16])
{
	memcpy(key, keyValue, 16);
	enable = true;
	return 0;
}

int SafeModule::getKey(BYTE keyValue[16])
{
	memcpy(keyValue, key, 16);
	return 0;
}





