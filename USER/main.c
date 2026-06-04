#include "stm32f10x.h"
#include "delay.h"
#include "oled.h"
#include "vk36n16i.h"

int main(void) {
    // 1. 基础初始化
    delay_init();
    
    // 2. 只初始化 OLED（不初始化触摸键盘）
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (u8 *)"OLED Test OK", 16, 1);
    OLED_Refresh();
    
    // 3. 等待3秒
    delay_ms(3000);
    
    // 4. 再初始化触摸键盘
    VK36N16I_I2C_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, (u8 *)"Touch OK", 16, 1);
    OLED_Refresh();
    
    while(1) {
        delay_ms(1000);
    }
}
