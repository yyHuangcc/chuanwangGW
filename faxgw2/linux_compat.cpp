/************************************************************************
 *                                                                      *
 *   Linux compatibility layer implementation                           *
 *   Provides Windows API equivalents on Linux platform.                *
 *                                                                      *
 ************************************************************************/

#include "linux_compat.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <climits>
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
#include <fnmatch.h>

// ============================================================
// Event handle implementation
// ============================================================

HANDLE CreateEvent(void* lpEventAttributes, BOOL bManualReset,
                   BOOL bInitialState, const char* lpName)
{
    _LinuxEvent* evt = new _LinuxEvent;
    if (!evt) return INVALID_HANDLE_VALUE;

    pthread_mutex_init(&evt->mutex, NULL);
    pthread_cond_init(&evt->cond, NULL);
    evt->signaled = bInitialState ? 1 : 0;
    evt->manual_reset = bManualReset;
    if (lpName)
        strncpy(evt->name, lpName, sizeof(evt->name) - 1);
    else
        evt->name[0] = '\0';

    return (HANDLE)evt;
}

BOOL SetEvent(HANDLE hEvent)
{
    if (hEvent == INVALID_HANDLE_VALUE || hEvent == NULL) return FALSE;
    _LinuxEvent* evt = (_LinuxEvent*)hEvent;
    pthread_mutex_lock(&evt->mutex);
    evt->signaled = 1;
    pthread_cond_broadcast(&evt->cond);
    pthread_mutex_unlock(&evt->mutex);
    return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
    if (hEvent == INVALID_HANDLE_VALUE || hEvent == NULL) return FALSE;
    _LinuxEvent* evt = (_LinuxEvent*)hEvent;
    pthread_mutex_lock(&evt->mutex);
    evt->signaled = 0;
    pthread_mutex_unlock(&evt->mutex);
    return TRUE;
}

// ============================================================
// Semaphore handle implementation
// ============================================================

HANDLE CreateSemaphore(void* lpSemaphoreAttributes,
                       long lInitialCount, long lMaximumCount,
                       const char* lpName)
{
    _LinuxSemaphore* sem = new _LinuxSemaphore;
    if (!sem) return INVALID_HANDLE_VALUE;

    sem_init(&sem->sem, 0, lInitialCount);
    sem->max_count = lMaximumCount;
    return (HANDLE)sem;
}

BOOL ReleaseSemaphore(HANDLE hSemaphore, long lReleaseCount, long* lpPreviousCount)
{
    if (hSemaphore == INVALID_HANDLE_VALUE || hSemaphore == NULL) return FALSE;
    _LinuxSemaphore* sem = (_LinuxSemaphore*)hSemaphore;
    for (long i = 0; i < lReleaseCount; i++) {
        sem_post(&sem->sem);
    }
    return TRUE;
}

// ============================================================
// Thread functions
// ============================================================

// Wrapper struct for _beginthread compatibility
struct _BeginThreadArg {
    void (*start_address)(void*);
    void* arglist;
};

static void* _beginthread_wrapper(void* arg)
{
    _BeginThreadArg* bta = (_BeginThreadArg*)arg;
    void (*start_address)(void*) = bta->start_address;
    void* arglist = bta->arglist;
    delete bta;

    start_address(arglist);
    return NULL;
}

void _beginthread(void (*start_address)(void*), unsigned stack_size, void* arglist)
{
    _BeginThreadArg* bta = new _BeginThreadArg;
    bta->start_address = start_address;
    bta->arglist = arglist;

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size > 0)
        pthread_attr_setstacksize(&attr, stack_size);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &attr, _beginthread_wrapper, bta);
    pthread_attr_destroy(&attr);
}

// CreateThread wrapper
struct _CreateThreadArg {
    LPTHREAD_START_ROUTINE start_address;
    void* param;
};

static void* _createthread_wrapper(void* arg)
{
    _CreateThreadArg* cta = (_CreateThreadArg*)arg;
    LPTHREAD_START_ROUTINE start = cta->start_address;
    void* param = cta->param;
    delete cta;
    return start(param);
}

HANDLE CreateThread(void* lpThreadAttributes, size_t dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    void* lpParameter, DWORD dwCreationFlags,
                    DWORD* lpThreadId)
{
    _LinuxThread* lt = new _LinuxThread;
    if (!lt) return INVALID_HANDLE_VALUE;

    _CreateThreadArg* cta = new _CreateThreadArg;
    cta->start_address = lpStartAddress;
    cta->param = lpParameter;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (dwStackSize > 0)
        pthread_attr_setstacksize(&attr, dwStackSize);

    int ret = pthread_create(&lt->thread, &attr, _createthread_wrapper, cta);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        delete lt;
        delete cta;
        return INVALID_HANDLE_VALUE;
    }

    if (lpThreadId)
        *lpThreadId = (DWORD)(intptr_t)lt->thread;

    return (HANDLE)lt;
}

BOOL SetThreadPriority(HANDLE hThread, int nPriority)
{
    // Linux doesn't have direct thread priority setting like Windows
    // For migration compatibility, just return TRUE
    return TRUE;
}

HANDLE GetCurrentThread()
{
    // Return a pseudo-handle - on Linux, we just return NULL
    // The original code only uses this for SetThreadPriority which is a no-op
    return NULL;
}

void ResumeThread(HANDLE hThread)
{
    // On Linux, threads are not created suspended
    // This is a no-op for compatibility
}

// ============================================================
// File I/O functions
// ============================================================

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess,
                  DWORD dwShareMode, void* lpSecurityAttributes,
                  DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                  HANDLE hTemplateFile)
{
    int flags = 0;
    mode_t mode = 0644;

    if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE))
        flags = O_RDWR;
    else if (dwDesiredAccess & GENERIC_WRITE)
        flags = O_WRONLY;
    else
        flags = O_RDONLY;

    switch (dwCreationDisposition) {
    case CREATE_ALWAYS:
        flags |= O_CREAT | O_TRUNC;
        break;
    case OPEN_EXISTING:
        // no additional flags
        break;
    case OPEN_ALWAYS:
        flags |= O_CREAT;
        break;
    default:
        flags |= O_CREAT;
        break;
    }

    int fd = open(lpFileName, flags, mode);
    if (fd < 0)
        return INVALID_HANDLE_VALUE;

    _LinuxFile* lf = new _LinuxFile;
    if (!lf) {
        close(fd);
        return INVALID_HANDLE_VALUE;
    }
    lf->fd = fd;
    return (HANDLE)lf;
}

BOOL ReadFile(HANDLE hFile, void* lpBuffer, DWORD nNumberOfBytesToRead,
              DWORD* lpNumberOfBytesRead, void* lpOverlapped)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) return FALSE;
    _LinuxFile* lf = (_LinuxFile*)hFile;
    ssize_t ret = ::read(lf->fd, lpBuffer, nNumberOfBytesToRead);
    if (ret < 0) {
        if (lpNumberOfBytesRead) *lpNumberOfBytesRead = 0;
        return FALSE;
    }
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = (DWORD)ret;
    return TRUE;
}

BOOL WriteFile(HANDLE hFile, const void* lpBuffer, DWORD nNumberOfBytesToWrite,
               DWORD* lpNumberOfBytesWritten, void* lpOverlapped)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) return FALSE;
    _LinuxFile* lf = (_LinuxFile*)hFile;
    ssize_t ret = ::write(lf->fd, lpBuffer, nNumberOfBytesToWrite);
    if (ret < 0) {
        if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = 0;
        return FALSE;
    }
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = (DWORD)ret;
    return TRUE;
}

DWORD SetFilePointer(HANDLE hFile, long lDistanceToMove,
                     long* lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) return 0xFFFFFFFF;
    _LinuxFile* lf = (_LinuxFile*)hFile;
    off_t ret = lseek(lf->fd, lDistanceToMove, dwMoveMethod);
    if (ret == (off_t)-1) return 0xFFFFFFFF;
    return (DWORD)ret;
}

BOOL SetEndOfFile(HANDLE hFile)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) return FALSE;
    _LinuxFile* lf = (_LinuxFile*)hFile;
    off_t pos = lseek(lf->fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) return FALSE;
    if (ftruncate(lf->fd, pos) != 0) return FALSE;
    return TRUE;
}

DWORD GetFileSize(HANDLE hFile, DWORD* lpFileSizeHigh)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) return 0xFFFFFFFF;
    _LinuxFile* lf = (_LinuxFile*)hFile;
    struct stat st;
    if (fstat(lf->fd, &st) != 0) return 0xFFFFFFFF;
    if (lpFileSizeHigh) *lpFileSizeHigh = (DWORD)(st.st_size >> 32);
    return (DWORD)(st.st_size & 0xFFFFFFFF);
}

BOOL CloseHandle(HANDLE hObject)
{
    if (hObject == INVALID_HANDLE_VALUE || hObject == NULL) return FALSE;

    // Try to determine the type of handle by checking for known patterns
    // This is a simplistic approach - we check the first bytes

    // For file handles (struct _LinuxFile), close the fd
    _LinuxFile* lf = (_LinuxFile*)hObject;
    // We can't reliably distinguish types, so we use a heuristic:
    // Check if the fd is valid before closing
    if (lf->fd >= 0 && lf->fd < 65536) {
        // Could be a file handle - try to close it
        close(lf->fd);
    }
    delete lf;
    return TRUE;
}

// Specific close for event handles
static BOOL CloseEventHandle(HANDLE hObject)
{
    if (hObject == INVALID_HANDLE_VALUE || hObject == NULL) return FALSE;
    _LinuxEvent* evt = (_LinuxEvent*)hObject;
    pthread_mutex_destroy(&evt->mutex);
    pthread_cond_destroy(&evt->cond);
    delete evt;
    return TRUE;
}

// Specific close for semaphore handles
static BOOL CloseSemaphoreHandle(HANDLE hObject)
{
    if (hObject == INVALID_HANDLE_VALUE || hObject == NULL) return FALSE;
    _LinuxSemaphore* sem = (_LinuxSemaphore*)hObject;
    sem_destroy(&sem->sem);
    delete sem;
    return TRUE;
}

// ============================================================
// File search functions
// ============================================================

HANDLE FindFirstFile(const char* lpFileName, WIN32_FIND_DATA* lpFindFileData)
{
    if (!lpFileName || !lpFindFileData) return INVALID_HANDLE_VALUE;

    // Extract directory and pattern from lpFileName
    char dir[PATH_MAX];
    char pattern[PATH_MAX];
    strncpy(dir, lpFileName, PATH_MAX - 1);
    dir[PATH_MAX - 1] = '\0';

    char* last_slash = strrchr(dir, '/');
    if (last_slash) {
        strncpy(pattern, last_slash + 1, PATH_MAX - 1);
        pattern[PATH_MAX - 1] = '\0';
        *last_slash = '\0';
    } else {
        strncpy(pattern, dir, PATH_MAX - 1);
        pattern[PATH_MAX - 1] = '\0';
        strcpy(dir, ".");
    }

    _LinuxFindFile* lff = new _LinuxFindFile;
    if (!lff) return INVALID_HANDLE_VALUE;

    strncpy(lff->pattern, pattern, MAX_FILENAME_LEN - 1);
    lff->pattern[MAX_FILENAME_LEN - 1] = '\0';
    lff->first = 1;

    lff->dir = opendir(dir);
    if (!lff->dir) {
        delete lff;
        return INVALID_HANDLE_VALUE;
    }

    // Find the first matching file
    while ((lff->entry = readdir(lff->dir)) != NULL) {
        if (fnmatch(lff->pattern, lff->entry->d_name, FNM_PERIOD) == 0) {
            strncpy(lpFindFileData->cFileName, lff->entry->d_name, MAX_FILENAME_LEN - 1);
            lpFindFileData->cFileName[MAX_FILENAME_LEN - 1] = '\0';
            lpFindFileData->dwFileAttributes = 0;
            return (HANDLE)lff;
        }
    }

    // No matching file found
    closedir(lff->dir);
    delete lff;
    return INVALID_HANDLE_VALUE;
}

BOOL FindNextFile(HANDLE hFindFile, WIN32_FIND_DATA* lpFindFileData)
{
    if (hFindFile == INVALID_HANDLE_VALUE || hFindFile == NULL) return FALSE;
    _LinuxFindFile* lff = (_LinuxFindFile*)hFindFile;

    while ((lff->entry = readdir(lff->dir)) != NULL) {
        if (fnmatch(lff->pattern, lff->entry->d_name, FNM_PERIOD) == 0) {
            strncpy(lpFindFileData->cFileName, lff->entry->d_name, MAX_FILENAME_LEN - 1);
            lpFindFileData->cFileName[MAX_FILENAME_LEN - 1] = '\0';
            lpFindFileData->dwFileAttributes = 0;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL FindClose(HANDLE hFindFile)
{
    if (hFindFile == INVALID_HANDLE_VALUE || hFindFile == NULL) return FALSE;
    _LinuxFindFile* lff = (_LinuxFindFile*)hFindFile;
    closedir(lff->dir);
    delete lff;
    return TRUE;
}

// ============================================================
// File operations
// ============================================================

BOOL DeleteFile(LPCTSTR lpFileName)
{
    return unlink(lpFileName) == 0;
}

BOOL CopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
    if (bFailIfExists) {
        struct stat st;
        if (stat(lpNewFileName, &st) == 0)
            return FALSE; // File exists and we should fail
    }

    int src = open(lpExistingFileName, O_RDONLY);
    if (src < 0) return FALSE;

    int dst = open(lpNewFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst < 0) {
        close(src);
        return FALSE;
    }

    char buf[8192];
    ssize_t n;
    while ((n = ::read(src, buf, sizeof(buf))) > 0) {
        if (::write(dst, buf, n) != n) {
            close(src);
            close(dst);
            return FALSE;
        }
    }

    close(src);
    close(dst);
    return TRUE;
}

BOOL CreateDirectory(LPCTSTR lpPathName, void* lpSecurityAttributes)
{
    return mkdir(lpPathName, 0755) == 0;
}

BOOL SetCurrentDirectory(LPCTSTR lpPathName)
{
    return chdir(lpPathName) == 0;
}

// ============================================================
// Synchronization wait functions
// ============================================================

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    if (hHandle == INVALID_HANDLE_VALUE || hHandle == NULL)
        return WAIT_FAILED;

    // Check if it's an event handle
    _LinuxEvent* evt = (_LinuxEvent*)hHandle;
    // We assume event handles are the primary use case for WaitForSingleObject
    // in this codebase (besides semaphores)

    // Try event first
    // Simple heuristic: if it looks like it could be an event, try event wait
    pthread_mutex_lock(&evt->mutex);

    if (!evt->signaled) {
        if (dwMilliseconds == INFINITE) {
            while (!evt->signaled) {
                pthread_cond_wait(&evt->cond, &evt->mutex);
            }
        } else {
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += dwMilliseconds / 1000;
            ts.tv_nsec += (dwMilliseconds % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }
            while (!evt->signaled) {
                int ret = pthread_cond_timedwait(&evt->cond, &evt->mutex, &ts);
                if (ret == ETIMEDOUT) {
                    pthread_mutex_unlock(&evt->mutex);
                    return WAIT_TIMEOUT;
                }
                if (evt->signaled) break;
            }
        }
    }

    if (!evt->manual_reset) {
        evt->signaled = 0; // Auto-reset
    }

    pthread_mutex_unlock(&evt->mutex);
    return WAIT_OBJECT_0;
}

DWORD WaitForMultipleObjects(DWORD nCount, HANDLE* lpHandles,
                             BOOL bWaitAll, DWORD dwMilliseconds)
{
    // Simplified implementation - wait on first handle
    // This codebase doesn't seem to use WaitForMultipleObjects extensively
    if (nCount > 0 && lpHandles)
        return WaitForSingleObject(lpHandles[0], dwMilliseconds);
    return WAIT_FAILED;
}

// ============================================================
// Time functions
// ============================================================

void GetLocalTime(SYSTEMTIME* lpSystemTime)
{
    if (!lpSystemTime) return;
    time_t t = time(NULL);
    struct tm* local = localtime(&t);
    lpSystemTime->wYear = local->tm_year + 1900;
    lpSystemTime->wMonth = local->tm_mon + 1;
    lpSystemTime->wDayOfWeek = local->tm_wday;
    lpSystemTime->wDay = local->tm_mday;
    lpSystemTime->wHour = local->tm_hour;
    lpSystemTime->wMinute = local->tm_min;
    lpSystemTime->wSecond = local->tm_sec;
    lpSystemTime->wMilliseconds = 0;
}

// ============================================================
// Module info
// ============================================================

DWORD GetModuleFileName(void* hModule, char* lpFilename, DWORD nSize)
{
    ssize_t len = readlink("/proc/self/exe", lpFilename, nSize - 1);
    if (len == -1) {
        lpFilename[0] = '.';
        lpFilename[1] = '/';
        lpFilename[2] = '\0';
        return 2;
    }
    lpFilename[len] = '\0';
    return (DWORD)len;
}

// ============================================================
// INI file functions (replacing Windows INI API)
// ============================================================

static char* ini_find_section(FILE* fp, const char* section)
{
    char line[1024];
    char sec[256];
    snprintf(sec, sizeof(sec), "[%s]", section);

    rewind(fp);
    while (fgets(line, sizeof(line), fp)) {
        // Trim whitespace
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ';' || *p == '#') continue; // Comment

        char* end = p + strlen(p) - 1;
        while (end > p && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t'))
            *end-- = '\0';

        if (strcmp(p, sec) == 0)
            return p;
    }
    return NULL;
}

static BOOL ini_find_key(FILE* fp, const char* section, const char* key,
                         char* value, int valsize)
{
    char line[1024];
    char sec[256];
    snprintf(sec, sizeof(sec), "[%s]", section);

    int in_section = 0;
    rewind(fp);

    while (fgets(line, sizeof(line), fp)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ';' || *p == '#') continue;

        char* end = p + strlen(p) - 1;
        while (end > p && (*end == '\n' || *end == '\r' || *end == ' ' || *end == '\t'))
            *end-- = '\0';

        if (*p == '[') {
            in_section = (strcmp(p, sec) == 0);
            continue;
        }

        if (in_section) {
            char* eq = strchr(p, '=');
            if (eq) {
                *eq = '\0';
                char* k = p;
                char* v = eq + 1;
                // Trim key
                char* ke = k + strlen(k) - 1;
                while (ke > k && (*ke == ' ' || *ke == '\t')) *ke-- = '\0';
                // Trim value
                while (*v == ' ' || *v == '\t') v++;

                if (strcmp(k, key) == 0) {
                    strncpy(value, v, valsize - 1);
                    value[valsize - 1] = '\0';
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

UINT GetPrivateProfileInt(const char* lpAppName, const char* lpKeyName,
                          int nDefault, const char* lpFileName)
{
    FILE* fp = fopen(lpFileName, "r");
    if (!fp) return nDefault;

    char value[256];
    if (ini_find_key(fp, lpAppName, lpKeyName, value, sizeof(value))) {
        fclose(fp);
        return (UINT)atoi(value);
    }

    fclose(fp);
    return nDefault;
}

DWORD GetPrivateProfileString(const char* lpAppName, const char* lpKeyName,
                              const char* lpDefault, char* lpReturnedString,
                              DWORD nSize, const char* lpFileName)
{
    FILE* fp = fopen(lpFileName, "r");
    if (!fp) {
        if (lpDefault) {
            strncpy(lpReturnedString, lpDefault, nSize - 1);
            lpReturnedString[nSize - 1] = '\0';
        } else {
            lpReturnedString[0] = '\0';
        }
        return strlen(lpReturnedString);
    }

    char value[256];
    if (ini_find_key(fp, lpAppName, lpKeyName, value, sizeof(value))) {
        fclose(fp);
        strncpy(lpReturnedString, value, nSize - 1);
        lpReturnedString[nSize - 1] = '\0';
        return strlen(lpReturnedString);
    }

    fclose(fp);
    if (lpDefault) {
        strncpy(lpReturnedString, lpDefault, nSize - 1);
        lpReturnedString[nSize - 1] = '\0';
    } else {
        lpReturnedString[0] = '\0';
    }
    return strlen(lpReturnedString);
}

BOOL WritePrivateProfileString(const char* lpAppName, const char* lpKeyName,
                               const char* lpString, const char* lpFileName)
{
    // Read entire file into memory, modify, and write back
    FILE* fp = fopen(lpFileName, "r");
    char** lines = NULL;
    int line_count = 0;
    int line_capacity = 0;
    char line_buf[1024];

    if (fp) {
        while (fgets(line_buf, sizeof(line_buf), fp)) {
            if (line_count >= line_capacity) {
                line_capacity = line_capacity ? line_capacity * 2 : 64;
                lines = (char**)realloc(lines, line_capacity * sizeof(char*));
            }
            lines[line_count] = strdup(line_buf);
            line_count++;
        }
        fclose(fp);
    }

    // Find the section and key
    char sec[256];
    snprintf(sec, sizeof(sec), "[%s]", lpAppName);
    int in_section = 0;
    int insert_pos = -1;
    int found = 0;

    for (int i = 0; i < line_count; i++) {
        char* p = lines[i];
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '[') {
            char* end = strchr(p, ']');
            if (end) {
                *end = '\0';
                if (strcmp(p, sec) == 0) {
                    in_section = 1;
                } else {
                    if (in_section && !found) {
                        // Section ended without finding key, insert before this line
                        insert_pos = i;
                        in_section = 0;
                        break; // Don't need to search further
                    }
                    in_section = 0;
                }
                *end = ']';
            }
            continue;
        }

        if (in_section) {
            char* eq = strchr(p, '=');
            if (eq) {
                char* k = p;
                char* ke = eq;
                while (ke > k && (*(ke-1) == ' ' || *(ke-1) == '\t')) ke--;
                *ke = '\0';
                if (strcmp(k, lpKeyName) == 0) {
                    // Found the key, replace the line
                    char newline[1024];
                    snprintf(newline, sizeof(newline), "%s=%s\n", lpKeyName, lpString ? lpString : "");
                    free(lines[i]);
                    lines[i] = strdup(newline);
                    found = 1;
                    break;
                }
            }
        }
    }

    if (in_section && !found) {
        // Key not found but section exists, append to end of section
        insert_pos = line_count;
        for (int i = 0; i < line_count; i++) {
            char* p = lines[i];
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '[' && in_section) {
                insert_pos = i;
                break;
            }
        }
    }

    if (!found && insert_pos >= 0) {
        // Insert new key=value at insert_pos
        if (line_count >= line_capacity) {
            line_capacity = line_capacity ? line_capacity * 2 : 64;
            lines = (char**)realloc(lines, line_capacity * sizeof(char*));
        }
        memmove(&lines[insert_pos + 1], &lines[insert_pos],
                (line_count - insert_pos) * sizeof(char*));
        char newline[1024];
        snprintf(newline, sizeof(newline), "%s=%s\n", lpKeyName, lpString ? lpString : "");
        lines[insert_pos] = strdup(newline);
        line_count++;
    } else if (!found) {
        // Section doesn't exist, append section and key
        if (line_count + 2 >= line_capacity) {
            line_capacity = line_capacity ? line_capacity * 2 : 64;
            lines = (char**)realloc(lines, (line_capacity + 2) * sizeof(char*));
        }
        char sec_line[256];
        snprintf(sec_line, sizeof(sec_line), "[%s]\n", lpAppName);
        lines[line_count] = strdup(sec_line);
        line_count++;
        char kv_line[1024];
        snprintf(kv_line, sizeof(kv_line), "%s=%s\n", lpKeyName, lpString ? lpString : "");
        lines[line_count] = strdup(kv_line);
        line_count++;
    }

    // Write back
    fp = fopen(lpFileName, "w");
    if (!fp) {
        for (int i = 0; i < line_count; i++) free(lines[i]);
        free(lines);
        return FALSE;
    }

    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], fp);
        free(lines[i]);
    }
    free(lines);
    fclose(fp);
    return TRUE;
}
