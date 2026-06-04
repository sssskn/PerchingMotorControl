#include "ui_display.h"
#include "oled.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief  更新OLED显示内容 (128*64 像素精准对齐、零频闪、零截断版本)
 * @note   纵向 Y 坐标统一精细规划为: 0, 15, 30, 46，确保最后一行在 64 像素内完美呈现
 */
void UI_Update_Display(SystemState_t state, float speed, int32_t pos, int32_t brake, int32_t home, char* speedInput) {
    char buffer[20];
    int x_pos;

    switch(state) {
        // ===== 1. 待机状态 (STATE_IDLE) =====
        case STATE_IDLE:
            OLED_ShowString(0, 0, (u8 *)"Mode: IDLE      ", 16, 1);
            
            // Speed 右对齐
            OLED_ShowString(0, 15, (u8 *)"Speed:          ", 16, 1);
            if (speedInput != NULL && speedInput[0] != '\0') {
                x_pos = 128 - (strlen(speedInput) * 8);
                OLED_ShowString(x_pos, 15, (u8 *)speedInput, 16, 1);
            } else {
                OLED_ShowString(120, 15, (u8 *)"0", 16, 1);
            }
            
            // CurPos 右对齐
            OLED_ShowString(0, 30, (u8 *)"CurPos:         ", 16, 1);
            sprintf(buffer, "%d", pos);
            x_pos = 128 - (strlen(buffer) * 8);
            OLED_ShowString(x_pos, 30, (u8 *)buffer, 16, 1);
            
            // BrkPos 右对齐
            OLED_ShowString(0, 46, (u8 *)"BrkPos:         ", 16, 1);
            sprintf(buffer, "%d", brake);
            x_pos = 128 - (strlen(buffer) * 8);
            OLED_ShowString(x_pos, 46, (u8 *)buffer, 16, 1);
            break;
            
        // ===== 2. 菜单状态 (STATE_MENU) =====
        case STATE_MENU:
            OLED_ShowString(0, 0,  (u8 *)"1.Set TargetSpd ", 16, 1);
            OLED_ShowString(0, 15, (u8 *)"2.Set Home Point", 16, 1);
            OLED_ShowString(0, 30, (u8 *)"3.Set BrakePoint", 16, 1);
            OLED_ShowString(0, 46, (u8 *)"4.AutoRun #:Back", 16, 1); // 完美抬高，防止字脚缺失
            break;
            
        // ===== 3. 设置速度状态 (STATE_SET_SPEED) =====
        // ===== 3. 设置速度状态 (STATE_SET_SPEED) =====
        case STATE_SET_SPEED:
            OLED_ShowString(0, 0, (u8 *)"Set Target Speed", 16, 1); // 第1行：标题
            
            // "Input:"占6个字宽（48像素），后面留出整行空格直接覆盖历史输入残影
            OLED_ShowString(0, 15, (u8 *)"Input:          ", 16, 1); // 第2行：输入框
            if (speedInput != NULL && speedInput[0] != '\0') {
                OLED_ShowString(48, 15, (u8 *)speedInput, 16, 1); // 紧跟在 X=48 后面显示数字
            } else {
                OLED_ShowString(48, 15, (u8 *)"0", 16, 1);
            }
            
            // 修正：利用第3行清晰展示 C 键的功能
            OLED_ShowString(0, 30, (u8 *)"C: Clear Input  ", 16, 1); // 第3行：提示清空
            
            // 第4行：提示保存与退出（抬高到 Y=46，确保 64 像素高屏幕底端不截断）
            OLED_ShowString(0, 46, (u8 *)"*:Save  #:Cancel", 16, 1); 
            break;
            
        // ===== 4. 设置起点状态 (STATE_SET_HOME) =====
        case STATE_SET_HOME:
            OLED_ShowString(0, 0,  (u8 *)"Set Home Point  ", 16, 1);
            OLED_ShowString(0, 15, (u8 *)"A/B: Manual Move", 16, 1);
            OLED_ShowString(0, 30, (u8 *)"C: CalibrateHome", 16, 1);
            
            sprintf(buffer, "Pos: %d         ", pos);
            OLED_ShowString(0, 46, (u8 *)buffer, 16, 1);
            break;
            
        // ===== 5. 设置刹车点状态 (STATE_SET_BRAKE) =====
        case STATE_SET_BRAKE:
            OLED_ShowString(0, 0,  (u8 *)"Set Brake Point ", 16, 1);
            OLED_ShowString(0, 15, (u8 *)"A/B: Manual Move", 16, 1);
            OLED_ShowString(0, 30, (u8 *)"C: Calibrate Brk", 16, 1);
            
            sprintf(buffer, "Pos: %d         ", pos);
            OLED_ShowString(0, 46, (u8 *)buffer, 16, 1);
            break;
            
        // ===== 6. 手动控制状态 (STATE_MANUAL) =====
        case STATE_MANUAL:
            OLED_ShowString(0, 0,  (u8 *)"Mode: MANUAL    ", 16, 1);
            OLED_ShowString(0, 15, (u8 *)"A: Forward      ", 16, 1);
            OLED_ShowString(0, 30, (u8 *)"B: Reverse      ", 16, 1);
            OLED_ShowString(0, 46, (u8 *)"C: Set Home     ", 16, 1);
            break;
            
        // ===== 7. 自动运行状态 (STATE_AUTO_RUN) =====
        case STATE_AUTO_RUN:
            // 第一行：显示当前运动速度 (右对齐)
            OLED_ShowString(0, 0, (u8 *)"CurSpeed:          ", 16, 1);
            sprintf(buffer, "%.1f RPM", speed); // speed传参此时应传入实时测得的编码器速度
            x_pos = 128 - (strlen(buffer) * 8);
            OLED_ShowString(x_pos, 0, (u8 *)buffer, 16, 1);
            
            // 第二行：显示当前运动位置 (右对齐)
            OLED_ShowString(0, 20, (u8 *)"CurPos:         ", 16, 1);
            sprintf(buffer, "%d", pos);         // pos为编码器计数的实时绝对位置
            x_pos = 128 - (strlen(buffer) * 8);
            OLED_ShowString(x_pos, 20, (u8 *)buffer, 16, 1);
            
            // 底部两行留白或清除残影
            OLED_ShowString(0, 40, (u8 *)"                ", 16, 1);
            OLED_ShowString(0, 52, (u8 *)"                ", 16, 1);
            break;

        // ===== 8. 新增：复位中状态 (STATE_RESET) =====
        case STATE_RESET:
            OLED_ShowString(0, 10, (u8 *)" System Reset   ", 16, 1);
            OLED_ShowString(0, 30, (u8 *)" Returning Home ", 16, 1);
            
            // 实时显示回原点时的脉冲位置
            sprintf(buffer, "Pos: %d         ", pos);
            OLED_ShowString(24, 48, (u8 *)buffer, 16, 1);
            break;

        default:
            break;
    }
}
