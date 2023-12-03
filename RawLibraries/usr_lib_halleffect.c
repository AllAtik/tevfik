#include "usr_lib_halleffect.h"

#define _io static
#define _iov static volatile

#define HALL_EFFECT_DELAY_MS    50

_io S_HALLEFFECT_PARAMETERS m_sHalleffectParameter; 

//_io void UsrSleepHallEffectSensor(GPIO_TypeDef *f_portGpio, uint16_t f_pinGpio);
//_io void UsrSleepHallEffectSensor(GPIO_TypeDef *f_portGpio, uint16_t f_pinGpio, GPIO_PinState f_ePinstate);        

void UL_HalleffectPeripheral(S_HALLEFFECT_PARAMETERS *f_pParameter ,EHalleffectControl f_eControl)
{
    m_sHalleffectParameter = *f_pParameter;
    if(f_eControl == enableHalleffect)
    {
        UL_HalleffectInitial(&m_sHalleffectParameter);
    }
    else
    {
        _BATTERY_COVER_HALL_POWER(0);
        _TOP_COVER_HALL_POWER(0);
    }
}

bool UL_HalleffectInitial(S_HALLEFFECT_PARAMETERS *f_pParameter)
{
    m_sHalleffectParameter = *f_pParameter;
    HAL_Delay(HALL_EFFECT_DELAY_MS);
    _BATTERY_COVER_HALL_POWER(1);
    _TOP_COVER_HALL_POWER(1);
    /*
    HAL_GPIO_WritePin(m_sHalleffectParameter.batteryhalleffectpowerport, m_sHalleffectParameter.batteryhalleffectpowerpin, (GPIO_PinState) m_sHalleffectParameter.batteryhalleffectstatus);
    HAL_GPIO_WritePin(m_sHalleffectParameter.tophalleffectpowerport,     m_sHalleffectParameter.tophalleffectpowerpin,     (GPIO_PinState) m_sHalleffectParameter.tophalleffectStatus);
    */
    return true;
}

_io void UsrSleepHallEffectSensor(GPIO_TypeDef *f_portGpio, uint16_t f_pinGpio)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin  = f_pinGpio;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(f_portGpio, &GPIO_InitStruct);
}


_io void UsrWakeUpHallEffectSensor(GPIO_TypeDef *f_portGpio, uint16_t f_pinGpio, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    HAL_GPIO_WritePin(f_portGpio, f_pinGpio, f_ePinstate);
    GPIO_InitStruct.Pin  = f_pinGpio;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(f_portGpio, &GPIO_InitStruct);
}
