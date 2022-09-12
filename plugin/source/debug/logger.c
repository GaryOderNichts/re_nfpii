#include "logger.h"
#include <stdio.h>

void DumpHex(const void* data, size_t size)
{
    char buffer[512];
    char* ptr = buffer;
    #define my_printf(x, ...) ptr += snprintf(ptr, (buffer + 512) - ptr, x, ##__VA_ARGS__)

    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        my_printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            my_printf(" ");
            if ((i+1) % 16 == 0) {
                my_printf("|  %s \n", ascii);
                DEBUG_FUNCTION_LINE_WRITE("%s", buffer);
                ptr = buffer;
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    my_printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    my_printf("   ");
                }
                my_printf("|  %s \n", ascii);
                DEBUG_FUNCTION_LINE_WRITE("%s", buffer);
                ptr = buffer;
            }
        }
    }

    #undef my_printf
}
