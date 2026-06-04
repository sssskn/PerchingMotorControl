#include "vk36n16i.h"

// 全局变量，用于 Keil 窗口观测 
uint16_t g_touch_key_value = 0x0000;

/**
 * @brief  微秒级简易延时
 * @note   VK36N16I 传输速率最高 60Kbps 左右，时钟低/高电平时间需 >= 10us 
 */
static void I2C_Delay(void)
{
    volatile uint32_t i = 500; // 根据主频调整，确保产生 10us 以上的延时 
    while(i--);
}

/**
 * @brief  内部函数：将 SDA 引脚切换为开漏输出（或输入模式）
 * @note   为了方便切换，这里将 SDA 配置为开漏输出模式。
 * 在开漏模式下，向其写 1 即可直接通过外部/内部上拉电阻作为输入读取。
 */
static void SDA_Mode_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
}

/**
 * @brief  初始化模拟 I2C 的 GPIO 引脚
 */
void VK36N16I_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开启 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_I2C_PORT, ENABLE);
    
    // SCL 配置为开漏或推挽输出 (这里用推挽)
    GPIO_InitStructure.GPIO_Pin = I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
    GPIO_Init(I2C_PORT, &GPIO_InitStructure);
    
    // SDA 配置为开漏输出 (方便释放总线进行双向通讯)
    SDA_Mode_Output();
    
    // 释放总线，初始状态为高电平
    SCL_H;
    SDA_H;
}

/**
 * @brief  I2C 起始信号 [cite: 142]
 */
static void I2C_Start(void)
{
    SDA_H;
    SCL_H;
    I2C_Delay();
    SDA_L; // SCL 为高时，SDA 产生下降沿 [cite: 142]
    I2C_Delay();
    SCL_L; // 钳住总线，准备发送数据
    I2C_Delay();
}

/**
 * @brief  I2C 停止信号 [cite: 142]
 */
static void I2C_Stop(void)
{
    SDA_L;
    SCL_H; // SCL 为高时，SDA 产生上升沿 [cite: 142]
    I2C_Delay();
    SDA_H;
    I2C_Delay();
}

/**
 * @brief  I2C 发送一个字节
 */
static void I2C_Send_Byte(uint8_t byte)
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        if(byte & 0x80) SDA_H;
        else SDA_L;
        byte <<= 1;
        I2C_Delay();
        SCL_H;
        I2C_Delay();
        SCL_L;
        I2C_Delay();
    }
}

/**
 * @brief  I2C 读取一个字节
 * @param  ack: 0 发送 ACK, 1 发送 NACK [cite: 167, 168]
 */
static uint8_t I2C_Read_Byte(uint8_t ack)
{
    uint8_t i, receive = 0;
    SDA_H; // 释放数据线，准备接收
    
    for(i = 0; i < 8; i++)
    {
        SCL_H;
        I2C_Delay();
        receive <<= 1;
        if(READ_SDA) receive |= 0x01;   
        SCL_L;
        I2C_Delay();
    }
    
    // 发送应答或非应答信号 [cite: 164, 171]
    if (!ack) SDA_L; // ACK [cite: 167]
    else SDA_H;      // NACK [cite: 168]
    I2C_Delay();
    SCL_H;
    I2C_Delay();
    SCL_L;
    I2C_Delay();
    
    return receive;
}

/**
 * @brief  等待从机应答信号 (ACK) [cite: 164]
 * @retval 0: 接收到应答; 1: 接收应答失败
 */
static uint8_t I2C_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    SDA_H; I2C_Delay();
    SCL_H; I2C_Delay();
    
    while(READ_SDA)
    {
        ucErrTime++;
        if(ucErrTime > 250)
        {
            I2C_Stop();
            return 1;
        }
    }
    SCL_L;
    I2C_Delay();
    return 0;  
}

/**
 * @brief  软件模拟 I2C 轮询读取 VK36N16I 的 2 字节键值 [cite: 114, 160]
 * @retval 0: 读取成功; 1: 通讯失败
 */
uint8_t VK36N16I_Read_Keys(void)
{
    uint8_t data1 = 0, data2 = 0;
    
    // 1. 发送起始条件 [cite: 142, 161]
    I2C_Start();
    
    // 2. 发送读地址 0xCB [cite: 148, 162]
    I2C_Send_Byte(VK36N16I_READ_ADDR);
    if(I2C_Wait_Ack() != 0) // 如果没有应答则退出 [cite: 164]
    {
        I2C_Stop();
        return 1;
    }
    
    // 3. 读取第 1 个字节 (发送 ACK) [cite: 164, 165]
    data1 = I2C_Read_Byte(0); 
    
    // 4. 读取第 2 个字节 (发送 NACK) [cite: 170, 171]
    data2 = I2C_Read_Byte(1); 
    
    // 5. 发送停止条件 [cite: 142, 172]
    I2C_Stop();
    
    // 6. 组合 16 位键值数据 [cite: 120]
    g_touch_key_value = (uint16_t)((data2 << 8) | data1);
    
    return 0;
}
