/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/
#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include "Log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const int Log::ToDebug   =  1;
const int Log::ToFile    =  2;
const int Log::ToConsole =  4;

const static int LINE_BUFFER_SIZE = 1024;

CRITICAL_SECTION	Log::m_csSession;

Log::Log(int mode, int level, char* filename, bool append)
{
    hlogfile = NULL;
    m_todebug = false;
    m_toconsole = false;
    m_tofile = false;
    SetMode(mode);
	InitializeCriticalSection(&m_csSession);
    if (mode & ToFile)  {
        SetFile(filename, append);
    }
}

void Log::SetMode(int mode) {
    
    if (mode & ToDebug)
        m_todebug = true;
    else
        m_todebug = false;

    if (mode & ToFile)  {
        m_tofile = true;
    } else {
        CloseFile();
        m_tofile = false;
    }
    
    if (mode & ToConsole) {
        m_toconsole = true;
    } else {
        m_toconsole = false;
    }
}


void Log::SetLevel(int level) {
    m_level = level;
}

void Log::SetFile(char* filename, bool append) 
{
    // if a log file is open, close it now.
    CloseFile();

    m_tofile  = true;
    
    hlogfile = CreateFile(
        filename,  GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
    
    if (hlogfile == INVALID_HANDLE_VALUE) {
        m_todebug = true;
        m_tofile = false;
        Print(0, "Error opening log file %s\n", filename ? filename : "null");
    } else {
        if (append) {
            SetFilePointer( hlogfile, 0, NULL, FILE_END );
        } else {
            SetEndOfFile( hlogfile );
        }
    }
}

// if a log file is open, close it now.
void Log::CloseFile() {
    if (hlogfile != NULL) {
        CloseHandle(hlogfile);
        hlogfile = NULL;
    }
}


#ifndef UNDER_CE

// Non-CE version 

void Log::ReallyPrint(char* format, va_list ap) 
{
	SYSTEMTIME		stCur;
    char line1[LINE_BUFFER_SIZE];
    char line[LINE_BUFFER_SIZE];
    vsprintf(line1, format, ap);
	GetLocalTime(&stCur);
	//sprintf(line,"%02d [%02d:%02d:%02d] %s",stCur.wDay,stCur.wHour,stCur.wMinute,stCur.wSecond,line1);
    snprintf(line, sizeof(line), "%02d [%02d:%02d:%02d] %s", stCur.wDay, stCur.wHour, stCur.wMinute, stCur.wSecond, line1);
    if (m_todebug) OutputDebugString(line);

    if (m_toconsole) {
        printf("%s", line);
        fflush(stdout);
    }

	EnterCriticalSection(&m_csSession);
    if (m_tofile && (hlogfile != NULL)) {
        DWORD byteswritten;
        WriteFile(hlogfile, line, strlen(line), &byteswritten, NULL); 
    }	
	LeaveCriticalSection(&m_csSession);
}

#else

// CE version 

void Log::ReallyPrint(char* format, va_list ap) 
{
    char line[LINE_BUFFER_SIZE];
    vsprintf(line, format, ap);
    if (m_todebug) OutputDebugString(line);

    if (m_tofile && (hlogfile != NULL)) {
        DWORD byteswritten;
		WriteFile(hlogfile, line, strlen(line), &byteswritten, NULL); 
    }	
}

#endif

Log::~Log()
{
    CloseFile();
}

Log theLog;