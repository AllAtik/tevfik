#include "usr_general.h"

extern S_GSM_PARAMETERS g_sUsrSystemGsmParameters;
extern S_GSM_MODULE_INFO g_sUsrSystemGsmModuleInfo;
extern S_GSM_MQTT_CONNECTION_PARAMETERS g_sUsrSystemGsmMqttInitialParameters;
// extern S_GSM_FTP                         g_sUsrSystemGsmFtpParameters;

extern S_ADC_PARAMETERS g_sUsrSystemAdcParameters;
extern S_ADC_RAW_PARAMETERS g_sUsrSystemAdcRawParameters;

extern S_HALLEFFECT_PARAMETERS g_sUsrSystemHalleffectParameters;

extern S_ULTRASONIC_SENSOR_PARAMETERS g_sUsrSystemUltrasonicParameters;

_io void BaseTimerControlProc(bool f_eControl);
_io void ClearFlagsProc(void);
_io bool FireAlarmProc(void);
_io bool CoverAlarmProc(void);

bool g_sleepFlag = false;


extern bool g_UsrSystemGsmErrorFlag;

#ifdef _accModuleCompile
extern bool g_UsrSystemAccelometerWakeUpFlag; // harbiden uyandı flag'i
extern bool g_UsrSytemAccelometerInterruptDetectedFlag;
#endif

extern bool g_UsrSystemTemperatureSensorErrorFlag;
extern bool g_UsrSystemDistanceSensorErrorFlag;
extern bool g_UsrSystemWakeUpFromRtcCheckDataFlag;
extern bool g_UsrSystemDeviceStatusCheckOkFlag;
extern bool g_UsrSystemDeviceStatusCheckMissionFlag;
extern bool g_UsrSystemWakeUpRtcCheckDataFlag;
extern bool g_UsrSystemGsmModuleMqttInitialFlag;
extern uint8_t g_UsrSystemCoverLastStatus;
uint32_t g_UsrSleepDeviceStatusLastCheck = 0;

/// interrupt geldikten sonra, (accInterruptTime+controlPeriod_Min). saniyeden (accInterruptTime+controlPeriod_Max). saniye'ye kadar yine interrupt gelirse harbiden sallandi flag'i 1 olur.
#ifdef _accModuleCompile
uint8_t controlPeriod_Min = 5;
uint8_t controlPeriod_Max = 10;
uint8_t controlInterval = 0;
uint32_t accInterruptTime = 0;
uint32_t lastAccInterruptTime = 0;
uint32_t controlPeriod_Min_Time = 0;
#endif

#define _fire_alarm_log_count 6
_io uint8_t m_lastHallAlarm;

void UsrSleepGeneral(void)
{
    if (g_sleepFlag)
    {
        bool alarmCheckFlg = false;
        g_sleepFlag = false;
#ifdef _accModuleCompile
        g_UsrSystemAccelometerWakeUpFlag = false;
#endif
        m_lastHallAlarm = g_sAllSensorValues.halleffectalarmstatus;
        int lastTs = UL_RtcGetTs();
    main_sleep:;
        __logse("Device enter sleep %d", lastTs);
        MX_DMA_Deinit();
        BaseTimerControlProc(false);
    sleep_label:;
        HAL_SuspendTick();
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
        PWR->CR |= 0x00000600;
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        g_dailyResetTimer += 10000;

        UsrSystemWatchdogRefresh();
        int ts = UL_RtcGetTs();
        if (g_sNvsDeviceInfo.deviceStatus)
        {
            if ((ts - g_dataSendTs) >= g_sNvsDeviceInfo.sendingDataInterval)
                goto wakeup_label;
            else if ((ts - g_sensorReadTs) >= g_sNvsDeviceInfo.sensorWakeUpTime)
                goto wakeup_label;
            else if ((ts - lastTs) > 30)
            {
                lastTs = ts;
                alarmCheckFlg = true;
                goto wakeup_label;
            }
            else if(g_dailyResetTimer > _USR_SYSTEM_DAILY_RESET_TIME)
                HAL_NVIC_SystemReset();
            else
                goto sleep_label;
        }
        else
        {
            if ((ts - lastTs) >= g_sNvsDeviceInfo.deviceStatusCheckTime)
                goto wakeup_label;
            else
                goto sleep_label;
        }

    wakeup_label:;
        HAL_ResumeTick();
        SystemClock_Config();
        MX_GPIO_Init();
        MX_DMA_Init();
        BaseTimerControlProc(true);
        if (alarmCheckFlg)
        {
            alarmCheckFlg = false;
            UsrSensorGetAdcAndHalleffectValues();
            if (!FireAlarmProc() && !CoverAlarmProc())
                goto main_sleep;
        }

        g_sensorsReadingFlag = true;
    }
}

void UsrSleepEnterSubSleep(uint32_t f_time)
{
    UsrSystemWatchdogRefresh();
    MX_DMA_Deinit();
    BaseTimerControlProc(false);
    int waitCount = 0;
    while (waitCount <= f_time)
    {
        HAL_SuspendTick();
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
        PWR->CR |= 0x00000600;
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        UsrSystemWatchdogRefresh();
        waitCount++;
    }
    HAL_ResumeTick();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    BaseTimerControlProc(true);
}

void _UsrSleepGeneral(void)
{
    if (g_sleepFlag)
    {
        g_sleepFlag = false;
        g_UsrSystemGsmErrorFlag = false; // sonra
#ifdef _accModuleCompile
        g_UsrSystemAccelometerWakeUpFlag = false;
#endif

        ClearFlagsProc();
        MX_DMA_Deinit();
        /* Gsm ve Mqtt kapatma anı */
        UL_GsmModuleMqttPublishTopic("topic1eren", "GsmClosed Sleep Func", 0, 0);
        UL_GsmModuleMqttClosed();
        UL_GsmModulePeripheral(disableGsmPeripheral);
        // UL_GsmModuleHardwareReset();
        // BaseTimerControlProc(disable);
        UL_AdcPeripheral(&g_sUsrSystemAdcParameters, disableAdcPeripheral);
        UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, disableUltrasonicSensor);
        UL_LedPeripheral(disableLedPeripheral);

        // enter sleep step
    sleep_label:;
        int lastTs = UL_RtcGetTs();
        __logsw("Su Saatte: %d UYUDUM!\n", lastTs); /// gecici
// UsrSystemWatchdogRefresh();                         // Clean watchdog
#ifdef _accModuleCompile
        g_UsrSytemAccelometerInterruptDetectedFlag = false; /// interruptin (uyandirmanin) cihaz zaten uyanıkken gelmesi durumu icin !!!!!1 onemli !!!!!
#endif
        HAL_SuspendTick();                                                  /// Tick Counter durduruldu
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);                                  /// PWR_CSR->WakeUpFlag 0 yapildi, stop mode tavsiyesi
        PWR->CR |= 0x00000600;                                              /// PWR_CR -> FWU ve ULP "BİRLİKTE" 1 yapildi !!!, böylece uyurken Vrefint'te söndürüldü (Ultra Low Power'a gecildi), uyandırken ise bu geriimin geri gelmlesi beklenmesin dndi.
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); /// direkt regulator sonduruldu, wait for interrupt olarak uyutuldu

        // uyaninca HAL_RTCEx_WakeUpTimerEventCallback'da SysTick devam ettiriliyor
        // UsrSystemWatchdogRefresh();                 // Clean watchdog

        int currentTime = UL_RtcGetTs();
#ifdef _accModuleCompile

        if (g_UsrSytemAccelometerInterruptDetectedFlag) /// herhangi bir sarsilma
        {
            g_UsrSytemAccelometerInterruptDetectedFlag = false;
            HAL_ResumeTick();     // STM32Cube HAL zamanlayıcısını tekrar etkinleştirmek (systick timer)
            SystemClock_Config(); // sistem saatini yapılandırmak ve başlatmak için kullanılır

            if (!g_sNvsDeviceInfo.deviceStatus)
            {
                __logsw("Device wake up from acc for disable status goto sleep again");
                goto sleep_label;
            }
            else
            {
                controlInterval = (controlPeriod_Max - controlPeriod_Min); //  CONTROL_nPRIV_Pos
                accInterruptTime = UL_RtcGetTs();
                if ((accInterruptTime - lastAccInterruptTime) > controlInterval)
                {
                    controlPeriod_Min_Time = (accInterruptTime + controlPeriod_Min);
                }
                else
                {
                    // degismesine gerek yok gibi suan
                }

                if (accInterruptTime > controlPeriod_Min_Time)
                {
                    lastAccInterruptTime = accInterruptTime;
                    /// Bu Bir 2. tetiklenmeydi ! devam edip uyansin

                    g_UsrSystemAccelometerWakeUpFlag = true; // Harbiden uyandı
                    g_sAllSensorValues.eWakeuptype = wakeupFromRtc;
                    __logsi("Su Saatte: %d Kucuk Sarsilmadan Sonraki 5-10sn icinde yine Sarsildm ! Bu sebeple artik geri uyumak yerine rutin gorevlerim icin uyaniyorum !\n", accInterruptTime); /// gecici         /// Degistirdim
                    goto wakeup_step;
                }
                else
                {
                    /// 2. tetikleme durumu yok, geri uyutacaz arkadasi
                    lastAccInterruptTime = accInterruptTime;
                    __logsi("Su Saatte: %d Bi Kucuk Sardildim, Geri Uyuyorum !\n", lastAccInterruptTime); /// gecici        /// Degistirdim
                    goto sleep_label;
                }
            }
        }
#endif

    wakeup_step:;
        UsrSystemWatchdogRefresh(); // uyanır uyanmaz WD reset
        SystemClock_Config();
        MX_DMA_Init();
        UL_AdcPeripheral(&g_sUsrSystemAdcParameters, enableAdcPeripheral);
        UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, enableUltrasonicSensor);
        UsrSensorHallEffectGivePower();
        /* gsm ve mqtt uyandırma fonksiyonları*/
        UsrSystemGsmPeripheralInitial();
        if (UL_GsmModuleCheck())
        {
            g_UsrSystemGsmErrorFlag = false;
        }
        else
        {
            g_UsrSystemGsmErrorFlag = true;
        }

        if (UL_GsmModuleGetInfo(&g_sUsrSystemGsmModuleInfo))
        {
            __logsi("Gsm : %s", g_sUsrSystemGsmModuleInfo.iccidBuf);
            __logsi("Gsm : %s", g_sUsrSystemGsmModuleInfo.imeiBuf);
            __logsi("Gsm : %s", g_sUsrSystemGsmModuleInfo.moduleInfoBuf);
            __logsi("Gsm : %d", g_sUsrSystemGsmModuleInfo.signal);

            if (UL_GsmModuleMqttInitial((const S_GSM_MQTT_CONNECTION_PARAMETERS *)&g_sUsrSystemGsmMqttInitialParameters))
            {
                g_UsrSystemGsmModuleMqttInitialFlag = true;
                if (!UL_GsmModuleMqttSubcribeTopic("topic1eren", 0))
                {
                    __logse("Subcribe error");
                }
                UL_GsmModuleMqttPublishTopic("topic1eren", "GsmOpened Sleep Func", 0, 0);
            }
            else
            {
                __logse("Mqtt error");
                g_UsrSystemGsmModuleMqttInitialFlag = false;
            }
        }
        HAL_Delay(5);

        lastTs = UL_RtcGetTs();
        __logsi("Su Saatte: %d  UYANDiM @@@@@@@@@@@@ \n", lastTs); // gecici

        // deviceStatuscheckTime kontrolü
        uint32_t currentTimeDeviceControl = UL_RtcGetTs();

        // check time kontrolü ve gsm den data alma ve deviceStatus Check zamanı
        if ((currentTimeDeviceControl - g_UsrSleepDeviceStatusLastCheck) >= (g_sNvsDeviceInfo.deviceStatusCheckTime - 1))
        {
            g_UsrSleepDeviceStatusLastCheck = currentTimeDeviceControl;
            // g_sAllSensorValues.eWakeuptype = wakeupFromCoverAlarm;
            g_UsrSystemDeviceStatusCheckMissionFlag = true; // 5 dk da bir deviceStatus un 1 olduğuna baktı, 1 ise gsm'ye gidiyor
            __logse("DeviceStatusCheck Mission basladi, Diger Gorevler askiya alindi !!");
        }

        if ((currentTime) >= (g_sNvsDeviceInfo.dailyReset))
        {
            HAL_NVIC_SystemReset();
        }
    }
}

void UsrSleepAgain(void)
{
    g_sleepFlag = 1;

    int XX = UL_RtcGetTs();
    __logsi("Su Saatte: %d tekrar uyumama izin verildi\n", XX); /// gecici
}

_io void ClearFlagsProc(void)
{
    // wake up type
    g_sAllSensorValues.eWakeuptype = noWakeup;

    // distance data ok flag
    g_sAllSensorValues.distancedataokflag = false;
    // adc data ok flag
    g_sAllSensorValues.adcdataokflag = false;
    // cover check flag
    // g_sUsrSleepSensorAllValues.covercheckflag = false;

    g_sAllSensorValues.senddataflag = false;
    // rtc uyandirdi
    g_UsrSystemWakeUpRtcCheckDataFlag = false;
}

_io bool FireAlarmProc(void)
{
    _io uint8_t _temperatureArrayCnt = 0;
    _io float _temperatureBuf[_fire_alarm_log_count] = {0};

    if (_temperatureArrayCnt < _fire_alarm_log_count)
        _temperatureBuf[_temperatureArrayCnt++] = g_sAllSensorValues.tempvalue;
    else
    {
        for (int i = 0; i < (_fire_alarm_log_count - 1); i++)
            _temperatureBuf[i] = _temperatureBuf[i + 1];
        _temperatureBuf[_fire_alarm_log_count - 1] = g_sAllSensorValues.tempvalue;
        float diff = _temperatureBuf[_fire_alarm_log_count - 1] - _temperatureBuf[0];
        if (diff >= g_sNvsDeviceInfo.enterFireAlarmValue)
        {
            if (!g_fireAlarmFlag)
            {
                __logse("Fire alarm detected");
                g_fireAlarmFlag = true;
                return true;
            }
        }
        else if (diff < g_sNvsDeviceInfo.enterFireAlarmValue)
        {
            if (g_fireAlarmFlag)
            {
                __logse("Fire alarm exit");
                g_fireAlarmFlag = false;
                return true;
            }
        }
    }

    return false;
}

_io bool CoverAlarmProc(void)
{
    if (m_lastHallAlarm != g_sAllSensorValues.halleffectalarmstatus)
    {
        m_lastHallAlarm = g_sAllSensorValues.halleffectalarmstatus;
        return true;
    }

    return false;
}

_io void BaseTimerControlProc(bool f_eControl)
{
    if (f_eControl == true)
    {
        MX_TIM6_Init();
        HAL_TIM_Base_Start_IT(&_USR_SYSTEM_BASE_TIMER_CHANNEL);
    }
    else
    {
        HAL_TIM_Base_DeInit(&_USR_SYSTEM_BASE_TIMER_CHANNEL);
    }
}

void UsrSleepAdcPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}

void UsrSleepGpioOutPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}

void UsrSleepGpioInputPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}
