/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#include "StdAfx.h"
#include "packet.h"
//#include "socket.h"
#include <string.h>


OutPacket::OutPacket()
{
	reset();
}

OutPacket &OutPacket::operator <<(uint8 b)
{
	if (getBytesLeft() >= sizeof(b))
		*cursor++ = b;

	return (*this);
}

OutPacket &OutPacket::operator <<(uint16 w)
{
	if (getBytesLeft() >= sizeof(w)) {
		*(uint16 *) cursor = htons(w);
		cursor += sizeof(w);
	}
	return (*this);
}

OutPacket &OutPacket::operator <<(uint32 dw)
{
	if (getBytesLeft() >= sizeof(dw)) {
		*(uint32 *) cursor = htonl(dw);
		cursor += sizeof(dw);
	}
	return (*this);
}

OutPacket &OutPacket::operator <<(const char *str)
{
	writeData(str, strlen(str) + 1);
	return *this;
}

void OutPacket::writeData(const char *data, int n)
{
	if (n <= getBytesLeft() - 2) {
		operator <<((uint16) n);

		memcpy(cursor, data, n);
		cursor += n;
	}
}

OutPacket &OutPacket::operator <<(OutPacket &p)
{
	int n = p.getLength();
	if (n <= getBytesLeft()) {
		memcpy(cursor, p.data, n);
		cursor += n;
	}
	return *this;
}


InPacket::InPacket(unsigned char *data, int n)
{
	this->data = cursor = data;
	length = n;
}

InPacket &InPacket::operator >>(uint8 &b)
{
	if (getBytesLeft() >= sizeof(b))
		b = *cursor++;
	else
		b = 0;

	return (*this);
}

InPacket &InPacket::operator >>(uint16 &w)
{
	if (getBytesLeft() >= sizeof(w)) {
		w = ntohs(*(uint16 *) cursor);
		cursor += sizeof(w);
	} else
		w = 0;
	return (*this);
}

InPacket &InPacket::operator >>(uint32 &dw)
{
	if (getBytesLeft() >= sizeof(dw)) {
		dw = ntohl(*(uint32 *) cursor);
		cursor += sizeof(dw);
	} else
		dw = 0;
	return (*this);
}

InPacket &InPacket::operator >>(uint64 &dw)
{
	if (getBytesLeft() >= sizeof(dw)) {
		dw = *(uint64 *) cursor;
		cursor += sizeof(dw);
	} else
		dw = 0;
	return (*this);
}

InPacket &InPacket::operator >>(const char *&str)
{
	int n;
	unsigned char *data = readData(n);

	if (n > 0 && !data[n - 1])
		str = (const char *)data;
	else
		str = "";

	return (*this);
}

InPacket &InPacket::operator >>(std::string &str)
{
	const char *p;
	*this >> p;
	str = p;
	return (*this);
}

void* InPacket::readData1(int nLen)
{
	void* lpData;

	if (getBytesLeft() >= nLen) 
	{
		lpData = (void*)cursor;
		cursor += nLen;
	} 
	else
		lpData = NULL;
	return lpData;
}

unsigned char *InPacket::readData(int &n)
{
	unsigned char *data = NULL;
	n = 0;

	uint16 len;
	operator >>(len);

	if (len && getBytesLeft() >= len) {
		n = len;
		data = cursor;
		cursor += len;
	}
	return data;
}


