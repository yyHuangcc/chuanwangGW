/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#ifndef _ECSIM_TYPES_H_
#define _ECSIM_TYPES_H_

#include <string>
#include <list>
#include <vector>
#include <bitset>

using namespace std;

#include "linux_compat.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint32_t uint32;
typedef double uint64;

// Login reply code
enum {
	LOGIN_SUCCESS,
	LOGIN_INVALID_USER,
	LOGIN_WRONG_PASSWD,
	LOGIN_ERROR_UNKNOWN,
};

// send request reply code
enum {
	SEND_DISABLE,
	SEND_ENABLE,
	SEND_USELCR,
};

enum {
	GW_LOGIN = 1,
	GW_LOGOUT,
	GW_FAX_REQUESTSEND,
	GW_FAX_SEND,
	GW_FAX_STOPSEND,
	GW_KEEPALIVE,
	GW_FAX_REQUESTRECV,
	GW_FAX_RECV,
	GW_FAX_STOPRECV,
	GW_LCR_QUERY = 20,
	GW_LCR_CANCEL,
	GW_LCR_REPORT,
	GW_FAX_EXT,
	GW_SEND_MSG,
};


#define IS_SYSMSG(msg)	\
	(((msg).type == MSG_AUTH_ACCEPTED) ||	\
	((msg).type == MSG_AUTH_REQUEST) ||	\
	((msg).type == MSG_AUTH_REJECTED) ||	\
	((msg).type == MSG_ADDED) || \
	((msg).type == MSG_ANNOUNCE))


#pragma pack(1)
struct PAG_HEADER {
	BYTE	byHeader1;
	BYTE	byHeader2;
	int		nVer;
	int		nLength;
};

struct GW_HEADER {
	BYTE	byHeader1;
	BYTE	byHeader2;
	BYTE	byVer;
	int		nLength;
};

struct UDP_PACKET_HDR {
	uint8 ver;
	uint8 cmd;
	uint16 sid;
	uint32 reserved;
	uint16 seq;
	uint16 ackseq;
	//uint32 uin;
};
#pragma pack()

struct FAX_SESSION {
	int		nType;
	bool	bIsLogin;
	bool	bIsEnd;
	HANDLE	hSendFile;
	DWORD	dwFileSize;
};

struct FAX_RECVLIST {
	uint8	byType;
	uint8	nFAXType;
	char	strTID[40];
	char	strIP[20];
	uint32	nPort;
	uint32	nSuccess;
	HANDLE	hRecvFile;
	char	strCountryCode[10];
	char	strAreaCode[10];
	char	strFaxCode[20];
	char	strExtCode[10];
	char	strCallerID[30];
	char	strCSID[21];
	char	strFileName[MAX_PATH];
};

typedef list<string> 			StrList;
typedef list<FAX_RECVLIST *>	RecvFAXList;

#define TCP_VER			1

#endif
