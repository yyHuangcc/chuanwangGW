#include "StdAfx.h"
#include "crc32.h"
#include <cstring>

CRC32::CRC32()
{
    uint32_t ulPolynomial = 0x04c11db7;
    for(int i = 0; i <= 0xFF; i++)
    {
        crc32_table[i] = Reflect(i, 8) << 24;
        for (int j = 0; j < 8; j++)
            crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
        crc32_table[i] = Reflect(crc32_table[i], 32);
    }
}

uint32_t CRC32::Reflect(uint32_t ref, char ch)
{
    uint32_t value = 0;
    for(int i = 1; i < (ch + 1); i++)
    {
        if(ref & 1)
            value |= 1 << (ch - i);
        ref >>= 1;
    }
    return value;
}

uint32_t CRC32::Get_CRC(char* csData, DWORD dwSize)
{
    uint32_t crc = 0xffffffff;
    int len = dwSize;
    unsigned char* buffer = (unsigned char*)csData;
    while(len--)
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *buffer++];
    return crc ^ 0xffffffff;
}