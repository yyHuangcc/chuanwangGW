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

// int CleanFile()
// {
// 	WIN32_FIND_DATA FileData; 
// 	HANDLE hSearch; 
// 	int				nRet;
 
// 	BOOL fFinished = FALSE; 
// 	nRet = 0;

// 	hSearch = FindFirstFile((char*)"*.ted", &FileData); 
// 	if (hSearch != INVALID_HANDLE_VALUE) 
// 	{ 
// 		while (!fFinished) 
// 		{
// 			DeleteFile(FileData.cFileName);

// 			if (!FindNextFile(hSearch, &FileData)) 
// 			{ 
// 				if (GetLastError() == ERROR_NO_MORE_FILES) 
// 				{ 
// 					fFinished = TRUE; 
// 				} 
// 				else 
// 				{ 
// 					OutputDebugString("Couldn't find next file.\n");
// 				} 
// 			}
// 		} 
// 		FindClose(hSearch);
// 	} 
 
// 	fFinished = FALSE; 
// 	hSearch = FindFirstFile((char*)"*.ini", &FileData); 
// 	if (hSearch != INVALID_HANDLE_VALUE) 
// 	{ 
// 		while (!fFinished) 
// 		{
// 			DeleteFile(FileData.cFileName);

// 			if (!FindNextFile(hSearch, &FileData)) 
// 			{ 
// 				if (GetLastError() == ERROR_NO_MORE_FILES) 
// 				{ 
// 					fFinished = TRUE; 
// 				} 
// 				else 
// 				{ 
// 					OutputDebugString("Couldn't find next file.\n");
// 				} 
// 			}
// 		} 
// 		FindClose(hSearch);
// 	}

// 	return nRet;
// }

// int SearchFile()
// {
// 	WIN32_FIND_DATA FileData; 
// 	HANDLE hSearch; 
// 	char szHome[MAX_PATH]; 
// 	FAX_RECVLIST*	pFaxRecvlist;
// 	int				nRet;
 
// 	BOOL fFinished = FALSE; 
// 	nRet = 0;

// 	CreateDirectory(g_sysParam.strPath,NULL);
// 	SetCurrentDirectory(g_sysParam.strPath);

// 	hSearch = FindFirstFile((char*)"*.tif", &FileData);
// 	if (hSearch == INVALID_HANDLE_VALUE) 
// 	{ 
// 		OutputDebugString("No .tif files found.\n"); 
// 		CleanFile();
// 		return 0;
// 	} 
 
// 	while (!fFinished) 
// 	{
// 		//FileData.cFileName = 
// 		pFaxRecvlist = new FAX_RECVLIST;
// 		ZeroMemory(pFaxRecvlist,sizeof(FAX_RECVLIST));
// 		Crwini		rwinTemp;
// 		lstrcpyn(pFaxRecvlist->strTID,FileData.cFileName,40);
// 		(strrchr(pFaxRecvlist->strTID,'.'))[0] = 0; 

// 		lstrcpyn(szHome,g_sysParam.strPath,MAX_PATH);
// 		lstrcpyn(&szHome[lstrlen(szHome)],FileData.cFileName,MAX_PATH);
// 		lstrcpy(&szHome[lstrlen(szHome)-3],"ini");
		
// 		rwinTemp.GetSetting(szHome,pFaxRecvlist);
// 		lstrcpyn(pFaxRecvlist->strFileName,g_sysParam.strPath,MAX_PATH);
// 		lstrcpyn(&pFaxRecvlist->strFileName[lstrlen(pFaxRecvlist->strFileName)],FileData.cFileName,MAX_PATH);
// 		nRet++;
// 		EnterCriticalSection(&g_csRecvList);
// 		g_lsRecvFax.push_back(pFaxRecvlist);
// 		LeaveCriticalSection(&g_csRecvList);

// 		if (!FindNextFile(hSearch, &FileData)) 
// 		{
// 			if (GetLastError() == ERROR_NO_MORE_FILES) 
// 			{ 
// 				fFinished = TRUE; 
// 			} 
// 			else 
// 			{ 
// 				OutputDebugString("Couldn't find next file.\n");
// 			} 
// 		}
// 	} 
 
// 	FindClose(hSearch);

// 	return nRet;
// }
int CleanFile()
{
    WIN32_FIND_DATA FileData; 
    HANDLE hSearch; 
    int nRet;
    BOOL fFinished = FALSE; 
    nRet = 0;

    g_log.Print(3, "CleanFile: cleaning .ted files\n");
    
    hSearch = FindFirstFile((char*)"*.ted", &FileData); 
    if (hSearch != INVALID_HANDLE_VALUE) 
    { 
        while (!fFinished) 
        {
            g_log.Print(3, "CleanFile: deleting .ted file: %s\n", FileData.cFileName);
            DeleteFile(FileData.cFileName);

            if (!FindNextFile(hSearch, &FileData)) 
            { 
                if (GetLastError() == ERROR_NO_MORE_FILES) 
                { 
                    fFinished = TRUE; 
                } 
                else 
                { 
                    g_log.Print(3, "CleanFile: FindNextFile failed for .ted\n");
                } 
            }
        } 
        FindClose(hSearch);
    } 
    else
    {
        g_log.Print(3, "CleanFile: no .ted files found\n");
    }
 
    fFinished = FALSE; 
    g_log.Print(3, "CleanFile: cleaning .ini files\n");
    
    hSearch = FindFirstFile((char*)"*.ini", &FileData); 
    if (hSearch != INVALID_HANDLE_VALUE) 
    { 
        while (!fFinished) 
        {
            g_log.Print(3, "CleanFile: deleting .ini file: %s\n", FileData.cFileName);
            DeleteFile(FileData.cFileName);

            if (!FindNextFile(hSearch, &FileData)) 
            { 
                if (GetLastError() == ERROR_NO_MORE_FILES) 
                { 
                    fFinished = TRUE; 
                } 
                else 
                { 
                    g_log.Print(3, "CleanFile: FindNextFile failed for .ini\n");
                } 
            }
        } 
        FindClose(hSearch);
    }
    else
    {
        g_log.Print(3, "CleanFile: no .ini files found\n");
    }

    return nRet;
}

int SearchFile()
{
    WIN32_FIND_DATA FileData; 
    HANDLE hSearch; 
    char szHome[MAX_PATH]; 
    FAX_RECVLIST* pFaxRecvlist;
    int nRet;
    BOOL fFinished = FALSE; 
    nRet = 0;

    g_log.Print(3, "SearchFile: start, path=%s\n", g_sysParam.strPath);

    CreateDirectory(g_sysParam.strPath, NULL);
    SetCurrentDirectory(g_sysParam.strPath);

    hSearch = FindFirstFile((char*)"*.tif", &FileData);
    if (hSearch == INVALID_HANDLE_VALUE) 
    { 
        g_log.Print(3, "SearchFile: no .tif files found\n");
        CleanFile();
        return 0;
    } 
 
    g_log.Print(3, "SearchFile: found .tif files, starting loop\n");
    
    while (!fFinished) 
    {
        g_log.Print(3, "SearchFile: processing file: %s\n", FileData.cFileName);
        
        pFaxRecvlist = new FAX_RECVLIST;
        if (!pFaxRecvlist)
        {
            g_log.Print(3, "SearchFile: new FAX_RECVLIST failed\n");
            break;
        }
        ZeroMemory(pFaxRecvlist, sizeof(FAX_RECVLIST));
        
        g_log.Print(3, "SearchFile: allocated FAX_RECVLIST at %p\n", pFaxRecvlist);
        
        Crwini rwinTemp;
        
        // 提取 TID（去掉 .tif 扩展名）
        lstrcpyn(pFaxRecvlist->strTID, FileData.cFileName, 40);
        g_log.Print(3, "SearchFile: TID=%s\n", pFaxRecvlist->strTID);
        
        char* dot = strrchr(pFaxRecvlist->strTID, '.');
        if (dot) *dot = 0;
        g_log.Print(3, "SearchFile: TID after removing extension=%s\n", pFaxRecvlist->strTID);

        // 构建 ini 文件路径
        lstrcpyn(szHome, g_sysParam.strPath, MAX_PATH);
        lstrcpyn(&szHome[lstrlen(szHome)], FileData.cFileName, MAX_PATH - lstrlen(szHome));
        int len = lstrlen(szHome);
        if (len >= 3)
        {
            lstrcpy(&szHome[len-3], "ini");
        }
        g_log.Print(3, "SearchFile: ini path=%s\n", szHome);
        
        // 读取 ini 文件
        g_log.Print(3, "SearchFile: calling GetSetting for %s\n", szHome);
        rwinTemp.GetSetting(szHome, pFaxRecvlist);
        g_log.Print(3, "SearchFile: GetSetting completed\n");
        
        // 构建完整文件路径
        lstrcpyn(pFaxRecvlist->strFileName, g_sysParam.strPath, MAX_PATH);
        int currentLen = lstrlen(pFaxRecvlist->strFileName);
        int remaining = MAX_PATH - currentLen - 1;
        if (remaining > 0)
        {
            lstrcpyn(&pFaxRecvlist->strFileName[currentLen], FileData.cFileName, remaining);
        }
        g_log.Print(3, "SearchFile: strFileName=%s\n", pFaxRecvlist->strFileName);
        
        nRet++;
        
        EnterCriticalSection(&g_csRecvList);
        g_lsRecvFax.push_back(pFaxRecvlist);
        LeaveCriticalSection(&g_csRecvList);
        g_log.Print(3, "SearchFile: added to queue, total=%d\n", nRet);

        if (!FindNextFile(hSearch, &FileData)) 
        {
            if (GetLastError() == ERROR_NO_MORE_FILES) 
            { 
                fFinished = TRUE; 
                g_log.Print(3, "SearchFile: no more files\n");
            } 
            else 
            { 
                g_log.Print(3, "SearchFile: FindNextFile error\n");
            } 
        }
    } 
 
    FindClose(hSearch);
    g_log.Print(3, "SearchFile: completed, found %d files\n", nRet);

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
		// DWORD dwWaitRet = WaitForSingleObject(m_hSemaphore,INFINITE);
		// if(dwWaitRet==WAIT_OBJECT_0)
		// {
			iAddrSize = sizeof(client);
			sClient = accept(sMain, (struct sockaddr *)&client, (socklen_t*)&iAddrSize);
			if (sClient < 0)
			{            
				g_log.Print(3, "accept failed, errno=%d\n", errno);

				//printf("accept() failed: %d\n", errno);
				closesocket(sMain);
				return (void*)1;
			}
			g_log.Print(3, "MainThreadProc: accepted client, socket=%d\n", sClient);

			StartThread(TCPSThreadProc,(LPVOID)sClient);

		//}
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

// int ConnectFileSrv(SOCKET sClientSock)
// {
//     struct hostent    *host = NULL;
// 	sockaddr_in m_destAddr;
// 	BYTE			m_byLastChar;
// 	int				ntype;
// 	int				nPointer;
// 	PAG_HEADER		header;
// 	int				nSize;
// 	BYTE			byResult;

// 	memset(&m_destAddr, 0, sizeof(m_destAddr));
// 	m_destAddr.sin_family = AF_INET;
// 	if ((m_destAddr.sin_addr.s_addr = inet_addr(g_strIP)) == INADDR_NONE)
// 	{
// 		host = gethostbyname(g_strIP);
// 		if(host != NULL)
// 		{
// 			memcpy(&m_destAddr.sin_addr, host->h_addr_list[0],
// 			    host->h_length);
// 		}
// 		//need check function success?
// 	}
// 	m_destAddr.sin_port = htons((uint16)g_nPort);

//     g_sSend = socket(AF_INET, SOCK_STREAM, 0);
//     if (g_sSend >= 0)
// 	{
// 		if (::connect(g_sSend, (struct sockaddr *)&m_destAddr, 
// 			sizeof(m_destAddr)) < 0)
// 		{
// 			return -1;
// 		}
// 	}

// 	g_log.Print(5,"connect to File server %s.\r\n",g_strIP);
// 	{
// 		TCPOutPacket p;
// 		p << (uint8)TCP_FAX_REQUESTSEND;
// 		p << g_strUUID;
// 		if(SendRSAPacket(g_sSend,&p)==-1)
// 			return -1;

// 		g_log.Print(5,"send TCP_FAX_REQUESTSEND.\r\n");
// 		//GetRSAPacket(g_sSend,sClientSock,&header,ntype, nPointer,m_byLastChar);
// 	}
// 	return 0;
// }

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

	g_log.Print(3, "ConnectFileSrv: START, g_strIP=%s, g_nPort=%d, g_strUUID=%s\n", 
	            g_strIP, g_nPort, g_strUUID);

	memset(&m_destAddr, 0, sizeof(m_destAddr));
	m_destAddr.sin_family = AF_INET;
	
	g_log.Print(3, "ConnectFileSrv: resolving IP address for %s\n", g_strIP);
	
	if ((m_destAddr.sin_addr.s_addr = inet_addr(g_strIP)) == INADDR_NONE)
	{
		g_log.Print(3, "ConnectFileSrv: %s is not an IP address, trying gethostbyname\n", g_strIP);
		host = gethostbyname(g_strIP);
		if(host != NULL)
		{
			memcpy(&m_destAddr.sin_addr, host->h_addr_list[0], host->h_length);
			char *resolvedIp = inet_ntoa(m_destAddr.sin_addr);
			g_log.Print(3, "ConnectFileSrv: resolved %s to IP: %s\n", g_strIP, resolvedIp);
		}
		else
		{
			g_log.Print(3, "ConnectFileSrv: gethostbyname failed for %s, errno=%d\n", g_strIP, errno);
		}
	}
	else
	{
		g_log.Print(3, "ConnectFileSrv: %s is already an IP address\n", g_strIP);
	}
	
	m_destAddr.sin_port = htons((uint16)g_nPort);
	g_log.Print(3, "ConnectFileSrv: target address %s:%d\n", inet_ntoa(m_destAddr.sin_addr), g_nPort);

    g_sSend = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sSend < 0)
	{
		g_log.Print(3, "ConnectFileSrv: socket creation failed, errno=%d\n", errno);
		return -1;
	}
	g_log.Print(3, "ConnectFileSrv: socket created successfully, fd=%d\n", g_sSend);

	g_log.Print(3, "ConnectFileSrv: attempting to connect to %s:%d\n", inet_ntoa(m_destAddr.sin_addr), g_nPort);
	
	if (::connect(g_sSend, (struct sockaddr *)&m_destAddr, sizeof(m_destAddr)) < 0)
	{
		g_log.Print(3, "ConnectFileSrv: connection failed, errno=%d\n", errno);
		closesocket(g_sSend);
		g_sSend = 0;
		return -1;
	}

	g_log.Print(3, "ConnectFileSrv: connected successfully to %s:%d\n", inet_ntoa(m_destAddr.sin_addr), g_nPort);
	g_log.Print(5,"connect to File server %s.\r\n",g_strIP);
	
	{
		g_log.Print(3, "ConnectFileSrv: building TCP_FAX_REQUESTSEND packet, UUID=%s\n", g_strUUID);
		
		TCPOutPacket p;
		p << (uint8)TCP_FAX_REQUESTSEND;
		p << g_strUUID;
		
		g_log.Print(3, "ConnectFileSrv: packet built, length=%d, calling SendRSAPacket\n", p.getLength());
		
		int sendResult = SendRSAPacket(g_sSend, &p);
		if(sendResult == -1)
		{
			g_log.Print(3, "ConnectFileSrv: SendRSAPacket failed, return -1\n");
			return -1;
		}

		g_log.Print(3, "ConnectFileSrv: SendRSAPacket succeeded\n");
		g_log.Print(5,"send TCP_FAX_REQUESTSEND.\r\n");
		
		// 添加：尝试读取响应（非阻塞，仅用于调试）
		fd_set readSet;
		struct timeval tv;
		FD_ZERO(&readSet);
		FD_SET(g_sSend, &readSet);
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		
		int selectResult = select(g_sSend + 1, &readSet, NULL, NULL, &tv);
		if (selectResult > 0 && FD_ISSET(g_sSend, &readSet))
		{
			char recvBuf[1024];
			int recvLen = recv(g_sSend, recvBuf, sizeof(recvBuf), MSG_DONTWAIT);
			if (recvLen > 0)
			{
				g_log.Print(3, "ConnectFileSrv: received %d bytes of response immediately\n", recvLen);
				// 打印前16字节的十六进制
				char hexBuf[64] = {0};
				for(int i = 0; i < (recvLen > 16 ? 16 : recvLen); i++) {
					sprintf(hexBuf + i*3, "%02X ", (unsigned char)recvBuf[i]);
				}
				g_log.Print(3, "ConnectFileSrv: response hex: %s\n", hexBuf);
			}
			else if (recvLen == 0)
			{
				g_log.Print(3, "ConnectFileSrv: connection closed by peer\n");
			}
			else
			{
				g_log.Print(3, "ConnectFileSrv: recv error, errno=%d\n", errno);
			}
		}
		else if (selectResult == 0)
		{
			g_log.Print(3, "ConnectFileSrv: no immediate response (timeout after 3 seconds)\n");
		}
		else
		{
			g_log.Print(3, "ConnectFileSrv: select error, errno=%d\n", errno);
		}
	}
	
	g_log.Print(3, "ConnectFileSrv: END, returning 0\n");
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
int ReceivePacketEx(SOCKET tcpSock, BYTE cmd, BYTE* body, int bodyLen, FAX_SESSION &faxSess)
{
    g_log.Print(3, "ReceivePacketEx: cmd=%d, bodyLen=%d\n", cmd, bodyLen);
    
    switch(cmd)
    {
        case GW_LOGIN:
        {
            if(bodyLen < 2) {
                g_log.Print(3, "GW_LOGIN: body too short\n");
                return -1;
            }
            int strLen = (body[0] << 8) | body[1];
            if(bodyLen < 2 + strLen) {
                g_log.Print(3, "GW_LOGIN: string length mismatch\n");
                return -1;
            }
            char* strCode1 = new char[strLen + 1];
            memcpy(strCode1, body + 2, strLen);
            strCode1[strLen] = '\0';
            
            g_log.Print(3, "GW_LOGIN: gatewayKey from client=%s, expected=%s\n", 
                        strCode1, g_sysParam.strGWKey);
            
            if(strcmp(strCode1, g_sysParam.strGWKey) == 0)
            {
                faxSess.bIsLogin = true;
                TCPOutPacket out;
                out << (uint8)GW_LOGIN;
                out << (uint8)LOGIN_SUCCESS;
                out << (uint8)LOGIN_SUCCESS;
                if(SendPacket(tcpSock, out, faxSess) == -1)
                {
                    delete[] strCode1;
                    return -1;
                }
                g_log.Print(3, "GW_LOGIN success.\n");
            }
            else
            {
                TCPOutPacket out;
                out << (uint8)GW_LOGIN;
                out << (uint8)LOGIN_INVALID_USER;
                SendPacket(tcpSock, out, faxSess);
                delete[] strCode1;
                return -1;
            }
            delete[] strCode1;
            break;
        }
    	case GW_KEEPALIVE:
        {
            g_log.Print(3, "GW_KEEPALIVE received\n");
            
            // 构建心跳响应
            TCPOutPacket out;
            out << (uint8)GW_KEEPALIVE;
            out << g_nReceive;      // 待接收传真数量
            out << (uint8)g_bySystem;  // 系统状态
            if(SendPacket(tcpSock, out, faxSess) == -1)
            {
                g_log.Print(3, "GW_KEEPALIVE: SendPacket failed\n");
                return -1;
            }
            g_log.Print(3, "GW_KEEPALIVE response sent, g_nReceive=%d, g_bySystem=%d\n", g_nReceive, g_bySystem);
            break;
        }
        
		case GW_FAX_REQUESTSEND:
        {
            g_log.Print(3, "GW_FAX_REQUESTSEND received, bodyLen=%d\n", bodyLen);
            // 处理前检查并重置
            if(faxSess.nType != 0 && faxSess.nType != 1)
            {
                g_log.Print(3, "GW_FAX_REQUESTSEND: invalid session state\n");
                return -1;
            }
            if(faxSess.nType != 0)
            {
                g_log.Print(3, "GW_FAX_REQUESTSEND: invalid session state\n");
                return -1;
            }
            
            faxSess.nType = 1;
            ResetEvent(g_hSendRequest);
            
            // 解析 body 中的数据
            // body 格式: [国家代码][区号][传真号码][分机号][文件大小][CSID]
            int offset = 0;
            
            // 国家代码
            int strLen = (body[offset] << 8) | body[offset+1];
            offset += 2;
            char* strCountryCode = new char[strLen + 1];
            memcpy(strCountryCode, body + offset, strLen);
            strCountryCode[strLen] = '\0';
            offset += strLen;
            
            // 区号
            strLen = (body[offset] << 8) | body[offset+1];
            offset += 2;
            char* strAreaCode = new char[strLen + 1];
            memcpy(strAreaCode, body + offset, strLen);
            strAreaCode[strLen] = '\0';
            offset += strLen;
            
            // 传真号码
            strLen = (body[offset] << 8) | body[offset+1];
            offset += 2;
            char* strFaxCode = new char[strLen + 1];
            memcpy(strFaxCode, body + offset, strLen);
            strFaxCode[strLen] = '\0';
            offset += strLen;
            
            // 分机号
            strLen = (body[offset] << 8) | body[offset+1];
            offset += 2;
            char* strExtCode = new char[strLen + 1];
            memcpy(strExtCode, body + offset, strLen);
            strExtCode[strLen] = '\0';
            offset += strLen;
            
            // 文件大小
            DWORD dwFileSize = (body[offset] << 24) | (body[offset+1] << 16) | 
                               (body[offset+2] << 8) | body[offset+3];
            offset += 4;
            
            // CSID
            strLen = (body[offset] << 8) | body[offset+1];
            offset += 2;
            char* strCSID = new char[strLen + 1];
            memcpy(strCSID, body + offset, strLen);
            strCSID[strLen] = '\0';
            offset += strLen;
            
            g_log.Print(5, "send FAX request %s%s-%s-%s, CSID=%s, fileSize=%u\n", 
                        strCountryCode, strAreaCode, strFaxCode, strExtCode, strCSID, dwFileSize);
            
            // 调用 TcpSession 处理发送请求
            g_tcpSession.onSendRequest(strCountryCode, strAreaCode, strFaxCode, strExtCode, dwFileSize, strCSID);
            
            // 等待响应
            DWORD dwResult = WaitForSingleObject(g_hSendRequest, 5000);
            if(dwResult != WAIT_OBJECT_0)
            {
                g_log.Print(5, "server not respond the send FAX request\n");
                delete[] strCountryCode;
                delete[] strAreaCode;
                delete[] strFaxCode;
                delete[] strExtCode;
                delete[] strCSID;
                return -1;
            }
            
            // 连接到文件服务器
            if(g_bySendCode != 0)
            {
                if(ConnectFileSrv(tcpSock) == -1)
                {
                    g_bySendCode = 0;
                }
            }
            
            int ntemp = g_bySendCode;
            g_log.Print(5, "return send FAX request: %d, IP:%s Port:%d, UUID:%s\n", 
                        ntemp, g_strIP, g_nPort, g_strUUID);
            
            // 构建响应
            TCPOutPacket out;
            out.cmd = GW_FAX_REQUESTSEND;
            out << (uint8)GW_FAX_REQUESTSEND;
            out << g_bySendCode;
            out << g_strUUID;
			out << g_strIP;      // 添加文件服务器 IP
            out << g_nPort;      // 添加文件服务器端口
            
            if(SendPacket(tcpSock, out, faxSess) == -1)
            {
                delete[] strCountryCode;
                delete[] strAreaCode;
                delete[] strFaxCode;
                delete[] strExtCode;
                delete[] strCSID;
                return -1;
            }
            
            delete[] strCountryCode;
            delete[] strAreaCode;
            delete[] strFaxCode;
            delete[] strExtCode;
            delete[] strCSID;

            break;
        }
		case GW_FAX_SEND:
        {
			g_log.Print(3, "GW_FAX_SEND received, nType=%d, bodyLen=%d\n", faxSess.nType, bodyLen);
			
			if(faxSess.nType != 1)
			{
				g_log.Print(3, "GW_FAX_SEND: invalid session state (expected 1, got %d)\n", faxSess.nType);
				return -1;
			}
			
			if(bodyLen <= 0)
			{
				g_log.Print(3, "GW_FAX_SEND: no data\n");
				return -1;
			}
			
			// 数据从 body[0] 开始（没有额外的命令码，因为 cmd 已经单独传过来了）
			// 所以整个 body 就是文件数据
			g_log.Print(3, "GW_FAX_SEND: sending %d bytes to File Server\n", bodyLen);
			
			if(SendFaxData(tcpSock, (const char*)body, bodyLen) == -1)
				return -1;
			
			break;
		}

		case GW_FAX_STOPSEND:
		{
			g_log.Print(3, "GW_FAX_STOPSEND received, nType=%d\n", faxSess.nType);
			
			if(faxSess.nType != 1)
			{
				g_log.Print(3, "GW_FAX_STOPSEND: invalid session state (expected 1, got %d)\n", faxSess.nType);
				return -1;
			}
			
			if(SendFaxEnd(tcpSock) == -1)
				return -1;
			faxSess.nType = 0;
			g_log.Print(3, "GW_FAX_STOPSEND: completed\n");
			return 0;
		}
        // ============================================================
        // 接收传真处理 - 文件服务器发送数据给网关
        // ============================================================
        case GW_FAX_RECV:
        {
            g_log.Print(3, "GW_FAX_RECV received, nType=%d, bodyLen=%d\n", faxSess.nType, bodyLen);
            
            if(faxSess.nType != 4)
            {
                g_log.Print(3, "GW_FAX_RECV: invalid session state (expected 4, got %d)\n", faxSess.nType);
                return -1;
            }
            
            if(bodyLen < 1) {
                g_log.Print(3, "GW_FAX_RECV: no data\n");
                return -1;
            }
            
            // 第一个字节是 byCode（0=成功，1=失败）
            BYTE byCode = body[0];
            BYTE* fileData = body + 1;
            int dataLen = bodyLen - 1;
            
            g_log.Print(3, "GW_FAX_RECV: byCode=%d, dataLen=%d\n", byCode, dataLen);
            
            if(faxSess.bIsEnd)
            {
                // 文件已结束，发送停止接收
                g_log.Print(3, "GW_FAX_RECV: file end, sending STOPRECV\n");
                faxSess.nType = 5;
                TCPOutPacket out;
                out << (uint8)GW_FAX_STOPRECV;
                if(SendPacket(tcpSock, out, faxSess) == -1)
                    return -1;
            }
            else
            {
                if(faxSess.hSendFile == NULL || faxSess.hSendFile == INVALID_HANDLE_VALUE)
                {
                    g_log.Print(3, "GW_FAX_RECV: hSendFile is NULL, cannot write\n");
                    return -1;
                }
                
                DWORD dwWritten;
                if(WriteFile(faxSess.hSendFile, fileData, dataLen, &dwWritten, NULL))
                {
                    if(dwWritten == (DWORD)dataLen)
                    {
                        // 发送接收成功响应
                        TCPOutPacket out;
                        out << (uint8)GW_FAX_RECV;
                        out << (uint8)0;
                        if(SendPacket(tcpSock, out, faxSess) == -1)
                            return -1;
                        g_log.Print(3, "GW_FAX_RECV: wrote %d bytes, sent ACK\n", dataLen);
                        
                        if(faxSess.dwFileSize >= (DWORD)dataLen)
                            faxSess.dwFileSize -= dataLen;
                        else
                            faxSess.dwFileSize = 0;
                    }
                    else
                    {
                        g_log.Print(3, "GW_FAX_RECV: WriteFile wrote %d/%d bytes\n", dwWritten, dataLen);
                        TCPOutPacket out;
                        out << (uint8)GW_FAX_RECV;
                        out << (uint8)1;
                        SendPacket(tcpSock, out, faxSess);
                        return -1;
                    }
                }
                else
                {
                    g_log.Print(3, "GW_FAX_RECV: WriteFile failed, errno=%d\n", errno);
                    TCPOutPacket out;
                    out << (uint8)GW_FAX_RECV;
                    out << (uint8)1;
                    SendPacket(tcpSock, out, faxSess);
                    return -1;
                }
            }
            break;
        }
        
        // ============================================================
        // 接收传真完成处理
        // ============================================================
        case GW_FAX_STOPRECV:
        {
            g_log.Print(3, "GW_FAX_STOPRECV received, nType=%d, bodyLen=%d\n", faxSess.nType, bodyLen);
            
            if(faxSess.nType != 5)
            {
                g_log.Print(3, "GW_FAX_STOPRECV: invalid session state (expected 5, got %d)\n", faxSess.nType);
                // 仍然尝试关闭
            }
            
            BYTE byCode = 0;
            if(bodyLen >= 1) {
                byCode = body[0];
            }
            
            g_log.Print(3, "GW_FAX_STOPRECV: byCode=%d\n", byCode);
            
            if(byCode == 0)
            {
                // 关闭文件句柄
                if(faxSess.hSendFile != NULL && faxSess.hSendFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(faxSess.hSendFile);
                    faxSess.hSendFile = NULL;
                    g_log.Print(3, "GW_FAX_STOPRECV: file handle closed\n");
                }
                
                // 发送停止接收响应
                TCPOutPacket out;
                out << (uint8)GW_FAX_STOPRECV;
                out << (uint8)0;
                SendPacket(tcpSock, out, faxSess);
                
                g_log.Print(3, "GW_FAX_STOPRECV: receive completed successfully\n");
            }
            else
            {
                g_log.Print(3, "GW_FAX_STOPRECV: receive failed\n");
            }
            
            // 关闭 socket 连接
            closesocket(tcpSock);
            break;
        }
   case GW_FAX_REQUESTRECV:
{
    g_log.Print(3, "GW_FAX_REQUESTRECV received, pending count=%d\n", g_nReceive);
    
    if(faxSess.nType == 0)
    {
        faxSess.nType = 4;
        
        if(g_nReceive > 0)
        {
            EnterCriticalSection(&g_csRecvList);
            list<FAX_RECVLIST *>::iterator iter = g_lsRecvFax.begin();
            
            if(iter != g_lsRecvFax.end())
            {
                FAX_RECVLIST* pFax = *iter;
                
                // 获取文件大小
                DWORD dwFileLow = 0;
                DWORD dwFileHigh = 0;
                HANDLE hFile = CreateFile(pFax->strFileName, GENERIC_READ, FILE_SHARE_READ, 
                                          NULL, OPEN_EXISTING, 0, NULL);
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    dwFileLow = GetFileSize(hFile, &dwFileHigh);
                    CloseHandle(hFile);
                }
                
                // 确保传真类型有效
                if(pFax->nFAXType != 1 && pFax->nFAXType != 2)
                    pFax->nFAXType = 1;
                
                g_log.Print(5, "%s request recv %d, fileSize=%u\n", 
                           pFax->strTID, pFax->nFAXType, dwFileLow);
                
                TCPOutPacket out;
                out << (uint8)GW_FAX_REQUESTRECV;
                out << (uint8)pFax->nFAXType;
                out << (uint32)dwFileLow;
                out << pFax->strCountryCode;
                out << pFax->strAreaCode;
                out << pFax->strFaxCode;
                out << pFax->strExtCode;
                out << pFax->strCallerID;
                out << pFax->strTID;
                out << pFax->strCSID;
                
                // if(SendPacket(tcpSock, out, faxSess) == -1)
                // {
                //     LeaveCriticalSection(&g_csRecvList);
                //     return -1;
                // }
				if(SendPacket(tcpSock, out, faxSess) != -1)
                {
                    // ✅ 发送成功后，从队列中删除
                    g_lsRecvFax.erase(iter);
                    g_nReceive--;
                    g_log.Print(3, "GW_FAX_REQUESTRECV: sent and removed, TID=%s, remaining=%d\n", 
                               pFax->strTID, g_nReceive);
                    delete pFax;  // 释放内存
                }
            }
            LeaveCriticalSection(&g_csRecvList);
        }
        else
        {
            // 无待接收传真，只发送类型0
            TCPOutPacket out;
            out << (uint8)GW_FAX_REQUESTRECV;
            out << (uint8)0;
            if(SendPacket(tcpSock, out, faxSess) == -1)
                return -1;
        }
        
        faxSess.nType = 0;  // 重置状态
    }
    break;
}
				
        default:
            g_log.Print(3, "ReceivePacketEx: unknown cmd=%d\n", cmd);
            break;
    }
    return 0;
}

int ReceivePacket(SOCKET tcpSock,TCPInPacket &in,FAX_SESSION &faxSess)
{
	g_log.Print(3, "ReceivePacket: cmd=%d\n", in.header.cmd);
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
		    g_log.Print(3, "ReceivePacket: processing GW_LOGIN\n");

			in >> strCode1;
   g_log.Print(3, "ReceivePacket: gatewayKey from client=%s, expected=%s\n", 
                strCode1, g_sysParam.strGWKey);
			if(strcmp(strCode1,g_sysParam.strGWKey)==0)
			{
				        g_log.Print(3, "ReceivePacket: gatewayKey matched!\n");

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
		    g_log.Print(3, "GW_FAX_REQUESTSEND received,  faxSess.nType=%d\n", 
                faxSess.nType);
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
		    g_log.Print(3, "GW_KEEPALIVE received, g_nReceive=%d, g_bySystem=%d\n", g_nReceive, g_bySystem);

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
    BYTE            byBuff[512];
    BYTE            m_byLastChar = 0;
    int             ntype = 0;
    int             nPointer = 0;
    GW_HEADER       header;           // 使用 GW_HEADER (7字节)
    BYTE            byPacket[2048];
    int             nSize;
    FAX_SESSION     faxSess;
    SOCKET          tcpSock;

    tcpSock = (SOCKET)(long)lpParam;
    g_log.Print(3, "TCPSThreadProc: entered, socket=%d\n", tcpSock);

	if (tcpSock < 0) {
	g_log.Print(3, "TCPSThreadProc: invalid socket\n");
	return (void*)-1;
    }
    
    ZeroMemory(&faxSess, sizeof(faxSess));
    faxSess.bIsLogin = false;
    faxSess.nType = 0;

    while(true)
    {
        nSize = 512;
		        g_log.Print(3, "TCPSThreadProc: calling recv on socket %d\n", tcpSock);

        nSize = recv(tcpSock, (char *)byBuff, nSize, 0);
		        g_log.Print(3, "TCPSThreadProc: recv returned %d, errno=%d\n", nSize, errno);

        if(nSize > 0)
        {

			g_log.Print(3, "TCPSThreadProc: received %d bytes\n", nSize);
            // 打印前16字节
            char hexbuf[64] = {0};
            // for(int i = 0; i < (nSize > 16 ? 16 : nSize); i++) {
            //     sprintf(hexbuf + i*3, "%02X ", byBuff[i]);
            // }
            //g_log.Print(3, "TCPSThreadProc: data: %s\n", hexbuf);
            for(int i = 0; i < nSize; i++)
            {
				//   g_log.Print(3, "TCPSThreadProc: byte[%d]=0x%02X, lastChar=0x%02X, ntype=%d\n", 
                // i, byBuff[i], m_byLastChar, ntype);
    
                if((BYTE)byBuff[i] == 0xE3 && m_byLastChar == 0x3E && ntype == 0)
                {
					        g_log.Print(3, "TCPSThreadProc: found packet start\n");

                    ntype = 1;
                    nPointer = 0;
                    *((PBYTE)&header + nPointer) = 0x3E;
                    nPointer++;
                    *((PBYTE)&header + nPointer) = 0xE3;
                    nPointer++;
                }
                else
                {
                    if(ntype == 1)
                    {
                        *((PBYTE)&header + nPointer) = (BYTE)byBuff[i];
                        nPointer++;
						            //g_log.Print(3, "TCPSThreadProc: header pos %d, value 0x%02X\n", nPointer-1, (BYTE)byBuff[i]);
                        if(nPointer >= sizeof(GW_HEADER))
                        {
							    header.nLength = ntohl(header.nLength);

							               // g_log.Print(3, "TCPSThreadProc: header complete, nLength=%d\n", header.nLength);

                            ntype = 2;
                            nPointer = 0;
                        }
                    }
                    else if(ntype == 2)
                    {
                        byPacket[nPointer] = (BYTE)byBuff[i];
                        nPointer++;
                        // if(nPointer >= header.nLength)
                        // {
						// 	                g_log.Print(3, "TCPSThreadProc: packet complete, body length=%d\n", header.nLength);

                        //     ntype = 0;
                        //     nPointer = 0;

                        //     // 明文数据，直接处理
                        //     // TCPInPacket in(byPacket, header.nLength, 1);
    					// 	TCPInPacket in(byPacket, header.nLength, 1);

						// 	g_log.Print(3, "TCPSThreadProc: calling ReceivePacket\n");

                        //     if(ReceivePacket(tcpSock, in, faxSess) == -1)
                        //     {
                        //         closesocket(tcpSock);
                        //         ReleaseSemaphore(FileSession::m_hSemaphore, 1, NULL);
                        //         return (void*)-1;
                        //     }
                        // }
						if(nPointer >= header.nLength)
						{
							ntype = 0;
							nPointer = 0;

							if(header.nLength > 0)
							{
								BYTE cmd = byPacket[0];
								BYTE* body = byPacket + 1;
								int bodyLen = header.nLength - 1;
								
								g_log.Print(3, "TCPSThreadProc: calling ReceivePacketEx, cmd=%d, bodyLen=%d\n", cmd, bodyLen);
								
								if(ReceivePacketEx(tcpSock, cmd, body, bodyLen, faxSess) == -1)
								{
									closesocket(tcpSock);
									ReleaseSemaphore(FileSession::m_hSemaphore, 1, NULL);
									return (void*)-1;
								}
							}
						}
                    }
                }
                m_byLastChar = (BYTE)byBuff[i];
            }
        }
		else if(nSize == 0)
        {
            g_log.Print(3, "TCPSThreadProc: connection closed by peer\n");
            closesocket(tcpSock);
            break;
        }
        else
        {
            closesocket(tcpSock);
            break;
        }
    }
    ReleaseSemaphore(FileSession::m_hSemaphore, 1, NULL);
    return 0;
}
// void* FileSession::TCPSThreadProc(void* lpParam)
// {

// 	BYTE	byBuff[512];
// 	BYTE			m_byLastChar;
// 	int				ntype;
// 	int				nPointer;
// 	GW_HEADER		header;
// 	BYTE			byPacket[2048];
// 	int				nSize;
// 	FAX_SESSION		faxSess;
// 	SOCKET			tcpSock;

// 	tcpSock = (SOCKET)(long)lpParam;
// 	g_log.Print(3, "TCPSThreadProc: entered, socket=%d\n", tcpSock);
// 	ZeroMemory(&faxSess,sizeof(faxSess));
// 	faxSess.bIsLogin = false;
// 	faxSess.nType = 0;
// 	ntype=0;
// 	while(true)
// 	{
// 		nSize = 512;
// 		g_log.Print(3, "TCPSThreadProc: waiting for recv on socket %d\n", tcpSock);
// 		nSize = recv(tcpSock, (char *)byBuff, nSize, 0);
// 		g_log.Print(3, "TCPSThreadProc: recv returned %d\n", nSize);
// 		if(nSize > 0) {
// 		    char hexbuf[512] = {0};
// 		    for(int i = 0; i < (nSize > 32 ? 32 : nSize); i++) {
// 		        sprintf(hexbuf + i*3, "%02X ", byBuff[i]);
// 		    }
// 		    g_log.Print(3, "TCPSThreadProc: data: %s\n", hexbuf);
// 		}
// 		g_log.Print(3, "TCPSThreadProc: recv returned %d\n", nSize);

// 		if(nSize > 0)
// 		{
// 			for(int i=0;i< nSize ;i++)
// 			{
// 				if((BYTE)byBuff[i] == 0xE3 && m_byLastChar == 0x3E && ntype==0)
// 				{
// 					//��ʼ�����Ϣ��.
// 					ntype = 1;
// 					nPointer= 0;

// 					*((PBYTE)&header + nPointer)=0x3E;
// 					nPointer++;
// 					*((PBYTE)&header + nPointer)=0xE3;
// 					nPointer++;
// 				}
// 				else
// 				{
// 					if(ntype == 1)
// 					{
// 						*((PBYTE)&header + nPointer)=(BYTE)byBuff[i];
// 						nPointer++;
// 						if(nPointer >= sizeof(GW_HEADER))
// 						{
// 							ntype = 2;
// 							nPointer = 0;
// 						}
// 					}
// 					else if(ntype == 2)
// 					{
// 						byPacket[nPointer] = (BYTE)byBuff[i];
// 						nPointer++; 
// 						if(nPointer >= header.nLength)
// 						{
// 							ntype = 0;
// 							nPointer = 0;

// 								// ����Ϣ���������
// 								TCPInPacket in(byPacket, header.nLength,1);
// 								if(ReceivePacket(tcpSock,in,faxSess)==-1)
// 								{
// 									closesocket(tcpSock);
// 									if(faxSess.nType==1)
// 									{
// 										if(g_sSend)
// 										{
// 											closesocket(g_sSend);
// 											g_sSend = 0;
// 										}
// 									}
// 									else if(faxSess.nType==4 || faxSess.nType==5)
// 									{
// 										if(!(INVALID_HANDLE_VALUE == faxSess.hSendFile || NULL == faxSess.hSendFile))
// 										{
// 											CloseHandle(faxSess.hSendFile);
// 											faxSess.hSendFile = NULL;
// 										}
// 									}

// 									ReleaseSemaphore(FileSession::m_hSemaphore,1,NULL);
// 									return (void*)-1;

// 								}
// 								//in.handle();
// 									//onPacketReceived(in,&SenderAddr);
// 						}

// 					}
// 				}
// 				m_byLastChar = (BYTE)byBuff[i];
// 			}
// 		}
// 		else
// 		{
// 			closesocket(tcpSock);
// 			break;
// 		}
// 	}
// 	if(faxSess.nType==1)
// 	{
// 		if(g_sSend)
// 		{
// 			closesocket(g_sSend);
// 			g_sSend = 0;
// 		}
// 	}
// 	else if(faxSess.nType==4 || faxSess.nType==5)
// 	{
// 		if(!(INVALID_HANDLE_VALUE == faxSess.hSendFile || NULL == faxSess.hSendFile))
// 		{
// 			CloseHandle(faxSess.hSendFile);
// 			faxSess.hSendFile = NULL;
// 		}
// 	}
// 	ReleaseSemaphore(FileSession::m_hSemaphore,1,NULL);


// /*    SOCKET        sMain,
//                   sClient;
//     int           iAddrSize;
//     struct sockaddr_in local,
//                        client;
// 	DWORD			dwFileSize;
// 	DWORD			dwReadSize;
// 	DWORD			dwRet;

// 	FileSession *filesess=(FileSession *)lpParam;
// 	BYTE bNewActive=1,bOldActive;


//     // Create our main socket
//     sMain = socket(AF_INET, SOCK_STREAM, 0);
//     if (sMain < 0)
//     {
//         //printf("socket() failed: %d\n", errno);
// 		SetEvent(filesess->m_hReadyEvent);
//         return 1;
//     }
//     local.sin_family = AF_INET;

// 	if(filesess -> m_bIsListen)
// 	{
// 		local.sin_addr.s_addr = htonl(INADDR_ANY);
		
// 		for(;filesess->m_nPort<10000;filesess->m_nPort++)
// 		{
// 			local.sin_port = htons(filesess->m_nPort);
// 			if (bind(sMain, (struct sockaddr *)&local, 
// 				sizeof(local)) >= 0)
// 				break;
// 		}
// 		if(filesess->m_nPort == 10000)
// 		{
// 			//printf("bind() failed: %d\n", errno);
// 			closesocket(sMain);
// 			SetEvent(filesess->m_hReadyEvent);
// 			return 1;
// 		}

// 		listen(sMain, 1);
		
// 		SetEvent(filesess->m_hReadyEvent);
// 		iAddrSize = sizeof(client);
// 		sClient = accept(sMain, (struct sockaddr *)&client,
// 			&iAddrSize); 
// 	}
// 	else
// 	{
// 		local.sin_addr.s_addr = htonl(filesess->m_nHost);
// 		local.sin_port = htons(filesess->m_nPort);
// 		if (connect(sMain, (struct sockaddr *)&local, 
// 			sizeof(local)) < 0)
// 		{
// 			//printf("connect() failed: %d\n", errno);
// 			return 1;
// 		}
// 		sClient = sMain;
// 	}
// 	if (sClient < 0)
// 	{        
// 		//printf("accept() failed: %d\n", errno);
// 		closesocket(sMain);
// 		return 1;
// 	}
// 	//printf("Accepted client: %s:%d\n", 
// 	//	inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	
// 	filesess->m_dwCurSize =0;
// 	filesess->ReadData(sClient,(unsigned char *)&dwFileSize,sizeof(DWORD));
// 	if(dwFileSize == -1) 
// 	{        
// 		closesocket(sMain);
// 		return 1;
// 	}
// 	if(filesess->InitializeFile(TRUE,TRUE) == -1)
// 	{
// 		dwRet = -1;
// 		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
// 	}
// 	else
// 	{
// 		dwRet = 0;
// 		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
// 		filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
// 		while(1)
// 		{
// 			bOldActive=bNewActive;
// 			if(WaitForSingleObject(filesess->m_hExitEvent,0)!=WAIT_TIMEOUT)
// 			{
// 				if(bOldActive) InterlockedDecrement(&filesess->m_ActiveCount);
// 				break;
// 			}
			
// 			if(!filesess->m_ActiveCount)
// 			{
// 				SetEvent(filesess->m_hExitEvent);
// 				filesess->m_ExitCode=filesess->EXIT;
// 				break;
// 			}
			
// 			if(bNewActive!=bOldActive)
// 			{
// 				bNewActive?InterlockedIncrement(&filesess->m_ActiveCount):InterlockedDecrement(&filesess->m_ActiveCount);
// 			}
// 			else if(!bNewActive)continue;
			
// 			// add function code
// 			dwRet = dwFileSize - filesess->m_dwCurSize;
// 			if(dwRet < MAX_FILEBUFF)
// 			{
// 				dwReadSize = filesess->ReadData(sClient,filesess->m_byFileBuff,dwRet);
// 				if(dwRet != dwReadSize)
// 				{
// 					dwRet = -1;
// 					break;
// 				}
// 				filesess->m_dwCurSize = dwFileSize;
// 				break;
// 			}
// 			else
// 			{
// 				if(MAX_FILEBUFF != filesess->ReadData(sClient,filesess->m_byFileBuff,MAX_FILEBUFF))
// 				{
// 					dwRet = -1;
// 					break;
// 				}
// 				filesess->m_dwCurSize += MAX_FILEBUFF;
// 			}
// 			if(filesess->WriteFile(filesess->m_byFileBuff,MAX_FILEBUFF)== -1)
// 			{
// 				dwRet = -1;
// 				send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
// 				break;
// 			}
// 			dwReadSize = 0;
// 			dwRet = 0;
// 			send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
// 			filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
// 		}
// 	}
// 	if(dwReadSize!=0 && dwRet!=-1)
// 	{
// 		if(filesess->WriteFile(filesess->m_byFileBuff,dwReadSize)== -1)
// 		{
// 			dwRet = -1;
// 		}
// 		else
// 			dwRet = 0;
// 		send(sClient, (const char *)&dwRet, sizeof(DWORD), 0);
// 	}
// 	filesess->m_TransCallBackFunc(filesess->m_dwCallID,dwFileSize,filesess->m_dwCurSize);
// 	filesess->CloseFile();
//     if(filesess -> m_bIsListen) closesocket(sClient);
//     closesocket(sMain);
	
// 	delete filesess;*/
// 	return 0;
// }

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