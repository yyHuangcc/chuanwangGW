/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#ifndef _PACKET_H
#define _PACKET_H

#include "linux_compat.h"
#include "gwtypes.h"
#include <time.h>
#include <string>

#define MAX_PACKET_SIZE		4096//1024


class OutPacket {
public:
	OutPacket();

	void reset() { cursor = data; }
	int getBytesLeft() { return (MAX_PACKET_SIZE - getLength()); }

	OutPacket &operator <<(uint8 b);
	OutPacket &operator <<(uint16 w);
	OutPacket &operator <<(uint32 dw);
	OutPacket &operator <<(const char *str);
	OutPacket &operator <<(const std::string &str) {
		return (*this << str.c_str());
	}
	OutPacket &operator <<(OutPacket &p);
	void writeData(const char *data, int n);

	const char *getData() {	return data; }
	int getLength() { return (cursor - data); }

	char data[MAX_PACKET_SIZE];
	char *cursor;
};


class InPacket {
public:
	InPacket(unsigned char *data, int n);

	InPacket &operator >>(uint8 &b);
	InPacket &operator >>(uint16 &w);
	InPacket &operator >>(uint32 &dw);
	InPacket &operator >>(uint64 &dw);
	InPacket &operator >>(const char *&str);
	InPacket &operator >>(std::string &str);
	unsigned char *readData(int &n);
	LPBYTE getData();
	void* readData1(int nLen);

	int getBytesLeft() {
		return (length - (cursor - data));
	}

	unsigned char *cursor;

private:
	unsigned char *data;
	int length;
};

#endif