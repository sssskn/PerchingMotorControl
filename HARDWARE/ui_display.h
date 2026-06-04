#ifndef __UI_DISPLAY_H
#define __UI_DISPLAY_H

#include "stm32f10x.h"
#include "oled.h"
#include <string.h>

// 系统状态枚举
typedef enum {
    STATE_IDLE,        // 待机：显示主界面，只能查看状态
    STATE_MENU,        // 菜单：选择要设置的参数
    STATE_SET_SPEED,   // 设置速度：输入速度值
    STATE_SET_HOME,    // 设置起点：手动控制电机，标定起点
    STATE_SET_BRAKE,   // 设置刹车点：手动控制电机，标定刹车点
    STATE_MANUAL,      // 手动模式：用于标定时的电机控制
    STATE_AUTO_RUN,    // 自动运行
    STATE_BRAKING,     // 刹车
    STATE_RESET        // 复位
} SystemState_t;
extern char displaySpeedBuffer[20];

// 函数声明
void UI_Update_Display(SystemState_t state, float speed, int32_t pos, int32_t brake, int32_t home, char* speedInput);

#endif /* __UI_DISPLAY_H */
