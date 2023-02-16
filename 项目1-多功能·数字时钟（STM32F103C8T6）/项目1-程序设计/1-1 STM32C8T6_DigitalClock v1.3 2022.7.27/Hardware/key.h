#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"	 


//#define KEY1 PEin(4)   	//PE4
//#define KEY2 PEin(3)	//PE3 
//#define KEY3 PEin(2)	//PE2
//#define KEY4 PAin(0)	//PA0  KEY4

#define KEY1  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_4)//读取按键1
#define KEY2  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_5)//读取按键2
#define KEY3  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_6)//读取按键3
#define KEY4  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_7)//读取按键4

 

#define KEY1_PRES 	1	//KEY1按下
#define KEY2_PRES	2	//KEY2按下
#define KEY3_PRES	3	//KEY3按下
#define KEY4_PRES   4	//KEY4按下


void KEY_Init(void);//IO初始化

u8 KEY_Scan(u8);  	//按键扫描函数					    
#endif
