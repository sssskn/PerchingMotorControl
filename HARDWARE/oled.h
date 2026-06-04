#ifndef __OLED_H
#define __OLED_H 

#include "sys.h"
#include "stdlib.h"	

//-----------------OLED端口定义---------------- 
 

#define OLED_SCL_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_10)//SCL
#define OLED_SCL_Set() GPIO_SetBits(GPIOB, GPIO_Pin_10)

#define OLED_SDA_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_11)//SDA
#define OLED_SDA_Set() GPIO_SetBits(GPIOB, GPIO_Pin_11)

#define OLED_RES_Clr() GPIO_ResetBits(GPIOB, GPIO_Pin_12)//RES
#define OLED_RES_Set() GPIO_SetBits(GPIOB, GPIO_Pin_12)

#define OLED_DC_Clr()  GPIO_ResetBits(GPIOB, GPIO_Pin_13)//DC
#define OLED_DC_Set()  GPIO_SetBits(GPIOB, GPIO_Pin_13)

#define OLED_CS_Clr()  GPIO_ResetBits(GPIOB, GPIO_Pin_14)//CS
#define OLED_CS_Set()  GPIO_SetBits(GPIOB, GPIO_Pin_14)


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

void OLED_ClearPoint(u8 x,u8 y);
void OLED_ColorTurn(u8 i);
void OLED_DisplayTurn(u8 i);
void OLED_WR_Byte(u8 dat,u8 mode);
void OLED_DisPlay_On(void);
void OLED_DisPlay_Off(void);
void OLED_Refresh(void);
void OLED_Clear(void);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode);
void OLED_DrawCircle(u8 x,u8 y,u8 r);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode);
void OLED_ShowChar6x8(u8 x,u8 y,u8 chr,u8 mode);
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode);
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode);
void OLED_ScrollDisplay(u8 num,u8 space,u8 mode);
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode);
void OLED_SpeedShow(void);
void OLED_CLCLine2_Num(void);//清除设定的速度值
void OLED_CLCLine3_Char(void);//清除状态
void OLED_ShowCurrentSpeed(float input_speed);//在第一栏显示当前速度的值
void OLED_Init(void);

#endif

