/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#ifndef _UDP_PACKET_H
#define _UDP_PACKET_H

#include "linux_compat.h"
#include <time.h>
#include "packet.h"
//#include "ecsimsocket.h"


class TCPOutPacket : public OutPacket {
public:
	TCPOutPacket();

	time_t expire;
	int attempts;
	uint32 cmd;
	uint16 seq;
	uint32 CallID;
};


class TCPInPacket : public InPacket {
public:
	TCPInPacket(unsigned char *data, int len, int nType);

	UDP_PACKET_HDR header;
};

#endif
