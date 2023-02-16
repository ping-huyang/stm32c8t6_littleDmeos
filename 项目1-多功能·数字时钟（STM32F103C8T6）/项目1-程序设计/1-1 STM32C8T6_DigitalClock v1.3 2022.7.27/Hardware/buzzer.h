#ifndef __BUZZER_H
#define __BUZZER_H	 
#include "sys.h"

#define BUZZER_PORT	GPIOB	//定义IO接口
#define BUZZER_Pin	GPIO_Pin_0	//定义IO接口
#define BUZZER PBout(0)

void BUZZER_Init(void);//初始化
void BUZZER_BEEP1(void);//响一声
		 				    
#endif
