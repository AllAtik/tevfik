#include "usr_general.h"

extern uint8_t m_globalRxBuffer;
char mainbuffer[100] = {0};

_io void AllPeripheralFirstParametersProc(void);
#ifdef _accModuleCompile
_io void AccelInitialProc(void);
#endif
_io void AllPeripheralDisableProc(void);
_io void GsmInitialSetupProc(void);

_io void PreProcessorProc(void);

S_USR_NVS_DEVICE_INFO_BOOT_DATA g_sUsrSystemNvsDeviceInfoBootData;

S_GSM_PARAMETERS g_sUsrSystemGsmParameters;
S_GSM_MODULE_INFO g_sUsrSystemGsmModuleInfo;
S_GSM_MQTT_CONNECTION_PARAMETERS g_sUsrSystemGsmMqttInitialParameters;
S_GSM_FTP g_sUsrSystemGsmFtpParameters;

S_ADC_PARAMETERS g_sUsrSystemAdcParameters;
S_ADC_RAW_PARAMETERS g_sUsrSystemAdcRawParameters;
S_BATTERY_DATA g_sUsrSystemBatteryParameters;
S_NTC_PARAMETERS g_sUsrSystemNtcParameters;

S_HALLEFFECT_PARAMETERS g_sUsrSystemHalleffectParameters;

S_ULTRASONIC_SENSOR_PARAMETERS g_sUsrSystemUltrasonicParameters;

S_TIMEDATE g_sUsrSystemTimeParameters;

S_LED_PARAMETERS g_sUsrSystemLedParameters;

S_ACC_PARAMETERS g_sUsrSystemAccelParameters;

// Adc Sensors flags
bool g_UsrSystemAdcSensorInitialFlag = false;
bool g_UsrSystemTemperatureSensorInitialFlag = false;
bool g_UsrSystemBatterySensorInitialFlag = false;
bool g_UsrSystemTemperatureSensorErrorFlag = false;
bool g_UsrSystemAdcSensorErrorFlag = false;
bool g_UsrSystemBatterySensorErrorFlag = false;

// Accelometer flags
bool g_UsrSystemAccelometerWakeUpFlag = false; // harbiden uyandı flag'i
bool g_UsrSytemAccelometerInterruptDetectedFlag = false;
bool g_UsrSystemAccelometerInterruptDetectedFlag = false;
bool g_UsrSystemAccelometerChipOkFlag = false;
bool g_UsrSystemAccelometerChipFlag = false;
bool g_UsrSystemAccelometerHardwareOkFlag = false;
bool g_UsrSystemAccelometerFabricationOkFlag = false;
bool g_UsrSystemAccelometerChipErrorFlag = false;
bool g_UsrSystemAccelometerErrorOpenLedFlag = false;
bool g_UsrSystemAccelometerIsReadyFlag = false;
bool g_UsrSystemAccelometerFabricationInitFlag = false;
uint32_t g_UsrSystemGlobalTimerCount = 0;

// Distance Sensor Flags
bool g_UsrSystemDistanceSensorErrorFlag = false;
bool g_UsrSystemSendDecideDataFlag = false;

// Gsm Flags
bool g_UsrSystemGsmModuleInitialFlag = false;
bool g_UsrSystemGsmModuleMqttInitialFlag = false;
bool g_UsrSystemGsmErrorFlag = false;

// Rtc WakeUp Flag
bool g_UsrSystemWakeUpRtcCheckDataFlag = false;
bool g_UsrSystemWakeUpFromRtcCheckDataFlag = false;

// Halleffect cover status
uint8_t g_UsrSystemCoverLastStatus = 0;

// led flags
bool g_UsrSystemLedInitialFlag = false;

// usr_sensor.c sensor interval parameters
uint32_t g_UsrSystemUltrasonicSensorIntervalTime = 10;       // s                 /// USR Sensor interval Suresi
bool g_UsrSystemUltrasonicSensorRoutineReadingFlag = false;  /// USR Sensor rutin okuma flag'i
bool g_UsrSystemUltrasonicSensorRoutineReadDoneFlag = false; /// USR Sensor rutin okuma tamamlandi flag'i
uint32_t g_UsrSystemUltrasonicSensorRoutineReadingTime = 0;  /// USR Sensor rutin zaman sayaci

uint32_t g_UsrSystemADCandHallSensorsIntervalTime = 10;       // s                /// ADC ve Hall sensörleri interval suresi
bool g_UsrSystemADCandHallSensorsRoutineReadingFlag = false;  /// ADC ve Hall sensörleri okuma flag'i
bool g_UsrSystemADCandHallSensorsRoutineReadDoneFlag = false; /// ADC ve Hall sensörleri okuma tamamlandi flag'i
uint32_t g_UsrSystemADCandHallSensorsRoutineReadingTime = 0;  /// ADC ve Hall sensörleri okuma sayaci

uint32_t g_UsrSystemRoutineDataSendIntervalTime = 20; // s                          /// Rutin data basma periyodum
bool g_UsrSystemRoutineDataSendFlag = false;          /// Rutin data basma flag'i
uint32_t g_UsrSystemRoutineDataSendTime = 0;          /// En son ne zaman bastım

// GSM'in acilma izinleri
bool g_UsrSystemSendAlarmMissionFlag = false;         ///  GSM'e acilma izni ve nedeni (Alarm Durumu Soz Konusuymus)
bool g_UsrSystemSendDataMissionFlag = false;          ///  GSM'e acilma izni ve nedeni (Data Gonderilmesi Gerekiyormus)
bool g_UsrSystemDeviceStatusCheckMissionFlag = false; ///  GSM'e acilma izni ve nedeni (deviceStatus Check etme zamani gelmis)

EDeviceSendDataAlarmProcess g_DeviceSendType; /// Alarm'in ne alarmi oldugu

// gsm json request for mqtt

bool g_accStatusFlag;
uint32_t g_dataSendTs;
bool g_fireAlarmFlag;
uint32_t g_dailyResetTimer;
uint32_t g_packageEventBits;

void UsrSystemInitial(void)
{
    // // Log initial
    // UsrSystemLogInitial();
    // //Watchdog Refresh
    // UsrSystemWatchdogRefresh();
    PreProcessorProc();
    // EEProm Initial
    UsrNvsInitial();
    // start timer
    HAL_TIM_Base_Start_IT(&_USR_SYSTEM_BASE_TIMER_CHANNEL);
    // update timestamp
    UsrSystemUpdateTsValues();          // normalde alttaydı
    AllPeripheralFirstParametersProc(); // geçerli olacak ilk değerlerin verilmesi
    AllPeripheralDisableProc();         // tüm peripherallerin disable başlatılması
    // Led Animation
    UsrProcessLedOpenAnimation();
    // Halleffect Give Power
    UsrSensorGetHalleffectStatusDirectly();
    // Decide first state
    UsrProcessDecideFirstState(); // uyanış sebebi disable olarak ayarlanır, iki tane flag ayarlanır.  g_sleepFlag = false;  ve   g_UsrProcessFirstRunFlag = true;
    // // Gsm Initial Setup
    // UsrSystemGsmPeripheralInitial();    // şu anlık çalışmak için bunu kapatmıyorum
    /// gsm initial and send request json payload
    // GsmInitialSetupProc();

#ifdef _accModuleCompile
    AccelInitialProc();
#endif

    /// DEBUG SIRASINDA GECİCİ !!!
    g_UsrSystemDeviceStatusCheckMissionFlag = true; // deviceStatus check ederek başlıyor
    /// DEBUG SIRASINDA GECİCİ !!!
}

void UsrSystemGeneral(void)
{
    /// DEBUG SIRASINDA GECİCİ !!!
    g_sNvsDeviceInfo.dailyReset = 10 * 60 + g_sAllSensorValues.rtc;
    g_sNvsDeviceInfo.deviceStatusCheckTime = 5 * 60 + g_sAllSensorValues.rtc; // 5 dk da bir checkmission 1 yapıyor.
    /// DEBUG SIRASINDA GECİCİ !!!

    UsrSystemWatchdogRefresh(); // Clean watchdog
#ifdef _accModuleCompile
    UsrSystemAccelometerGeneral();
#endif
    UsrSleepGeneral();    // access_flags: g_sleepFlag                     // potansiyel_turn_flags: g_UsrSystemAccelometerWakeUpFlag
    UsrSensorGetValues(); // access_flags: (g_sNvsDeviceInfo.deviceStatus && !g_UsrSystemDeviceStatusCheckMissionFlag) ve ( g_UsrSystemAccelometerWakeUpFlag || gUsrSystemSensorRoutineReadingFlag || g_UsrProcessFirstRunFlag )   turn_flags: g_UsrSensorMeasurementFinish
    UsrProcess();
    // UsrProcessAlarmProcess();                   // access_flags: g_UsrSensorMeasurementFinish   turn_flags: ToplananDatalarBasilacak
    // UsrSleepAgain();                            // turn flags: g_sleepFlag
    // UsrProcessGsmModuleOpenProcess();           // access_flags: ToplananDatalarBasilacak
    // UL_GsmModuleMqttGeneral();
}

// interrupta dene UL_GsmModuleMqttSubcribeDataCallback
void UL_GsmModuleMqttSubcribeDataCallback(const char *f_pTopic, uint16_t f_topicLen, const char *f_pPayload, uint16_t f_payloadLen)
{
    __logsw("Topic: %.*s Data: %.*s", f_topicLen, f_pTopic, f_payloadLen, f_pPayload);
}

void UsrSystemWatchdogRefresh(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

void UsrSystemHardFault(void)
{
    HAL_NVIC_SystemReset();
}

void UsrSystemUpdateTsValues(void)
{
    uint32_t timestamp = UL_RtcGetTs();

    g_sAllSensorValues.rtc = timestamp;
    __logse("Start ts:%d", timestamp);
    // g_LastSensorReadingTimeStamp = ts;
    // g_LastDeviceActiveCheckTimeStamp = ts;
    // g_LastPeriodicDataSendTimeStamp = ts;
}

void UsrSystemLogInitial(void)
{
    // HAL_UART_Transmit(&_USR_SYSTEM_LOG_UART_CHANNEL, mainbuffer, sizeof(mainbuffer), 100);
    __logsi("Main app was started!!!!");
}

#ifdef _accModuleCompile
void UsrSystemAccelometerGeneral(void)
{
    _io uint32_t _acctimercount = 0;

    // if(!g_UsrSystemAccelometerChipOkFlag)
    // {
    //     if(acctimercount > g_UsrSystemGlobalTimerCount)
    //         acctimercount = g_UsrSystemGlobalTimerCount;

    //     if((g_UsrSystemGlobalTimerCount - acctimercount) > 5000)
    //     {
    //         acctimercount = g_UsrSystemGlobalTimerCount;
    //         AccelInitialProc();
    //         if(!UL_AccelChipOk())
    //             g_UsrSystemAccelometerErrorOpenLedFlag = true;

    //         g_UsrSystemAccelometerChipFlag = g_UsrSystemAccelometerChipOkFlag;
    //     }
    // }
    // else
    //     acctimercount = g_UsrSystemGlobalTimerCount;

    // UL_AccelometerClearFlag();

    if (!g_accStatusFlag)
    {
        if ((HAL_GetTick() - _acctimercount) > 5000)
        {
            _acctimercount = HAL_GetTick();
            AccelInitialProc();
        }
    }

    UL_AccelometerClearFlag();
}
#endif

void UsrSystemGsmPeripheralInitial(void)
{
    UsrSystemWatchdogRefresh();
    UL_GsmModuleInitial(&g_sUsrSystemGsmParameters);
    UL_GsmModulePeripheral(disableGsmPeripheral);
    _gsm_delay(250);
    UL_GsmModulePeripheral(enableGsmPeripheral);
    g_UsrSystemGsmModuleInitialFlag = true;
}

_io void GsmInitialSetupProc(void)
{
    if (g_UsrSystemGsmModuleInitialFlag)
    {
        if (UL_GsmModuleCheck())
        {
            __logsi("Gsm module ok");
            g_UsrSystemGsmErrorFlag = false;
        }
        else
        {
            __logse("Gsm Module error");
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
                __logsi("Mqtt initial ok");
                g_UsrSystemGsmModuleMqttInitialFlag = true;
                if (!UL_GsmModuleMqttSubcribeTopic("topic1eren", 0))
                {
                    __logse("Subcribe error");
                }
                UL_GsmModuleMqttPublishTopic("topic1eren", "GsmOpened", 0, 0);
            }
            else
            {
                __logse("Mqtt error");
                g_UsrSystemGsmModuleMqttInitialFlag = false;
            }
        }
        g_UsrSystemGsmModuleInitialFlag = false; // bu açarsak kapatma olur
    }
}

#ifdef _accModuleCompile
_io void AccelInitialProc(void)
{
    // UL_AccelInitial(&g_sUsrSystemAccelParameters);

    // g_UsrSystemAccelometerChipOkFlag = UL_AccelCheckChip();
    // if(g_UsrSystemAccelometerChipOkFlag)
    // {
    //     // accHardwareokFlg
    //     g_UsrSystemAccelometerHardwareOkFlag = true;
    //     __logsi("acc chip hardware initial ok");
    //     // // Save eeprom
    //     // UsrNvsUpdate();
    // }
    // else
    // {
    //     g_UsrSystemAccelometerChipErrorFlag = false;
    //     __logse("usr_acc:acc chip error");
    // }

    // if(g_UsrSystemAccelometerChipOkFlag)
    // {
    //     g_UsrSystemAccelometerFabricationInitFlag = UL_AccelFabrication();
    //     if(g_UsrSystemAccelometerFabricationInitFlag)
    //     {
    //         g_UsrSystemAccelometerFabricationOkFlag = true;
    //     }
    //     else
    //     {
    //         g_UsrSystemAccelometerFabricationOkFlag = false;
    //     }
    // }

    g_accStatusFlag = UL_AccelCheckChip();
    if (g_accStatusFlag)
        UL_AccelFabrication();
}
#endif

_io void AllPeripheralDisableProc(void)
{
    UL_AdcPeripheral(&g_sUsrSystemAdcParameters, disableAdcPeripheral);
    UL_BatteryPeripheral(&g_sUsrSystemBatteryParameters, disableBatteryPeripheral);
    UL_NtcPeripheral(&g_sUsrSystemNtcParameters, disableNtcPeripheral);
    UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, disableUltrasonicSensor);
    UL_GsmModulePeripheral(disableGsmPeripheral);
    // UL_LedPeripheral(disableLedPeripheral);
}

_io void AllPeripheralFirstParametersProc(void)
{
    g_sUsrSystemLedParameters.pBluePort = RGB_LED_BLUE_PWM_GPIO_Port;
    g_sUsrSystemLedParameters.pGreenPort = RGB_LED_GREEN_PWM_GPIO_Port;
    g_sUsrSystemLedParameters.pRedPort = RGB_LED_RED_PWM_GPIO_Port;
    g_sUsrSystemLedParameters.bluePin = RGB_LED_BLUE_PWM_Pin;
    g_sUsrSystemLedParameters.greenPin = RGB_LED_GREEN_PWM_Pin;
    g_sUsrSystemLedParameters.redPin = RGB_LED_RED_PWM_Pin;

    g_sUsrSystemUltrasonicParameters.pUart = &_USR_SYSTEM_UART_1_CHANNEL;
    g_sUsrSystemUltrasonicParameters.pDistanceSensorOnOffPort = DISTANCE_SENSOR_ON_OFF_GPIO_Port;
    g_sUsrSystemUltrasonicParameters.sensorOnOffPin = DISTANCE_SENSOR_ON_OFF_Pin;
    g_sUsrSystemUltrasonicParameters.sensorPowerStatus = GPIO_PIN_SET;
    g_sUsrSystemUltrasonicParameters.eSensorType = model1Sensor;

    g_sUsrSystemAdcParameters.pAdcforDma = &_USR_SYSTEM_ADC_CHANNEL;
    g_sUsrSystemAdcRawParameters.rawtempvalue = 0;
    g_sUsrSystemAdcRawParameters.rawvreftempvalue = 0;
    g_sUsrSystemAdcRawParameters.rawbatteryhighvalue = 0;
    g_sUsrSystemAdcRawParameters.rawbatterylowvalue = 0;

    g_sUsrSystemNtcParameters.pNtcPort = NTC_ACTIVE_GPIO_Port;
    g_sUsrSystemNtcParameters.pNtcPin = NTC_ACTIVE_Pin;
    g_sUsrSystemNtcParameters.ntcstatus = GPIO_PIN_SET;

    g_sUsrSystemBatteryParameters.pbatteryPort = VBAT_ADC_ON_OFF_GPIO_Port;
    g_sUsrSystemBatteryParameters.pbatteryPin = VBAT_ADC_ON_OFF_Pin;
    g_sUsrSystemBatteryParameters.batteryrawvalue = 0;
    g_sUsrSystemBatteryParameters.batterypowerstatus = GPIO_PIN_SET;

    g_sUsrSystemHalleffectParameters.batteryhalleffectpowerport = BATTERY_COVER_HALL_SWITCH_POWER_GPIO_Port;
    g_sUsrSystemHalleffectParameters.batteryhalleffectpowerpin = BATTERY_COVER_HALL_SWITCH_POWER_Pin;
    g_sUsrSystemHalleffectParameters.batteryhalleffectinterruptport = BATTERY_COVER_HALL_SWITCH_OUT_INT_GPIO_Port;
    g_sUsrSystemHalleffectParameters.batteryhalleffectinterruptpin = BATTERY_COVER_HALL_SWITCH_OUT_INT_Pin;
    g_sUsrSystemHalleffectParameters.tophalleffectpowerport = TOP_COVER_HALL_SWITCH_POWER_GPIO_Port;
    g_sUsrSystemHalleffectParameters.tophalleffectpowerpin = TOP_COVER_HALL_SWITCH_POWER_Pin;
    g_sUsrSystemHalleffectParameters.tophalleffectinterruptport = TOP_COVER_HALL_SWITCH_OUT_INT_GPIO_Port;
    g_sUsrSystemHalleffectParameters.tophalleffectinterruptpin = TOP_COVER_HALL_SWITCH_OUT_INT_Pin;
    g_sUsrSystemHalleffectParameters.tophalleffectStatus = GPIO_PIN_SET;
    g_sUsrSystemHalleffectParameters.batteryhalleffectstatus = GPIO_PIN_SET;

    g_sUsrSystemGsmParameters.pUart = &_USR_SYSTEM_UART_2_CHANNEL;
    g_sUsrSystemGsmParameters.pPowerkeyPort = PWRKEY_CONTROL_GPIO_Port;
    g_sUsrSystemGsmParameters.powerKeyPin = PWRKEY_CONTROL_Pin;
    g_sUsrSystemGsmParameters.resetPin = GPRS_POWER_ON_OFF_Pin;
    g_sUsrSystemGsmParameters.gsmProcessMcuStatus = 1;
    g_sUsrSystemGsmParameters.pPowerPort = DC_DC_POWER_ON_OFF_GPIO_Port;
    g_sUsrSystemGsmParameters.powerPin = DC_DC_POWER_ON_OFF_Pin;
    g_sUsrSystemGsmParameters.powerKeyPinEnableStatus = 1;
    g_sUsrSystemGsmParameters.powerPinEnableStatus = 1;
    g_sUsrSystemGsmParameters.resetPinEnableStatus = 1;
    g_sUsrSystemGsmParameters.gsmProcessMcuStatus = 1;
    g_sUsrSystemGsmParameters.eModuleType = quectelM65GsmModule;

    sprintf(g_sUsrSystemGsmMqttInitialParameters.sGsmApn.name, "internet");
    sprintf(g_sUsrSystemGsmMqttInitialParameters.sMqtt.urlBuf, "95.70.201.96");
    g_sUsrSystemGsmMqttInitialParameters.sMqtt.port = 39039;
    g_sUsrSystemGsmMqttInitialParameters.sMqtt.keepAlive = 30;
    sprintf(g_sUsrSystemGsmMqttInitialParameters.sMqtt.randomIdBuf, "%s-gesk", g_sUsrSystemGsmModuleInfo.imeiBuf);

    sprintf(g_sUsrSystemGsmFtpParameters.sGsmApn.name, "internet");
    sprintf(g_sUsrSystemGsmFtpParameters.userNameBuf, "larryftp");
    sprintf(g_sUsrSystemGsmFtpParameters.userPasswordBuf, "gesk2017");
    sprintf(g_sUsrSystemGsmFtpParameters.urlBuf, "159.65.112.154");
    g_sUsrSystemGsmFtpParameters.port = 21;
    sprintf(g_sUsrSystemGsmFtpParameters.fileNameBuf, "test.txt");
    sprintf(g_sUsrSystemGsmFtpParameters.filePathBuf, "/");

#ifdef _accModuleCompile
    g_sUsrSystemAccelParameters.accelpowerport = ACC_POWER_GPIO_Port;
    g_sUsrSystemAccelParameters.accelpin = ACC_POWER_Pin;
    g_sUsrSystemAccelParameters.accelpowerstatus = 1;
    g_sUsrSystemAccelParameters.interruptport = ACC_INT_1_GPIO_Port;
    g_sUsrSystemAccelParameters.interruptpin = ACC_INT_1_Pin;
#endif

    UL_GsmModuleInitial(&g_sUsrSystemGsmParameters);
    UL_LedInitial(&g_sUsrSystemLedParameters);
    UL_AdcInitial(&g_sUsrSystemAdcParameters);
#ifdef _accModuleCompile
    UL_AccelInitial(&g_sUsrSystemAccelParameters);
#endif
}

_io void PreProcessorProc(void)
{
    HAL_Delay(500);
    __logsi("Main app was started version number : %s", _version);
}