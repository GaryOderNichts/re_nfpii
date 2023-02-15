#include "LogHandler.hpp"
#include "debug/logger.h"
#include "re_nfpii/Lock.hpp"

#include <nfpii.h>
#include <wums.h>
#include <string>
#include <cstdarg>

#include <coreinit/mutex.h>

static NfpiiLogHandler logHandler;
static char logBuf[0x200];
static OSMutex logMutex;

static void Log(NfpiiLogVerbosity verb, const char* fmt, va_list arg)
{
    Lock lock(&logMutex);

    std::vsnprintf(logBuf, sizeof(logBuf), fmt, arg);

    if (logHandler) {
        logHandler(verb, logBuf);
    }
}

void LogHandler::Init(void)
{
    OSInitMutex(&logMutex);
}

void LogHandler::Info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_VERBOSITY_INFO, fmt, args);
    va_end(args);
}

void LogHandler::Warn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_VERBOSITY_WARN, fmt, args);
    va_end(args);
}

void LogHandler::Error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Log(LOG_VERBOSITY_ERROR, fmt, args);
    va_end(args);
}

void NfpiiSetLogHandler(NfpiiLogHandler handler)
{
    logHandler = handler;

    LogHandler::Info("Module: Log Handler %s", handler ? "set" : "cleared");
}

WUMS_EXPORT_FUNCTION(NfpiiSetLogHandler);
