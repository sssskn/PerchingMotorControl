#include "flash_storage.h"
#include "stm32f10x_flash.h"

// 使用最后2页Flash保存数据 (16KB Flash: 0x0800F800 - 0x0800FFFF)
#define FLASH_HOME_ADDR   0x0800FC00  // 第127页
#define FLASH_BRAKE_ADDR  0x0800FE00  // 第128页

/**
 * @brief  保存起点位置到Flash
 */
void FLASH_Save_HomePoint(int32_t value) {
    FLASH_Unlock();
    FLASH_ErasePage(FLASH_HOME_ADDR);
    FLASH_ProgramWord(FLASH_HOME_ADDR, value);
    FLASH_Lock();
}

/**
 * @brief  从Flash读取起点位置
 */
int32_t FLASH_Read_HomePoint(void) {
    return *(int32_t*)FLASH_HOME_ADDR;
}

/**
 * @brief  保存刹车点到Flash
 */
void FLASH_Save_BrakePoint(int32_t value) {
    FLASH_Unlock();
    FLASH_ErasePage(FLASH_BRAKE_ADDR);
    FLASH_ProgramWord(FLASH_BRAKE_ADDR, value);
    FLASH_Lock();
}

/**
 * @brief  从Flash读取刹车点
 */
int32_t FLASH_Read_BrakePoint(void) {
    return *(int32_t*)FLASH_BRAKE_ADDR;
}
