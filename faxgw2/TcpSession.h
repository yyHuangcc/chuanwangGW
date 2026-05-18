/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#ifndef _TCP_SESSION_H
#define _TCP_SESSION_H

#include "linux_compat.h"
#include "gwtypes.h"

//#include "ecsimsession.h"
//#include "ecsimsocket.h"

enum {
	TCP_LOGIN = 1,
	TCP_LOGOUT,
	TCP_KEEPALIVE,
	TCP_FAX_REQUESTSEND,
	TCP_FAX_SEND,
	TCP_FAX_STOPSEND,
	TCP_FAX_REQUESTRECV,
	TCP_FAX_RECV,
	TCP_FAX_STOPRECV,
	TCP_LCR_QUERY = 20,
	TCP_LCR_CANCEL,
	TCP_LCR_REPORT,
	TCP_FAX_EXT,

	// TCP_SRV_USER_ONLINE = 0x0100,
	// TCP_SRV_USER_OFFLINE,
	// TCP_SRV_MULTI_ONLINE,
	// TCP_SRV_STATUS_CHANGED,
	// TCP_SRV_MESSAGE,
	// TCP_SRV_SEARCH,
	// TCP_SRV_UPDATE,
};

#define SEND_TIMEOUT			5

//class IcqLink;
class TCPOutPacket;
class TCPInPacket;

class TcpSession {
public:
	//TcpSession(/*IcqLink *link, */const char *name, uint32 uin);
	TcpSession();
	virtual ~TcpSession();

	virtual bool onReceive(TCPInPacket &in);

	int receive(char *data, int n, sockaddr_in *pSockAddr);

	void	connect(const char* strSrvAdd, int nPort);
	void	connect(uint32 ip, uint16 port);
	void	Close();
	int		onLogin();
	int		onKeepAlive();
	int		onSendRequest(const char* strCountryCode,const char* strAreaCode,const char* strPhoneCode,const char* strExtCode,DWORD dwSize,const char* strCSID);
	int		onLCRQuery(char* strTID,uint8 bycode,uint8 byResult);
	int		onLCRCancel(char* strTID,uint8 bycode,uint8 byResult);
	int		onLCRReport(char* strTID,uint8 bycode,uint8 byResult);
	int		onRecvRequest(const char* strCode);
	uint16	sendPacket(TCPOutPacket *, sockaddr_in* destAddr = NULL);
	bool	onAck(uint16 seq,uint16& cmd,uint32& dwCallID);
	int		ReadData(char* lpBuff,int nSize);

	void	onLoginReply(TCPInPacket &in);
	void	onKeepAliveReply(TCPInPacket &in);
	void	onSendRequestReply(TCPInPacket &in);
	void	onRecvRequestReply(TCPInPacket &in);
	void	onLCRQueryReply(TCPInPacket &in);
	void	onLCRCancelReply(TCPInPacket &in);
	void	onLCRReportReply(TCPInPacket &in);
	void	onSrvUpdate(TCPInPacket &in);

	int			m_nWaitAlive;
	uint32		m_sessionCount;
	uint32		m_ServerID;
	BOOL		m_bIsLogin;
	char		m_strSrvAdd[128];
	int			m_nPort;

protected:
	virtual void onSendError(TCPOutPacket *p);

	void sendDirect(TCPOutPacket *out);
	void initSession();
	void createPacket(TCPOutPacket &out, uint16 cmd, uint16 seq, uint16 ackseq = 0);
	TCPOutPacket *createPacket(uint8 cmd, uint16 ackseq = 0);
	void sendAckPacket(uint16 seq);
	bool setWindow(uint16 seq);

	static string destHost;
	static sockaddr_in proxyAddr;

	uint32	sid;
	uint16	sendSeq;
	uint16	recvSeq;
	uint32	window;
	SOCKET	m_tcpSock;
	uint32	m_realIP;
	uint32	m_ServerIP;

	sockaddr_in m_destAddr;
	sockaddr_in m_inAddr;
};

#endif
