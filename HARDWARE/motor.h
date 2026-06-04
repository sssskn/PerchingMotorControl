#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

void Motor_Init(void);
void Motor_Run_Forward(uint32_t speed_rpm);
void Motor_Run_Reverse(uint32_t speed_rpm);
void Motor_Stop(void);
void Motor_Brake_On(void);
void Motor_Brake_Off(void);

#endif /* __MOTOR_H */
