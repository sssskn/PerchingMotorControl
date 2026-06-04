#include "stm32f10x.h"
#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "vk36n16i.h"
#include "flash_storage.h"
#include "ui_display.h"
#include <string.h>
#include <stdlib.h>

// ==================== 全局变量 ====================
volatile SystemState_t currentState = STATE_IDLE;
volatile float targetSpeed = 0.0;
volatile float homePosition = 0.0;
volatile float brakePosition = 0.0;

// 物理转换常量
#define PULSE_PER_MM   (64.0f / 3.0f)
#define MM_PER_PULSE   (3.0f / 64.0f)

// 延迟计数
volatile uint8_t enterDelay = 0;
#define ENTER_DELAY_MS 500

// 显示缓冲区
char displaySpeedBuffer[20] = {0};

// 提示信息
char popupMessage[20] = {0};
volatile uint8_t popupTimer = 0;
#define POPUP_DISPLAY_TIME 30

// 速度输入缓冲区
char speedBuffer[20] = {0};
uint8_t bufferIndex = 0;

// ==================== 函数声明 ====================
void Hardware_Init(void);
void System_Run_Loop(void);
void Process_Key_Input(uint8_t key);
uint8_t Map_Key_Code(uint16_t raw_val);

// ==================== 主函数 ====================
int main(void) {
    Hardware_Init();
    
    homePosition = FLASH_Read_HomePoint() / 100.0f;
    brakePosition = FLASH_Read_BrakePoint() / 100.0f;
    
    if (homePosition < 0) homePosition = 0;
    if (brakePosition < 0) brakePosition = 0;

    while(1) {
        System_Run_Loop();
        delay_ms(10);
    }
}

// ==================== 硬件初始化 ====================
void Hardware_Init(void) {
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    // 注意：不调用 Motor_Init()
    VK36N16I_I2C_Init();
    OLED_Init();
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
        case STATE_IDLE:
            if (key == '*') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
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
                    OLED_Clear();
                }
            } else if (key == '#') {
                currentState = STATE_IDLE;
                OLED_Clear();
            }
            break;
            
        case STATE_SET_SPEED:
            if (enterDelay > 0) return;
            
            if (key >= '0' && key <= '9') {
                if (bufferIndex < 15) {
                    if (bufferIndex == 1 && speedBuffer[0] == '0') {
                        speedBuffer[0] = key;
                        speedBuffer[1] = '\0';
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
                targetSpeed = atof(speedBuffer);
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
            
        case STATE_SET_HOME:
            if (key == 'C') {
                FLASH_Save_HomePoint((int32_t)(homePosition * 100.0f));
                strcpy(popupMessage, "Home Calibrated");
                popupTimer = POPUP_DISPLAY_TIME;
                OLED_Clear();
            } else if (key == 'D' || key == '#') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        case STATE_SET_BRAKE:
            if (key == 'C') {
                FLASH_Save_BrakePoint((int32_t)(brakePosition * 100.0f));
                strcpy(popupMessage, "Brake Set!");
                popupTimer = POPUP_DISPLAY_TIME;
                OLED_Clear();
            } else if (key == 'D' || key == '#') {
                currentState = STATE_MENU;
                OLED_Clear();
            }
            break;
            
        case STATE_AUTO_RUN:
        case STATE_BRAKING:
        case STATE_RESET:
            break;
            
        default:
            break;
    }
}

// ==================== 系统主循环 ====================
void System_Run_Loop(void) {
    uint16_t raw_key = 0;
    uint8_t key = 0;
    
    if (enterDelay > 0) enterDelay--;
    
    if (popupTimer > 0) {
        OLED_Clear();
        OLED_ShowString(24, 24, (u8 *)popupMessage, 16, 1);
        OLED_Refresh();
        popupTimer--;
        return;
    }
    
    if (VK36N16I_Read_Keys() == 0) {
        raw_key = g_touch_key_value;
        key = Map_Key_Code(raw_key);
        if (key != 0) {
            Process_Key_Input(key);
            delay_ms(150);
        }
    }
    
    // 模拟位置为0
    float float_pos_mm = 0;
    
    if (currentState == STATE_SET_SPEED) {
        UI_Update_Display(currentState, targetSpeed, float_pos_mm, brakePosition, homePosition, speedBuffer);
    } else {
        UI_Update_Display(currentState, targetSpeed, float_pos_mm, brakePosition, homePosition, displaySpeedBuffer);
    }
}
