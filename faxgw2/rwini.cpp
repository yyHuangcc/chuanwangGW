// rwini.cpp: implementation of the Crwini class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdlib.h>
#include "rwini.h"

static char g_strIniFile[]="faxgateway.dat";
static char g_strsysApp[]="SYSTEM";
static char g_strPortKey[]="Port";
static char g_strHostKey[]="Host";
static char g_strUserKey[]="UID";
static char g_strPWKey[]="SCode";
static char g_strPathKey[]="Path";
static char g_strGatewayKey[]="GatewayKey";
static char g_strCallerIDKey[]="CallerID";
static char g_strTIDKey[]="TID";
static char g_strFilenameKey[]="Filename";
static char g_strFAXTypeKey[]="FAXType";
static char g_strCountryCodeKey[]="CountryCode";
static char g_strAreaCodeKey[]="AreaCode";
static char g_strFaxCodeKey[]="FaxCode";
static char g_strExtCodeKey[]="ExtCode";
static char g_strCSIDKey[]="CSID";
static char g_strServerPortKey[]="ServerPort";
static char g_strFilePortKey[]="FilePort";
static char g_strVersionApp[]="Version";
static char g_strVersionNoKey[]="VersionNo";


long		gCurrentTotalMinute;
long		gCurrentTotalHour;

extern	char			g_strFilePath[MAX_PATH]; 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Crwini::Crwini()
{
	GetModuleFileName(NULL,m_strFilePath,MAX_PATH); 
	char* p = strrchr(m_strFilePath,'/');
	if(p) p[1] = 0;
	else { char* p2 = strrchr(m_strFilePath,'\\'); if(p2) p2[1] = 0; }
	lstrcpyn(g_strFilePath,m_strFilePath,MAX_PATH);
}

Crwini::~Crwini()
{

}

bool Crwini::WriteString(LPCTSTR appname, LPCTSTR keyname,char* s, char *fn)
{
   //MessageBox(hWnd,FilePath,"a",0);
	WritePrivateProfileString(appname,keyname,s,fn);
	return 1;
}

int Crwini::GetInt(LPCTSTR appname, LPCTSTR keyname, char *fn)
{
/*CHAR FilePath[255]; 
    GetModuleFileName(NULL,FilePath,255); 
(strrchr(FilePath,'\\'))[1] = 0;*/ 
	//gCurrentTotalMinute = GetPrivateProfileInt("Mail","MAPITRANS",0,"win.ini");
	//gCurrentTotalHour   = GetPrivateProfileInt("386enh","COMMCACHELEN",0,"system.ini");

	//CHAR strFilePath[MAX_PATH]; 
	//strcpy(strFilePath,m_strFilePath);
	//strcat(strFilePath,fn);
   //MessageBox(hWnd,FilePath,"a",0);
   return GetPrivateProfileInt(appname,keyname,1,fn);
}


void Crwini::ReadString(char *key, char *s, char *fn,char str[])
{
/*	CHAR FilePath[255]; 
    GetModuleFileName(NULL,FilePath,255); 
	(strrchr(FilePath,'\\'))[1] = 0; 
	strcat(FilePath,fn);*/
	//CHAR strFilePath[MAX_PATH]; 
	//strcpy(strFilePath,m_strFilePath);
	//strcat(strFilePath,fn);
	
	::GetPrivateProfileString(key,s,NULL,str,255,fn);
	
	
}

void Crwini::ReadString(char* strFilename, char *key, char *s, char str[],int nLength)
{
/*	CHAR FilePath[255]; 
    GetModuleFileName(NULL,FilePath,255); 
	(strrchr(FilePath,'\\'))[1] = 0; 
	strcat(FilePath,fn);*/

	
	::GetPrivateProfileString(key,s,NULL,str,nLength,strFilename);
	
	
}

void Crwini::WriteInt(char *keyname, char *appname, char *fn, int i)
{
	char	strNum[20];
	sprintf(strNum,"%d",i);
	WritePrivateProfileString(appname,keyname,strNum,fn);
}

BOOL Crwini::GetSetting(char* strIniFile,FAX_RECVLIST* pfaxParam)
{
	ReadString(strIniFile,g_strsysApp,g_strCallerIDKey, pfaxParam->strCallerID,30);
	ReadString(strIniFile,g_strsysApp,g_strTIDKey, pfaxParam->strTID,40);
	ReadString(strIniFile,g_strsysApp,g_strFilenameKey, pfaxParam->strFileName,MAX_PATH);
	ReadString(strIniFile,g_strsysApp,g_strCountryCodeKey, pfaxParam->strCountryCode ,10);
	ReadString(strIniFile,g_strsysApp,g_strAreaCodeKey, pfaxParam->strAreaCode ,10);
	ReadString(strIniFile,g_strsysApp,g_strFaxCodeKey, pfaxParam->strFaxCode,20);
	ReadString(strIniFile,g_strsysApp,g_strExtCodeKey, pfaxParam->strExtCode,10);
	ReadString(strIniFile,g_strsysApp,g_strCSIDKey, pfaxParam->strCSID,20);
	pfaxParam->nFAXType=GetPrivateProfileInt(g_strsysApp,g_strFAXTypeKey,1,strIniFile);
	return TRUE;
}

BOOL Crwini::WriteSetting(char* strIniFile,FAX_RECVLIST* pfaxParam)
{
	WriteString(g_strsysApp,g_strCallerIDKey, pfaxParam->strCallerID,strIniFile);
	WriteString(g_strsysApp,g_strTIDKey, pfaxParam->strTID,strIniFile);
	WriteString(g_strsysApp,g_strFilenameKey, pfaxParam->strFileName,strIniFile);
	WriteString(g_strsysApp,g_strCountryCodeKey, pfaxParam->strCountryCode ,strIniFile);
	WriteString(g_strsysApp,g_strAreaCodeKey, pfaxParam->strAreaCode ,strIniFile);
	WriteString(g_strsysApp,g_strFaxCodeKey, pfaxParam->strFaxCode,strIniFile);
	WriteString(g_strsysApp,g_strExtCodeKey, pfaxParam->strExtCode,strIniFile);
	WriteString(g_strsysApp,g_strCSIDKey, pfaxParam->strCSID,strIniFile);
	WriteInt(g_strFAXTypeKey,g_strsysApp,strIniFile,pfaxParam->nFAXType);
	return TRUE;
}

BOOL Crwini::GetSetting(SYS_PARAM* pdbParam)
{
	char	strFilePath[MAX_PATH];
	lstrcpyn(strFilePath,m_strFilePath,MAX_PATH);
	lstrcat(strFilePath,g_strIniFile);
	pdbParam->nPort = GetInt(g_strsysApp,g_strPortKey,strFilePath);
	pdbParam->nServerPort = GetInt(g_strsysApp,g_strServerPortKey,strFilePath);
	pdbParam->nFilePort = GetInt(g_strsysApp,g_strFilePortKey,strFilePath);

	ReadString(g_strsysApp,g_strPathKey, strFilePath,pdbParam->strPath);
	ReadString(g_strsysApp,g_strHostKey, strFilePath,pdbParam->strHost);
	ReadString(g_strsysApp,g_strPWKey, strFilePath,pdbParam->strPasswd);
	ReadString(g_strsysApp,g_strUserKey, strFilePath,pdbParam->strUser);
	ReadString(g_strsysApp,g_strGatewayKey, strFilePath,pdbParam->strGWKey);
	return TRUE;
}

BOOL Crwini::GetVersionInfo(char* strVersion)
{
	char		strTemp[512];
	char	strFilePath[MAX_PATH];
	lstrcpyn(strFilePath,m_strFilePath,MAX_PATH);
	lstrcat(strFilePath,"Autoupdate.dat");
	ReadString(g_strVersionApp,g_strVersionNoKey, strFilePath,strTemp);
	lstrcpyn(strVersion,strTemp,20);
	return TRUE;
}