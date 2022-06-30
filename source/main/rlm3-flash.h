#pragma once

#include "rlm3-base.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RLM3_FLASH_SIZE (2048)

extern void RLM3_Flash_Init();
extern void RLM3_Flash_Deinit();
extern bool RLM3_Flash_IsInit();

extern bool RLM3_Flash_Write(uint32_t address, const uint8_t* data, size_t size);
extern bool RLM3_Flash_Read(uint32_t address, uint8_t* data, size_t size);


#ifdef __cplusplus
}
#endif
