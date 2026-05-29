/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#include "StdAfx.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include <unistd.h>

#include "gwtypes.h"
#include "TcpSession.h"
#include "tcppacket.h"
#include "crc32.h"
#include "d3des.h"
#include "rwini.h"
#include "Log.h"

#define MAX_SEND_ATTEMPTS		2
#define	ECSIM_CLIENT_UDPPORT	4201
typedef unsigned long	xLONG32;
typedef unsigned char	xLONG8;	

#define	CMD_ACK		1

extern bool CheckPacket(PBYTE pbyPacket);
extern int SendLCROper(const char* strUUID,BYTE byLCRcmd,BYTE byLCRcode,BYTE byLCRResult);

extern	RecvFAXList				g_lsRecvFax;
extern	uint16					g_nReceive;
extern	Log						g_log;
extern	HANDLE					g_hRecvEvent;
extern	HANDLE					g_hConnectEvent;
extern	TcpSession				g_tcpSession;
extern	char					g_strFilePath[MAX_PATH]; 

void ReceiveFax(void* lpParam);
void SendFax(void* lpParam);

extern unsigned char fixedkey[16];// = {23,82,107,6,35,78,88,7,43,96,5,114,37,89,25,53};

BYTE		g_bySendCode;
BYTE		g_byLCRCode;
BYTE		g_byQueryResult;
BYTE		g_byLCRCancel;
BYTE		g_byCancelResult;
BYTE		g_byLCRReport;
BYTE		g_byReportResult;
BYTE		g_bySystem;
char		g_strQueryUUID[40];
char		g_strCancelUUID[40];
char		g_strReportUUID[40];
char		g_strUUID[40];
char		g_strIP[20];
uint32		g_nPort;
uint32		g_nFileSize;
HANDLE		g_hSendRequest;
HANDLE		g_hLCRQuery;
HANDLE		g_hLCRCancel;
HANDLE		g_hLCRReport;

CRITICAL_SECTION	g_csRecvList;
string TcpSession::destHost;
sockaddr_in TcpSession::proxyAddr;
extern  SYS_PARAM		g_sysParam;
extern  char			g_strVersion[30];

void Output(char* strString,int nParam)
{
	//CString		szErr;
	//szErr.Format(strString,nParam);
	//OutputDebugString(szErr);
}

void Output(char* strString,int nParam1,int nParam2)
{
	//CString		szErr;
	//szErr.Format(strString,nParam1,nParam2);
	//OutputDebugString(szErr);
}

//TcpSession::TcpSession(/*IcqLink *link,*/ const char *name, uint32 uin)
/*{
	memset(&m_destAddr, 0, sizeof(m_destAddr));

	initSession();
}*/

TcpSession::TcpSession()
{
	memset(&m_destAddr, 0, sizeof(m_destAddr));

	initSession();
	m_bIsLogin = FALSE;
	m_nWaitAlive = 0;
	m_strSrvAdd[0]=0;
	m_nPort=0;
	g_bySystem = 0;
}

TcpSession::~TcpSession()
{
	m_bIsLogin = FALSE;
	m_nWaitAlive = 0;
	m_strSrvAdd[0]=0;
	m_nPort=0;
}

void TcpSession::Close()
{
    //shutdown(m_tcpSock,0);
	closesocket(m_tcpSock);
}

// void TcpSession::connect(const char* strSrvAdd, int nPort)
// {
// 	//SOCKET	sTest;
// 	//sockaddr_in TestAddr;
//     struct hostent    *host = NULL;

// 	lstrcpyn(m_strSrvAdd,strSrvAdd,128);
// 	m_nPort = nPort;

// 	memset(&m_destAddr, 0, sizeof(m_destAddr));
// 	m_destAddr.sin_family = AF_INET;
// 	if ((m_destAddr.sin_addr.s_addr = inet_addr(strSrvAdd)) == INADDR_NONE)
// 	{
// 		host = gethostbyname(strSrvAdd);
// 		if(host != NULL)
// 		{
// 			memcpy(&m_destAddr.sin_addr, host->h_addr_list[0],
// 			    host->h_length);
// 		}
// 		//need check function success?
// 	}
// 	m_destAddr.sin_port = htons((uint16)nPort);

//     m_tcpSock = socket(AF_INET, SOCK_STREAM, 0);
//     if (m_tcpSock >= 0)
// 	{
// 		if (::connect(m_tcpSock, (struct sockaddr *)&m_destAddr, 
// 			sizeof(m_destAddr)) < 0)
// 		{
// 			g_bySystem = 0;
// 			return;
// 		}
// 	}

// 	SetEvent(g_hRecvEvent);
// 	//Get my ip to server.
// 	/*sTest = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
// 	if(INVALID_SOCKET != sTest)
// 	{
// 		memcpy(&TestAddr,&m_destAddr,sizeof(TestAddr));
// 		m_ServerIP = ntohl(TestAddr.sin_addr.S_un.S_addr);
// 		if (::connect(sTest, (sockaddr *) &TestAddr, sizeof(TestAddr)) == 0)
// 		{
// 			int nlen = sizeof(TestAddr);
// 			getsockname(sTest, (sockaddr *) &TestAddr, &nlen);
// 			m_realIP = ntohl(TestAddr.sin_addr.s_addr);
// 		}
// 		::closesocket(sTest);
// 	}*/
// }

void TcpSession::connect(LPCTSTR strSrvAdd, int nPort)
{
    struct hostent    *host = NULL;

    g_log.Print(3, "connect: trying to connect to %s:%d\n", strSrvAdd, nPort);

    lstrcpyn(m_strSrvAdd, strSrvAdd, 128);
    m_nPort = nPort;

    memset(&m_destAddr, 0, sizeof(m_destAddr));
    m_destAddr.sin_family = AF_INET;
    if ((m_destAddr.sin_addr.s_addr = inet_addr(strSrvAdd)) == INADDR_NONE)
    {
        host = gethostbyname(strSrvAdd);
        if(host != NULL)
        {
            memcpy(&m_destAddr.sin_addr, host->h_addr_list[0], host->h_length);
        }
        else
        {
            g_log.Print(3, "connect: gethostbyname failed for %s\n", strSrvAdd);
        }
    }
    m_destAddr.sin_port = htons((uint16)nPort);

    m_tcpSock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_tcpSock < 0)
    {
        g_log.Print(3, "connect: socket creation failed, errno=%d\n", errno);
        return;
    }

    g_log.Print(3, "connect: socket created, connecting...\n");

    if (::connect(m_tcpSock, (struct sockaddr *)&m_destAddr, sizeof(m_destAddr)) < 0)
    {
        g_log.Print(3, "connect: connection failed, errno=%d\n", errno);
        g_bySystem = 0;
        return;
    }

    g_log.Print(3, "connect: connected successfully to %s:%d\n", strSrvAdd, nPort);
    SetEvent(g_hRecvEvent);
}
void TcpSession::connect(uint32 ip, uint16 port)
{
	memset(&m_destAddr, 0, sizeof(m_destAddr));
	m_destAddr.sin_family = AF_INET;
	m_destAddr.sin_addr.s_addr = htonl(ip);
	m_destAddr.sin_port = htons(port);
}

// int TcpSession::ReadData(char* lpBuff,int nSize)
// {
// 	int nRet=recv(m_tcpSock, lpBuff, nSize, 0);
// 	if(nRet <=0)
// 	{
// 		//m_bIsAlive = FALSE;
// 		m_bIsLogin = FALSE;
// 		ResetEvent(g_hConnectEvent);
// 	}
// 	return nRet;
// }
int TcpSession::ReadData(char* lpBuff, int nSize)
{
    int nRet = recv(m_tcpSock, lpBuff, nSize, 0);
    //g_log.Print(3, "ReadData: recv returned %d, errno=%d\n", nRet, errno);
    
    if(nRet <= 0)
    {
        g_log.Print(3, "ReadData: setting m_bIsLogin=FALSE\n");
        m_bIsLogin = FALSE;
        ResetEvent(g_hConnectEvent);
    }
    return nRet;
}


void TcpSession::initSession()
{
    //sockaddr_in addr;
	//int			i;
	int			nErr;
	BOOL		optval;
	
	sid = (rand() & 0x7fffffff) + 1;
	sendSeq = (rand() & 0x3fff);
	recvSeq = 0;
	window = 0;


	m_tcpSock = socket(AF_INET, SOCK_STREAM, 0);
	if(m_tcpSock < 0)
		nErr = errno;

    /*memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
	for(i=ECSIM_CLIENT_UDPPORT;i<8000;i++)
	{
		addr.sin_port = htons(i);
		if (bind(m_tcpSock, (SOCKADDR *)&addr, sizeof(addr)) == SOCKET_ERROR)
		{
			int nErr=errno;
		}
		else 
			break;
	}*/
	//m_realIP = GetLocalRealIP();
	optval = FALSE;
	/*if(setsockopt(m_tcpSock,SOL_SOCKET,SO_KEEPALIVE,
		(LPCTSTR)&optval,sizeof(optval))== SOCKET_ERROR)
	{
		nErr = errno;
	}*/
}

void TcpSession::createPacket(TCPOutPacket &out, uint16 cmd, uint16 seq, uint16 ackseq)
{
	/*IcqOption &options = icqLink->options;

	if (options.flags.test(UF_USE_PROXY)) {
		switch (options.proxyType) {
		case PROXY_HTTP:
			out << (uint16) 0;
			break;

		case PROXY_SOCKS:
			out << (uint16) 0 << (uint8) 0;
			if (options.proxy[PROXY_SOCKS].resolve) {
				out << (uint8) 1;
				out.writeData((const char *) &m_destAddr.sin_addr.s_addr, 4);
			} else {
				uint8 len = destHost.length();
				out << (uint8) 3 << len;
				out.writeData(destHost.c_str(), len);
			}
			out.writeData((const char *) &m_destAddr.sin_port, 2);
			break;
		}
	}*/
	//out.beginData();
}

TCPOutPacket *TcpSession::createPacket(uint8 cmd, uint16 ackseq)
{
	TCPOutPacket *p = new TCPOutPacket;
	p->cmd = cmd;
	p->seq = ++sendSeq;
	//createPacket(*p, cmd, sendSeq, ackseq);
	*p << (uint8) TCP_VER << (uint8)cmd;
	*p << (uint16) 0 << (uint32) 0 ;
	return p;
}

void TcpSession::sendAckPacket(uint16 seq)
{
	//TCPOutPacket out;
	//createPacket(out, UDP_ACK, seq);
	//sendDirect(&out);
}

void TcpSession::sendDirect(TCPOutPacket *p)
{
	
    char *pData;
    int  nSize;
    uint32_t nCRC32;
    int  nLoopTime;
    CRC32 crc32Gen;
    xLONG32 outkey[40] = { 0 };
    xLONG8 sendbuf[MAX_PACKET_SIZE] = { 0 };        
    int  nCryptSize;
    PAG_HEADER* ppagHeader;
    char* pchar1;
    char* pchar2;

    // 产生CRC32校验
    pData = (char *)p->getData();
	 if(pData && p->getLength() > 0) {
        uint8 cmd = (uint8)pData[0];
        g_log.Print(3, "sendDirect: sending cmd=%d, length=%d\n", cmd, p->getLength());
    }
    nSize = p->getLength();
    nCRC32 = crc32Gen.Get_CRC(pData, nSize);
    *((uint32_t *)&pData[nSize]) = nCRC32;
    nSize += sizeof(uint32_t);

    // 加密发送信息
    if(nSize % 8 != 0) 
    {
        nLoopTime = nSize / 8 + 1;
        nCryptSize = nLoopTime * 8;
    }
    else 
    {
        nLoopTime = nSize / 8;
        nCryptSize = nSize;
    }
    
    deskey(fixedkey, EN0);
    pchar1 = pData;
    pchar2 = (char *)sendbuf + sizeof(PAG_HEADER) + sizeof(int);
    for(int i = 0; i < nLoopTime; i++)
    {
        des((unsigned char *)pchar1, (unsigned char *)pchar2);
        pchar1 += 8;
        pchar2 += 8;
    }

    nCryptSize += sizeof(int);
    ppagHeader = (PAG_HEADER*)sendbuf;
    ppagHeader->byHeader1 = 0x3E;
    ppagHeader->byHeader2 = 0xE3;
    ppagHeader->nLength = nCryptSize;
    ppagHeader->nVer = 100;
    nCryptSize += sizeof(PAG_HEADER);
    *((int*)(sendbuf + sizeof(PAG_HEADER))) = nSize;

    if(send(m_tcpSock, (char *)sendbuf, nCryptSize, 0) < 0)
    {
		        g_log.Print(3, "sendDirect: send failed, setting m_bIsLogin=FALSE\n");

        m_bIsLogin = FALSE;
        ResetEvent(g_hConnectEvent);
    }
}
void TcpSession::onSendError(TCPOutPacket *p)
{
	//icqLink->onSendError(p->seq);
}

uint16 TcpSession::sendPacket(TCPOutPacket *p, sockaddr_in* destAddr)
{

	return 0;
}

bool TcpSession::setWindow(uint16 seq)
{
	if (seq >= recvSeq + 32 || seq < recvSeq)
		return false;

	if (seq == recvSeq) {
		do {
			recvSeq++;
			window >>= 1;
		} while (window & 0x1);
	} else {
		uint32 mask = (1 << (seq - recvSeq));
		if (window & mask)
			return false;
		else
			window |= mask;
	}
	return true;
}

bool TcpSession::onAck(uint16 seq, uint16& cmd, uint32& dwCallID)
{

	return false;
}

// void TcpSession::onLoginReply(TCPInPacket &in)
// {
// 	uint8 error;
// 	in >> error >> m_ServerID;

// 	if(error == LOGIN_SUCCESS)
// 	{
// 		//m_uin = in.header.uin;
// 		m_bIsLogin = TRUE;
// 		m_nWaitAlive = 0;
// 		g_log.Print(5,"%d login in success.\r\n",m_ServerID);
// 		SetEvent(g_hConnectEvent);
// 		in >> g_bySystem;
// 	}
// 	else
// 	{
// 		ResetEvent(g_hConnectEvent);
// 		m_bIsLogin = FALSE;
// 		g_bySystem = 0;
// 	}
// }
void TcpSession::onLoginReply(TCPInPacket &in)
{
    uint8 error;
    in >> error >> m_ServerID;
    
    g_log.Print(3, "onLoginReply: error=%d, ServerID=%d\n", error, m_ServerID);
    
    if(error == LOGIN_SUCCESS)
    {
        m_bIsLogin = TRUE;
        m_nWaitAlive = 0;
        g_log.Print(3, "%d login in success.\r\n", m_ServerID);
        SetEvent(g_hConnectEvent);
        in >> g_bySystem;
		        onKeepAlive();

        g_log.Print(3, "g_bySystem=%d\n", g_bySystem);
    }
    else
    {
        g_log.Print(3, "Login failed! error code: %d\n", error);
        ResetEvent(g_hConnectEvent);
        m_bIsLogin = FALSE;
        g_bySystem = 0;
    }
}

void TcpSession::onKeepAliveReply(TCPInPacket &in)
{
	in >> m_sessionCount;
}

void TcpSession::onRecvRequestReply(TCPInPacket &in)
{
	FAX_RECVLIST*	pRecvFaxInfo;
	Crwini			Tifini;
	char			strFileName[MAX_PATH];
	char			strShortFileName[MAX_PATH];
	uint32			nTemp;

	pRecvFaxInfo = new FAX_RECVLIST;
	ZeroMemory(pRecvFaxInfo,sizeof(FAX_RECVLIST));
	const char* strTemp;
	in >> pRecvFaxInfo->byType >> strTemp;
	lstrcpyn(pRecvFaxInfo->strTID,strTemp,40);
	in >> strTemp >> pRecvFaxInfo->nPort;
	if(lstrlen(strTemp)==0)
		lstrcpyn(pRecvFaxInfo->strIP,g_sysParam.strHost,20);
	else
		lstrcpyn(pRecvFaxInfo->strIP,strTemp,20);
	if(pRecvFaxInfo->nPort==0) pRecvFaxInfo->nPort = g_sysParam.nFilePort;
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strCountryCode,strTemp,10);
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strAreaCode,strTemp,10);
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strFaxCode,strTemp,20);
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strExtCode,strTemp,10);
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strCallerID,strTemp,30);
	in >> nTemp;
	in >> strTemp;
	lstrcpyn(pRecvFaxInfo->strCSID,strTemp,20);
	

	pRecvFaxInfo->nFAXType = pRecvFaxInfo->byType;
	lstrcpyn(strFileName,g_sysParam.strPath,MAX_PATH);
	lstrcat(strFileName,pRecvFaxInfo->strTID);
	lstrcat(strFileName,".ini");
	lstrcpyn(strShortFileName,pRecvFaxInfo->strTID,MAX_PATH);
	lstrcat(strShortFileName,".tif");
	lstrcpyn(pRecvFaxInfo->strFileName,strShortFileName,MAX_PATH);
	Tifini.WriteSetting(strFileName,pRecvFaxInfo);
	int	ntemp=pRecvFaxInfo->nFAXType;
	g_log.Print(5,"Get Recv FAX request:%d,IP:%s Port:%d,%s,CSID %s.\r\n",pRecvFaxInfo->nFAXType,pRecvFaxInfo->strIP,pRecvFaxInfo->nPort,pRecvFaxInfo->strTID,pRecvFaxInfo->strCSID);

	_beginthread( ReceiveFax, 0 , (void*)pRecvFaxInfo );

}
	
void TcpSession::onSendRequestReply(TCPInPacket &in)
{
	const char* strTemp;
	in >> g_bySendCode >> strTemp;
	lstrcpyn(g_strUUID,strTemp,40);
	in >> strTemp >> g_nPort;
	if(lstrlen(strTemp)==0)
		lstrcpyn(g_strIP,g_sysParam.strHost,20);
	else
		lstrcpyn(g_strIP,strTemp,20);
	if(g_nPort==0) g_nPort = g_sysParam.nFilePort;
	in >> g_nFileSize;
	    // 添加详细日志
    g_log.Print(3, "onSendRequestReply: g_bySendCode=%d, g_strIP=%s, g_nPort=%d, g_strUUID=%s, g_nFileSize=%d\n",
                g_bySendCode, g_strIP, g_nPort, g_strUUID, g_nFileSize);
	SetEvent(g_hSendRequest);
}

void TcpSession::onLCRQueryReply(TCPInPacket &in)
{
	BYTE		byResult;
	BYTE		byLCRCode;
	const char*	strQueryUUID;

	in >> strQueryUUID >> byLCRCode ;
	if(byLCRCode==1)
	{
		in >> byResult;
		int	nTemp=byResult;
		g_log.Print(5,"%s LCR Query Get Result:%d\r\n",strQueryUUID,nTemp);
	}
	else
	{
		byResult = 0;
		g_log.Print(5,"%s LCR Query Get Request.\r\n",strQueryUUID);
	}
	SendLCROper(strQueryUUID,GW_LCR_QUERY,byLCRCode,byResult);
	//SetEvent(g_hLCRQuery);
}

void TcpSession::onLCRCancelReply(TCPInPacket &in)
{
	BYTE		byResult;
	BYTE		byLCRCode;
	const char*	strQueryUUID;
	BOOL		bIsCanceled;
	char		strFileName[MAX_PATH];

	in >> strQueryUUID >> byLCRCode ;
	bIsCanceled = FALSE;
	if (byLCRCode==0)
	{
		RecvFAXList::iterator	iter;
		
		EnterCriticalSection(&g_csRecvList);
		for(iter = g_lsRecvFax.begin(); 
		iter != g_lsRecvFax.end(); ++iter)
		{
			if(lstrcmp((*iter)->strTID,strQueryUUID)==0)
			{
				lstrcpyn(strFileName,g_sysParam.strPath,MAX_PATH);
				lstrcat(strFileName,(*iter)->strTID);
				lstrcat(strFileName,".ini");
				DeleteFile(strFileName);
				DeleteFile((*iter)->strFileName);
				delete (*iter);
				g_lsRecvFax.erase(iter);
				g_nReceive--;
				bIsCanceled = TRUE;
				break;
			}
		}
		LeaveCriticalSection(&g_csRecvList);
	}
	
	if(bIsCanceled)
	{
		onLCRCancel((char*)strQueryUUID,1,0);
		return;
	}

	if(byLCRCode==1)
		in >> byResult;
	else
		byResult = 0;
	SendLCROper(strQueryUUID,GW_LCR_CANCEL,byLCRCode,byResult);
}

void TcpSession::onLCRReportReply(TCPInPacket &in)
{
	BYTE		byResult;
	BYTE		byLCRCode;
	const char*	strQueryUUID;

	in >> strQueryUUID >> byLCRCode ;
	in >> byResult;
	if(byLCRCode==1)
	{
		int	nTemp=byResult;
		g_log.Print(5,"%s LCR Report Get Result:%d\r\n",strQueryUUID,nTemp);
	}
	else
	{
		int	nTemp=byResult;
		g_log.Print(5,"%s LCR Report Request:%d\r\n",strQueryUUID,nTemp);
	}
	SendLCROper(strQueryUUID,GW_LCR_REPORT,byLCRCode,byResult);
}

void TcpSession::onSrvUpdate(TCPInPacket &in)
{
	// char	strFilePath[MAX_PATH];
	// lstrcpyn(strFilePath,g_strFilePath,MAX_PATH);
	// lstrcat(strFilePath,"AutoUpdate.exe");
	// // On Linux, just log the update request
	// g_log.Print(3,"Server update requested: %s\r\n",strFilePath);
	    g_log.Print(3, "Server update requested, ignored on Linux\n");

}

bool TcpSession::onReceive(TCPInPacket &in)
{

	uint8 cmd = in.header.cmd;
	//g_log.Print(3, "onReceive: received cmd=%d\n", cmd);
	string		strVersion;	

	switch (cmd) {
	case TCP_LOGIN:
		onLoginReply(in);
		break;

	case TCP_KEEPALIVE:
	//     g_log.Print(3, "TCP_KEEPALIVE response received!\n");
    // g_log.Print(3, "Keepalive response: sessionCount=%d, version=%s\n", m_sessionCount, strVersion.c_str());
	// 	m_nWaitAlive = 0;
	// 	in >> m_sessionCount >> strVersion;
	// 	if(strVersion.length()!=0 && strVersion.compare(g_strVersion)!=0)
	// 	{
	// 		onSrvUpdate(in);
	// 		lstrcpyn(g_strVersion,strVersion.c_str(),30);
	// 	}
	// 	g_log.Print(6,"get keep alive return.\r\n");
	// 	break;
	    g_log.Print(3, "TCP_KEEPALIVE response received!\n");
    m_nWaitAlive = 0;
    
    // 根据协议，心跳响应应该是: 是否有传真要接收(2B) + 网关连接状态(1B)
    uint16 hasFaxToReceive;
    uint8 connectionStatus;
    in >> hasFaxToReceive >> connectionStatus;
    
    g_log.Print(3, "Keepalive response: hasFaxToReceive=%d, connectionStatus=%d\n", 
                hasFaxToReceive, connectionStatus);
				break;

	case TCP_FAX_REQUESTSEND:
		onSendRequestReply(in);
		break;
	
	case TCP_FAX_REQUESTRECV:
		onRecvRequestReply(in);
		break;

	case TCP_LCR_QUERY:
		onLCRQueryReply(in);
		break;

	case TCP_LCR_CANCEL:
		onLCRCancelReply(in);
		break;

	case TCP_LCR_REPORT:
		onLCRReportReply(in);
		break;

	//case TCP_KICK_OFF:
	//	onKickOff(in);
	//	break;
	// case TCP_SRV_UPDATE:
	// 	onSrvUpdate(in);
	// 	break;

	}
	return true;
}

int TcpSession::receive(char *data, int n, sockaddr_in *pSockAddr)
{
	char *p = data;
	int	 addrlen = sizeof(m_inAddr);
	int  nLoopTime;
	//unsigned char recvbuf[512] = { 0 };		
	//int	 nCryptSize;
	PAG_HEADER*		ppagHeader;
	unsigned char *		pchar1;
	unsigned char*		pchar2;
	CRC32		crcCheck;
	int			nErr;

	n = ::recvfrom(m_tcpSock, data, n, 0, (sockaddr *) &m_inAddr, (socklen_t*)&addrlen);

	if (n < 0) {
		//icqLink->onSendError(0);
		nErr = errno;
		Output("udp recvfrom error, code is %d\n",nErr);
		Sleep(5);
		return -1;
	}
/*/	if (icqLink->isProxyType(PROXY_SOCKS)) {
		if (data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1)
			return false;

		p += 10;
		n -= 10;
//	}*/

	//////////
	if(n > sizeof(PAG_HEADER))
	{
		ppagHeader = (PAG_HEADER*)data;
		if(ppagHeader->byHeader1 == 0x3E &&
		ppagHeader->byHeader2 == 0xE3 &&
		ppagHeader->nVer == 100)
		{
			nLoopTime = (n - sizeof(PAG_HEADER))/8;
			n = ppagHeader->nLength;

			//{���ܽ�����Ϣ
			deskey(fixedkey, DE1);
			pchar1 = (unsigned char*)data + sizeof(PAG_HEADER);
			pchar2 = (unsigned char*)data;
			for(int i=0;i<nLoopTime;i++)
			{
				des(pchar1, pchar2);
				pchar1+=8;
				pchar2+=8;
			}

		}
		else n = 0;
	}
	else n = 0;
	////////////////

	if (n < (int) sizeof(UDP_PACKET_HDR)) {
		OutputDebugString("packet size is too small");
		return -1;
	}

	//CRC32���
	int nCRC1 = *((int *)&data[n-sizeof(int)]);
	int nCRC2 = crcCheck.Get_CRC(data,n-sizeof(int));
	if(nCRC1 != nCRC2)
	{
    OutputDebugString("packet CRC-32 check error!");
        return -1;
	}

	if(pSockAddr!=NULL) CopyMemory(pSockAddr,&m_inAddr,addrlen);
	return n;//s->onPacketReceived(in);

/*	TCPInPacket in(p, n);
	uint16 cmd = in.header.cmd;
	uint32 uin = in.header.uin;
	const char *name;

	if (cmd >= UDP_MSG_FIRST)
		name = ICQ_SESSION_MSG;
	else {
		name = ICQ_SESSION_SERVER;
		uin = 0;
	}
	TcpSession *s = (TcpSession *) icqLink->findSession(name, uin);
	if (!s && strcmp(name, ICQ_SESSION_SERVER) != 0) {
		s = (TcpSession *) icqLink->createSession(name, uin);
		s->sid = in.getSID();

		if (icqLink->isProxyType(PROXY_SOCKS)) {
			sockaddr_in &addr = s->m_destAddr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr = *(in_addr *) &data[4];
			addr.sin_port = *(uint16 *) &data[8];
		} else
			s->m_destAddr = addr;
	}
	if (!s)
		return false;*/

}


// int TcpSession::onLogin()
// {
// 	TCPOutPacket *out =createPacket(TCP_LOGIN);
// 	*out << (const char *)g_sysParam.strUser << (const char *)g_sysParam.strPasswd;
// 	sendDirect(out);
// 	delete out;
// 	return 0;
// }
int TcpSession::onLogin()
{
    g_log.Print(3, "onLogin: preparing login...\n");
    g_log.Print(3, "Username: %s, Password: %s\n", 
                g_sysParam.strUser, g_sysParam.strPasswd);
    
    TCPOutPacket *out = createPacket(TCP_LOGIN);
    
    // 使用 writeData（会自动加长度前缀）
    out->writeData(g_sysParam.strUser, strlen(g_sysParam.strUser) + 1);
    out->writeData(g_sysParam.strPasswd, strlen(g_sysParam.strPasswd) + 1);
    
    sendDirect(out);
    delete out;
    
    g_log.Print(3, "onLogin: login packet sent\n");
    return 0;
}

int TcpSession::onKeepAlive()
{
	static int last_login_state = -1;
    if(last_login_state != m_bIsLogin) {
        //g_log.Print(3, "onKeepAlive: m_bIsLogin changed from %d to %d\n", last_login_state, m_bIsLogin);
        last_login_state = m_bIsLogin;
    }

	g_log.Print(3, "onKeepAlive: m_bIsLogin=%d, m_nWaitAlive=%d\n", m_bIsLogin, m_nWaitAlive);

	if(m_bIsLogin && m_nWaitAlive <6)
	{
		m_nWaitAlive++;
		       // g_log.Print(3, "onKeepAlive: sending heartbeat, attempt %d/6\n", m_nWaitAlive);

		TCPOutPacket *out =createPacket(TCP_KEEPALIVE);
		sendDirect(out);
		delete out;
		        g_log.Print(3, "onKeepAlive: heartbeat sent\n");

	}
	else
	{
		if(m_nPort!=0)
		{
			            //g_log.Print(3,"onKeepAlive: timeout, reconect the fax center.\n");

			g_log.Print(3,"reconect the fax center.\r\n");
			Close();
			connect(m_strSrvAdd,m_nPort);
			onLogin();
		}
	}
	return 0;
}

int TcpSession::onLCRQuery(char* strTID,uint8 bycode,uint8 byResult)
{
	if(m_bIsLogin)
	{
		TCPOutPacket *out =createPacket(TCP_LCR_QUERY);
		*out << strTID << bycode;
		if(bycode==1)
		{
			*out << byResult;
			int	nTemp=byResult;
			g_log.Print(5,"%s LCR Query Return Result:%d\r\n",strTID,nTemp);
		}
		else
			g_log.Print(5,"%s LCR Query send\r\n",strTID);
		sendDirect(out);
		delete out;
	}
	else
	{
		return -1;	
	}
	return 0;
}

int TcpSession::onLCRCancel(char* strTID,uint8 bycode,uint8 byResult)
{
	if(m_bIsLogin)
	{

		TCPOutPacket *out =createPacket(TCP_LCR_CANCEL);
		*out << strTID << bycode;
		if(bycode==1)
			*out << byResult;
		sendDirect(out);
		delete out;
	}
	else
	{
		return -1;	
	}
	return 0;
}

int TcpSession::onLCRReport(char* strTID,uint8 bycode,uint8 byResult)
{
	if(m_bIsLogin)
	{
		TCPOutPacket *out =createPacket(TCP_LCR_REPORT);
		*out << strTID << bycode << byResult;
		sendDirect(out);
		delete out;
		if(bycode==1)
		{
			int	nTemp=byResult;
			g_log.Print(5,"%s LCR Report Return:%d\r\n",strTID,nTemp);
		}
		else
		{
			int	nTemp=byResult;
			g_log.Print(5,"%s LCR Report send:%d\r\n",strTID,nTemp);
		}
	}
	else
	{
		return -1;	
	}
	return 0;
}

int TcpSession::onSendRequest(const char* strCountryCode,const char* strAreaCode,const char* strPhoneCode,const char* strExtCode,DWORD dwSize,const char* strCSID)
{
	if(m_bIsLogin)
	{
		TCPOutPacket *out =createPacket(TCP_FAX_REQUESTSEND);
		*out << strCountryCode << strAreaCode << strPhoneCode << strExtCode << (uint32)dwSize << strCSID;
		sendDirect(out);
		delete out;
	}
	else
	{
		return -1;	
	}
	return 0;
}

int SendRSAPacket(SOCKET tcpSock,TCPOutPacket* p)
{
	char *pData;
	int	 nSize;
	uint32_t  nCRC32;
	int  nLoopTime;
	CRC32 crc32Gen;
	xLONG32 outkey[40] = { 0 };
	xLONG8 sendbuf[MAX_PACKET_SIZE] = { 0 };		
	int	 nCryptSize;
	PAG_HEADER*		ppagHeader;
	char*		pchar1;
	char*		pchar2;
//	char		strDebug[256];

	//����CRC32У��
	pData = (char *)p->getData();
	nSize = p->getLength();
	nCRC32 = crc32Gen.Get_CRC(pData,nSize);
	*((uint32_t *)&pData[nSize]) = nCRC32;
	nSize += sizeof(int);

	//{���ܷ�����Ϣ
	if(nSize%8!=0) 
	{
		nLoopTime = nSize/8+1;
		nCryptSize = nLoopTime*8;
	}
	else 
	{
		nLoopTime = nSize/8;
		nCryptSize = nSize;
	}
    deskey(fixedkey, EN0);
	pchar1 = pData;
	pchar2 = (char *)sendbuf+sizeof(PAG_HEADER)+sizeof(int);
	for(int i=0;i<nLoopTime;i++)
	{
		des((unsigned char *)pchar1, (unsigned char *)pchar2);
		pchar1+=8;
		pchar2+=8;
	}

	nCryptSize += sizeof(int);
	ppagHeader=(PAG_HEADER*)sendbuf;
	ppagHeader->byHeader1 = 0x3E;
	ppagHeader->byHeader2 = 0xE3;
	ppagHeader->nLength = nCryptSize;
	ppagHeader->nVer = 100;
	nCryptSize += sizeof(PAG_HEADER);
	*((int*)(sendbuf + sizeof(PAG_HEADER)))= nSize;
	//}

	if(send(tcpSock, (char *)sendbuf, nCryptSize , 0) < 0)
	{
		int nErr = errno;
		return -1;
	}
	return 0;
}

int ReceivePacket(SOCKET tcpSock,FAX_RECVLIST* pRecvFaxInfo,TCPInPacket &in)
{
	unsigned char *strCode;
	int			nSize;	
	DWORD		dwResult;
	DWORD		dwReadData;
	BOOL		bIsSuccess;

	bIsSuccess = FALSE;
	//���հ�����������
	switch(in.header.cmd)
	{
		case TCP_LOGIN:
			break;
		case TCP_LOGOUT:
			break;
		case TCP_FAX_REQUESTSEND:
			break;
		case TCP_FAX_SEND:
			g_log.Print(5,"get TCP_FAX_SEND packet\r\n");
			strCode = in.readData(nSize);
			if(pRecvFaxInfo->hRecvFile==NULL)
			{
				char	szFileName[MAX_PATH];
				lstrcpyn(szFileName,g_sysParam.strPath,MAX_PATH);
				lstrcat(szFileName,pRecvFaxInfo->strTID);
				lstrcat(szFileName,".ted");
				g_log.Print(5,"%s\r\n",szFileName);
				pRecvFaxInfo->hRecvFile = CreateFile(szFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
				if(pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
				{
					if(WriteFile(pRecvFaxInfo->hRecvFile,strCode,nSize,&dwReadData,NULL))
					{
						if(nSize==dwReadData)
						{
							TCPOutPacket out;
							out << (uint8)TCP_FAX_SEND;
							out << (uint8)0;
							SendRSAPacket(tcpSock,&out);
							bIsSuccess = TRUE;
						}
					}

				}
			}
			else if(pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
			{
					if(WriteFile(pRecvFaxInfo->hRecvFile,strCode,nSize,&dwReadData,NULL))
					{
						if(nSize==dwReadData)
						{
							TCPOutPacket out;
							out << (uint8)TCP_FAX_SEND;
							out << (uint8)0;
							if(SendRSAPacket(tcpSock,&out)==-1)
								return -1;
							bIsSuccess = TRUE;
						}
					}

			}
			if(!bIsSuccess)
			{
				if(pRecvFaxInfo->hRecvFile && pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
				{
					CloseHandle(pRecvFaxInfo->hRecvFile);
					pRecvFaxInfo->hRecvFile = NULL;
				}
				g_log.Print(3,"Write FAX file error!\r\n");
				TCPOutPacket out;
				out << (uint8)TCP_FAX_SEND;
				out << (uint8)1;
				if(SendRSAPacket(tcpSock,&out)==-1)
					return -1;
			}
			return 2;
			break;
		case TCP_FAX_STOPSEND:
			{
				g_log.Print(5,"get TCP_FAX_STOPSEND packet\r\n");
				if(pRecvFaxInfo->hRecvFile && pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
				{
					char	szFileName1[MAX_PATH];
					char	szFileName2[MAX_PATH];
					pRecvFaxInfo->nSuccess = 1;
					CloseHandle(pRecvFaxInfo->hRecvFile);
					pRecvFaxInfo->hRecvFile = NULL;
					lstrcpyn(szFileName1,g_sysParam.strPath,MAX_PATH);
					lstrcat(szFileName1,pRecvFaxInfo->strTID);
					lstrcat(szFileName1,".ted");
					lstrcpyn(szFileName2,g_sysParam.strPath,MAX_PATH);
					lstrcat(szFileName2,pRecvFaxInfo->strTID);
					lstrcat(szFileName2,".tif");
					CopyFile(szFileName1,szFileName2,FALSE);
					DeleteFile(szFileName1);

					lstrcpyn(pRecvFaxInfo->strFileName,szFileName2,MAX_PATH);
					EnterCriticalSection(&g_csRecvList);
					g_lsRecvFax.push_back(pRecvFaxInfo);
					g_nReceive ++;
					LeaveCriticalSection(&g_csRecvList);

					ResetEvent(g_hLCRReport);
					g_tcpSession.onLCRReport(pRecvFaxInfo->strTID,0,2);
				}

				TCPOutPacket out;
				out << (uint8)TCP_FAX_STOPSEND;
				out << (uint8)0;
				int result =SendRSAPacket(tcpSock,&out);
				if (result == -1)
    {
        g_log.Print(3, "TCP_FAX_STOPSEND: SendRSAPacket failed!\n");
    }
    else
    {
        g_log.Print(3, "TCP_FAX_STOPSEND: response sent successfully\n");
    }
			}
			return 0;
			break;
		case TCP_KEEPALIVE:
			break;
		case TCP_FAX_REQUESTRECV:
			break;
		case TCP_FAX_RECV:
			//in >> strCode;
			g_log.Print(5,"get TCP_FAX_RECV packet\r\n");
			strCode = in.readData(nSize);
			if(pRecvFaxInfo->hRecvFile==NULL)
			{
				char	szFileName[MAX_PATH];
				lstrcpyn(szFileName,pRecvFaxInfo->strTID,40);
				lstrcat(szFileName,".ted");
				pRecvFaxInfo->hRecvFile = CreateFile(szFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
				if(pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
				{
					if(WriteFile(pRecvFaxInfo->hRecvFile,strCode,nSize,&dwReadData,NULL))
					{
						if(nSize==dwReadData)
						{
							TCPOutPacket out;
							out << (uint8)TCP_FAX_RECV;
							out << (uint8)0;
							SendRSAPacket(tcpSock,&out);
						}
					}
				}
			}
			else if(pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
			{
					if(WriteFile(pRecvFaxInfo->hRecvFile,strCode,nSize,&dwReadData,NULL))
					{
						if(nSize==dwReadData)
						{
							TCPOutPacket out;
							out << (uint8)TCP_FAX_RECV;
							out << (uint8)0;
							SendRSAPacket(tcpSock,&out);
						}
					}

			}
			return 2;
			break;
		case TCP_FAX_STOPRECV:
			{
				g_log.Print(5,"get TCP_FAX_STOPRECV packet\r\n");
				if(pRecvFaxInfo->hRecvFile && pRecvFaxInfo->hRecvFile!=INVALID_HANDLE_VALUE)
				{
					char	szFileName1[MAX_PATH];
					char	szFileName2[MAX_PATH];
					lstrcpyn(szFileName1,pRecvFaxInfo->strTID,40);
					lstrcat(szFileName1,".ted");
					lstrcpyn(szFileName2,pRecvFaxInfo->strTID,40);
					lstrcat(szFileName2,".tif");
					CopyFile(szFileName1,szFileName2,FALSE);
					DeleteFile(szFileName1);

					lstrcpyn(pRecvFaxInfo->strFileName,szFileName2,MAX_PATH);
					EnterCriticalSection(&g_csRecvList);
					g_lsRecvFax.push_back(pRecvFaxInfo);
					g_nReceive ++;
					LeaveCriticalSection(&g_csRecvList);
				}

				TCPOutPacket out;
				out << (uint8)TCP_FAX_STOPRECV;
				out << (uint8)0;
				SendRSAPacket(tcpSock,&out);
			}
			return 0;
			break;
		case TCP_LCR_QUERY:
			break;
		case TCP_LCR_CANCEL:
			break;
		case TCP_LCR_REPORT:
			break;
		case TCP_FAX_EXT:
			break;
		// case TCP_SRV_UPDATE:
		// 	break;
	}
	return -1;
}
int GetPacket(SOCKET tcpSock, PAG_HEADER* pheader, int& ntype, BYTE* byPacket,
              int& nPointer, BYTE& m_byLastChar, FAX_RECVLIST* pRecvFaxInfo)
{
    BYTE byBuff[2048];
    int nSize;

    while (true)
    {
        nSize = recv(tcpSock, (char*)byBuff, sizeof(byBuff), 0);
        if (nSize > 0)
        {
            for (int i = 0; i < nSize; i++)
            {
                if ((BYTE)byBuff[i] == 0xE3 && m_byLastChar == 0x3E && ntype == 0)
                {
                    ntype = 1;
                    nPointer = 0;
                    *((PBYTE)pheader + nPointer) = 0x3E;
                    nPointer++;
                    *((PBYTE)pheader + nPointer) = 0xE3;
                    nPointer++;
                }
                else
                {
                    if (ntype == 1)
                    {
                        *((PBYTE)pheader + nPointer) = (BYTE)byBuff[i];
                        nPointer++;
                        if (nPointer >= sizeof(PAG_HEADER))
                        {
                            ntype = 2;
                            nPointer = 0;
                        }
                    }
                    else if (ntype == 2)
                    {
                        byPacket[nPointer] = (BYTE)byBuff[i];
                        nPointer++;
                        if (nPointer >= pheader->nLength)
                        {
                            ntype = 0;
                            nPointer = 0;

                            // 构建完整包并处理
                            PBYTE pbyPacket = new BYTE[sizeof(DWORD) + sizeof(PAG_HEADER) + pheader->nLength];
                            if (pbyPacket)
                            {
                                CopyMemory(pbyPacket, pheader, sizeof(PAG_HEADER));
                                CopyMemory(pbyPacket + sizeof(PAG_HEADER), byPacket, pheader->nLength);
                                if (CheckPacket(pbyPacket))
                                {
                                    // 解密成功后，创建 TCPInPacket 并调用 ReceivePacket
                                    TCPInPacket in(pbyPacket + sizeof(PAG_HEADER), pheader->nLength, 1);
                                    int ret = ReceivePacket(tcpSock, pRecvFaxInfo, in);
                                    delete[] pbyPacket;
                                    return ret;  // 直接返回 ReceivePacket 的结果
                                }
                                delete[] pbyPacket;
                            }
                        }
                    }
                }
                m_byLastChar = (BYTE)byBuff[i];
            }
        }
        else
        {
            return -1;
        }
    }
    return 0;
}
// void ReceiveFax(void* lpParam)
// {
// 	FAX_RECVLIST*	pRecvFaxInfo;
//     struct hostent	*host = NULL;
// 	sockaddr_in		destAddr;
// 	BYTE			m_byLastChar;
// 	int				ntype;
// 	int				nPointer;
// 	PAG_HEADER		header;
// 	BYTE			byPacket[2048];
// 	int				nSize;
// 	BYTE			byResult;
// 	SOCKET			sRecvSock;

// 	pRecvFaxInfo = (FAX_RECVLIST*)lpParam;
 
// 	memset(&destAddr, 0, sizeof(destAddr));
// 	destAddr.sin_family = AF_INET;
// 	if ((destAddr.sin_addr.s_addr = inet_addr(pRecvFaxInfo->strIP)) == INADDR_NONE)
// 	{
// 		host = gethostbyname(pRecvFaxInfo->strIP);
// 		if(host != NULL)
// 		{
// 			memcpy(&destAddr.sin_addr, host->h_addr_list[0],
// 			    host->h_length);
// 		}
// 		//need check function success?
// 	}
// 	destAddr.sin_port = htons((uint16)pRecvFaxInfo->nPort);

//     sRecvSock = socket(AF_INET, SOCK_STREAM, 0);
//     if (sRecvSock >= 0)
// 	{
// 		if (::connect(sRecvSock, (struct sockaddr *)&destAddr, 
// 			sizeof(destAddr)) < 0)
// 		{
// 			closesocket(sRecvSock);
// 			return;
// 		}
// 	}
// 	g_log.Print(5,"connect to File server %s\r\n", pRecvFaxInfo->strIP);

// 	{
// 		TCPOutPacket p;
// 		p << (uint8)GW_FAX_REQUESTRECV;
// 		p << pRecvFaxInfo->strTID;
// 		SendRSAPacket(sRecvSock,&p);
// 		//GetPacket(sRecvSock,byBuff,&header,ntype, nPointer,m_byLastChar, pRecvFaxInfo);
// 		g_log.Print(5,"send GW_FAX_REQUESTRECV\r\n");
// 	}

// 	ntype = 0;
// 	while(1)
// 	{
// 		byResult = 1;
// 		if(GetPacket(sRecvSock,&header,ntype, byPacket, nPointer,m_byLastChar, pRecvFaxInfo)==-1)
// 			break;
		
// 		/*TCPOutPacket p;
// 		p << (uint8)GW_FAX_REQUESTRECV;
// 		p << pRecvFaxInfo->strTID;
// 		SendPacket(sRecvSock,&p);*/

// 	}

// 	closesocket(sRecvSock);

// }

void ReceiveFax(void* lpParam)
{
    FAX_RECVLIST* pRecvFaxInfo = (FAX_RECVLIST*)lpParam;
    struct hostent* host = NULL;
    sockaddr_in destAddr;
    BYTE m_byLastChar = 0;
    int ntype = 0;
    int nPointer = 0;
    PAG_HEADER header;
    BYTE byPacket[2048];
    int nSize;
    BYTE byResult;
    SOCKET sRecvSock;
    HANDLE hRecvFile = NULL;
    char szFileName1[MAX_PATH];
    char szFileName2[MAX_PATH];

    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    if ((destAddr.sin_addr.s_addr = inet_addr(pRecvFaxInfo->strIP)) == INADDR_NONE)
    {
        host = gethostbyname(pRecvFaxInfo->strIP);
        if (host != NULL)
        {
            memcpy(&destAddr.sin_addr, host->h_addr_list[0], host->h_length);
        }
    }
    destAddr.sin_port = htons((uint16)pRecvFaxInfo->nPort);

    sRecvSock = socket(AF_INET, SOCK_STREAM, 0);
    if (sRecvSock < 0)
    {
        g_log.Print(3, "ReceiveFax: socket creation failed\n");
        delete pRecvFaxInfo;
        return;
    }

    g_log.Print(3, "ReceiveFax: connecting to File server %s:%d\n", pRecvFaxInfo->strIP, pRecvFaxInfo->nPort);

    if (::connect(sRecvSock, (struct sockaddr*)&destAddr, sizeof(destAddr)) < 0)
    {
        g_log.Print(3, "ReceiveFax: connect failed, errno=%d\n", errno);
        closesocket(sRecvSock);
        delete pRecvFaxInfo;
        return;
    }

    g_log.Print(3, "ReceiveFax: connected successfully, TID=%s\n", pRecvFaxInfo->strTID);

    // 发送 GW_FAX_REQUESTRECV 命令
    {
        TCPOutPacket p;
        p << (uint8)GW_FAX_REQUESTRECV;
        p << pRecvFaxInfo->strTID;
        SendRSAPacket(sRecvSock, &p);
        g_log.Print(3, "ReceiveFax: sent GW_FAX_REQUESTRECV, TID=%s\n", pRecvFaxInfo->strTID);
    }

    // 创建临时文件 (.ted)
    lstrcpyn(szFileName1, g_sysParam.strPath, MAX_PATH);
    lstrcat(szFileName1, pRecvFaxInfo->strTID);
    lstrcat(szFileName1, ".ted");
    
    hRecvFile = CreateFile(szFileName1, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (hRecvFile == INVALID_HANDLE_VALUE)
    {
        g_log.Print(3, "ReceiveFax: failed to create file %s, errno=%d\n", szFileName1, errno);
        closesocket(sRecvSock);
        delete pRecvFaxInfo;
        return;
    }

    ntype = 0;
    nPointer = 0;
    m_byLastChar = 0;
    
    g_log.Print(3, "ReceiveFax: entering receive loop, waiting for data...\n");
    
    int loopCount = 0;
   while (1)
    {
        int ret = GetPacket(sRecvSock, &header, ntype, byPacket, nPointer, m_byLastChar, pRecvFaxInfo);
        
        if (ret == -1)
        {
            g_log.Print(3, "ReceiveFax: GetPacket failed\n");
            break;
        }
        else if (ret == 0)
        {
            // TCP_FAX_STOPSEND，接收完成
            g_log.Print(3, "ReceiveFax: received stop, completing\n");
            break;
        }
        // ret == 2 表示 TCP_FAX_SEND，继续接收
    }


if (hRecvFile != INVALID_HANDLE_VALUE && hRecvFile != NULL)
    {
        CloseHandle(hRecvFile);
    }
    closesocket(sRecvSock);
}