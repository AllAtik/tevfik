#include "usr_lib_battery.h"

#define _io static
#define _iov static volatile

_io S_BATTERY_DATA m_sBatteryParameters;
bool m_batterypowerokflag = false;

#define _BATTERY_CONTROL_POWER(x)      HAL_GPIO_WritePin(m_sBatteryParameters.pbatteryPort, m_sBatteryParameters.pbatteryPin, (GPIO_PinState) x)

_io void UsrSleepSensorGpioPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);
_io void UsrWakeUpSensorGpioPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);

void UL_BatteryPeripheral(S_BATTERY_DATA *pbattery, EBatteryControl f_eControl)
{
    m_sBatteryParameters = *pbattery;   
    if(f_eControl == enableBatteryPeripheral)
    {
        m_batterypowerokflag = true;
        UsrWakeUpSensorGpioPins(m_sBatteryParameters.pbatteryPort, m_sBatteryParameters.pbatteryPin, (GPIO_PinState)m_sBatteryParameters.batterypowerstatus);
    }
    else
    {
        m_batterypowerokflag = false; 
        UsrSleepSensorGpioPins(m_sBatteryParameters.pbatteryPort, m_sBatteryParameters.pbatteryPin, (GPIO_PinState)!m_sBatteryParameters.batterypowerstatus);
    }
}


bool UL_BatteryInitial(S_BATTERY_DATA *f_pParameter)
{
  m_sBatteryParameters = *f_pParameter;
  _BATTERY_CONTROL_POWER((GPIO_PinState)m_sBatteryParameters.batterypowerstatus);
  m_batterypowerokflag = true;
  return true;
}


_io void UsrSleepSensorGpioPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    m_batterypowerokflag = false;
    if(m_batterypowerokflag)
    {
        HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    }
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}


_io void UsrWakeUpSensorGpioPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{ 
    GPIO_InitTypeDef GPIO_InitStruct;
    m_batterypowerokflag = true;
    if(m_batterypowerokflag)
    {
        HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    }
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);  
}


