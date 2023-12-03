#ifndef __USR_SLEEP_H
#define __USR_SLEEP_H

#include "usr_general.h"

#define _USR_SLEEP_SYSTEM_TIMEOUT_TIME             (uint32_t)300000

typedef enum
{
    noWakeup,
    wakeupFromRtc,
    wakeupFromCoverAlarm,
    wakeupFromDisableStatus,
    wakeupFromFireAlarmStatus,
#ifdef _accModuleCompile
    wakeupFromAccInterrupt,
#endif   
}EWakeuptype;

// typedef enum 
// {
//     disable,
//     enable
// }Etimercontrol;

void UsrSleepGeneral(void);
void UsrSleepEnterSubSleep(uint32_t f_time);
void UsrSleepAgain(void);
void UsrSleepAdcPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);
void UsrSleepGpioOutPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);
void UsrSleepGpioInputPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup);

extern bool g_sleepFlag;

#endif

