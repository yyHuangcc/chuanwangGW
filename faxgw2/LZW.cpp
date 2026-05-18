#include "StdAfx.h"
#include <cstdio>
#include "LZW.h"

const static int LINE_BUFFER_SIZE = 1024;

CRITICAL_SECTION	FileLzw::m_csSession;

typedef int (WINAPI * ON_CALLBACK_FUNC ) (char* pLogBuf, int nLogSize);

FileLzw::FileLzw()
{
	InitializeCriticalSection(&m_csSession);
	code_value=(int*)new BYTE[TABLE_SIZE*sizeof(int)];
	prefix_code=(unsigned int *)new BYTE[TABLE_SIZE*sizeof(unsigned int)];
	append_character=(unsigned char *)new BYTE[TABLE_SIZE*sizeof(unsigned char)];
	GetModuleFileName(NULL,m_strFilePath,255); 
	char* p = strrchr(m_strFilePath,'/');
	if(p) p[1] = 0;
	else { char* p2 = strrchr(m_strFilePath,'\\'); if(p2) p2[1] = 0; }
}

FileLzw::~FileLzw()
{
	DeleteCriticalSection(&m_csSession);
	if(code_value!=NULL)
		delete [] code_value;
	if(prefix_code!=NULL)
		delete [] prefix_code;
	if(append_character!=NULL)
		delete [] append_character;
}

char *FileLzw::mem_compress(char *input, int *clen)
{
	
	char *compressed=(char*)new BYTE[FLD_LIMIT+1];
	char *buff;
	int nC=0, k, kk;
	unsigned int next_code;
	unsigned int character;
	unsigned int string_code;
	unsigned int index;
	int i, nlen, input_len=*clen;
	
	*clen=0;
	next_code=256;              /* Next code is the next available string code*/
	for (i=0;i<TABLE_SIZE;i++)  /* Clear out the string table before starting */
		code_value[i]=-1;
	
	i=0;
	//printf("Compressing %d bytes", input_len);
	string_code=(int)(unsigned char)input[0];    /* Get the first code                         */
												 /*
												 * step through the input string and compress
	*/
	for(k=1; k<input_len; k++){ /* !!!! starting from the index=1 !!!!, since index=0 is already processed */
		character=(int)(unsigned char)input[k];
		if(++i==NPAC){ /* prints out a pacifier @ every NPAC characters */
			i=0; //printf(".");
		}
		index=find_match(string_code,character);/* See if the string is in */
		if (code_value[index] != -1)            /* the table.  If it is,   */
			string_code=code_value[index];        /* get the code value.  If */
		else{                                   /* the string is not in the table, try to add it */
			if (next_code <= MAX_CODE){
				code_value[index]=next_code++;
				prefix_code[index]=string_code;
				append_character[index]=character;
			}
			nlen=0; buff=mem_output_code(string_code, &nlen);
			for(kk=0; kk<nlen; kk++)compressed[nC++]=buff[kk];
			*clen+=nlen;
			delete [] buff;
			string_code=character;            /* that is not in the table*/
		}                                   /* I output the last string*/
	}                                     /* after adding the new one*/
										  /*
										  ** End of the main loop.
	*/
	nlen=0; buff=mem_output_code(string_code, &nlen); /* output the last code */
	for(kk=0; kk<nlen; kk++)compressed[nC++]=buff[kk];
	*clen+=nlen;
	delete [] buff;
	
	nlen=0; buff=mem_output_code(MAX_VALUE, &nlen); /* output the end of buffer code */
	for(kk=0; kk<nlen; kk++)compressed[nC++]=buff[kk];
	*clen+=nlen;
	delete [] buff;
	
	nlen=0; buff=mem_output_code(0, &nlen); /* flush the output buffer */
	for(kk=0; kk<nlen; kk++)compressed[nC++]=buff[kk];
	*clen+=nlen;
	delete [] buff;
	
	return compressed;
}

char *FileLzw::mem_output_code(unsigned int code, int *nlen){
  char *output=(char*)new BYTE[FLD_LIMIT+1];
  static int output_bit_count=0;
  static unsigned long output_bit_buffer=0L;
  int i=0;

  output_bit_buffer |= (unsigned long) code << (32-BITS-output_bit_count);
  output_bit_count += BITS;
  while(output_bit_count >= 8){
    output[i++]=output_bit_buffer >> 24;
    output[i]='\0';

    output_bit_buffer <<= 8;
    output_bit_count -= 8;
  }
  *nlen=i;

  return output;
}

unsigned char *FileLzw::mem_expand(char *input, int *elen){
  unsigned char *expanded=(unsigned char *)new BYTE[EXP_LIMIT+1];
  int nE=0, k, nlen;
  unsigned int next_code;
  unsigned int new_code;
  unsigned int old_code;
  int character;
  int counter;
  unsigned char *string;
  int input_len=*elen;

  *elen=0;
  next_code=256;           /* This is the next available code to define */
  counter=0;               /* Counter is used as a pacifier            */
  //printf("Expanding %d bytes", input_len);

  nlen=0; old_code=mem_input_code(input, &nlen); for(k=0; k<nlen; k++)*input++;
  character=old_code;          /* initialize the character variable */
  expanded[nE++]=old_code;

  nlen=0;
  /*
   **  This is the main expansion loop.  It processes characters from the LZW stream of bytes
   **  until it sees the special code used to indicate the end of the data.
   */
  while((new_code=mem_input_code(input, &nlen)) != (MAX_VALUE)){
    for(k=0; k<nlen; k++)*input++; /* inc the pointer the number of times it's been processed by function call */
    nlen=0;
    if(++counter==NPAC){ /* prints out a pacifier @ every NPAC characters */
      counter=0; //printf(".");
    }
    /*
     ** This code checks for the special STRING+CHARACTER+STRING+CHARACTER+STRING
     ** case which generates an undefined code.  It handles it by decoding
     ** the last code, and adding a single character to the end of the decode string
     */
    if(new_code>=next_code){
      *decode_stack=character;
		if(!decode_string(decode_stack+1, old_code,string))
			return NULL;
    }
    /*
     ** Otherwise we do a straight decode of the new code.
     */
    else
	{		
		if(!decode_string(decode_stack, new_code,string))
			return NULL;
	}
    /*
     ** Now we write the decoded string in reverse order
     */
    character=*string;
    while(string >= decode_stack)
      expanded[nE++]=*string--;
    /*
     ** Finally, if possible, add a new code to the string table
     */
    if(next_code <= MAX_CODE){
      prefix_code[next_code]=old_code;
      append_character[next_code]=character;
      next_code++;
    }
    old_code=new_code;
  }
  //printf(" done\n");
  *elen=nE;

  expanded[nE]='\0';
  return expanded;
}

bool FileLzw::decode_string(unsigned char *buffer, unsigned int code,unsigned char* &Result)
{
  int i=0;

  while(code>255){
    *buffer++ = append_character[code];
    code=prefix_code[code];
    if(i++>=MAX_CODE){
      //printf("Fatal error during code expansion!\n");
      return false;
    }
  }
  *buffer=code;
	Result = buffer;
  return true;
}

unsigned int FileLzw::mem_input_code(char *input, int *nlen){
  unsigned int return_value;
  static int input_bit_count=0;
  static unsigned long input_bit_buffer=0L;
  int i=0;

  while(input_bit_count <= 24){ 
    input_bit_buffer |= 
      (unsigned char) input[i++] << (24-input_bit_count);
    input_bit_count += 8;
  }
  *nlen=i;

  return_value=input_bit_buffer >> (32-BITS);
  input_bit_buffer <<= BITS;
  input_bit_count -= BITS;

  return return_value;
}

int FileLzw::find_match(int hash_prefix, unsigned int hash_character){
  int index;
  int offset;

  index = (hash_character << HASHING_SHIFT) ^ hash_prefix;
  if(index == 0)
    offset = 1;
  else
    offset = TABLE_SIZE - index;
  while(1){
    if(code_value[index] == -1)
      return(index);
    if(prefix_code[index] == hash_prefix && 
        append_character[index] == hash_character)
      return(index);
    index -= offset;
    if(index < 0)
      index += TABLE_SIZE;
  }
}

int FileLzw::OpenLog(LPTSTR filename) 
{
    // if a log file is open, close it now.
    CloseLog();

    // If filename is NULL or invalid we should throw an exception here
    
    hlogfile = CreateFile(
        filename,  GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL  );
    
    if (hlogfile == INVALID_HANDLE_VALUE) {
        // We should throw an exception here
        //Print(0, _T("Error opening log file %s\n"), filename);
		return -1;
    }
    SetFilePointer( hlogfile, 0, NULL, FILE_END );
	return 0;
}

// if a log file is open, close it now.
int FileLzw::CloseLog() {
    if (hlogfile != NULL) {
        CloseHandle(hlogfile);
        hlogfile = NULL;
		return 0;
    }
	else
		return -1;
}

int FileLzw::ReadOne(LPVOID lpFunc)
{
	if(lpFunc)
	{
		ON_CALLBACK_FUNC lpLog;
		char	Header[4];
		DWORD	dwRead;
		DWORD	dwRequire;
		char	buff[1024];

		lpLog = (ON_CALLBACK_FUNC)lpFunc;
		if(hlogfile)
		{
			if(ReadFile(hlogfile,Header,4,&dwRead,NULL))
				if(Header[0]==0xFF && Header[1]==0xFF)
				{
					dwRequire = *((WORD*)&Header[2]) > 1024 ? 1024 : *((WORD*)&Header[2]);
					if(ReadFile(hlogfile,buff,dwRequire,&dwRead,NULL))
					{
						unsigned char *strdecodeStr=mem_expand(buff,(int*)&dwRead);
						if(strdecodeStr)
						{
							lpLog((char*)strdecodeStr,dwRead);
							delete [] strdecodeStr;
						}
					}
				}
		}
	}
	return true;
}

DWORD FileLzw::GetLog(int cout,DWORD dwPointer,LPVOID lpFunc)
{
	if(cout > 0)
	{
		if(dwPointer==0)
		{
			//check log header
			char	Header[8];
			DWORD	dwRead;
			if(hlogfile)
			{
				if(ReadFile(hlogfile,Header,6,&dwRead,NULL))
				{
					if(Header[0]=='L' && Header[1]=='L' && *((WORD*)&Header[2])==101)
					{
						if(0xFFFFFFFF!=SetFilePointer( hlogfile, *((WORD*)&Header[4])+6, NULL, FILE_BEGIN ))
						{
							for(int i=0;i<cout; i++)
							{
								if(!ReadOne(lpFunc))
									break;
							}

						}
					}
				}
			}
		}
		else
		{
			if(hlogfile && 0xFFFFFFFF!=SetFilePointer( hlogfile, dwPointer, NULL, FILE_BEGIN ))
			{
				for(int i=0;i<cout; i++)
				{
					if(!ReadOne(lpFunc))
						break;
				}
			}

		}
	}
	return 0;
}

int FileLzw::ReallyPut(char* format, va_list ap) 
{
	SYSTEMTIME		stCur;
	DWORD			dwRead;
    char line1[LINE_BUFFER_SIZE];
    char line[LINE_BUFFER_SIZE];
	char	szLogFile[MAX_PATH];
	char			strHeader[4];
    vsprintf(line1, format, ap);
	GetLocalTime(&stCur);
	sprintf(line,"[%02d:%02d:%02d] %s",stCur.wHour,stCur.wMinute,stCur.wSecond,line1);
	dwRead = strlen(line);
	char *strencodeStr=mem_compress(line,(int*)&dwRead);
	strHeader[0]=0xFF;
	strHeader[1]=0xFF;
	*((WORD*)&strHeader[2])=dwRead;
	if(hlogfile == NULL || hlogfile == INVALID_HANDLE_VALUE)
	{
		sprintf(szLogFile,"%slog%02d%02d",m_strFilePath,stCur.wMonth,stCur.wDay);
		OpenLog(szLogFile);
	}
	if(strencodeStr)
	{
		EnterCriticalSection(&m_csSession);
		if (hlogfile != NULL && hlogfile != INVALID_HANDLE_VALUE) {
			DWORD byteswritten;
			WriteFile(hlogfile, strHeader, 4, &byteswritten, NULL); 
			WriteFile(hlogfile, strencodeStr, dwRead, &byteswritten, NULL); 

		}	
		LeaveCriticalSection(&m_csSession);
		delete [] strencodeStr;
	}

	return 0;
}

//��־����:
/*bool FileLzw::SetLogPath(const char *_LogPath)
{
	if( _LogPath == NULL )
		return false;
	else
	{
		if( LogPath != NULL )
			delete []LogPath;
		int len;
		len = strlen(_LogPath);
		LogPath = new char[strlen(_LogPath)];
		if( LogPath == NULL )
			return false;
		memcpy(LogPath, _LogPath, len);
		LogPath[len] = '\0';
		return true;
	}
}

void FileLzw::ManageFile(char FileName[])
{
	HANDLE hFile;
	hFile = CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
		                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	char FileHeadData[20];
	memset(FileHeadData, 0, 20);
	FileHeadData[0] = 'L';
	FileHeadData[1] = 'L';

	FileHeadData[2] = 0x01;   //�汾��;
	FileHeadData[3] = 0x00;

	*(StInt *)(FileHeadData+4) = 4; //������Ϣ����
	*(StInt *)(FileHeadData+10) = 0;
	FileHeadData[12] = '\0';

	DWORD DwWrite;
	WriteFile(hFile, FileHeadData, 12, &DwWrite, NULL);
	CloseHandle(hFile);
}

void FileLzw::ManageSourceIn(Bitchar *InBuffer, StInt len)
{
	InBuffer[0] = 0xFF;
	InBuffer[1] = 0xFF;        //����ͷ
	
	*(StInt *)(InBuffer + 2) = len;  //���ݳ�

}

bool FileLzw::NoteLog(const char SourceIn[])
{
	char StrDate[30];             //��¼���ε���־ʱ��
	GetDateFormat(NULL, 0, NULL, "lo'g'yyMMdd", StrDate, 30);
//	if( strcmp(LogDateTime, StrDate) )

	char *FileFullName;          //��¼��־�ļ���������·����Ϣ
//	char LogHeadData[5];         // ����ͷ

	FileFullName = new char[ strlen(LogPath) + strlen(StrDate) + 5];
//	FileFullName[0] = '\0';

	strcpy(FileFullName, LogPath);

	strcat(FileFullName, StrDate);

	strcat(FileFullName, ".dat");

	HANDLE hFind;
	WIN32_FIND_DATA fd;

	hFind = FindFirstFile(FileFullName, &fd); 

	if( hFind == INVALID_HANDLE_VALUE )   //��־�ļ�������
	{
		strcpy(LogDateTime, StrDate);
		ManageFile(FileFullName);        //Ԥ��������־�ļ�,���ļ�ͷ

		CloseHandle(hFind);
	
	}
	else
		FindClose(hFind);

	HANDLE hFile;

	HANDLE hFileMap;

	int FileLen;     

	Bitchar *pMapAddress;

	hFile = CreateFile(FileFullName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 
					   NULL,OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return false;

	FileLen = GetFileSize(hFile, NULL);              //��־ԭ���ļ���С

	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 1024*1024, NULL);
	if(hFileMap == NULL)
		return false;

	pMapAddress = (Bitchar *)MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0);
	if(pMapAddress == NULL)
		return false;

	int DataLen ;   //���ݳ�

	DataLen = *(StInt *)(pMapAddress+10);

	Bitchar *InBuffer;

	int SourceLen;
	int DestLen;

	SourceLen = strlen(SourceIn);

	InBuffer = new Bitchar[strlen(SourceIn) + 4]; 

//	LogHeadData[0] = 0xFF; LogHeadData[1] = 0xFF;
//	*(StInt *)(LogHeadData + 2) = (StInt)SourceLen;

//	memcpy(pMapAddress, LogHeadData, 4);

	memcpy(InBuffer + 4, SourceIn, SourceLen );

//	memcpy(InBuffer , SourceIn, SourceLen )

	this->ManageSourceIn(InBuffer, SourceLen );     //Ԥ�������������Ϣ

	//����ѹ�������ݵĴ�СDestLen
	DestLen = this->lzw_Encode(InBuffer, (pMapAddress + FileLen ), SourceLen + 4 );

//	*(StInt *)(pMapAddress+10) = (StInt)(DestLen + FileLen -12) ;  //�ļ���С

	*(StInt *)(pMapAddress+10) = (StInt)(DataLen + SourceLen + 4);

	FlushViewOfFile(pMapAddress + FileLen, DestLen);

	SetFilePointer(hFile, DestLen + FileLen, NULL, FILE_BEGIN);

	UnmapViewOfFile(pMapAddress);
	CloseHandle(hFileMap);
	SetEndOfFile(hFile);
	CloseHandle(hFile);

	delete []InBuffer;

	delete []FileFullName;

	return true;
}

int FileLzw::OpenNote(const char FileName[], Bitchar *pOutMapAddress)
{
/*	char *FileOutName;
	FileOutName = new char[strlen(FileName) + strlen(LogPath) + 1];
	FileOutName[0] = '\0';
	strcat(FileOutName, LogPath);
	strcat(FileOutName, FileName);
	strcpy(FileOutName + strlen(FileName) + strlen(LogPath) - 4, ".txt");*/
	

/*	HANDLE hFile;
	HANDLE hFileMap;
	Bitchar *pInMapAddress;
	int FileSize;
	int FileLen;

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 
					   NULL,OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
		return false;

	FileLen = GetFileSize(hFile, NULL);        //����Դ�ļ��Ĵ�С

	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if(hFileMap == NULL)
		return false;

	pInMapAddress = (Bitchar *)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
	if(pInMapAddress == NULL)
		return false;
	
	FileSize = *(StInt*)(pInMapAddress + 10);       //�ļ��������ݴ�С

/*	HANDLE hOFile;
	HANDLE hOFileMap;
	Bitchar *pOutMapAddress;

	hOFile = CreateFile(FileOutName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 
					   NULL,OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(hOFile == INVALID_HANDLE_VALUE)
		return false;

	hOFileMap = CreateFileMapping(hOFile, NULL, PAGE_READWRITE, 0, 1024*1024, NULL);
	if(hOFileMap == NULL)
		return false;

	pOutMapAddress = (Bitchar *)MapViewOfFile(hOFileMap, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if(pOutMapAddress == NULL)
		return false;

//	memcpy(pOutMapAddress, pInMapAddress, 12);
	
	FlushViewOfFile(pOutMapAddress , OutFileLen);

	UnmapViewOfFile(pOutMapAddress);
	SetFilePointer(hOFile, OutFileLen , NULL, FILE_BEGIN);
	CloseHandle(hOFileMap);
	SetEndOfFile(hOFile);
	CloseHandle(hOFile);*/

/*	int OutFileLen = 0;

	OutFileLen = this->lzw_Decond(pInMapAddress + 12 , pOutMapAddress, FileSize, FileLen -12);

	UnmapViewOfFile(pInMapAddress);
	CloseHandle(hFileMap);
	CloseHandle(hFile);

	if( !(OutFileLen - FileSize) )
		return OutFileLen ;
	else
		return 0;
}*/
