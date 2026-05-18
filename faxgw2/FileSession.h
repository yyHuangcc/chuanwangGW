// FileSession.h: interface for the FileSession class.
/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/

#ifndef _FILE_SESSION_H
#define _FILE_SESSION_H

#pragma once

#include "linux_compat.h"
//#include "ecsimtypes.h"

#define OP_FILENAME    0x01
#define OP_FILETEXT    0x02
 
#define WM_THREADEXIT    (0x400+100) 
#define WM_THREADCOUNT   (0x400+101) 
#define	WM_FINDERITEM    (0x400+102) 
#define WM_THREADPAUSE   (0x400+103) 
#define WM_FINDERFOLDER  (0x400+104)

#define MAX_SOCKBUFF		  1024
#define MAX_FILEBUFF		  20*1024 //100k
#define MAX_THREAD			  100
#define MAX_FILELEN			  100

typedef int (* TF_CALLBACK_FUNC ) (DWORD dwCallID, DWORD dwFileSize, DWORD dwCurSize);

class FileSession  
{

//Function areas������������������������������������������������������������������������
public:
	FileSession();
	FileSession(BOOL bIsSend,BOOL bIsTCP, BOOL bIsListen,int nPort);
	virtual ~FileSession();
	
	//initialize setting
	void SessSet(BOOL bIsSend,BOOL bIsTCP, BOOL bIsListen,int nPort);
	void ThreadSet(LONG MaxThreadCount=5,int priority=0);
	void SessSet(const char* strFileName);
	void SessSet(int nPort);
	//void SessSet(SOCKET sSocket);

	//Option of file session
	void SessOption(DWORD dwSessOption);
    
    //operate of programming
	BOOL StartSession();
	void PauseSession();
	void ResumeSession();
	void StopSession();
	void SessionReset();
	BOOL QueryReady();
    
	int InitializeFile(const char* szDesFileName,BOOL bIsReadWrite, BOOL bIsCreate);
	int ReadFile(LPBYTE lpBuff,int nSize);
	int WriteFile(LPBYTE lpBuff,int nSize);
	int GetFileSize(DWORD* lpdwLow,DWORD* lpdwHigh);
	int CloseFile();
	BOOL GetDirectory();

    static DWORD ReadData(SOCKET sSock,LPBYTE lpBuff,int nSize);

	static char m_strDirectory[MAX_PATH];

	//Get result
	LONG GetThreadCount(){return m_MaxThreadCount;};     //get current thread number
	int  GetThreadPrioriy(){return m_Priority;};
	
	LPCTSTR GetErrMsg()const;
	int GetErrCode()const{return m_nErrCode;};

private:
	
	inline void SetErrCode(int nErrCode){m_nErrCode=nErrCode;};
    
	//create a thread
	static HANDLE StartThread(void* (*lpStartAddress)(void*),void* lpParam);
    //main thread of trans/recv
	static void* TCPSThreadProc(void* lpParam);
	static void* TCPCThreadProc(void* lpParam);
	static void* UDPThreadProc(void* lpParam);
    static void* MainThreadProc(void* lpParam);

//data areas����������������������������������������������������������������������
private:
	LONG	m_MaxThreadCount;
	BYTE	m_Option;
	BOOL	m_bIsSend;
	BOOL	m_bIsTCP;
	BYTE	m_byBuff[MAX_SOCKBUFF];
	BYTE	m_byFileBuff[MAX_FILEBUFF];
	DWORD	m_dwCurSize;
	DWORD	m_dwCurBuffSize;
	HANDLE	m_hFile;
	SOCKET	m_sSocket;
	
	LONG	m_ActiveCount;     //Currect active thread number
	int		m_Priority;        //thread Priority
	HANDLE	*m_hThrds;
	int		m_nErrCode;		   //error code
	//CString	m_szErrText;	   //error text

    HANDLE		m_hExitEvent;
    HANDLE		m_hReadyEvent;
	enum EXIT_CODE{EXIT,PAUSE,STOP,ERR};
	EXIT_CODE	m_ExitCode;

	//Priority const
	const static int HIGHEST_PRIORITY;
	const static int ABOVE_NORMAL;
	const static int NORMAL_PRIORITY;
	const static int BELOW_NORMAL;
	const static int LOWEST_PRIORITY;

public:
	int		m_nPort;
	DWORD	m_dwCallID;
	DWORD	m_bIsListen;

	TF_CALLBACK_FUNC m_TransCallBackFunc;

	static HANDLE m_hSemaphore;
};

#endif