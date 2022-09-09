#pragma once

#include <coreinit/mutex.h>

// Simple RAII style lock guard class Nintendo seems to be using
// The compiler inlines most of this so it's hard to tell how this looks exactly
class Lock {
public:
    Lock(OSMutex* mutex) : mutex(mutex)
    {
        OSLockMutex(mutex);
        isLocked = true;
    }

    ~Lock()
    {
        if (mutex && isLocked) {
            OSUnlockMutex(mutex);
        }
    }

private:
    OSMutex* mutex;
    bool isLocked;
};
