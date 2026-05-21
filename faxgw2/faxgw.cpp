// faxgw.cpp : Defines the entry point for the console application.
//

#include "linux_compat.h"
#include <iostream>
#include "gwtypes.h"
#include "rwini.h"
#include "TcpSession.h"
#include "d3des.h"
#include "crc32.h"
#include "tcppacket.h"
#include "FileSession.h"
#include "userdata.h"
#include "Log.h"
#include <unistd.h>

TcpSession		g_tcpSession;
SYS_PARAM		g_sysParam;
Log				g_log;
HANDLE			g_hRecvEvent;
HANDLE			g_hConnectEvent;
char			g_strFilePath[MAX_PATH]; 
char			g_strVersion[30];
unsigned char fixedkey[16] = {23,82,107,6,35,78,88,7,43,96,5,114,37,89,25,53};
extern CRITICAL_SECTION	g_csRecvList;

void MsgReceiver(void* lpParam);

int main(int argc, char* argv[])
{
	Crwini		SysIni;
	FileSession	FaxSession;

	fnUserdata6();

	g_log.SetLevel(5);
	g_log.SetMode(Log::ToConsole | Log::ToDebug);
	g_log.SetFile((char*)"faxgw.log");
	g_log.Print(3, "sizeof(PAG_HEADER)=%d (should be 12)\n", (int)sizeof(PAG_HEADER));
    g_log.Print(3, "sizeof(GW_HEADER)=%d (should be 7)\n", (int)sizeof(GW_HEADER));
	// On Linux, no WSAStartup needed - sockets work natively
	
	InitializeCriticalSection(&g_csRecvList);
	g_hRecvEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	g_hConnectEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	SysIni.GetSetting(&g_sysParam);
	if(g_sysParam.strPath[lstrlen(g_sysParam.strPath)-1]!='/' &&
	   g_sysParam.strPath[lstrlen(g_sysParam.strPath)-1]!='\\')
		lstrcat(g_sysParam.strPath,"/FAXDATA/");
	else
		lstrcat(g_sysParam.strPath,"FAXDATA/");
	if(lstrlen(g_sysParam.strGWKey)==0)
		lstrcpy(g_sysParam.strGWKey,"TIEUFKS4EFS!$&");
	SysIni.GetVersionInfo(g_strVersion);

	FaxSession.SessSet((int)g_sysParam.nServerPort);
	if(!FaxSession.StartSession())
		cout << "start listen service failed!" << endl;

	g_tcpSession.connect(g_sysParam.strHost,g_sysParam.nPort);
	g_tcpSession.onLogin();

    g_log.Print(3,"The fax gateway has started.\r\n");
	_beginthread( MsgReceiver, 0, NULL );

	while(true)
	{
		//g_tcpSession.onLogin();
		Sleep(5*1000);
		g_tcpSession.onKeepAlive();
	}

	return 0;
}

bool CheckPacket(PBYTE pbyPacket)
{
	int					nLoopTime;
	int					datalen;
	PAG_HEADER*			ppagHeader;
	unsigned char *		pchar1;
	unsigned char*		pchar2;
	BYTE				data[2048];
	CRC32				crcCheck;


		ppagHeader = (PAG_HEADER*)pbyPacket;
		if(ppagHeader->byHeader1 == 0x3E &&
		ppagHeader->byHeader2 == 0xE3 &&
		ppagHeader->nVer == 100)
		{
			nLoopTime = ppagHeader->nLength/8;
			datalen = *((int*)(pbyPacket + sizeof(PAG_HEADER)));
			CopyMemory(data,pbyPacket,sizeof(PAG_HEADER));

			//{���ܽ�����Ϣ
			deskey(fixedkey, DE1);
			pchar1 = (unsigned char*)pbyPacket + sizeof(PAG_HEADER) + sizeof(int);
			pchar2 = (unsigned char*)data;
			for(int i=0;i<nLoopTime;i++)
			{
				des(pchar1, pchar2);
				pchar1+=8;
				pchar2+=8;
			}
		}
		else 
		{
			datalen = 0;
			return false;
		}
		
	if (datalen < 2) {
		g_log.Print(5,"packet size is too small.\r\n");
		return false;
	}

	if(IsBadReadPtr(data,datalen))
	{
		g_log.Print(5,"bad memory address.\r\n");
		return false;
	}
	//CRC32���
	// int nCRC1 = *((int *)&data[datalen-sizeof(int)]);
	// int nCRC2 = crcCheck.Get_CRC((char *)data,datalen-sizeof(int));
	// if(nCRC1 != nCRC2)
	// {
	// 	g_log.Print(5,"packet CRC-32 check error.\r\n");
    //     return false;
	// }
		//CRC32���
	int nCRC1 = *((int *)&data[datalen-sizeof(int)]);
	int nCRC2 = crcCheck.Get_CRC((char *)data,datalen-sizeof(int));
	
	// 添加日志
	g_log.Print(3, "CheckPacket: nCRC1=0x%08X, nCRC2=0x%08X, datalen=%d\n", nCRC1, nCRC2, datalen);
	
	if(nCRC1 != nCRC2)
	{
		g_log.Print(3, "CheckPacket: CRC MISMATCH!\n");
		g_log.Print(5,"packet CRC-32 check error.\r\n");
        return false;
	}

	CopyMemory(pbyPacket + sizeof(PAG_HEADER),data,datalen-sizeof(int));
	ppagHeader->nLength = datalen-sizeof(int);
		
	return true;
}

///thread for receive the msg
void MsgReceiver(void* lpParam)
{
	char			buff[256];
	BYTE			m_byLastChar;
	int				ntype;
	int				nPointer;
	PAG_HEADER		header;
	BYTE			byPacket[256];

	// On Linux, thread priority is set differently
	ntype = 0;

	while(true)
	{
		//if(m_bIsStopAudio) return -1;
		//int nRet = g_tcpSession.ReadData(buff, 256);
        int nRet = g_tcpSession.ReadData(buff, 256);
		if(nRet > 0)
		{
			Sleep(0);
			//cout << "receive data" << nRet << endl;
			for(int i=0;i< nRet ;i++)
			{
				if((BYTE)buff[i] == 0xE3 && m_byLastChar == 0x3E && ntype==0)
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
						*((PBYTE)&header + nPointer)=(BYTE)buff[i];
						nPointer++;
						if(nPointer >= sizeof(PAG_HEADER))
						{
							ntype = 2;
							nPointer = 0;
						}
					}
					else if(ntype == 2)
					{
						byPacket[nPointer] = (BYTE)buff[i];
						nPointer++; 
						if(nPointer >= header.nLength)
						{
							if(header.nLength > 2 && header.nLength < 2048)
							{
								PBYTE	pbyPacket;
								ntype = 0;
								nPointer = 0;

								pbyPacket = new BYTE [sizeof(DWORD)+sizeof(PAG_HEADER)+header.nLength];

								if(pbyPacket)
								{
									CopyMemory(pbyPacket,&header,sizeof(PAG_HEADER));
									CopyMemory(pbyPacket+sizeof(PAG_HEADER),byPacket,header.nLength);
									// ����Ϣ���������
									if(CheckPacket(pbyPacket))
									{
										TCPInPacket in(pbyPacket+sizeof(PAG_HEADER), header.nLength,0);
										g_tcpSession.onReceive(in);
										//onPacketReceived(in,&SenderAddr);
									}
									delete [] pbyPacket;
								}
							}
							else
							{
								g_tcpSession.Close();
							}
						}

					}
				}
				m_byLastChar = (BYTE)buff[i];
			}
			//EnterCriticalSection(&UdpSession::m_csInPacket);
			//UdpSession::m_TransPacketQueue.push_back(pRtpPacket);
			//LeaveCriticalSection(&UdpSession::m_csInPacket);
		}
		else
		{
			if(WAIT_OBJECT_0!=WaitForSingleObject(g_hRecvEvent,INFINITE))
				Sleep(5);
			m_byLastChar = 0;
			ntype = 0;
			nPointer= 0;
		}
	}
}