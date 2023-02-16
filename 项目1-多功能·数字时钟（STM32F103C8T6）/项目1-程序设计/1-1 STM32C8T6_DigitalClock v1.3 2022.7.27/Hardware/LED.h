#ifndef __LED_H__
#define __LED_H__
#include "sys.h"

#define LED PCout(13)
#define RED_LED PBout(1)
void LED_Init(void);

#endif

