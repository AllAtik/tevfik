#include "usr_lib_adc.h"

#define _io static
#define _iov static volatile
#define _adc_debug

_io S_ADC_PARAMETERS m_sAdcParameters;
_io S_ADC_RAW_PARAMETERS m_sAdcRawParameters;

_io void SleepAdcPinsProc(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);
_io void SleepGpioOutPinsProc(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);
_io void WakeUpAdcGpioPinsProc(S_ADC_PARAMETERS *f_pAdc);
_io void SleepAdcGpioPinsProc(S_ADC_PARAMETERS *f_pAdc);

bool m_adcFinishedFlg = false;
bool m_adcInitialFlag = false;
bool m_adcStartGetValueFlag = false;

extern bool g_UsrSystemAccelometerChipOkFlag;

_io uint8_t m_adcSampleCounter = 0;

uint16_t adc_values_dma[ADC_CHANNEL_COUNT];
_iov uint16_t m_adcRawValueBuf[ADC_CHANNEL_COUNT][SAMPLE_COUNT];

bool UL_AdcInitial(S_ADC_PARAMETERS *f_pParameter)
{
    m_sAdcParameters = *f_pParameter;
    m_adcInitialFlag = true; // Bu flag'ı UL_Adc_Peripheral'da kullandık
    m_adcStartGetValueFlag = true;

    // HAL_ADC_DeInit(m_sAdcParameters.pAdcforDma);
    // _USR_ADC_INIT_FUNC;

    // ADC1->CR &= (uint32_t)0xfffffffe;          // Önce adc disable
    // ADC1->CR |= (uint32_t)0x80000000;          // adc kalibrasyon enable
    // while(ADC1->CR & ((uint32_t)0x80000000))   // calibrasyon bekleme
    // ;
    // ADC1->CR |= (uint32_t)0x00000001;          // sonra adc enable

    return true;
}

void UL_AdcPeripheral(S_ADC_PARAMETERS *f_pParameter, EAdcControl f_eControl)
{
    m_sAdcParameters = *f_pParameter;
    if (f_eControl == disableAdcPeripheral)
    {
        HAL_ADC_DeInit(&_USR_ADC_CHANNEL);
        if (g_UsrSystemAccelometerChipOkFlag)
            SleepAdcPinsProc(VBAT_ADC_HIGH_GPIO_Port, VBAT_ADC_HIGH_Pin, GPIO_PIN_SET);
        else
            SleepGpioOutPinsProc(VBAT_ADC_HIGH_GPIO_Port, VBAT_ADC_HIGH_Pin, GPIO_PIN_SET);
        SleepAdcPinsProc(VBAT_ADC_LOW_GPIO_Port, VBAT_ADC_LOW_Pin, GPIO_PIN_SET);
        SleepAdcPinsProc(TEMP_ADC_GPIO_Port, TEMP_ADC_Pin, GPIO_PIN_RESET);
        // m_adc_step = 0
    }
    else
    {
        // HAL_ADC_DeInit(&_USR_ADC_CHANNEL);
        _USR_ADC_INIT_FUNC;
        // UL_AdcInitial(&m_sAdcParameters);
        ADC1->CR &= (uint32_t)0xfffffffe;         // Önce adc disable
        ADC1->CR |= (uint32_t)0x80000000;         // adc kalibrasyon enable
        while (ADC1->CR & ((uint32_t)0x80000000)) // calibrasyon bekleme
            ;
        ADC1->CR |= (uint32_t)0x00000001; // sonra adc enable
    }
}

// blokeli olarak adc değerleri burada toplanancak
bool UL_AdcGetValues(S_ADC_PARAMETERS *f_pParameter, S_ADC_RAW_PARAMETERS *f_pData)
{
    if (f_pData != NULL && f_pParameter != NULL)
    {
        *f_pParameter = m_sAdcParameters;
        *f_pData = m_sAdcRawParameters;
    }

    m_adcFinishedFlg = false;
    if ((HAL_OK == HAL_ADC_Start_DMA(m_sAdcParameters.pAdcforDma, (uint32_t *)adc_values_dma, ADC_CHANNEL_COUNT)))
    {
        while (!m_adcFinishedFlg)
            ;

        uint16_t temp = 0;
        uint32_t batteryhigh = 0;
        uint32_t batterylow = 0;
        uint32_t vreftemp = 0;

        if (!m_adcStartGetValueFlag)
        {
            return false;
        }
        else
        {
            for (uint16_t m = 0; m < ADC_CHANNEL_COUNT; m++)
            {
                for (uint8_t i = 0; i < SAMPLE_COUNT; i++)
                {
                    temp += m_adcRawValueBuf[TEMP_ADC_CHANNEL][i];
                    batteryhigh += m_adcRawValueBuf[VBAT_ADC_HIGH_CHANNEL][i];
                    batterylow += m_adcRawValueBuf[VBAT_ADC_LOW_CHANNEL][i];
                    vreftemp += m_adcRawValueBuf[VREF_ADC_CHANNEL][i];
                }

                temp /= SAMPLE_COUNT;
                batteryhigh /= SAMPLE_COUNT;
                batterylow /= SAMPLE_COUNT;
                vreftemp /= SAMPLE_COUNT;
            }

            f_pData->rawtempvalue = temp;
            f_pData->rawbatteryhighvalue = batteryhigh;
            f_pData->rawbatterylowvalue = batterylow;
            f_pData->rawvreftempvalue = vreftemp;

            return true;
        }
    }
    else
    {
        HAL_ADC_Stop_DMA(m_sAdcParameters.pAdcforDma);
        // UL_AdcPeripheral(&m_sAdcParameters, disableAdcPeripheral);
        return false;
    }
}

void UL_AdcCallback(S_ADC_PARAMETERS *f_pParameter)
{
    *f_pParameter = m_sAdcParameters;
    //// Take the values from dma
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++)
    {
        m_adcRawValueBuf[i][m_adcSampleCounter] = adc_values_dma[i];
    }
    //// Increase the sample count
    m_adcSampleCounter++;
    //// Check adc sampling that is enough
    if (m_adcSampleCounter >= SAMPLE_COUNT)
    {
        //// Close the dma interrupt reading
        HAL_ADC_Stop_DMA(m_sAdcParameters.pAdcforDma);
        //// Clear sampling count
        m_adcSampleCounter = 0;
        //// Show the system that the reading finished
        m_adcFinishedFlg = true;
    }
}

_io void SleepAdcPinsProc(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}

_io void SleepGpioOutPinsProc(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}

_io void SleepAdcGpioPinsProc(S_ADC_PARAMETERS *f_pAdc)
{
    m_sAdcParameters = *f_pAdc;
    if (!m_adcInitialFlag)
    {
        HAL_ADC_Stop(m_sAdcParameters.pAdcforDma);
    }
    m_adcStartGetValueFlag = false;
}

_io void WakeUpAdcGpioPinsProc(S_ADC_PARAMETERS *f_pAdc)
{
    m_sAdcParameters = *f_pAdc;
    if (m_adcInitialFlag)
    {
        HAL_ADC_Start_DMA(m_sAdcParameters.pAdcforDma, (uint32_t *)adc_values_dma, ADC_CHANNEL_COUNT);
    }
    m_adcStartGetValueFlag = true;
}
