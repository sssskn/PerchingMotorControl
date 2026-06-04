#include "stm32f10x.h"
#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "vk36n16i.h"
#include "motor.h"
#include "flash_storage.h"
#include "ui_display.h"
#include <string.h>
#include <stdlib.h>

// ==================== 全局变量 ====================
volatile SystemState_t currentState = STATE_IDLE;
volatile float targetSpeed = 0.0;      // 目标速度 (mm/s)
volatile float homePosition = 0.0;     // 起点位置 (mm)
volatile float brakePosition = 0.0;    // 刹车点位置 (mm)
//extern volatile uint32_t currentPulseCount;   // 当前已发送的脉冲数 (通过 TIM2 中断累加)

// 物理转换常量
#define PULSE_PER_MM   (64.0f / 3.0f)   // 每mm对应的脉冲数
#define MM_PER_PULSE   (3.0f / 64.0f)   // 每个脉冲对应的mm长度

// 进入 SET_SPEED 后的延迟计数
volatile uint8_t enterDelay = 0;
#define ENTER_DELAY_MS 500  // 延迟 500ms

// 用于待机界面显示的速度字符串
char displaySpeedBuffer[20] = {0};

// 提示信息显示相关
char popupMessage[20] = {0};
volatile uint8_t popupTimer = 0;
#define POPUP_DISPLAY_TIME 30  // 提示显示时间 (300ms)

// 手动控制标志
volatile uint8_t manual_fwd_flag = 0;
volatile uint8_t manual_rev_flag = 0;

// 速度输入缓冲区
char speedBuffer[20] = {0};
uint8_t bufferIndex = 0;

// 目标位置（脉冲数）
uint32_t targetBrakePulse = 0;
uint32_t targetHomePulse = 0;

// ==================== 函数声明 ====================
void Hardware_Init(void);
void System_Run_Loop(void);
void Process_Key_Input(uint8_t key);
uint8_t Map_Key_Code(uint16_t raw_val);

// ==================== 主函数 ====================
int main(void) {
    Hardware_Init();
    
    // 从Flash读取上次保存的数据
    homePosition = FLASH_Read_HomePoint() / 100.0f;
    brakePosition = FLASH_Read_BrakePoint() / 100.0f;
    
    // 如果有保存的数据，显示
    if (homePosition < 0) homePosition = 0;
    if (brakePosition < 0) brakePosition = 0;

    // 上电初始状态：刹车释放（电机可转动）
    Motor_Brake_Off();

    while(1) {
        System_Run_Loop();
        delay_ms(10);
    }
}

// ==================== 硬件初始化 ====================
void Hardware_Init(void) {
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // uart_init(115200);
    
    Motor_Init();          // 电机控制引脚
    VK36N16I_I2C_Init();   // 触摸键盘
    OLED_Init();           // OLED屏幕
    OLED_Clear();
    
    OLED_ShowString(0, 0, (u8 *)"System Init OK", 16, 1);
    delay_ms(1000);
    OLED_Clear();
}

// ==================== 按键映射 ====================
uint8_t Map_Key_Code(uint16_t raw_val) {
    switch(raw_val) {
        case 0x0001: return '1';
        case 0x0010: return '2';
        case 0x0100: return '3';
        case 0x1000: return 'A';
        case 0x0002: return '4';
        case 0x0020: return '5';
        case 0x0200: return '6';
        case 0x2000: return 'B';
        case 0x0004: return '7';
        case 0x0040: return '8';
        case 0x0400: return '9';
        case 0x4000: return 'C';
        case 0x0008: return '*';
        case 0x0080: return '0';
        case 0x0800: return '#';
        case 0x8000: return 'D';
        default: return 0;
    }
}

// ==================== 按键处理 ====================
void Process_Key_Input(uint8_t key) {
    switch(currentState) {
        // ===== 待机状态 =====
        case STATE_IDLE:
            if (key == '*') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        // ===== 菜单状态 =====
        case STATE_MENU:
            if (key == '1') {
                currentState = STATE_SET_SPEED;
                memset(speedBuffer, 0, sizeof(speedBuffer));
                bufferIndex = 1;
                speedBuffer[0] = '0';
                speedBuffer[1] = '\0';
                enterDelay = ENTER_DELAY_MS / 10;
                OLED_Clear();
            } else if (key == '2') {
                currentState = STATE_SET_HOME;
                OLED_Clear();
            } else if (key == '3') {
                currentState = STATE_SET_BRAKE;
                OLED_Clear();
            } else if (key == '4') {
                if (targetSpeed > 0) {
                    currentState = STATE_AUTO_RUN;
                    Motor_Brake_Off();
                    float target_rpm = targetSpeed / 5.0f;
                    Motor_Run_Forward((uint32_t)target_rpm);
                    OLED_Clear();
                }
            } else if (key == 'C') {
                // 一键复位
//                if (currentPulseCount == (uint32_t)(homePosition * PULSE_PER_MM)) {
//                    strcpy(popupMessage, "Already at Home");
//                    popupTimer = POPUP_DISPLAY_TIME;
//                } else {
//                    currentState = STATE_RESET;
//                    Motor_Brake_Off();
//                    Motor_Run_Reverse((uint32_t)targetSpeed);
//                    OLED_Clear();
//                }
            } else if (key == '#') {
                currentState = STATE_IDLE;
                OLED_Clear();
            }
            break;
            
        // ===== 设置速度状态 =====
        case STATE_SET_SPEED:
            if (enterDelay > 0) return;
            
            if (key >= '0' && key <= '9') {
                if (bufferIndex < 15) {
                    if (bufferIndex == 1 && speedBuffer[0] == '0' && key != '.') {
                        speedBuffer[0] = key;
                        speedBuffer[1] = '\0';
                        bufferIndex = 1;
                    } else {
                        speedBuffer[bufferIndex++] = key;
                        speedBuffer[bufferIndex] = '\0';
                    }
                }
            } else if (key == 'D') {
                if (bufferIndex < 15 && !strchr(speedBuffer, '.')) {
                    speedBuffer[bufferIndex++] = '.';
                    speedBuffer[bufferIndex] = '\0';
                }
            } else if (key == 'C') {
                memset(speedBuffer, 0, sizeof(speedBuffer));
                bufferIndex = 1;
                speedBuffer[0] = '0';
                speedBuffer[1] = '\0';
                strcpy(popupMessage, "Cleared!");
                popupTimer = POPUP_DISPLAY_TIME;
                OLED_Clear();
            } else if (key == '*') {
                float speed = atof(speedBuffer);
                targetSpeed = speed;
                strcpy(displaySpeedBuffer, speedBuffer);
                strcpy(popupMessage, "Saved: ");
                strcat(popupMessage, speedBuffer);
                popupTimer = POPUP_DISPLAY_TIME;
                bufferIndex = 0;
                memset(speedBuffer, 0, sizeof(speedBuffer));
                currentState = STATE_MENU;
            } else if (key == '#') {
                bufferIndex = 0;
                memset(speedBuffer, 0, sizeof(speedBuffer));
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        // ===== 设置起点状态 =====
        case STATE_SET_HOME:
            if (key == 'A' || key == 'B') {
                currentState = STATE_MANUAL;
                manual_fwd_flag = 0;
                manual_rev_flag = 0;
                Motor_Brake_Off();
            } else if (key == 'C') {
                // 手动标定起点：记录当前脉冲数
//                homePosition = (float)currentPulseCount * MM_PER_PULSE;
                FLASH_Save_HomePoint((int32_t)(homePosition * 100.0f));
                strcpy(popupMessage, "Home Calibrated");
                popupTimer = POPUP_DISPLAY_TIME;
                OLED_Clear();
            } else if (key == 'D') {
                currentState = STATE_MENU;
                OLED_Clear();
            } else if (key == '#') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        // ===== 设置刹车点状态 =====
        case STATE_SET_BRAKE:
            if (key == 'A' || key == 'B') {
                currentState = STATE_MANUAL;
                manual_fwd_flag = 0;
                manual_rev_flag = 0;
                Motor_Brake_Off();
            } else if (key == 'C') {
//                brakePosition = (float)currentPulseCount * MM_PER_PULSE;
                FLASH_Save_BrakePoint((int32_t)(brakePosition * 100.0f));
                strcpy(popupMessage, "Brake Set!");
                popupTimer = POPUP_DISPLAY_TIME;
                OLED_Clear();
            } else if (key == 'D') {
                currentState = STATE_MENU;
                OLED_Clear();
            } else if (key == '#') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        // ===== 手动控制状态 =====
        case STATE_MANUAL:
            if (key == 'A') {
                manual_fwd_flag = 1;
                manual_rev_flag = 0;
                Motor_Brake_Off();
                Motor_Run_Forward(200);
            } else if (key == 'B') {
                manual_rev_flag = 1;
                manual_fwd_flag = 0;
                Motor_Brake_Off();
                Motor_Run_Reverse(200);
            } else if (key == 0) {
                // 按键释放
            }
            break;
            
        // ===== 自动运行状态 =====
        case STATE_AUTO_RUN:
            // 运行中不允许按键操作
            break;
            
        // ===== 刹车状态 =====
        case STATE_BRAKING:
            if (key == '#') {
                currentState = STATE_RESET;
                Motor_Brake_Off();
                Motor_Run_Reverse(300);
            }
            break;
            
        // ===== 复位状态 =====
        case STATE_RESET:
            // 复位中不允许按键操作
            break;
            
        default:
            break;
    }
}

// ==================== 系统主循环 ====================
void System_Run_Loop(void) {
    uint16_t raw_key = 0;
    uint8_t key = 0;
    
    // 1. 延迟计数递减
    if (enterDelay > 0) {
        enterDelay--;
    }
    
    // 2. 提示信息显示
    if (popupTimer > 0) {
        OLED_Clear();
        OLED_ShowString(24, 24, (u8 *)popupMessage, 16, 1);
        OLED_Refresh();
        popupTimer--;
        return;
    }
    
    // 3. 扫描触摸键盘
    if (VK36N16I_Read_Keys() == 0) {
        raw_key = g_touch_key_value;
        key = Map_Key_Code(raw_key);
        if (key != 0) {
            Process_Key_Input(key);
            delay_ms(150);
        } else {
            Process_Key_Input(0);
        }
    }
    
    // 4. 状态机执行
    switch (currentState) {
        case STATE_AUTO_RUN:
            targetBrakePulse = (uint32_t)(brakePosition * PULSE_PER_MM);
//            if (currentPulseCount >= targetBrakePulse) {
//                currentState = STATE_BRAKING;
//                Motor_Stop();
//                Motor_Brake_On();
//            }
            break;
            
        case STATE_RESET:
            targetHomePulse = (uint32_t)(homePosition * PULSE_PER_MM);
//            if (currentPulseCount > targetHomePulse) {
//                float target_rpm = targetSpeed / 5.0f;
//                Motor_Run_Reverse((uint32_t)target_rpm);
//            } else {
//                Motor_Stop();
//                Motor_Brake_On();
//                currentState = STATE_IDLE;
//                currentPulseCount = targetHomePulse;
//                strcpy(popupMessage, "Reset Success!");
//                popupTimer = POPUP_DISPLAY_TIME;
//            }
            break;
            
        case STATE_MANUAL:
            if (manual_fwd_flag) {
                // 已通过按键设置
            } else if (manual_rev_flag) {
                // 已通过按键设置
            } else {
                Motor_Stop();
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        default:
            break;
    }
    
    // 5. 刷新显示
//    float float_pos_mm = (float)currentPulseCount * MM_PER_PULSE;
//    if (currentState == STATE_SET_SPEED) {
//        UI_Update_Display(currentState, targetSpeed, float_pos_mm, brakePosition, homePosition, speedBuffer);
//    } else {
//        UI_Update_Display(currentState, targetSpeed, float_pos_mm, brakePosition, homePosition, displaySpeedBuffer);
//    }
}
//#include "stm32f10x.h"
//#include "delay.h"
//#include "oled.h"

//int main(void) {
//    delay_init();
//    OLED_Init();
//    
//    OLED_Clear();
//    OLED_ShowString(0, 0, (u8 *)"Hello", 16, 1);
//    OLED_ShowString(0, 16, (u8 *)"OLED Test", 16, 1);
//    OLED_Refresh();
//    
//    while(1) {
//        delay_ms(1000);
//    }
//}
