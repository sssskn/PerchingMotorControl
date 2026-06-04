#ifndef __FLASH_STORAGE_H
#define __FLASH_STORAGE_H

#include "stm32f10x.h"

// 函数声明
void FLASH_Save_HomePoint(int32_t value);
int32_t FLASH_Read_HomePoint(void);
void FLASH_Save_BrakePoint(int32_t value);
int32_t FLASH_Read_BrakePoint(void);

#endif /* __FLASH_STORAGE_H */
