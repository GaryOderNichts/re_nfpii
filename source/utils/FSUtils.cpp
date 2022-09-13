#include "FSUtils.hpp"
#include <debug/logger.h>

int FSUtils::Initialize()
{
    if (clientHandle >= 0) {
        return clientHandle;
    }

    FSAInit();
    clientHandle = FSAAddClient(nullptr);
    if (clientHandle < 0) {
        DEBUG_FUNCTION_LINE("FSAAddClient: %x", clientHandle);
        return clientHandle;
    }

    FSError err = FSAMount(clientHandle, "/dev/sdcard01", "/vol/external01", FSA_MOUNT_FLAG_LOCAL_MOUNT, nullptr, 0);
    if (err < 0 && err != FS_ERROR_ALREADY_EXISTS) {
        DEBUG_FUNCTION_LINE("FSAMount: %x", err);

        FSADelClient(clientHandle);
        clientHandle = -1;
        return err;
    }

    return 0;
}

int FSUtils::Finalize()
{
    if (clientHandle < 0) {
        return clientHandle;
    }

    FSAUnmount(clientHandle, "/vol/external01", FSA_UNMOUNT_FLAG_BIND_MOUNT);
    FSADelClient(clientHandle);
    clientHandle = -1;

    return 0;
}

int FSUtils::WriteToFile(const char* path, const void* data, uint32_t size)
{
    if (clientHandle < 0) {
        return clientHandle;
    }

    FSAFileHandle fileHandle;
    FSError err = FSAOpenFileEx(clientHandle, path, "wb", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fileHandle);
    if (err < 0) {
        return err;
    }

    __attribute__((aligned(0x40))) uint8_t buf[0x40];

    uint32_t bytesWritten = 0;
    while (bytesWritten < size) {
        uint32_t toWrite = size - bytesWritten;
        if (toWrite > sizeof(buf)) {
            toWrite = sizeof(buf); 
        }

        memcpy(buf, (uint8_t*) data + bytesWritten, toWrite);
        err = FSAWriteFile(clientHandle, buf, 1, toWrite, fileHandle, 0);
        if (err < 0) {
            break;
        }

        bytesWritten += err;

        if ((uint32_t) err != toWrite) {
            break;
        }
    }

    FSACloseFile(clientHandle, fileHandle);
    return bytesWritten;
}

int FSUtils::ReadFromFile(const char* path, void* data, uint32_t size)
{
    if (clientHandle < 0) {
        return clientHandle;
    }

    FSAFileHandle fileHandle;
    FSError err = FSAOpenFileEx(clientHandle, path, "rb", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fileHandle);
    if (err < 0) {
        return err;
    }

    __attribute__((aligned(0x40))) uint8_t buf[0x40];

    uint32_t bytesRead = 0;
    while (bytesRead < size) {
        uint32_t toRead = size - bytesRead;
        if (toRead > sizeof(buf)) {
            toRead = sizeof(buf); 
        }

        err = FSAReadFile(clientHandle, buf, 1, toRead, fileHandle, 0);
        if (err < 0) {
            break;
        }

        memcpy((uint8_t*) data + bytesRead, buf, err);
        bytesRead += err;

        if ((uint32_t) err != toRead) {
            break;
        }
    }

    FSACloseFile(clientHandle, fileHandle);
    return bytesRead;
}
