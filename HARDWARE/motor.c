#include "motor.h"
#include "stm32f10x.h"

// 引脚定义
#define MOTOR_PUL_PIN   GPIO_Pin_0
#define MOTOR_DIR_PIN   GPIO_Pin_1
#define MOTOR_ENA_PIN   GPIO_Pin_2
#define MOTOR_BRAKE_PIN GPIO_Pin_3
#define MOTOR_PORT      GPIOA
#define MOTOR_GPIO_CLK  RCC_APB2Periph_GPIOA

// 定时器用于PWM输出
#define MOTOR_TIM        TIM2
#define MOTOR_TIM_CLK    RCC_APB1Periph_TIM2
volatile uint32_t currentPulseCount = 0;

/**
 * @brief  初始化电机控制引脚和PWM
 */
void Motor_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(MOTOR_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(MOTOR_TIM_CLK, ENABLE);

    // 2. 配置GPIO
    GPIO_InitStructure.GPIO_Pin = MOTOR_PUL_PIN | MOTOR_DIR_PIN | MOTOR_ENA_PIN | MOTOR_BRAKE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT, &GPIO_InitStructure);

    // 初始状态
    GPIO_ResetBits(MOTOR_PORT, MOTOR_PUL_PIN);   // 脉冲低
    GPIO_SetBits(MOTOR_PORT, MOTOR_DIR_PIN);     // 默认正向
    GPIO_ResetBits(MOTOR_PORT, MOTOR_ENA_PIN);   // 使能 (低有效)
    GPIO_ResetBits(MOTOR_PORT, MOTOR_BRAKE_PIN); // 刹车解除 (低有效)

    // 3. 配置定时器PWM (用于产生脉冲)
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;    // 初始值
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;   // 72MHz / 72 = 1MHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(MOTOR_TIM, &TIM_TimeBaseStructure);

    // 4. 配置PWM通道 (CH1 - PA0)
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500; // 50% 占空比
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(MOTOR_TIM, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(MOTOR_TIM, TIM_OCPreload_Enable);
    TIM_Cmd(MOTOR_TIM, DISABLE); // 初始不输出
}

/**
 * @brief  设置电机速度 (单位: RPM)
 * @param  speed_rpm: 目标速度
 * @note   假设驱动器细分 6400 脉冲/转
 */
// 在 Motor_Set_Speed 函数中添加
void Motor_Set_Speed(uint32_t speed_rpm) {
    if (speed_rpm == 0) {
        TIM_Cmd(MOTOR_TIM, DISABLE);
        return;
    }

    // 计算脉冲频率和周期
    uint32_t pulse_freq = (speed_rpm * 6400) / 60;
    uint32_t period = 1000000 / pulse_freq;
    
    if (period < 1) period = 1;
    if (period > 65535) period = 65535;

    TIM_SetAutoreload(MOTOR_TIM, period - 1);
    TIM_SetCompare1(MOTOR_TIM, period / 2);
    
    // 开启 TIM2 更新中断
    TIM_ITConfig(MOTOR_TIM, TIM_IT_Update, ENABLE);
    NVIC_EnableIRQ(TIM2_IRQn);
    
    TIM_Cmd(MOTOR_TIM, ENABLE);
}

/**
 * @brief  电机正转
 */
void Motor_Run_Forward(uint32_t speed_rpm) {
    GPIO_SetBits(MOTOR_PORT, MOTOR_DIR_PIN);
    Motor_Set_Speed(speed_rpm);
}

/**
 * @brief  电机反转
 */
void Motor_Run_Reverse(uint32_t speed_rpm) {
    GPIO_ResetBits(MOTOR_PORT, MOTOR_DIR_PIN);
    Motor_Set_Speed(speed_rpm);
}

/**
 * @brief  电机停止 (停止脉冲，保持使能)
 */
void Motor_Stop(void) {
    TIM_Cmd(MOTOR_TIM, DISABLE);
    GPIO_ResetBits(MOTOR_PORT, MOTOR_PUL_PIN);
}

/**
 * @brief  开启刹车 (刹车抱死)
 */
void Motor_Brake_On(void) {
    // 输出低电平，让继电器吸合，切断 24V，刹车抱死
    GPIO_ResetBits(MOTOR_PORT, MOTOR_BRAKE_PIN);
}

/**
 * @brief  解除刹车 (刹车释放)
 */
void Motor_Brake_Off(void) {
    // 输出高电平，让继电器断开，接通 24V，刹车释放
    GPIO_SetBits(MOTOR_PORT, MOTOR_BRAKE_PIN);
}
