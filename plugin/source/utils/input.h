#pragma once

#include <padscore/kpad.h>
#include <vpad/input.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPAD_BUTTON_RESERVED_BIT 0x80000

uint32_t remapWiiMoteButtons(uint32_t buttons);
uint32_t remapClassicButtons(uint32_t buttons);
uint32_t remapProButtons(uint32_t buttons);

#ifdef __cplusplus
}
#endif
