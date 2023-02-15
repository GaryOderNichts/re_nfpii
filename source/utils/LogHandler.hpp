#pragma once

class LogHandler {
public:
    static void Init();
    static void Info(const char* fmt, ...);
    static void Warn(const char* fmt, ...);
    static void Error(const char* fmt, ...);
};
