#include "stm32f10x.h"       
#include "OLED.h"
#include "AD.h"
#include "delay.h"
#include "sys.h"
#include "rtc.h"
#include "led.h"
#include "Key.h"
#include "Timer.h"
#include "buzzer.h"
 /**
  * @brief  项目1：多功能数字时钟
  * @note          基本功能： 1：时间显示功能（RTC）	    ok
  * @note					  2：调时功能（TIM，EXTI）     ok
  * @note					  3：闹钟功能				  ok
  * @note					  4：按键音效功能			  ok
  * @note							  
  * @note		   扩展功能： 1：模拟信号测量转化显示（例如温湿度测量）（ADC和DMA配合使用）
  * @note					  2：闹钟扩展（设置多个闹钟，并控制每个闹钟是否开启）			ok
  * @note					  3：温度控制外设开关
  * @note					  4:倒计时功能，计时功能
  */
void BUZZER_BOOM(void);
void PageOne(void);
void PageTwo(void);
void PageThree(void);
void PageFive(void);
void PageSix(void);
void PageFour(void);
void PageSeven(void);
void ClockConfig(void);

#define ChangeTime 2 //跳转界面时间间隔（s）
#define TimerDownMinute 0
#define TimerDownSecond 30
vu16 year = 2022;
extern unsigned short int AD_Value[4];
signed char month = 7,day = 22,hour = 7,minute = 59,second = 40,arm_hour[4] = {8,8,8,8},arm_minute[4] = {0,0,0,0},arm_second[4] = {30,10,20,0};
unsigned char KeyNumber  = 0,MUNE = 1,j = 1,k = 1,i = ChangeTime,SelectedFlag = 0,ChangeFlag = 0,Flag = 1,SecondFlag = 1,a = 0;
unsigned char ClockStatus[4] = {0},array[4] = {0,1,2,3};
unsigned char BeginFlag = 0,Timer_minute = 0,Timer_second = 0,FirstFlag = 1,Minute_Save = 0,Second_Save = 0,Choose = 0;
unsigned char Minute_Save2 = 0,Second_Save2 = 0;
signed char Timer_down = TimerDownSecond,Timer_minute2 = TimerDownMinute;

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	
	OLED_Init();
	delay_init();
	BUZZER_Init();
	LED_Init();
	KEY_Init();
	Timer_Init();
	RTC_Init();
	AD_Init();
	RTC_Set(year,month,day,hour,minute,second);		//初始化时间信息
	ClockConfig();
	OLED_ShowString(1,1,"Init OK");
	OLED_ShowString(2,1,"While Going ....");
	BUZZER_BOOM();
	while (1)
	{
		//页面0：用于显示数字时钟，显示基本的时间信息，按1号按钮实现调整时间的功能
		if(MUNE == 1){ PageOne();}
		
		//页面2：用于显示闹钟，该函数需要添加设定闹钟的功能
		if(MUNE == 2){ PageTwo(); }
		
		//页面1：计时和倒计时功能
		if(MUNE == 3){ PageThree();}
		
		//界面3：光敏电阻控制RED_LED亮灭
		if(MUNE == 4){ PageFour(); }
		
		//界面5：时间修改界面
		if(MUNE == 5){ PageFive(); }
		
		//界面5：闹钟修改界面
		if(MUNE == 6){ PageSix(); }
		
		//界面7：闹钟到提示界面
		if(MUNE == 7){ PageSeven(); }
	}
}

//定时器2中断，完成设置时间时的闪烁功能
 void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		SelectedFlag = !SelectedFlag;	//产生一个频率为2Hz的方波脉冲信号
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
} 

//RTC中断，用于闹钟中断触发和更新时间
void RTC_IRQHandler(void)
{		

	if(RTC_GetITStatus(RTC_IT_ALR)!= RESET)//闹钟中断
	{  	
		RTC_Get();				//更新时间  
		Flag = 1;
		ClockConfig();
		MUNE = 7;
		RTC_ClearITPendingBit(RTC_IT_ALR);		//清闹钟中断			
  	}
	
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)//秒钟中断
	{	
		LED = !LED;
		SecondFlag = 1 - SecondFlag;
		Timer_second ++;
		Timer_down --;
		RTC_Get();//更新时间 
		RTC_ClearITPendingBit(RTC_IT_SEC);
 	}			  								 
//	RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//清闹钟中断
	RTC_WaitForLastTask();	  	    						 	   	 
}

/**
  * @brief  初始化后蜂鸣器响三声
  * @param  
  * @retval 
  */
void BUZZER_BOOM()
{
	for(u8 count = 0 ; count < 3 ; count++)
	{
		BUZZER_BEEP1();
		delay_ms(200);
	}
}

/**
  * @brief  多个闹钟配置
  * @param  
  * @retval 
  */
void ClockConfig()
{
	unsigned char loation = 0,temp;
	signed char count = 0,i,j;
	unsigned char ss_hour = calendar.hour,ss_minute = calendar.min,ss_second = calendar.sec;
	unsigned char arm_hour_copy[4] = {0},arm_minute_copy[4] = {0},arm_second_copy[4] = {0};
	
	//提取出有效的闹钟信息，count用于保存有效闹钟数目
	for(i = 0 ; i < 4 ; i ++)
	{
		if(ClockStatus[i] != 0)
		{
			arm_hour_copy[count] = arm_hour[i];
			arm_minute_copy[count] = arm_minute[i];
			arm_second_copy[count] = arm_second[i];
			count++;
		}
	}

	//将几个有效闹钟时间进行从小到大比较
	for(i = 0 ; i < count - 1 ; i ++)
	{
		for(j = 0 ; j < count-i-1 ; j ++)
		{		
			if((arm_hour_copy[j+1] < arm_hour_copy[j]) || (arm_hour_copy[j+1] == arm_hour_copy[j] && arm_minute_copy[j+1] < arm_minute_copy[j]) || (arm_hour_copy[j+1] == arm_hour_copy[j]  && arm_minute_copy[j+1] == arm_minute_copy[j] && arm_second_copy[j+1] < arm_second_copy[j]))
			{
				temp = arm_hour_copy[j];arm_hour_copy[j] = arm_hour_copy[j+1];arm_hour_copy[j+1] = temp;
				temp = arm_minute_copy[j];arm_minute_copy[j] = arm_minute_copy[j+1];arm_minute_copy[j+1] = temp;
				temp = arm_second_copy[j];arm_second_copy[j] = arm_second_copy[j+1];arm_second_copy[j+1] = temp;
			}
		}
	}
	
	//通过逐一比较，确定当前时间的位置
	for(i = 0 ; i < count; i ++)
	{
		if((ss_hour > arm_hour_copy[i] )|| (ss_hour == arm_hour_copy[i] && ss_minute > arm_minute_copy[i]) || (ss_hour == arm_hour_copy[i] && ss_minute == arm_minute_copy[i] && ss_second >= arm_second_copy[i]))
		{
			loation ++;
			loation %= 4;
		}
	}
	

	RTC_Alarm_Set(calendar.w_year,calendar.w_month,calendar.w_date,arm_hour_copy[loation],arm_minute_copy[loation],arm_second_copy[loation]);		

	if(ClockStatus[0] == 0 & ClockStatus[1] ==0 & ClockStatus[2] == 0 & ClockStatus[3] == 0)
	{
		RTC_Alarm_Set(1999,12,31,0,0,0);
	}

//	OLED_Clear();
//	OLED_ShowNum(1,7,loation,3);
//	OLED_ShowNum(1,11,count,3);
//	OLED_ShowNum(1,1,arm_second_copy[0],4);
//	OLED_ShowNum(2,1,arm_second_copy[1],4);
//	OLED_ShowNum(3,1,arm_second_copy[2],4);
//	OLED_ShowNum(4,1,arm_second_copy[3],4);
//	OLED_ShowNum(2,7,ss_hour,4);
//	OLED_ShowNum(3,7,ss_minute,4);
//	OLED_ShowNum(4,7,ss_second,4); 
}

/**
  * @brief  显示时间信息，调节时间
  * @param  
  * @retval 
  */
void PageOne()
{
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		OLED_ShowString(1,1,"Digital Time :");
		OLED_ShowString(2,1,"    -  -  ");
		OLED_ShowString(3,1,"  :  :  ");
		switch(calendar.week)
		{
			case 0:
				OLED_ShowString(4,1,"Sunday   ");
				break;
			case 1:
				OLED_ShowString(4,1,"Monday   ");
				break;
			case 2:
				OLED_ShowString(4,1,"Tuesday  ");
				break;
			case 3:
				OLED_ShowString(4,1,"Wednesday");
				break;
			case 4:
				OLED_ShowString(4,1,"Thursday ");
				break;
			case 5:
				OLED_ShowString(4,1,"Friday   ");
				break;
			case 6:
				OLED_ShowString(4,1,"Saturday ");
				break; 
		}
	}
	
	OLED_ShowNum(2,1,calendar.w_year,4);
	OLED_ShowNum(2,6,calendar.w_month,2);
	OLED_ShowNum(2,9,calendar.w_date,2);
	OLED_ShowNum(3,1,calendar.hour,2);
	OLED_ShowNum(3,4,calendar.min,2);
	OLED_ShowNum(3,7,calendar.sec,2);
		
	KeyNumber = KEY_Scan(0);
	switch(KeyNumber)
		{
			case 1 :
				OLED_Clear();
				OLED_ShowString(1,1,"Setting...");
				OLED_ShowChar(2,2,'s');	
				for(i = ChangeTime ; i > 0 ; i --)
				{
					OLED_ShowNum(2,1,i,1);
					delay_ms(1000);
				}
				i = ChangeTime ;
				ChangeFlag = 0;
				Flag = 1;
				MUNE = 5;			
				break;
			case 2 :
				
				break;
			case 3 :

				break;
			case 4 :
				Flag = 1;
				MUNE = 2;
				break;
		}
}

/**
  * @brief  设置闹钟的功能
  * @param  
  * @retval 
  */
void PageTwo()
{	
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		OLED_ShowString(1,1,"  :  :  ");
		OLED_ShowNum(1,1,arm_hour[0],2);
		OLED_ShowNum(1,4,arm_minute[0],2);
		OLED_ShowNum(1,7,arm_second[0],2);
		OLED_ShowString(2,1,"  :  :  ");
		OLED_ShowNum(2,1,arm_hour[1],2);
		OLED_ShowNum(2,4,arm_minute[1],2);
		OLED_ShowNum(2,7,arm_second[1],2);
		
		if(a == 0){  OLED_ShowChar(4,12,' '); OLED_ShowChar(1,12,'*');}
		else if(a == 1){ OLED_ShowChar(1,12,' '); OLED_ShowChar(2,12,'*');}
		else if(a == 2){ OLED_ShowChar(2,12,' '); OLED_ShowChar(3,12,'*');}
		else if(a == 3){ OLED_ShowChar(3,12,' '); OLED_ShowChar(4,12,'*');}
		
		OLED_ShowString(3,1,"  :  :  ");
		OLED_ShowNum(3,1,arm_hour[2],2);
		OLED_ShowNum(3,4,arm_minute[2],2);
		OLED_ShowNum(3,7,arm_second[2],2);
		OLED_ShowString(4,1,"  :  :  ");
		OLED_ShowNum(4,1,arm_hour[3],2);
		OLED_ShowNum(4,4,arm_minute[3],2);
		OLED_ShowNum(4,7,arm_second[3],2);
		
		if(ClockStatus[0]){OLED_ShowString(1,13,"ON ");}
			else { OLED_ShowString(1,13,"OFF");}
		if(ClockStatus[1]){OLED_ShowString(2,13,"ON ");}
			else { OLED_ShowString(2,13,"OFF");}
		if(ClockStatus[2]){OLED_ShowString(3,13,"ON ");}
			else { OLED_ShowString(3,13,"OFF");}
		if(ClockStatus[3]){OLED_ShowString(4,13,"ON ");}
			else { OLED_ShowString(4,13,"OFF");}
	}
	
	
	KeyNumber = KEY_Scan(0);
	switch(KeyNumber)
	{
		case 1 :
			OLED_Clear();
			OLED_ShowString(1,1,"Setting...");
			OLED_ShowChar(2,2,'s');	
			for(i = ChangeTime ; i > 0 ; i --)
			{
				OLED_ShowNum(2,1,i,1);
				delay_ms(1000);
			}
			i = ChangeTime ;
			ChangeFlag = 0;
			Flag = 1;
			MUNE = 6;
			break;
		case 2 :
			a ++;
			a %= 4;
			if(a == 0){  OLED_ShowChar(4,12,' '); OLED_ShowChar(1,12,'*');}
			else if(a == 1){ OLED_ShowChar(1,12,' '); OLED_ShowChar(2,12,'*');}
			else if(a == 2){ OLED_ShowChar(2,12,' '); OLED_ShowChar(3,12,'*');}
			else if(a == 3){ OLED_ShowChar(3,12,' '); OLED_ShowChar(4,12,'*');}
			break;
		case 3 :
			switch(a)
			{
				case 0:
					ClockStatus[0] = 1 - ClockStatus[0];
					if(ClockStatus[0]){OLED_ShowString(1,13,"ON ");}
					else { OLED_ShowString(1,13,"OFF");}
					break;
				case 1:
					ClockStatus[1] = 1 - ClockStatus[1];
					if(ClockStatus[1]){OLED_ShowString(2,13,"ON ");}
					else { OLED_ShowString(2,13,"OFF");}	
					break;
				case 2:
					ClockStatus[2] = 1 - ClockStatus[2];
					if(ClockStatus[2]){OLED_ShowString(3,13,"ON ");}
					else { OLED_ShowString(3,13,"OFF");}	
					break;
				case 3:
					ClockStatus[3] = 1 - ClockStatus[3];
					if(ClockStatus[3]){OLED_ShowString(4,13,"ON ");}
					else { OLED_ShowString(4,13,"OFF");}
					break;
			}
			ClockConfig();
			break;
		case 4 :
			Flag = 1;
			MUNE = 3;			
			break;
	}
}

/**
  * @brief  计时和倒计时功能
  * @param  
  * @retval 
  */
void PageThree()
{
	if(Flag == 1)
	{
		Flag = 0;	
		Second_Save = 0;
		Minute_Save = 0;
		Second_Save2 = 0;
		Minute_Save2 = 0;
		OLED_Clear();
		OLED_ShowString(1,1,"Ticking :"); 
		OLED_ShowString(2,1,"  :  :   "); 
		OLED_ShowString(3,1,"CountDown :"); 
		OLED_ShowString(4,1,"  :  :   "); 
		OLED_ShowNum(2,1,0,2);
		OLED_ShowNum(2,4,0,2);
		OLED_ShowNum(2,7,0,3);
		if(Choose == 0){OLED_ShowChar(2,15,'*');OLED_ShowChar(4,15,' ');}
			else if(Choose == 1){OLED_ShowChar(4,15,'*');OLED_ShowChar(2,15,' ');}
		OLED_ShowNum(4,1,TimerDownMinute,2);
		OLED_ShowNum(4,4,TimerDownSecond,2);
		OLED_ShowNum(4,7,0,3);
	}
	
	if(Choose == 0)
	{
		if(FirstFlag == 2)
		{
			Timer_second = Second_Save;
			Timer_minute = Minute_Save;
		}

		if(BeginFlag == 1)
		{
			if(FirstFlag == 1)
			{
				FirstFlag = 2;
				Timer_second = 0;
				Timer_minute = 0;
				Second_Save = 0;
				Minute_Save = 0;
			}
			while (1)
			{
				calendar.msec=(32767-RTC_GetDivider())*1000/32767;
				OLED_ShowNum(2,7,calendar.msec,3);	
				OLED_ShowNum(2,4,Timer_second,2);
				if(Timer_second >= 60){Timer_second = 0; Timer_minute ++;}
				if(Timer_minute >= 60){Timer_minute = 0;}
				OLED_ShowNum(2,1,Timer_minute,2);	
				if(KEY_Scan(0) == 2){BeginFlag = 0; Second_Save = Timer_second; Minute_Save = Timer_minute;break;}
			}
		}
	}

	if(Choose == 1)
	{
		if(BeginFlag == 1)
		{
			if(FirstFlag == 0)
			{
				OLED_ShowNum(4,1,TimerDownMinute,2);
				OLED_ShowNum(4,4,TimerDownSecond,2);
				OLED_ShowNum(4,7,0,3);
				while(1)
				
				{
					if(KEY_Scan(0) == 1){FirstFlag = 1;break;}
				}
			}
			if(FirstFlag == 2)
			{
				Timer_down = Second_Save2;
				Timer_minute2 = Minute_Save2;
			}
			if(FirstFlag == 1)
			{
				FirstFlag = 2;
				Timer_down = TimerDownSecond;
				Timer_minute2 = TimerDownMinute;
				Second_Save2 = 0;
				Minute_Save2 = 0;
			}
			while (1)
			{
				if(Timer_down < 0){Timer_down = 59;Timer_minute2 --;}
				if(Timer_minute2 < 0)
				{
					BeginFlag = 0;
					FirstFlag = 0;
					Timer_minute2 = TimerDownMinute;
					OLED_ShowNum(4,1,0,2);
					OLED_ShowNum(4,4,0,2);
					OLED_ShowNum(4,7,0,3);
					break;
				}
				OLED_ShowNum(4,1,Timer_minute2,2);	
				OLED_ShowNum(4,4,Timer_down,2);
				calendar.msec=999 - (32767-RTC_GetDivider())*1000/32767;
				OLED_ShowNum(4,7,calendar.msec,3);

				if(KEY_Scan(0) == 2){BeginFlag = 0 ; Minute_Save2 = Timer_minute2; Second_Save2 = Timer_down; break;}
			}
		}
	}

	KeyNumber = KEY_Scan(0);
	switch(KeyNumber)
	{
		case 1 :
			BeginFlag = 1;
			break;
		case 2 :
			Choose = 1 - Choose;
			if(Choose == 0){OLED_ShowChar(2,15,'*');OLED_ShowChar(4,15,' ');}
				else if(Choose == 1){OLED_ShowChar(4,15,'*');OLED_ShowChar(2,15,' ');}
			break;
		case 3 :
			if(Choose == 0)
			{
				OLED_ShowNum(2,1,0,2);
				OLED_ShowNum(2,4,0,2);
				OLED_ShowNum(2,7,0,3);
				FirstFlag = 1;
			}
			if(Choose == 1)
			{
				OLED_ShowNum(4,1,TimerDownMinute,2);
				OLED_ShowNum(4,4,TimerDownSecond,2);
				OLED_ShowNum(4,7,0,3);
				FirstFlag = 1;
			}
			break;
		case 4 :
			Flag = 1;
			MUNE = 4;
			break;
	}
}


/**
  * @brief  外接一个光敏传感器，通过光线强弱控制LED亮灭
  * @param  
  * @retval 
  */
void PageFour()
{
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		OLED_ShowString(1, 1, "Light: ");
		OLED_ShowString(1, 12, "OFF");
	}
	
	if(AD_Value[0] > 2000){RED_LED = 1;OLED_ShowString(1, 12, "ON  ");}
	else{RED_LED  = 0;OLED_ShowString(1, 12, "OFF");}
	
// 	OLED_ShowNum(1, 5, AD_Value[0], 4);
//	OLED_ShowNum(2, 5, AD_Value[1], 4);
//	OLED_ShowNum(3, 5, AD_Value[2], 4);
//	OLED_ShowNum(4, 5, AD_Value[3], 4); 
//	delay_ms(100);
	
	KeyNumber = KEY_Scan(0);
	switch(KeyNumber)
	{
		case 1 :
			
			break;
		case 2 :
			
			break;
		case 3 :
	
			break;
		case 4 :
			Flag = 1;
			MUNE = 1;		
			break;
	}
}


/**
  * @brief  调节时间
  * @param  
  * @retval 
  */
void PageFive()
{	
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		OLED_ShowString(1,1,"Modify:");
		OLED_ShowString(2,1,"    -  -  ");
		OLED_ShowString(3,1,"  :  :  ");
		year = calendar.w_year;month = calendar.w_month;day = calendar.w_date;
		hour = calendar.hour;minute = calendar.min;second = calendar.sec;
	}
	
	if(SelectedFlag && ChangeFlag == 0)
	{
		switch(j)
		{
			case 1:
				OLED_ShowString(2,1,"    ");
				break;
			case 2:
				OLED_ShowString(2,6,"  ");
				break;
			case 3:
				OLED_ShowString(2,9,"  ");
				break;
			case 4:
				OLED_ShowString(3,1,"  ");
				break;
			case 5:
				OLED_ShowString(3,4,"  ");
				break;
			case 6:
				OLED_ShowString(3,7,"  ");
				break;	
		}
	}else
	{
		OLED_ShowNum(2,1,year,4);
		OLED_ShowNum(2,6,month,2);
		OLED_ShowNum(2,9,day,2);
		OLED_ShowNum(3,1,hour,2);
		OLED_ShowNum(3,4,minute,2);
		OLED_ShowNum(3,7,second,2);			
	}
	
	KeyNumber = KEY_Scan(1);
	if(KeyNumber != 0){ ChangeFlag = 1;}
	switch(KeyNumber)
	{
		case 1 :
			OLED_Clear();
			BUZZER_BEEP1();
			if(RTC_Set(year,month,day,hour,minute,second) == 0){OLED_ShowString(1,1,"Success!!!");delay_ms(3000);}
			else{OLED_ShowString(1,1,"Faliure!!!");delay_ms(3000);}
			j = 1;
			Flag = 1;
			MUNE = 1;
			break;
		case 2 :
			switch(j)
			{
				case 1:
					year ++;if(year > 2099 ) year = 1999;
					break;
				case 2:
					month ++;
					if(month > 12) month = 1;
					break;
				case 3:
					day ++;
					if((month == 1 || month == 3 || month == 5 ||month == 7 ||month == 8 ||month == 10 ||month == 12)&&day > 31) day = 1;
					if((month == 4 || month == 6 ||month == 9 ||month == 11)&&day > 30) day = 1;
					if(Is_Leap_Year(year) && month == 2 && day > 29) day = 1;
					if(!Is_Leap_Year(year) && month == 2 && day > 28) day = 1;
					break;
				case 4:
					hour ++;
					if(hour > 23) hour = 0;
					break;
				case 5:
					minute ++;
					if(minute > 59) minute = 0;
					break;
				case 6:
					second ++;
					if(second > 59) second = 0;
					break;	
			}
			delay_ms(120);
			break;
		case 3 :
			switch(j)
			{
				case 1:
					year --;
					if(year < 1970) year = 1999;
					break;
				case 2:
					month --;
					if(month < 1) month = 12;
					break;
				case 3:
					day --;
					if((month == 1 || month == 3 || month == 5 ||month == 7 ||month == 8 ||month == 10 ||month == 12)&&day < 1) day = 31;
					if((month == 4 || month == 6 ||month == 9 ||month == 11)&&day < 1) day = 30;
					if(Is_Leap_Year(year) && month == 2 && day < 1) day = 29;
					if(!Is_Leap_Year(year) && month == 2 && day < 1) day = 28;
					break;
				case 4:
					hour --;
					if(hour < 0) hour = 23;
					break;
				case 5:
					minute --;
					if(minute < 0) minute = 59;
					break;
				case 6:
					second --;
					if(second < 0) second = 59;
					break;	
			}
			delay_ms(120);
			break;
		case 4 :
			j++;
			if(j > 6) j = 1;
			ChangeFlag = 0;
			delay_ms(200);
			break;
	}
}


/**
  * @brief  调节闹钟定时
  * @param  
  * @retval 
  */
void PageSix()
{
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		switch(a)
		{
			case 0:
				OLED_ShowString(1,1,"Clock1 Set:");
				break;
			case 1:
				OLED_ShowString(1,1,"Clock2 Set:");
				break;
			case 2:
				OLED_ShowString(1,1,"Clock3 Set:");
				break;
			case 3:
				OLED_ShowString(1,1,"Clock4 Set:");
				break;
		}
		OLED_ShowString(2,1,"  :  :  ");
	}
	
	if(SelectedFlag && ChangeFlag == 0)
	{
		switch(k)
		{
			case 1:
				OLED_ShowString(2,1,"  ");
				break;
			case 2:
				OLED_ShowString(2,4,"  ");
				break;
			case 3:
				OLED_ShowString(2,7,"  ");
				break;
		}
	}else
	{
		OLED_ShowNum(2,1,arm_hour[a],2);
		OLED_ShowNum(2,4,arm_minute[a],2);
		OLED_ShowNum(2,7,arm_second[a],2);	
	}

	KeyNumber = KEY_Scan(1);
	if(KeyNumber != 0){ ChangeFlag = 1;}
	switch(KeyNumber)
	{
		case 1 :
			OLED_Clear();
			ClockConfig();
			BUZZER_BEEP1();
//			if(RTC_Alarm_Set(calendar.w_year,calendar.w_month,calendar.w_date,arm_hour[a],arm_minute[a],arm_second[a]) == 0){OLED_ShowString(1,1,"Success!!!");delay_ms(3000);}
//			else{OLED_ShowString(1,1,"Faliure!!!");delay_ms(3000);}
			k =  1;
			Flag = 1;
			MUNE = 2;
			OLED_ShowString(1,1,"Success!!!");delay_ms(3000);
			break;
		case 2 :
			switch(k)
			{
				case 1:
					arm_hour[a] ++;
					if(arm_hour[a] > 23) arm_hour[a] = 0;
					break;
				case 2:
					arm_minute[a] ++;
					if(arm_minute[a] > 59) arm_minute[a] = 0;
					break;
				case 3:
					arm_second[a] ++;
					if(arm_second[a] > 59) arm_second[a] = 0;
					break;	
			}
			delay_ms(120);
			break;
		case 3 :
			switch(k)
			{
				case 1:
					arm_hour[a] --;
					if(arm_hour[a] < 0) arm_hour[a] = 23;
					break;
				case 2:
					arm_minute[a] --;
					if(arm_minute[a] < 0) arm_minute[a] = 59;
					break;
				case 3:
					arm_second[a] --;
					if(arm_second[a] < 0) arm_second[a] = 59;
					break;	
			}
			delay_ms(120);
			break;
		case 4 :
			k++;
			if(k > 3) k = 1;
			ChangeFlag = 0;
			delay_ms(200);
			break;
	}	
}


/**
  * @brief  闹钟时间到的显示画面
  * @param  
  * @retval 
  */
void PageSeven()
{
	if(Flag == 1)
	{
		Flag = 0;
		OLED_Clear();
		OLED_ShowString(1,1,"ClockTime");
		OLED_ShowString(2,1,"!!! Come ON !!!");	
	}
	
	if(SecondFlag == 0)
	{
		BUZZER = 0;
		while(SecondFlag == 0);
		BUZZER = 1;
	}
	
	KeyNumber = KEY_Scan(0);
	switch(KeyNumber)
	{
		case 1 :
			
			break;
		case 2 :
			
			break;
		case 3 :
	
			break;
		case 4 :
			BUZZER = 1;
			Flag = 1;
			MUNE = 1;	
			//MUNE = 66;	
			break;
	}
}

/**
 *****************brief：按键扫描程序****************************************
		KeyNumber = KEY_Scan(0);
		switch(KeyNumber)
		{
			case 1 :
				OLED_ShowString(2,1,"KEY1");
				break;
			case 2 :
				OLED_ShowString(2,1,"KEY2");
				break;
			case 3 :
				OLED_ShowString(2,1,"KEY3");
				break;
			case 4 :
				OLED_ShowString(2,1,"KEY4");
				break;
		}
 ****************************************************************************
 ***/


