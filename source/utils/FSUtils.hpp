#pragma once

#include <coreinit/filesystem_fsa.h>

class FSUtils {
public:
    static int Initialize();
    static int Finalize();

    static int WriteToFile(const char* path, const void* data, uint32_t size);
    static int ReadFromFile(const char* path, void* data, uint32_t size);

private:
    static inline FSAClientHandle clientHandle = -1;
};
