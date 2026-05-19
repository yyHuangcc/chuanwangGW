#ifndef _CRC32_
#define _CRC32_

#pragma once

#include "linux_compat.h"
#include <stdint.h>

class CRC32
{
public:
    CRC32();
    uint32_t Get_CRC(char* csData, DWORD dwSize);

protected:
    uint32_t crc32_table[256];
    uint32_t Reflect(uint32_t ref, char ch);
};

#endif