//#define DLLAPI  __attribute__((visibility("default")))

#pragma once
#include <stdarg.h>
#include "linux_compat.h"
#include <iostream>
#include <stdio.h>
#include <memory.h>

#define BITS 8                   /* Setting the number of bits to 12, 13*/
#define HASHING_SHIFT (BITS-8)    /* or 14 affects several constants.    */
#define MAX_VALUE (1 << BITS) - 1 /* Note that MS-DOS machines need to   */
#define MAX_CODE MAX_VALUE - 1    /* compile their code in large model if*/
                                  /* 14 bits are selected.               */
#if BITS == 14
  #define TABLE_SIZE 18041        /* The string table size needs to be a */
#endif                            /* prime number that is somewhat larger*/
#if BITS == 13                    /* than 2**BITS.                       */
  #define TABLE_SIZE 9029
#endif
#if BITS <= 12
  #define TABLE_SIZE 5021
#endif

/* BogdanB addon */
#define NPAC 10
#define FLD_LIMIT 996
#define EXP_LIMIT 65535

class FileLzw
{
private:
	int *code_value; /* This is the code value array */
	unsigned int *prefix_code; /* This array holds the prefix codes */
	unsigned char *append_character; /* This array holds the appended chars */
	unsigned char decode_stack[EXP_LIMIT]; /* This array holds the decoded string */
	HANDLE	hlogfile;

	WORD	m_wDay;
	
	static	CRITICAL_SECTION	m_csSession;
	char	m_strFilePath[MAX_PATH]; 


private:
	
	//����:
	char *mem_compress(char *input, int *clen);
	char *mem_output_code(unsigned int code, int *nlen);

	//����:
	unsigned char *mem_expand(char *input, int *elen);
	bool decode_string(unsigned char *buffer, unsigned int code,unsigned char* &Result);
	unsigned int mem_input_code(char *input, int *nlen);

	int find_match(int hash_prefix, unsigned int hash_character);

	int ReadOne(LPVOID lpFunc);
	int ReallyPut(char* format, va_list ap);

public:
	FileLzw();
	virtual ~FileLzw();
	
	//bool SetLogPath(const char *_LogPath = NULL);  //������־·��,Ĭ�ϵ�ǰ·��
	//bool NoteLog(const char SourceIn[]);           //��¼��־
    inline int PutOne(int level, char* format, ...) {
        va_list ap;
        va_start(ap, format);
        ReallyPut(format, ap);
        va_end(ap);
    }
	
	int OpenLog(char* filename);
	int CloseLog();

	DWORD GetLog(int cout,DWORD dwPointer,LPVOID lpFunc);
	//DWORD PutLog(char* strLog);
	//DWORD SearchLog(DWORD dwPointer,LPVOID lpFunc);
};