/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#if !defined _CRC32_
#define _CRC32_

#pragma once

#include "linux_compat.h"

typedef unsigned long ULONG;
typedef unsigned long       DWORD;

class CRC32
{
public:
    CRC32();
	int Get_CRC(char* csData, DWORD dwSize); // Creates a CRC from a string buffer

protected:
	ULONG crc32_table[256]; // Lookup table arrays

	ULONG Reflect(ULONG ref, char ch); // Reflects CRC bits in the lookup table
};

#endif