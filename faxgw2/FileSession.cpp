// FileSession.h: interface for the FileSession class.
/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/
#include "StdAfx.h"
#include <iostream>
#include "gwtypes.h"
#include "FileSession.h"
#include "TcpSession.h"
#include "tcppacket.h"
#include "rwini.h"
#include "Log.h"

extern CRITICAL_SECTION	g_csRecvList;
extern TcpSession		g_tcpSession;
extern BYTE		g_bySendCode;
extern BYTE		g_byLCRCode;
extern BYTE		g_byQueryResult;
extern BYTE		g_byLCRCancel;
extern BYTE		g_byCancelResult;
extern BYTE		g_byLCRReport;
extern BYTE		g_byReportResult;
extern BYTE		g_bySystem;
extern char		g_strQueryUUID[40];
extern char		g_strCancelUUID[40];
extern char		g_strReportUUID[40];
extern char				g_strUUID[40];
extern HANDLE			g_hSendRequest;
extern HANDLE			g_hLCRQuery;
extern HANDLE			g_hLCRCancel;
extern HANDLE			g_hLCRReport;
extern char				g_strIP[20];
extern uint32			g_nPort;
RecvFAXList				g_lsRecvFax;
uint16					g_nReceive=0;
SOCKET					g_sSend=0;
SOCKET					g_sMsg=0;
extern  SYS_PARAM		g_sysParam;
extern	Log				g_log;

FAX_RECVLIST*			g_pRecvFaxInfo;

extern	int SendRSAPacket(SOCKET tcpSock,TCPOutPacket* p);
extern bool CheckPacket(PBYTE pbyPacket);
int SendPacket(SOCKET tcpSock,TCPOutPacket &out,FAX_SESSION &faxSess);
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


const int FileSession::HIGHEST_PRIORITY=2;
const int FileSession::ABOVE_NORMAL=1;
const int FileSession::NORMAL_PRIORITY=0;
const int FileSession::BELOW_NORMAL=-1;
const int FileSession::LOWEST_PRIORITY=-2;
HANDLE FileSession::m_hSemaphore=NULL;
char FileSession::m_strDirectory[];

//---------------------------------------------------------------

FileSession::FileSession()
{
    m_hThrds=NULL;
	m_hExitEvent = NULL;
	m_hFile = INVALID_HANDLE_VALUE;
	//InitializeCriticalSection(&m_gCriticalSection);
	ThreadSet();
}

FileSession::FileSession(BOOL bIsSend,BOOL bIsTCP, BOOL bIsListen,int nPort)
{
	m_hThrds=NULL;
	m_hExitEvent = NULL;
	//InitializeCriticalSection(&m_gCriticalSection);
	ThreadSet();
	SessSet(bIsSend,bIsTCP,bIsListen,nPort);
}

FileSession::~FileSession()
{
	//DeleteCriticalSection(&m_gCriticalSection);  
	if(m_hExitEvent)CloseHandle(m_hExitEvent);
}

int CleanFile()
{
	WIN32_FIND_DATA FileData; 
	HANDLE hSearch; 
	int				nRet;
 
	BOOL fFinished = FALSE; 
	nRet = 0;

	hSearch = FindFirstFile((char*)"*.ted", &FileData); 
	if (hSearch != INVALID_HANDLE_VALUE) 
	{ 
		while (!fFinished) 
		{
			DeleteFile(FileData.cFileName);

			if (!FindNextFile(hSearch, &FileData)) 
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES) 
				{ 
					fFinished = TRUE; 
				} 
				else 
				{ 
					OutputDebugString("Couldn't find next file.\n");
				} 
			}
		} 
		FindClose(hSearch);
	} 
 
	fFinished = FALSE; 
	hSearch = FindFirstFile((char*)"*.ini", &FileData); 
	if (hSearch != INVALID_HANDLE_VALUE) 
	{ 
		while (!fFinished) 
		{
			DeleteFile(FileData.cFileName);

			if (!FindNextFile(hSearch, &FileData)) 
			{ 
				if (GetLastError() == ERROR_NO_MORE_FILES) 
				{ 
					fFinished = TRUE; 
				} 
				else 
				{ 
					OutputDebugString("Couldn't find next file.\n");
				} 
			}
		} 
		FindClose(hSearch);
	}

	return nRet;
}

int SearchFile()
{
	WIN32_FIND_DATA FileData; 
	HANDLE hSearch; 
	char szHome[MAX_PATH]; 
	FAX_RECVLIST*	pFaxRecvlist;
	int				nRet;
 
	BOOL fFinished = FALSE; 
	nRet = 0;

	CreateDirectory(g_sysParam.strPath,NULL);
	SetCurrentDirectory(g_sysParam.strPath);

	hSearch = FindFirstFile((char*)"*.tif", &FileData);
	if (hSearch == INVALID_HANDLE_VALUE) 
	{ 
		OutputDebugString("No .tif files found.\n"); 
		CleanFile();
		return 0;
	} 
 
	while (!fFinished) 
	{
		//FileData.cFileName = 
		pFaxRecvlist = new FAX_RECVLIST;
		ZeroMemory(pFaxRecvlist,sizeof(FAX_RECVLIST));
		Crwini		rwinTemp;
		lstrcpyn(pFaxRecvlist->strTID,FileData.cFileName,40);
		(strrchr(pFaxRecvlist->strTID,'.'))[0] = 0; 

		lstrcpyn(szHome,g_sysParam.strPath,MAX_PATH);
		lstrcpyn(&szHome[lstrlen(szHome)],FileData.cFileName,MAX_PATH);
		lstrcpy(&szHome[lstrlen(szHome)-3],"ini");
		
		rwinTemp.GetSetting(szHome,pFaxRecvlist);
		lstrcpyn(pFaxRecvlist->strFileName,g_sysParam.strPath,MAX_PATH);
		lstrcpyn(&pFaxRecvlist->strFileName[lstrlen(pFaxRecvlist->strFileName)],FileData.cFileName,MAX_PATH);
		nRet++;
		EnterCriticalSection(&g_csRecvList);
		g_lsRecvFax.push_back(pFaxRecvlist);
		LeaveCriticalSection(&g_csRecvList);

		if (!FindNextFile(hSearch, &FileData)) 
		{
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
				fFinished = TRUE; 
			} 
			else 
			{ 
				OutputDebugString("Couldn't find next file.\n");
			} 
		}
	} 
 
	FindClose(hSearch);

	return nRet;
}

//����������������������������������������������������������������������

void FileSession::SessSet(BOOL bIsSend,BOOL bIsTCP, BOOL bIsListen,int nPort)
{
    SessionReset();
	m_bIsSend = bIsSend;
	m_nPort   = nPort;
	m_bIsTCP  = bIsTCP;
	m_bIsListen = bIsListen;

	m_hExitEvent=CreateEvent(NULL,TRUE,FALSE,"filesession");
	//CreatePathList();
}
void FileSession::ThreadSet(LONG MaxThreadCount,int priority)
{
	m_Priority=priority;
	m_ActiveCount=m_MaxThreadCount=MaxThreadCount;
    if(m_hThrds)delete []m_hThrds;//release thread handle of array
	m_hThrds=new HANDLE[MaxThreadCount];
	
}
void FileSession::SessionReset()
{
	m_Option=0;
	ResetEvent(m_hExitEvent);
	m_ExitCode=ERR;
}

void FileSession::SessSet(int nPort)
{
	m_nPort = nPort;
}

// void FileSession::SessSet(SOCKET sSocket)
// {
// 	m_Priority = FileSession::NORMAL_PRIORITY;
// 	m_sSocket = sSocket;

// 	m_hExitEvent=CreateEvent(NULL,TRUE,FALSE,"filesession");
// }

//----------------------------------------------------------------------
/*BOOL FileSession::StartSession()
{
	
	DWORD ThreadID;
    //create main thread
	HANDLE hMainThread=CreateThread(NULL,0,MainThreadProc,(LPVOID)this,CREATE_SUSPENDED,&ThreadID);
	//ASSERT(hMainThread);
    BOOL re=SetThreadPriority(hMainThread,m_Priority);//adjust thread priority
	//ASSERT(re);
	ResumeThread(hMainThread);
	CloseHandle(hMainThread);
	
	return TRUE;
}*/

BOOL FileSession::StartSession()
{
	
	DWORD ThreadID;
	LPTHREAD_START_ROUTINE lpStartAddress;
	m_hReadyEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    //create main thread
	lpStartAddress = MainThreadProc;
	m_hThrds[0]=CreateThread(NULL,0,lpStartAddress,(LPVOID)this,CREATE_SUSPENDED,&ThreadID);
	////ASSERT(m_hThrds[0]);
    BOOL re=SetThreadPriority(m_hThrds[0],0);//adjust thread priority
	////ASSERT(re);
	ResumeThread(m_hThrds[0]);
	//CloseHandle(hMainThread);
	
	return TRUE;
}

void FileSession::PauseSession()
{
	if(m_ExitCode==PAUSE)return ;
	m_ExitCode=PAUSE;
	SetEvent(m_hExitEvent);
	Sleep(40);
}
void FileSession::ResumeSession()
{ 
	SetEvent(m_hExitEvent);  
}
void FileSession::StopSession()
{
	if(m_ExitCode==STOP)return ;
	if(m_ExitCode==PAUSE)
	{
		ResumeSession();
        Sleep(40);
	}
	m_ExitCode=STOP;
	SetEvent(m_hExitEvent);
}
//----------------------------------------------------------------------------------------------------
HANDLE FileSession::StartThread(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParam)
{
	DWORD ThreadID;
	//FileSession *filesess=(FileSession *)lpParam;
	
	HANDLE htmp=CreateThread(NULL,0,lpStartAddress,lpParam,CREATE_SUSPENDED,&ThreadID);
	BOOL re=SetThreadPriority(htmp,0);
	//ASSERT(re);
	ResumeThread(htmp);
	CloseHandle(htmp);
	return htmp;
}
//------------------------------------------------------------------------------------------------------
void* FileSession::MainThreadProc(void* lpParam)
{
	FileSession *filesess=(FileSession *)lpParam;
	
//resume:
	//Reset
	ResetEvent(filesess->m_hExitEvent);
	filesess->m_ExitCode=ERR;
	filesess->m_ActiveCount=filesess->m_MaxThreadCount;
	
	SOCKET        sMain,
                  sClient;
    int           iAddrSize;
    struct sockaddr_in local,
                       client;

	m_hSemaphore = CreateSemaphore(NULL,MAX_THREAD,MAX_THREAD,NULL);
	g_hSendRequest = ::CreateEvent(NULL,FALSE,FALSE,NULL);
	g_hLCRQuery = ::CreateEvent(NULL,FALSE,FALSE,NULL);
	g_hLCRCancel = ::CreateEvent(NULL,FALSE,FALSE,NULL);
	g_hLCRReport = ::CreateEvent(NULL,FALSE,FALSE,NULL);
	g_nReceive=SearchFile();

    // Create our main socket
    sMain = socket(AF_INET, SOCK_STREAM, 0);
    if (sMain < 0)
    {
		g_log.Print(5,"start fax service failed!\r\n");
        return (void*)1;
    }
    local.sin_family = AF_INET;

	local.sin_addr.s_addr = htonl(INADDR_ANY);
		
	local.sin_port = htons(filesess->m_nPort);
	if (bind(sMain, (struct sockaddr *)&local, sizeof(local)) < 0)
	{
		g_log.Print(5,"start fax service failed(bind)!\r\n");
		closesocket(sMain);
		return (void*)1;
	}
	listen(sMain, 1);

	while(TRUE)
	{
		DWORD dwWaitRet = WaitForSingleObject(m_hSemaphore,INFINITE);
		if(dwWaitRet==WAIT_OBJECT_0)
		{
			iAddrSize = sizeof(client);
			sClient = accept(sMain, (struct sockaddr *)&client, (socklen_t*)&iAddrSize);
			if (sClient < 0)
			{        
				//printf("accept() failed: %d\n", errno);
				closesocket(sMain);
				return (void*)1;
			}

			StartThread(TCPSThreadProc,(LPVOID)sClient);

		}
	}

	//create thread for thread

	//PostMessage(filesess->m_MainhWnd,WM_THREADCOUNT,(WPARAM)(filesess->m_ActiveCount),NULL);
	/*for(int i=0;i<filesess->m_MaxThreadCount;i++)
		filesess->m_hThrds[i]=StartThread(UDPThreadProc,lpParam);
	
	WaitForMultipleObjects(filesess->m_MaxThreadCount,filesess->m_hThrds,TRUE,INFINITE);
	
	for(i=0;i<filesess->m_MaxThreadCount;i++)
		CloseHandle(filesess->m_hThrds[i]); //close all thread handle
	
	
	//decide thread exit code
	switch(filesess->m_ExitCode)
	{
	case PAUSE://SendMessage(filesess->m_MainhWnd,WM_THREADPAUSE,NULL,NULL);
		ResetEvent(filesess->m_hExitEvent);
		//wait continue transmit
		WaitForSingleObject(filesess->m_hExitEvent,INFINITE);
		goto resume;
	case EXIT://SendMessage(filesess->m_MainhWnd,WM_THREADEXIT,EXIT,NULL);
		filesess->SessionReset();
		break;;
	case STOP://SendMessage(filesess->m_MainhWnd,WM_THREADEXIT,STOP,NULL);
		filesess->SessionReset();
		break;
	default:filesess->SetErrCode(1);return 0;
	}	
	return 1;*/
}

//----------------------------------------------------------------------------------
void* FileSession::UDPThreadProc(void* lpParam)
{
	FileSession *filesess=(FileSession *)lpParam;
	BYTE bNewActive=1,bOldActive;
	while(1)
	{
		bOldActive=bNewActive;
		if(WaitForSingleObject(filesess->m_hExitEvent,0)!=WAIT_TIMEOUT)
		{
			if(bOldActive) InterlockedDecrement(&filesess->m_ActiveCount);
			break;
		}
		
		if(!filesess->m_ActiveCount)
		{
			SetEvent(filesess->m_hExitEvent);
			filesess->m_ExitCode=filesess->EXIT;
			break;
		}
		
		if(bNewActive!=bOldActive)
		{
			bNewActive?InterlockedIncrement(&filesess->m_ActiveCount):InterlockedDecrement(&filesess->m_ActiveCount);
		}
		else if(!bNewActive)continue;
		
		// add function code
	}
	
	delete filesess;
	return 0;
}

int SendPacket(SOCKET tcpSock,TCPOutPacket &out,FAX_SESSION &faxSess)
{
	char *pData;
	int	 nSize;
	BYTE sendbuf[MAX_PACKET_SIZE] = { 0 };		
	int	 nCryptSize;
	GW_HEADER*		ppagHeader;

	pData = (char *)out.getData();
	nSize = out.getLength();

	ppagHeader=(GW_HEADER*)sendbuf;
	ppagHeader->byHeader1 = 0x3E;
	ppagHeader->byHeader2 = 0xE3;
	ppagHeader->nLength = nSize;
	ppagHeader->byVer = 1;
	nCryptSize = nSize + sizeof(GW_HEADER);
	CopyMemory(sendbuf+sizeof(GW_HEADER),pData,nSize);
	//}

	if(send(tcpSock, (char *)sendbuf, nCryptSize , 0) < 0)
	{
		int nErr = errno;
		return -1;
	}
	return 0;
}
int FileSession::ReadFile(LPBYTE lpBuff,int nSize)
{
	DWORD	dwNumRead;

	if(INVALID_HANDLE_VALUE == m_hFile)
		return -1;
	if(::ReadFile(m_hFile,lpBuff,nSize,&dwNumRead,NULL))
		return 0;
	else 
		return -1;
}

int FileSession::WriteFile(LPBYTE lpBuff,int nSize)
{
	DWORD	dwNumWrite;

	if(INVALID_HANDLE_VALUE == m_hFile)
		return -1;
	if(::WriteFile(m_hFile,lpBuff,nSize,&dwNumWrite,NULL))
		return 0;
	else 
		return -1;
}

int GetFileSize(char* strFilename,FAX_SESSION &faxSess,LPDWORD lpdwLow,LPDWORD lpdwHigh)
{
	DWORD	dwRet;
	DWORD	dwError;
	if(strFilename!=NULL)
	{
		faxSess.bIsEnd = false;
		faxSess.hSendFile = CreateFile(strFilename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
		if(INVALID_HANDLE_VALUE == faxSess.hSendFile)
			return -1;
		dwRet = ::GetFileSize(faxSess.hSendFile,lpdwHigh);
		if(dwRet == 0xFFFFFFFF && 
			(dwError = GetLastError()) != NO_ERROR)
			return -1;
		*lpdwLow = dwRet;
	}

	return 0;
}

int SendFax(SOCKET tcpSock,FAX_SESSION &faxSess)
{
	char	buff[1024];
	DWORD	dwNumRead;
	int		nSize;

	nSize = 1024;

	if(INVALID_HANDLE_VALUE == faxSess.hSendFile)
		return -1;
	if(::ReadFile(faxSess.hSendFile,buff,nSize,&dwNumRead,NULL))
	{
	}

	if(dwNumRead > 0)
	{
		TCPOutPacket out;
		out << (uint8)GW_FAX_RECV;
		out.writeData(buff,dwNumRead);
		if(SendPacket(tcpSock,out,faxSess)==-1)
			return -1;
		OutputDebugString("GW_FAX_RECV DATA\n");
		if(faxSess.dwFileSize >= dwNumRead)
			faxSess.dwFileSize -= dwNumRead;
		else
			return -1;
	}

	if(faxSess.dwFileSize <= 0)
	{
		faxSess.bIsEnd = true;
		CloseHandle(faxSess.hSendFile);
		faxSess.hSendFile = NULL;
	}
	return 0;
}

int	CheckRecvList()
{
	return 0;
}

int ReceiveFileSrvPacket(SOCKET ClientSock,TCPInPacket &in)
{
	unsigned char *strCode;
	BYTE		byCode;	
	DWORD		dwResult;
	DWORD		dwReadData;
	FAX_SESSION faxSess;

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
			{
				in >> byCode;
				TCPOutPacket out;
				out << (uint8)GW_FAX_SEND;
				out << (uint8)byCode;
				SendPacket(ClientSock,out,faxSess);
			}
			return 1;
			break;
		case TCP_FAX_STOPSEND:
			{
				in >> byCode;
				TCPOutPacket out;
				out << (uint8)GW_FAX_STOPSEND;
				out << (uint8)byCode;
				SendPacket(ClientSock,out,faxSess);
			}
			return 0;
			break;
		case TCP_KEEPALIVE:
			break;
		case TCP_FAX_REQUESTRECV:
			break;
		case TCP_FAX_RECV:
			break;
		case TCP_FAX_STOPRECV:
			break;
		case TCP_LCR_QUERY:
			break;
		case TCP_LCR_CANCEL:
			break;
		case TCP_LCR_REPORT:
			break;
		case TCP_FAX_EXT:
			break;
	}
	return -1;
}

int GetRSAPacket(SOCKET tcpSock,SOCKET ClientSock,PAG_HEADER* pheader,int& ntype,int& nPointer,BYTE& m_byLastChar)
{
	BYTE			byBuff[512];
	BYTE			byPacket[2048];
	int				nSize;

	while(true)
	{
		nSize = 512;
		nSize = recv(tcpSock, (char *)byBuff, nSize, 0);
		if(nSize > 0)
		{
			for(int i=0;i< nSize ;i++)
			{
				if((BYTE)byBuff[i] == 0xE3 && m_byLastChar == 0x3E && ntype==0)
				{
					//��ʼ�����Ϣ��.
					ntype = 1;
					nPointer= 0;

					*((PBYTE)pheader + nPointer)=0x3E;
					nPointer++;
					*((PBYTE)pheader + nPointer)=0xE3;
					nPointer++;
				}
				else
				{
					if(ntype == 1)
					{
						*((PBYTE)pheader + nPointer)=(BYTE)byBuff[i];
						nPointer++;
						if(nPointer >= sizeof(PAG_HEADER))
						{
							ntype = 2;
							nPointer = 0;
						}
					}
					else if(ntype == 2)
					{
						byPacket[nPointer] = (BYTE)byBuff[i];
						nPointer++; 
						if(nPointer >= pheader->nLength)
						{
							PBYTE	pbyPacket;
							ntype = 0;
							nPointer = 0;

							pbyPacket = new BYTE [sizeof(DWORD)+sizeof(PAG_HEADER)+pheader->nLength];

							if(pbyPacket)
							{
								CopyMemory(pbyPacket,pheader,sizeof(PAG_HEADER));
								CopyMemory(pbyPacket+sizeof(PAG_HEADER),byPacket,pheader->nLength);
								if(CheckPacket(pbyPacket))
								{
									TCPInPacket in(pbyPacket+sizeof(PAG_HEADER), pheader->nLength,1);
									if(ReceiveFileSrvPacket(ClientSock,in)==-1)
									{
										closesocket(tcpSock);
										delete [] pbyPacket;
										return -1;
									}
									delete [] pbyPacket;
									return 1;
								}
								delete [] pbyPacket;
							}
						}

					}
				}
				m_byLastChar = (BYTE)byBuff[i];
			}
		}
		else
		{
			closesocket(tcpSock);
			return -1;
		}
	}
	return 0;
}

int ConnectFileSrv(SOCKET sClientSock)
{
    struct hostent    *host = NULL;
	sockaddr_in m_destAddr;
	BYTE			m_byLastChar;
	int				ntype;
	int				nPointer;
	PAG_HEADER		header;
	int				nSize;
	BYTE			byResult;

	memset(&m_destAddr, 0, sizeof(m_destAddr));
	m_destAddr.sin_family = AF_INET;
	if ((m_destAddr.sin_addr.s_addr = inet_addr(g_strIP)) == INADDR_NONE)
	{
		host = gethostbyname(g_strIP);
		if(host != NULL)
		{
			memcpy(&m_destAddr.sin_addr, host->h_addr_list[0],
			    host->h_length);
		}
		//need check function success?
	}
	m_destAddr.sin_port = htons((uint16)g_nPort);

    g_sSend = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sSend >= 0)
	{
		if (::connect(g_sSend, (struct sockaddr *)&m_destAddr, 
			sizeof(m_destAddr)) < 0)
		{
			return -1;
		}
	}

	g_log.Print(5,"connect to File server %s.\r\n",g_strIP);
	{
		TCPOutPacket p;
		p << (uint8)TCP_FAX_REQUESTSEND;
		p << g_strUUID;
		if(SendRSAPacket(g_sSend,&p)==-1)
			return -1;

		g_log.Print(5,"send TCP_FAX_REQUESTSEND.\r\n");
		//GetRSAPacket(g_sSend,sClientSock,&header,ntype, nPointer,m_byLastChar);
	}
	return 0;
}

int SendFaxData(SOCKET tcpSock,const char* buff,int nSize)
{
	BYTE			byLastChar;
	int				ntype;
	int				nPointer;
	PAG_HEADER		header;

		TCPOutPacket p;
		p << (uint8)TCP_FAX_SEND;
		p.writeData(buff,nSize);
		if(SendRSAPacket(g_sSend,&p)==-1)
			return -1;
	ntype = 0;
	return GetRSAPacket(g_sSend,tcpSock,&header,ntype, nPointer,byLastChar);
}

int SendFaxEnd(SOCKET tcpSock)
{
	BYTE			byLastChar;
	int				ntype;
	int				nPointer;
	PAG_HEADER		header;
	int				nSize;

		TCPOutPacket p;
		p << (uint8)TCP_FAX_STOPSEND;
		if(SendRSAPacket(g_sSend,&p)==-1)
			return -1;
	ntype = 0;
	return GetRSAPacket(g_sSend,tcpSock,&header,ntype, nPointer,byLastChar);
}

int SendLCROper(const char* strUUID,BYTE byLCRcmd,BYTE byLCRcode,BYTE byLCRResult)
{
	FAX_SESSION		faxSess;
	faxSess.nType = 0;
	if(g_sMsg!=0)
	{
		TCPOutPacket out;
		out << (uint8)byLCRcmd;
		out << strUUID;
		out << (uint8)byLCRcode;
		if(byLCRcode==1 || byLCRcmd==GW_LCR_REPORT)
			out << (uint8)byLCRResult;
		SendPacket(g_sMsg,out,faxSess);
		return 1;
	}
	return 0;
}


int ReceivePacket(SOCKET tcpSock,TCPInPacket &in,FAX_SESSION &faxSess)
{
	const char *strCode1;
	const char *strCode2;
	const char *strCode3;
	const char *strCode4;
	const char *strCSID;
	unsigned char *buff;
	int			nSize;
	BYTE		byCode;
	uint8		byResult;
	DWORD		dwResult;
	char		strFileName[MAX_PATH];

	//���հ�����������
	switch(in.header.cmd)
	{
		case GW_LOGIN:
			in >> strCode1;

			if(strcmp(strCode1,g_sysParam.strGWKey)==0)
			{
				faxSess.bIsLogin = true;
				TCPOutPacket out;
				out << (uint8)GW_LOGIN;
				out << (uint8)LOGIN_SUCCESS;
				out << (uint8)LOGIN_SUCCESS;
				if(SendPacket(tcpSock,out,faxSess)==-1)
					return -1;
				g_log.Print(5,"GW_LOGIN success.\r\n");
			}
			else
			{
				TCPOutPacket out;
				out << (uint8)GW_LOGIN;
				out << (uint8)LOGIN_INVALID_USER;
				SendPacket(tcpSock,out,faxSess);
				return -1;
			}
			break;
		case GW_LOGOUT:
			g_sMsg = 0;
			return 0;
			break;
		case GW_FAX_REQUESTSEND:
			if(faxSess.nType==0)
			{
				faxSess.nType=1;
				ResetEvent(g_hSendRequest);
				in >> strCode1 >> strCode2 >> strCode3 >> strCode4 ;
				//in >> (uint32)dwResult >> strCSID;
				uint32 tempResult;
				in >> tempResult;
				dwResult = tempResult;
				in >> strCSID;
				g_log.Print(5,"send FAX request %s%s-%s-%s,CSID %s\r\n",strCode1,strCode2,strCode3,strCode4,strCSID);
				g_tcpSession.onSendRequest(strCode1,strCode2,strCode3,strCode4,dwResult,strCSID);
				dwResult = WaitForSingleObject(g_hSendRequest,5000);
				if(dwResult!=WAIT_OBJECT_0)
				{
					g_log.Print(5,"server not respond the send FAX request\r\n");
					return -1;
				}
				//���ӵ��ļ�������,����������
				if(g_bySendCode!=0)
					if(ConnectFileSrv(tcpSock) ==-1)
						g_bySendCode = 0;
				int		ntemp=g_bySendCode;
				g_log.Print(5,"return send FAX request:%d,IP:%s Port:%d,%s\r\n",ntemp,g_strIP,g_nPort,g_strUUID);
				TCPOutPacket out;
				out.cmd = GW_FAX_REQUESTSEND;
				out << (uint8)GW_FAX_REQUESTSEND;
				out << g_bySendCode << g_strUUID ;
				if(SendPacket(tcpSock,out,faxSess)==-1)
					return -1;
			}
			else
				return -1;
			break;
		case GW_FAX_SEND:
			if(faxSess.nType==1)
			{
				g_log.Print(5,"send GW_FAX_SEND\r\n");
				buff = in.readData(nSize);
				if(SendFaxData(tcpSock,(const char*)buff,nSize)==-1)
					return -1;
			}
			else
				return -1;
			break;
		case GW_FAX_STOPSEND:
			if(faxSess.nType==1)
			{
				//����ֹͣ��
				if(SendFaxEnd(tcpSock)==-1)
					return -1;
				return 0;
			}
			else
				return -1;
			break;
		case GW_KEEPALIVE:
			if(faxSess.nType==0)
			{
				TCPOutPacket out;
				out.cmd = GW_KEEPALIVE;
				out << (uint8)GW_KEEPALIVE;
				out << g_nReceive;
				out << (uint8)g_bySystem;
				if(SendPacket(tcpSock,out,faxSess)==-1)
					return -1;
				g_sMsg = tcpSock;
			}
			break;
		case GW_FAX_REQUESTRECV:
			if(faxSess.nType==0)
			{
				faxSess.nType=4;
				if(g_nReceive > 0)
				{
					list<FAX_RECVLIST *>::iterator	iter;
					DWORD							dwFileLow;
					DWORD							dwFileHigh;
					TCPOutPacket out;

					EnterCriticalSection(&g_csRecvList);
					iter = g_lsRecvFax.begin();
					LeaveCriticalSection(&g_csRecvList);
					g_pRecvFaxInfo = (*iter);
					if(GetFileSize((*iter)->strFileName,faxSess,&dwFileLow,&dwFileHigh)==0)
					{
						if((*iter)->nFAXType!=1 && (*iter)->nFAXType!=2)
							(*iter)->nFAXType=1;
						g_log.Print(5,"%s request recv %d\r\n",(*iter)->strTID,(*iter)->nFAXType);
						out <<(uint8) GW_FAX_REQUESTRECV;
						out << (uint8)(*iter)->nFAXType <<(uint32)dwFileLow;
						out << (*iter)->strCountryCode <<(*iter)->strAreaCode;
						out << (*iter)->strFaxCode <<(*iter)->strExtCode;
						out << (*iter)->strCallerID << (*iter)->strTID;
						out << (*iter)->strCSID;
						faxSess.dwFileSize = dwFileLow;
						if(SendPacket(tcpSock,out,faxSess)==-1)
							return -1;
						if(SendFax(tcpSock,faxSess)==-1)
							return -1;
					}
					else
					{
						EnterCriticalSection(&g_csRecvList);
						g_nReceive--;
						g_lsRecvFax.pop_front();
						LeaveCriticalSection(&g_csRecvList);
						out <<(uint8) GW_FAX_REQUESTRECV;
						out << (uint8)0;
						if(SendPacket(tcpSock,out,faxSess)==-1)
							return -1;
					}
				}
				else
				{
					TCPOutPacket out;
					out <<(uint8) GW_FAX_REQUESTRECV;
					out << (uint8)0;
					if(SendPacket(tcpSock,out,faxSess)==-1)
						return -1;
				}
			}
			else
				return -1;
			break;
		case GW_FAX_RECV:
			OutputDebugString("GW_FAX_RECV\n");
			if(faxSess.nType==4)
			{
				in >> byCode;
				if(faxSess.bIsEnd)
				{
					faxSess.nType=5;
					TCPOutPacket out;
					out<<(uint8) GW_FAX_STOPRECV;
					if(SendPacket(tcpSock,out,faxSess)==-1)
						return -1;
				}
				else
				{
					if(SendFax(tcpSock,faxSess)==-1)
						return -1;
				}
			}
			else
				return -1;
			break;
		case GW_FAX_STOPRECV:
			//�������,�Է��ɹ����վ�ɾ����¼
			OutputDebugString("GW_FAX_STOPRECV\n");
			if(faxSess.nType==5)
			{
				//faxSess.nType = 0;
				in >> byCode;
				if(byCode==0)
				{
					if(!(INVALID_HANDLE_VALUE == faxSess.hSendFile || NULL == faxSess.hSendFile))
					{
						CloseHandle(faxSess.hSendFile);
						faxSess.hSendFile = NULL;
					}

					list<FAX_RECVLIST *>::iterator	Recviter;
					EnterCriticalSection(&g_csRecvList);
					for(Recviter = g_lsRecvFax.begin(); 
						Recviter != g_lsRecvFax.end(); ++Recviter)
						if((*Recviter)==g_pRecvFaxInfo)
						{
							g_lsRecvFax.erase(Recviter);
							lstrcpyn(strFileName,g_sysParam.strPath,MAX_PATH);
							lstrcat(strFileName,(*Recviter)->strTID);
							lstrcat(strFileName,".ini");
							g_log.Print(5,"delete %s file...",(*Recviter)->strTID);
							if(!DeleteFile(strFileName))
								g_log.Print(5,"delete %s failed.",strFileName);
							//lstrcpyn(strFileName,g_sysParam.strPath,MAX_PATH);
							//lstrcat(strFileName,(*Recviter)->strTID);
							//lstrcat(strFileName,".tif");
							if(!DeleteFile((*Recviter)->strFileName))
								g_log.Print(5,"delete %s failed.",(*Recviter)->strFileName);
							delete (*Recviter);
							break;
						}
					g_nReceive --;
					LeaveCriticalSection(&g_csRecvList);
				}
				closesocket(tcpSock);
			}
			else
				return -1;
			break;
		case GW_LCR_QUERY:
			if(faxSess.nType==0)
			{
				ResetEvent(g_hLCRQuery);
				in >> strCode1 >> byCode;
				if(byCode==1)
					in >> byResult;
				g_tcpSession.onLCRQuery((char*)strCode1,byCode,byResult);
			}
			else 
				return -1;
			break;
		case GW_LCR_CANCEL:
			if(faxSess.nType==0)
			{
				ResetEvent(g_hLCRCancel);
				in >> strCode1 >> byCode;
				if(byCode==1)
					in >> byResult;
				g_tcpSession.onLCRCancel((char *)strCode1,byCode,byResult);
			}
			else 
				return -1;
			break;
		case GW_LCR_REPORT:
			if(faxSess.nType==0)
			{
				ResetEvent(g_hLCRReport);
				in >> strCode1 >> byCode >> byResult;
				g_tcpSession.onLCRReport((char *)strCode1,byCode,byResult);
			}
			else 
				return -1;
			break;
		case GW_FAX_EXT:
			break;
	}
	return 0;
}

void* FileSession::TCPSThreadProc(void* lpParam)
{

	BYTE	byBuff[512];
	BYTE			m_byLastChar;
	int				ntype;
	int				nPointer;
	GW_HEADER		header;
	BYTE			byPacket[2048];
	int				nSize;
	FAX_SESSION		faxSess;
	SOCKET			tcpSock;

	tcpSock = (SOCKET)(long)lpParam;
	ZeroMemory(&faxSess,sizeof(faxSess));
	faxSess.bIsLogin = false;
	faxSess.nType = 0;
	ntype=0;
	while(true)
	{
		nSize = 512;
		nSize = recv(tcpSock, (char *)byBuff, nSize, 0);
		if(nSize > 0)
		{
			for(int i=0;i< nSize ;i++)
			{
				if((BYTE)byBuff[i] == 0xE3 && m_byLastChar == 0x3E && ntype==0)
				{
					//��ʼ�����Ϣ��.
					ntype = 1;
					nPointer= 0;

					*((PBYTE)&header + nPointer)=0x3E;
					nPointer++;
					*((PBYTE)&header + nPointer)=0xE3;
					nPointer++;
				}
				else
				{
					if(ntype == 1)
					{
						*((PBYTE)&header + nPointer)=(BYTE)byBuff[i];
						nPointer++;
						if(nPointer >= sizeof(GW_HEADER))
						{
							ntype = 2;
							nPointer = 0;
						}
					}
					else if(ntype == 2)
					{
						byPacket[nPointer] = (BYTE)byBuff[i];
						nPointer++; 
						if(nPointer >= header.nLength)
						{
							ntype = 0;
							nPointer = 0;

								// ����Ϣ���������
								TCPInPacket in(byPacket, header.nLength,1);
								if(ReceivePacket(tcpSock,in,faxSess)==-1)
								{
									closesocket(tcpSock);
									if(faxSess.nType==1)
									{
										if(g_sSend)
										{
											closesocket(g_sSend);
											g_sSend = 0;
										}
									}
									else if(faxSess.nType==4 || faxSess.nType==5)
									{
										if(!(INVALID_HANDLE_VALUE == faxSess.hSendFile || NULL == faxSess.hSendFile))
										{
											CloseHandle(faxSess.hSendFile);
											faxSess.hSendFile = NULL;
										}
									}

									ReleaseSemaphore(FileSession::m_hSemaphore,1,NULL);
									return (void*)-1;

								}
								//in.handle();
									//onPacketReceived(in,&SenderAddr);
						}

					}
				}
				m_byLastChar = (BYTE)byBuff[i];
			}
		}
		else
		{
			closesocket(tcpSock);
			break;
		}
	}
	if(faxSess.nType==1)
	{
		if(g_sSend)
		{
			closesocket(g_sSend);
			g_sSend = 0;
		}
	}
	else if(faxSess.nType==4 || faxSess.nType==5)
	{
		if(!(INVALID_HANDLE_VALUE == faxSess.hSendFile || NULL == faxSess.hSendFile))
		{
			CloseHandle(faxSess.hSendFile);
			faxSess.hSendFile = NULL;
		}
	}
	ReleaseSemaphore(FileSession::m_hSemaphore,1,NULL);


/*    SOCKET        sMain,
                  sClient;
    int           iAddrSize;
    struct sockaddr_in local,
                       client;
	DWORD			dwFileSize;
	DWORD			dwReadSize;
	DWORD			dwRet;

	FileSession *filesess=(FileSession *)lpParam;
	BYTE bNewActive=1,bOldActive;


    // Create our main socket
    sMain = socket(AF_INET, SOCK_STREAM, 0);
    if (sMain < 0)
    {
        //printf("socket() failed: %d\n", errno);
		SetEvent(filesess->m_hReadyEvent);
        return 1;
    }
    local.sin_family = AF_INET;

	if(filesess -> m_bIsListen)
	{
		local.sin_addr.s_addr = htonl(INADDR_ANY);
		
		for(;filesess->m_nPort<10000;filesess->m_nPort++)
		{
			local.sin_port = htons(filesess->m_nPort);
			if (bind(sMain, (struct sockaddr *)&local, 
				sizeof(local)) >= 0)
				break;
		}
		if(filesess->m_nPort == 10000)
		{
			//printf("bind() failed: %d\n", errno);
			closesocket(sMain);
			SetEvent(filesess->m_hReadyEvent);
			return 1;
		}

		listen(sMain, 1);
		
		SetEvent(filesess->m_hReadyEvent);
		iAddrSize = sizeof(client);
		sClient = accept(sMain, (struct sockaddr *)&client,
			&iAddrSize); 
	}
	else
	{
		local.sin_addr.s_addr = htonl(filesess->m_nHost);
		local.sin_port = htons(filesess->m_nPort);
		if (connect(sMain, (struct sockaddr *)&local, 
			sizeof(local)) < 0)
		{
			//printf("connect() failed: %d\n", errno);
			return 1;
		}
		sClient = sMain;
	}
	if (sClient < 0)
	{        
		//printf("accept() failed: %d\n", errno);
		closesocket(sMain);
		return 1;
	}
	//printf("Accepted client: %s:%d\n", 
	//	inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	
	filesess->m_dwCurSize =0;
	filesess->ReadData(sClient,(unsigned char *)&dwFileSize,sizeof(DWORD));
	if(dwFileSize == -1) 
	{        
		closesocket(sMain);
		return 1;
	}
	if(filesess->InitializeFile(TRUE,TRUE) == -1)
	{
		dwRet = -1;
		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
	}
	else
	{
		dwRet = 0;
		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
		filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
		while(1)
		{
			bOldActive=bNewActive;
			if(WaitForSingleObject(filesess->m_hExitEvent,0)!=WAIT_TIMEOUT)
			{
				if(bOldActive) InterlockedDecrement(&filesess->m_ActiveCount);
				break;
			}
			
			if(!filesess->m_ActiveCount)
			{
				SetEvent(filesess->m_hExitEvent);
				filesess->m_ExitCode=filesess->EXIT;
				break;
			}
			
			if(bNewActive!=bOldActive)
			{
				bNewActive?InterlockedIncrement(&filesess->m_ActiveCount):InterlockedDecrement(&filesess->m_ActiveCount);
			}
			else if(!bNewActive)continue;
			
			// add function code
			dwRet = dwFileSize - filesess->m_dwCurSize;
			if(dwRet < MAX_FILEBUFF)
			{
				dwReadSize = filesess->ReadData(sClient,filesess->m_byFileBuff,dwRet);
				if(dwRet != dwReadSize)
				{
					dwRet = -1;
					break;
				}
				filesess->m_dwCurSize = dwFileSize;
				break;
			}
			else
			{
				if(MAX_FILEBUFF != filesess->ReadData(sClient,filesess->m_byFileBuff,MAX_FILEBUFF))
				{
					dwRet = -1;
					break;
				}
				filesess->m_dwCurSize += MAX_FILEBUFF;
			}
			if(filesess->WriteFile(filesess->m_byFileBuff,MAX_FILEBUFF)== -1)
			{
				dwRet = -1;
				send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
				break;
			}
			dwReadSize = 0;
			dwRet = 0;
			send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
			filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
		}
	}
	if(dwReadSize!=0 && dwRet!=-1)
	{
		if(filesess->WriteFile(filesess->m_byFileBuff,dwReadSize)== -1)
		{
			dwRet = -1;
		}
		else
			dwRet = 0;
		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
	}
	filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
	filesess->CloseFile();
    if(filesess -> m_bIsListen) closesocket(sClient);
    closesocket(sMain);
	
	delete filesess;*/
	return 0;
}

void* FileSession::TCPCThreadProc(void* lpParam)
{
    SOCKET			sClient;
	DWORD			dwFileSize;
	DWORD			dwReadSize;
	DWORD			dwRet;
	char			strFileName[MAX_FILELEN];
	char			strFilePath[MAX_PATH];

	FileSession *filesess=(FileSession *)lpParam;
	BYTE bNewActive=1,bOldActive;
	
	sClient = filesess->m_sSocket;

	while(TRUE)
	{
		if(MAX_FILELEN!=filesess->ReadData(sClient,(unsigned char *)strFileName,MAX_FILELEN))
			dwRet =-1;
		filesess->m_dwCurSize =0;
		lstrcpy(strFilePath,filesess->m_strDirectory);
		lstrcat(strFilePath,strFileName);
		//cout << "target " << strFilePath << endl;
		if(filesess->InitializeFile(strFilePath,FALSE,FALSE) == -1)
		{
			dwRet = -1;
			if (send(sClient, (const char *)&dwRet, sizeof(DWORD), 0) ==sizeof(DWORD))
				continue;
		}
		else
		{
			dwRet = 0;
			if(filesess->GetFileSize(&dwFileSize,NULL)==0)
			{
				if (send(sClient, (const char *)&dwFileSize, sizeof(DWORD), 0) == sizeof(DWORD))
					if (filesess->ReadData(sClient,(unsigned char *)&dwRet,sizeof(DWORD))!= sizeof(DWORD))
					{
						dwRet =-1;
						break;
					}
				//filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
				while(dwRet!=-1)
				{
					bOldActive=bNewActive;
					if(WaitForSingleObject(filesess->m_hExitEvent,0)!=WAIT_TIMEOUT)
					{
						if(bOldActive) InterlockedDecrement(&filesess->m_ActiveCount);
						break;
					}
					
					if(!filesess->m_ActiveCount)
					{
						SetEvent(filesess->m_hExitEvent);
						filesess->m_ExitCode=filesess->EXIT;
						break;
					}
					
					if(bNewActive!=bOldActive)
					{
						bNewActive?InterlockedIncrement(&filesess->m_ActiveCount):InterlockedDecrement(&filesess->m_ActiveCount);
					}
					else if(!bNewActive)continue;
					
					// add function code
					dwRet = dwFileSize - filesess->m_dwCurSize;
					if(dwRet < MAX_FILEBUFF)
					{
						dwReadSize = dwRet;
						if(filesess->ReadFile(filesess->m_byFileBuff,dwRet)!=0)
						{
							dwRet = -1;
							break;
						}
						filesess->m_dwCurSize = dwFileSize;
						break;
					}
					else
					{
						if(0 != filesess->ReadFile(filesess->m_byFileBuff,MAX_FILEBUFF))
						{
							dwRet = -1;
							break;
						}
						filesess->m_dwCurSize += MAX_FILEBUFF;
					}
					if(send(sClient,(const char *)filesess->m_byFileBuff,MAX_FILEBUFF,0) != MAX_FILEBUFF)
					{
						dwRet = -1;
						break;
					}
					dwReadSize = 0;
					dwRet = 0;
					/*CString szDebug;
					szDebug.Format("send %u,%u,%u \n",filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
					OutputDebugString(szDebug);*/
					//filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
					if(sizeof(DWORD)!=filesess->ReadData(sClient,(unsigned char *)&dwRet,sizeof(DWORD)))
						dwRet =-1;
					if(dwRet == -1)	
						break;
					
				}
			}
		}
		if(dwReadSize!=0 && dwRet!=-1)
		{
			if((DWORD)send(sClient,(const char *)filesess->m_byFileBuff,dwReadSize,0) == dwReadSize)
			{
				if(sizeof(DWORD)!=filesess->ReadData(sClient,(unsigned char *)&dwRet,sizeof(DWORD)))
					dwRet =-1;
			}
			else
				dwRet =-1;
		}

		/*			CString szDebug;
					szDebug.Format("send %u,%u,%u \n",filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
					OutputDebugString(szDebug);*/
		//filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
		//dwRet == -1 ? TRUE send file failed!
		filesess->CloseFile();
		if(dwRet == -1) break;
	}

    closesocket(sClient);
	
	filesess->CloseFile();
	ReleaseSemaphore(filesess->m_hSemaphore,1,NULL);
	delete filesess;
	return 0;
}

DWORD FileSession::ReadData(SOCKET sSock,LPBYTE lpBuff,int nSize)
{
	LPBYTE	lpCurPoint;
	int		nCurSize;
	int		nRet;

	lpCurPoint = lpBuff;
	nCurSize = 0;
	while(nSize > nCurSize)
	{
		nRet = recv(sSock, (char *)lpCurPoint, nSize - nCurSize, 0);
		if(nRet <= 0)
			return 0;
		lpCurPoint += nRet;
		nCurSize +=nRet;
	}
	return nCurSize;
}

int FileSession::InitializeFile(const char* szDesFileName,BOOL bIsWrite, BOOL bIsCreate)
{
	DWORD dwDesiredAccess;
	DWORD dwCreatOpt;

	//if(m_hFile != INVALID_HANDLE_VALUE)
	//	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	if(bIsWrite)
		dwDesiredAccess = GENERIC_WRITE;
	else
		dwDesiredAccess = GENERIC_READ;
	if(bIsCreate)
		dwCreatOpt = CREATE_ALWAYS;
	else
		dwCreatOpt = OPEN_EXISTING;
	m_hFile = CreateFile(szDesFileName,dwDesiredAccess,FILE_SHARE_READ,NULL,dwCreatOpt,0,NULL);
	if(INVALID_HANDLE_VALUE == m_hFile)
		return -1;
	else
		return 0;
}

int FileSession::GetFileSize(DWORD* lpdwLow,DWORD* lpdwHigh)
{
	DWORD	dwRet;
	DWORD	dwError;

	dwRet = ::GetFileSize(m_hFile,lpdwHigh);
	if(dwRet == 0xFFFFFFFF && 
		(dwError = GetLastError()) != NO_ERROR)
		return -1;
	*lpdwLow = dwRet;
	return 0;
}

int FileSession::CloseFile()
{
	if(INVALID_HANDLE_VALUE == m_hFile)
		return -1;
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	return 0;
}

BOOL FileSession::QueryReady()
{
	DWORD	dwRet;
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hReadyEvent,INFINITE))
	{
		dwRet = WaitForSingleObject(m_hThrds[0],0);
		if(WAIT_TIMEOUT == dwRet)
			return TRUE;
	}
	return FALSE;
}