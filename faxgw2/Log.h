/************************************************************************
 *																		*
 *   This file is a part of fax gateway system.							*
 *																		*
 *   copyright(C) 2007 by �ڽ�											*
 *   Auther	: Zhq														*
 ************************************************************************/
//
// Typical use:
//
//       Log log;
//       log.SetFile( _T("myapp.log") );
//       ...
//       log.Print(2, _T("x = %d\n"), x);
//

#pragma once
#include <stdarg.h>
#include "linux_compat.h"

class Log  
{
public:
    // Logging mode flags:
    static const int ToDebug;
    static const int ToFile;
    static const int ToConsole;
	static	CRITICAL_SECTION	m_csSession;

    // Create a new log object.
    // Parameters as follows:
    //    mode     - specifies where output should go, using combination
    //               of flags above. ToConsole won't do anything on CE.
    //    level    - the default level
    //    filename - if flag Log::ToFile is specified in the type,
    //               a filename must be specified here.
    //    append   - if logging to a file, whether or not to append to any
    //               existing log.
	Log(int mode = ToDebug, int level = 1, char* filename = NULL, bool append = false);

    inline void Print(int level, char* format, ...) {
        if (level > m_level) return;
        va_list ap;
        va_start(ap, format);
        ReallyPrint(format, ap);
        va_end(ap);
    }
    
    // Change the log level
    void SetLevel(int level);

    // Change the logging mode
    void SetMode(int mode);

    // Change or set the logging filename.  This enables ToFile mode if
    // not already enabled.
    void SetFile(char* filename, bool append = false);

	virtual ~Log();

private:
    void ReallyPrint(char* format, va_list ap);
    void CloseFile();
    bool m_tofile, m_todebug, m_toconsole;
    int m_level;
    HANDLE hlogfile;
};
