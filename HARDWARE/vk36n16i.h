#ifndef __VK36N16I_H
#define __VK36N16I_H

#include "stm32f10x.h" // 根据你的芯片修改，如 stm32f4xx.h

// VK36N16I 从机读地址 (7位地址 0x65 -> 左移一位 0xCA -> 读操作 0xCB) [cite: 102, 148]
#define VK36N16I_READ_ADDR    0xCB

// ------------------ 软件 I2C 引脚配置 ------------------
#define RCC_I2C_PORT       RCC_APB2Periph_GPIOB
#define I2C_PORT           GPIOB
#define I2C_SCL_PIN        GPIO_Pin_6
#define I2C_SDA_PIN        GPIO_Pin_7

// ------------------ GPIO 状态控制宏 ------------------
#define SCL_H    GPIO_SetBits(I2C_PORT, I2C_SCL_PIN)
#define SCL_L    GPIO_ResetBits(I2C_PORT, I2C_SCL_PIN)
#define SDA_H    GPIO_SetBits(I2C_PORT, I2C_SDA_PIN)
#define SDA_L    GPIO_ResetBits(I2C_PORT, I2C_SDA_PIN)

// 读取 SDA 引脚电平
#define READ_SDA GPIO_ReadInputDataBit(I2C_PORT, I2C_SDA_PIN)

// ------------------ 外部声明 ------------------
extern uint16_t g_touch_key_value;

void VK36N16I_I2C_Init(void);
uint8_t VK36N16I_Read_Keys(void);

#endif /* __VK36N16I_H */
