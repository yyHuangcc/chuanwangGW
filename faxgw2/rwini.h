// rwini.h: interface for the Crwini class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RWINI_H__7D3FE520_41BD_41E6_A155_5DE0DE3B4625__INCLUDED_)
#define AFX_RWINI_H__7D3FE520_41BD_41E6_A155_5DE0DE3B4625__INCLUDED_

#pragma once

#include "gwtypes.h"
#include "linux_compat.h"

struct SYS_PARAM						//���ݿ����
{
	char	strHost[255];			//������
	int		nPort;					//�����˿�
	int		nServerPort;			//����˿�
	int		nFilePort;				//�ļ��������˿�
	char	strUser[255];			//�û���
	char	strPasswd[255];			//����
	char	strPath[MAX_PATH];		//��־·��
	char	strGWKey[255];			//����ʶ����
};


class Crwini  
{
public:
	Crwini();
	virtual ~Crwini();

	BOOL GetSetting(SYS_PARAM* pdbParam);
	BOOL GetSetting(char* strIniFile,FAX_RECVLIST* pfaxParam);
	BOOL WriteSetting(char* strIniFile,FAX_RECVLIST* pfaxParam);
	BOOL GetVersionInfo(char* strVersion);

private:
	void WriteInt(char* keyname,char* appname,char *fn,int i);
	void ReadString(char* keyname,char* s,char *fn,char str[]);
	void ReadString(char* strFilename, char *key, char *s, char str[],int nLength);
	int GetInt(const char* appname, const char* kn, char *fn);
	bool WriteString(const char* appname,const char* keyname,char* s,char* fn);



	char	m_strFilePath[MAX_PATH]; 
};

#endif // !defined(AFX_RWINI_H__7D3FE520_41BD_41E6_A155_5DE0DE3B4625__INCLUDED_)
