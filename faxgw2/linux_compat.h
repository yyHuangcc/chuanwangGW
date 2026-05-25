/************************************************************************
 *                                                                      *
 *   Linux compatibility layer for fax gateway system.                  *
 *   Provides Windows API equivalents on Linux platform.                *
 *                                                                      *
 ************************************************************************/

#ifndef _LINUX_COMPAT_H_
#define _LINUX_COMPAT_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <signal.h>
#include <climits>
#include <csignal>

// ============================================================
// Basic type definitions (replacing Windows types)
// ============================================================

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef long                LONG;
typedef unsigned long       ULONG;
typedef const char*         LPCTSTR;
typedef char*               LPTSTR;
typedef unsigned char*      LPBYTE;
typedef BYTE*               PBYTE;
typedef void*               LPVOID;
typedef unsigned long*      LPDWORD;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_PATH PATH_MAX

// ============================================================
// Boolean constants
// ============================================================

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// ============================================================
// Handle types and constants
// ============================================================

// Generic handle - used for files, events, semaphores, threads
typedef void* HANDLE;

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

// Socket types
typedef int SOCKET;
#define SOCKET_ERROR   (-1)
#define INVALID_SOCKET (-1)
// #define INADDR_NONE    ((uint32_t)0xFFFFFFFF)
#define SOCKADDR       struct sockaddr

// ============================================================
// File operation constants
// ============================================================

#define GENERIC_READ           0x80000000
#define GENERIC_WRITE          0x40000000
#define FILE_SHARE_READ        0x00000001
#define CREATE_ALWAYS          2
#define OPEN_EXISTING          3
#define OPEN_ALWAYS            4
#define FILE_ATTRIBUTE_NORMAL  0x80

// File seek origins
#define FILE_BEGIN   SEEK_SET
#define FILE_CURRENT SEEK_CUR
#define FILE_END     SEEK_END

// ============================================================
// Synchronization constants
// ============================================================

#define INFINITE       ((unsigned long)-1)
#define WAIT_OBJECT_0  0
#define WAIT_TIMEOUT   258
#define WAIT_FAILED    ((unsigned long)-1)

// ============================================================
// Thread constants
// ============================================================

#define THREAD_PRIORITY_NORMAL   0
#define CREATE_SUSPENDED        0x00000004

// ============================================================
// Error constants
// ============================================================

#define ERROR_NO_MORE_FILES 18
#define NO_ERROR            0
#define ERROR_INVALID_HANDLE 6
// ============================================================
// Console constants
// ============================================================

#define STD_OUTPUT_HANDLE 1

// ============================================================
// CRITICAL_SECTION -> pthread_mutex_t
// ============================================================

typedef pthread_mutex_t CRITICAL_SECTION;

inline void InitializeCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_init(cs, NULL);
}

inline void EnterCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_lock(cs);
}

inline void LeaveCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_unlock(cs);
}

inline void DeleteCriticalSection(CRITICAL_SECTION* cs) {
    pthread_mutex_destroy(cs);
}

// ============================================================
// Event handle implementation
// ============================================================

struct _LinuxEvent {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             signaled;
    int             manual_reset;
    char            name[256];
};

// CreateEvent wrapper
HANDLE CreateEvent(void* lpEventAttributes, BOOL bManualReset,
                   BOOL bInitialState, const char* lpName);

BOOL SetEvent(HANDLE hEvent);
BOOL ResetEvent(HANDLE hEvent);

// ============================================================
// Semaphore handle implementation
// ============================================================

struct _LinuxSemaphore {
    sem_t           sem;
    int             max_count;
};

HANDLE CreateSemaphore(void* lpSemaphoreAttributes,
                       long lInitialCount, long lMaximumCount,
                       const char* lpName);

BOOL ReleaseSemaphore(HANDLE hSemaphore, long lReleaseCount, long* lpPreviousCount);

// ============================================================
// Thread functions
// ============================================================

typedef void* (*LPTHREAD_START_ROUTINE)(void*);

// Thread handle wrapper
struct _LinuxThread {
    pthread_t thread;
};

HANDLE CreateThread(void* lpThreadAttributes, size_t dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    void* lpParameter, DWORD dwCreationFlags,
                    DWORD* lpThreadId);

// _beginthread equivalent
void _beginthread(void (*start_address)(void*), unsigned stack_size, void* arglist);

BOOL SetThreadPriority(HANDLE hThread, int nPriority);
HANDLE GetCurrentThread();
void ResumeThread(HANDLE hThread);

// ============================================================
// File I/O functions
// ============================================================

// File handle wrapper
struct _LinuxFile {
    int fd;
};

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
                  DWORD dwShareMode, void* lpSecurityAttributes,
                  DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                  HANDLE hTemplateFile);

BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead,
              DWORD* lpNumberOfBytesRead, void* lpOverlapped);

BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite,
               DWORD* lpNumberOfBytesWritten, void* lpOverlapped);

DWORD SetFilePointer(HANDLE hFile, long lDistanceToMove,
                     long* lpDistanceToMoveHigh, DWORD dwMoveMethod);

BOOL SetEndOfFile(HANDLE hFile);

DWORD GetFileSize(HANDLE hFile, DWORD* lpFileSizeHigh);

BOOL CloseHandle(HANDLE hObject);

// ============================================================
// File search (replacing WIN32_FIND_DATA)
// ============================================================

#define MAX_FILENAME_LEN 256

struct WIN32_FIND_DATA {
    DWORD   dwFileAttributes;
    char    cFileName[MAX_FILENAME_LEN];
};

struct _LinuxFindFile {
    DIR*            dir;
    struct dirent*  entry;
    char            pattern[MAX_FILENAME_LEN];
    WIN32_FIND_DATA findData;
    int             first;
};

HANDLE FindFirstFile(const char* lpFileName, WIN32_FIND_DATA* lpFindFileData);
BOOL  FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData);
BOOL  FindClose(HANDLE hFindFile);

// ============================================================
// File operations
// ============================================================

BOOL DeleteFile(LPCTSTR lpFileName);
BOOL CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
BOOL CreateDirectory(LPCTSTR lpPathName, void* lpSecurityAttributes);
BOOL SetCurrentDirectory(LPCTSTR lpPathName);

// ============================================================
// Synchronization wait functions
// ============================================================

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WaitForMultipleObjects(DWORD nCount, HANDLE* lpHandles,
                             BOOL bWaitAll, DWORD dwMilliseconds);

// ============================================================
// Memory functions
// ============================================================

#define CopyMemory(dst, src, len)  memcpy((dst), (src), (len))
#define ZeroMemory(dst, len)       memset((dst), 0, (len))
#define FillMemory(dst, len, val)  memset((dst), (val), (len))

// IsBadReadPtr - just return false on Linux (not needed)
inline BOOL IsBadReadPtr(const void* ptr, unsigned int size) {
    return FALSE;
}

// ============================================================
// String functions (replacing lstrcpy, lstrcat, etc.)
// ============================================================

inline char* lstrcpy(char* dst, const char* src) {
    return strcpy(dst, src);
}

inline char* lstrcat(char* dst, const char* src) {
    return strcat(dst, src);
}

inline char* lstrcpyn(char* dst, const char* src, int len) {
    strncpy(dst, src, len);
    dst[len - 1] = '\0';
    return dst;
}

inline int lstrlen(const char* str) {
    return strlen(str);
}

inline int lstrcmp(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

inline int lstrcmpi(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

// ============================================================
// Sleep function
// ============================================================

inline void Sleep(DWORD dwMilliseconds) {
    usleep(dwMilliseconds * 1000);
}

// ============================================================
// Network functions
// ============================================================

// WSAStartup/WSACleanup - no-ops on Linux
inline int WSAStartup(WORD wVersionRequired, void* lpWSAData) {
    return 0;
}

inline void WSACleanup() {}

inline int WSAGetLastError() {
    return errno;
}

// On Linux, use close() instead of closesocket()
inline int closesocket(SOCKET s) {
    return close(s);
}

// MAKEWORD macro
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))

// WSADATA - dummy structure
typedef struct {
    WORD wVersion;
    WORD wHighVersion;
} WSADATA;

// ============================================================
// Console functions
// ============================================================

inline void AllocConsole() {}

inline BOOL WriteConsole(HANDLE hConsoleOutput, const void* lpBuffer,
                         DWORD nNumberOfCharsToWrite,
                         DWORD* lpNumberOfCharsWritten, void* lpReserved) {
    printf("%.*s", (int)nNumberOfCharsToWrite, (const char*)lpBuffer);
    if (lpNumberOfCharsWritten)
        *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
    return TRUE;
}

inline HANDLE GetStdHandle(DWORD nStdHandle) {
    return (HANDLE)(intptr_t)STDOUT_FILENO;
}

// ============================================================
// Debug output
// ============================================================

inline void OutputDebugString(const char* lpOutputString) {
    fprintf(stderr, "%s", lpOutputString);
}

// ============================================================
// Time functions
// ============================================================

struct SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
};

void GetLocalTime(SYSTEMTIME* lpSystemTime);

// ============================================================
// Module info
// ============================================================

DWORD GetModuleFileName(void* hModule, char* lpFilename, DWORD nSize);

// ============================================================
// Interlocked operations
// ============================================================

inline LONG InterlockedIncrement(LONG* lpAddend) {
    return __sync_add_and_fetch(lpAddend, 1);
}

inline LONG InterlockedDecrement(LONG* lpAddend) {
    return __sync_sub_and_fetch(lpAddend, 1);
}

// ============================================================
// Shell functions
// ============================================================

inline BOOL ShellExecute(void* hwnd, const char* lpOperation,
                         const char* lpFile, const char* lpParameters,
                         const char* lpDirectory, int nShowCmd) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s %s &", lpFile, lpParameters ? lpParameters : "");
    return system(cmd) == 0;
}

#define SW_SHOWNORMAL 1

// ============================================================
// TCHAR compatibility (just use char on Linux)
// ============================================================

typedef char TCHAR;
#define _T(x)       x
#define _tcslen     strlen
#define _vstprintf  vsprintf
#define _tfopen     fopen

// ============================================================
// Thread callback type for FileSession compatibility
// ============================================================

// typedef void* (WINAPI *PTHREAD_START_ROUTINE)(void*);
// #define WINAPI

#ifndef WINAPI
#define WINAPI
#endif

typedef void* (WINAPI *PTHREAD_START_ROUTINE)(void*);

// ============================================================
// INI file functions (replacing Windows registry/INI API)
// ============================================================
typedef unsigned int UINT;
UINT GetPrivateProfileInt(const char* lpAppName, const char* lpKeyName,
                          int nDefault, const char* lpFileName);

DWORD GetPrivateProfileString(const char* lpAppName, const char* lpKeyName,
                              const char* lpDefault, char* lpReturnedString,
                              DWORD nSize, const char* lpFileName);

BOOL WritePrivateProfileString(const char* lpAppName, const char* lpKeyName,
                               const char* lpString, const char* lpFileName);

// ============================================================
// GetFileSize wrapper for direct filename (used in FileSession.cpp)
// ============================================================

int GetFileSize(const char* strFilename, struct _LinuxFile* pFileHandle,
                DWORD* lpdwLow, DWORD* lpdwHigh);

// ============================================================
// GetLastError
// ============================================================

inline DWORD GetLastError() {
    return (DWORD)errno;
}

// ============================================================
// Custom types used in LZW module
// ============================================================

typedef short StInt;
typedef unsigned char Bitchar;

// ============================================================
// GetDateFormat - simple stub
// ============================================================

inline int GetDateFormat(void*, DWORD, void*, const char*, char*, int) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    // Format: "logyyMMdd"
    char buf[30];
    strftime(buf, sizeof(buf), "log%Y%m%d", tm_info);
    return 0;
}

#endif // _LINUX_COMPAT_H_
