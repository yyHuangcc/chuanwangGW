/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#include "tcppacket.h"
#include "md5.h"

// Hash a string to md5
static void md5String(char result[33], const char *str,int nlen)
{
	static char table[] = "0123456789abcdef";

	md5_context ctx;
	uint8 digest[16];

	md5_starts(&ctx);
	md5_update(&ctx, (uint8 *) str, nlen);
	md5_finish(&ctx, digest);

	for (int i = 0; i < 16; i++) {
		*result++ = table[digest[i] >> 4];
		*result++ = table[digest[i] & 0xf];
	}
	*result = '\0';
}

TCPOutPacket::TCPOutPacket()
{
	expire = 0;
	attempts = 0;
	cmd = 0;
	seq = 0;
}

TCPInPacket::TCPInPacket(unsigned char *data, int len, int nType)
	: InPacket(data, len)
{
	if(nType==1)
	{
		*this >> header.cmd;
	}
	else
	{
		*this >> header.ver >> header.cmd;
		*this >> header.sid >> header.reserved;
	}
}
