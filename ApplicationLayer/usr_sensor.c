#include "usr_general.h"

#define _io static
#define _iov static volatile

#define _USR_SENSOR_DISTANCE_SAMPLE 16
#define _USR_SENSOR_NTC_SAMPLE 16
#define _USR_SENSOR_BATTERY_SAMPLE 16

S_SENSOR_ALL_VALUES g_sAllSensorValues;
// extern S_DEVICE_NVS_INFO g_sNvsDeviceInfo;

extern S_ADC_PARAMETERS g_sUsrSystemAdcParameters;
extern S_ADC_RAW_PARAMETERS g_sUsrSystemAdcRawParameters;
extern S_BATTERY_DATA g_sUsrSystemBatteryParameters;
extern S_NTC_PARAMETERS g_sUsrSystemNtcParameters;

extern S_HALLEFFECT_PARAMETERS g_sUsrSystemHalleffectParameters;

extern S_ULTRASONIC_SENSOR_PARAMETERS g_sUsrSystemUltrasonicParameters;

extern S_TIMEDATE g_sUsrSystemTimeParameters;

#define _USR_SENSOR_DISTANCE_SENSOR_ON_OFF(x) HAL_GPIO_WritePin(g_sUsrSystemUltrasonicParameters.pDistanceSensorOnOffPort, g_sUsrSystemUltrasonicParameters.sensorOnOffPin, (GPIO_PinState)x)
#define _USR_SENSOR_NTC_SENSOR_ON_OFF(x) HAL_GPIO_WritePin(g_sUsrSystemNtcParameters.pNtcPort, g_sUsrSystemNtcParameters.pNtcPin, (GPIO_PinState)x)
#define _USR_SENSOR_BATTERY_SENSOR_ON_OFF(x) HAL_GPIO_WritePin(g_sUsrSystemBatteryParameters.pbatteryPort, g_sUsrSystemBatteryParameters.pbatteryPin, (GPIO_PinState)x)

_io float CalculateBatteryVoltageProc(void);
_io uint8_t CalculateBatteryVoltagePercentageProc(float f_battery);
_io int CalculateDistanceSensorValueProc(void);
_io void UltrasonicSensorInitialProc(void);
_io void AdcSensorsInitialProc(void);
_io void AllPeripheralDisableProc(void);
_io float CalculateNtcTempValueProc(void);

// Ultrasonic variables
int m_UltrasonicDistance = 0;
int m_UltrasonicRawDistance = 0;
int m_DistanceValues[_USR_SENSOR_DISTANCE_SAMPLE] = {0};
int m_DistanceSum = 0;
uint8_t m_SampleCounterDistane = 0;

// Error flags
bool g_MeasurementErrorFlag = false;
bool g_TemperatureErrorFlag = false;
bool g_BatteryErrorFlag = false;

// Adc variables
float m_NtcTempValue[_USR_SENSOR_NTC_SAMPLE] = {0};
float m_NtcSum = 0;
float m_NtcTemp = 0;
float m_NtcRawTemp = 0;
float m_BatteryVolt = 0;
float m_BatterySum = 0;
uint8_t m_BatteryVoltagePercentage = 0;

extern uint8_t g_UsrSystemUltrasonicSensorIntervalTime;        /// USR Sensor interval Suresi
extern bool g_UsrSystemUltrasonicSensorRoutineReadingFlag;     /// USR Sensor okuma, rutin zaman flag'i
extern bool g_UsrSystemUltrasonicSensorRoutineReadDoneFlag;    /// USR Sensor rutin okuma tamamlandi flag'i
extern uint32_t g_UsrSystemUltrasonicSensorRoutineReadingTime; /// USR Sensor okuma, rutin zaman sayaci

extern uint8_t g_UsrSystemADCandHallSensorsIntervalTime;        /// ADC ve Hall sens�rleri interval suresi
extern bool g_UsrSystemADCandHallSensorsRoutineReadingFlag;     /// ADC ve Hall sens�rleri okuma flag'i
extern bool g_UsrSystemADCandHallSensorsRoutineReadDoneFlag;    /// ADC ve Hall sens�rleri okuma tamamlandi flag'i
extern uint32_t g_UsrSystemADCandHallSensorsRoutineReadingTime; /// ADC ve Hall sens�rleri okuma sayaci

// sleep and initial flags
extern bool g_UsrSystemSleepOnFlag;
extern bool g_UsrSystemAdcSensorInitialFlag;
extern bool g_UsrSystemTemperatureSensorInitialFlag;
extern bool g_UsrSystemBatterySensorInitialFlag;
extern bool g_UsrProcessFirstRunFlag; // Sensor reading flag(Initial da resetden gelince set edilecek flag)
extern bool g_UsrSystemSendDecideDataFlag;
// bool g_UsrSensorHallEffectSensorInitialFlag = false;
bool g_UsrSystemUltrasonicSensorInitialFlag = false;

// Acc flags
// #ifdef _accModuleCompile
extern bool g_UsrSystemAccelometerWakeUpFlag; // harbiden uyandi flag'i
// #endif

// mission
extern bool g_UsrSystemDeviceStatusCheckMissionFlag;

// Sensors Reading Ok
bool g_UsrSensorMeasurementFinish = 0;

// Fill Struct Flag
bool g_FillStructFlag = false;

bool g_sensorsReadingFlag;
uint32_t g_sensorReadTs;


void UsrSensorGetValues(void)
{
    if (g_sensorsReadingFlag)
    {
        UsrSensorGetAdcAndHalleffectValues();
        UsrSensorGetDistance();
        g_sensorReadTs = UL_RtcGetTs();
        g_sensorsReadingFlag = false;
    }
}

void UsrSensorGetAdcAndHalleffectValues(void)
{
    UsrSystemWatchdogRefresh();
    UsrSensorHallEffectGivePower();
    AdcSensorsInitialProc();

    for (uint8_t i = 0; i < 5; i++)
        if (UL_AdcGetValues(&g_sUsrSystemAdcParameters, &g_sUsrSystemAdcRawParameters))
            break;
        else
            HAL_Delay(100);

    HAL_Delay(50);
    UsrSensorHallEffectPinStatus();
    g_sAllSensorValues.tempvalue = CalculateNtcTempValueProc();
    g_sAllSensorValues.batteryvoltage = CalculateBatteryVoltageProc();
    g_sAllSensorValues.batteryvoltagepercentage = CalculateBatteryVoltagePercentageProc(g_sAllSensorValues.batteryvoltage);
    UL_HalleffectPeripheral(&g_sUsrSystemHalleffectParameters, disableHalleffect);
    UL_AdcPeripheral(&g_sUsrSystemAdcParameters, disableAdcPeripheral);
    UL_NtcPeripheral(&g_sUsrSystemNtcParameters, disableNtcPeripheral);
    UL_BatteryPeripheral(&g_sUsrSystemBatteryParameters, disableBatteryPeripheral);
}

int UsrSensorGetDistance(void)
{
    UsrSystemWatchdogRefresh();
    UltrasonicSensorInitialProc();

    for (uint8_t i = 0; i < 5; i++)
    {
        g_sAllSensorValues.distancevalue = CalculateDistanceSensorValueProc();
        if (g_sAllSensorValues.distancevalue != -100)
            break;
        else
            HAL_Delay(100);
    }

    if (g_sAllSensorValues.distancevalue == -100)
        g_sAllSensorValues.distanceStatusFlag = false;
    else
        g_sAllSensorValues.distanceStatusFlag = true;

    UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, disableUltrasonicSensor);
}

void _UsrSensorGetValues(void)
{
    if ((g_sNvsDeviceInfo.deviceStatus && !g_UsrSystemDeviceStatusCheckMissionFlag) || g_UsrProcessFirstRunFlag) // mission flag 1 ise girmiyor, cunku deviceStatus 1 olmmadi, iptal
    {
        uint32_t currentTime = UL_RtcGetTs();
        g_UsrSystemUltrasonicSensorRoutineReadingFlag = ((currentTime - g_UsrSystemUltrasonicSensorRoutineReadingTime) >= (g_UsrSystemUltrasonicSensorIntervalTime - 1));
        if (g_UsrSystemAccelometerWakeUpFlag || g_UsrSystemUltrasonicSensorRoutineReadingFlag || g_UsrProcessFirstRunFlag)
        {
            if (g_UsrSystemUltrasonicSensorRoutineReadingFlag) // Ultrasonic Sensor olcum zamani gelmis, 10 saniye demisiz,
            {
                g_UsrSystemUltrasonicSensorRoutineReadingTime = currentTime; // time update
                UltrasonicSensorInitialProc();                               // Initial Ultrasonic Sensor
                g_UsrSystemUltrasonicSensorRoutineReadingFlag = false;       // clear flag
            }

            if (g_UsrSystemUltrasonicSensorInitialFlag)
            {
                m_UltrasonicDistance = CalculateDistanceSensorValueProc();
                g_FillStructFlag = true;
            }

            if (!g_sAllSensorValues.distancesensorerrorflag)
            {
                g_sAllSensorValues.distancedataokflag = true;
                g_UsrSystemUltrasonicSensorRoutineReadDoneFlag = true;
            }
            else
            {
                g_sAllSensorValues.distancedataokflag = false;
            }
        }

        currentTime = UL_RtcGetTs();
        g_UsrSystemADCandHallSensorsRoutineReadingFlag = ((currentTime - g_UsrSystemADCandHallSensorsRoutineReadingTime) >= (g_UsrSystemADCandHallSensorsIntervalTime - 1));
        if (g_UsrSystemAccelometerWakeUpFlag || g_UsrSystemADCandHallSensorsRoutineReadingFlag || g_UsrProcessFirstRunFlag)
        {
            if (g_UsrSystemADCandHallSensorsRoutineReadingFlag)
            {
                g_UsrSystemADCandHallSensorsRoutineReadingTime = currentTime;
                g_UsrSystemADCandHallSensorsRoutineReadingFlag = false;
            }

            AdcSensorsInitialProc();
            UsrSensorHallEffectGivePower();

            if (g_UsrSystemAdcSensorInitialFlag)
            {
                if (UL_AdcGetValues(&g_sUsrSystemAdcParameters, &g_sUsrSystemAdcRawParameters))
                {
                    g_sAllSensorValues.adcdataokflag = true;
                }
            }

            // if (g_UsrSensorHallEffectSensorInitialFlag)
            {
                UsrSensorHallEffectPinStatus(); /// struct'a yazma iceride yapildi, isi bittikten sonra iceride kapatildi,
            }

            if (g_sAllSensorValues.adcdataokflag)
            {
                m_BatteryVolt = CalculateBatteryVoltageProc();
                m_NtcTemp = CalculateNtcTempValueProc();
                m_BatteryVoltagePercentage = CalculateBatteryVoltagePercentageProc(m_BatteryVolt);
                g_FillStructFlag = true;
                g_UsrSystemADCandHallSensorsRoutineReadDoneFlag = true;
            }
        }

        AllPeripheralDisableProc(); // once kapatsin

        if (g_FillStructFlag)
        {
            g_FillStructFlag = false;
            g_sAllSensorValues.tempvalue = m_NtcTemp; // bu civarda ufak degisikliklere dikkat, ayberk
            g_sAllSensorValues.batteryvoltage = m_BatteryVolt;
            g_sAllSensorValues.batteryvoltagepercentage = m_BatteryVoltagePercentage;
            g_sAllSensorValues.distancevalue = m_UltrasonicDistance;

            //__logsi("temp: %.3f, charge: %.3f, distance: %d\n", g_sAllSensorValues.tempvalue, g_sAllSensorValues.batteryvoltage, g_sAllSensorValues.distancevalue);
        }
    }
}

void UsrSensorGetHalleffectStatusDirectly(void)
{
    UsrSensorHallEffectGivePower();
    HAL_Delay(100);
    UsrSensorHallEffectPinStatus();
}

void UsrSensorHallEffectGivePower(void)
{
    UsrSleepGpioInputPins(BATTERY_COVER_HALL_SWITCH_OUT_INT_GPIO_Port, BATTERY_COVER_HALL_SWITCH_OUT_INT_Pin | TOP_COVER_HALL_SWITCH_OUT_INT_Pin); // inputa cekerek guc cikisini yok eder
    UL_HalleffectPeripheral(&g_sUsrSystemHalleffectParameters, enableHalleffect);                                                                                    // gucu verdim hazirim demek
}

void UsrSensorHallEffectPinStatus(void)
{
    g_sAllSensorValues.halleffectalarmstatus = 0;
    if (_TOP_COVER_HALL_READ_PIN())
    {
        g_sAllSensorValues.halleffectalarmstatus |= 0x01;
    }
    else
    {
        g_sAllSensorValues.halleffectalarmstatus &= (~0x01);
    }

    if (_BATTERY_COVER_HALL_READ_PIN())
    {
        g_sAllSensorValues.halleffectalarmstatus |= 0x02;
    }
    else
    {
        g_sAllSensorValues.halleffectalarmstatus &= (~0x02);
    }

    UL_HalleffectPeripheral(&g_sUsrSystemHalleffectParameters, disableHalleffect);
    _BATTERY_COVER_HALL_POWER(0);
    _TOP_COVER_HALL_POWER(0);
    UsrSleepGpioOutPins(BATTERY_COVER_HALL_SWITCH_OUT_INT_GPIO_Port, BATTERY_COVER_HALL_SWITCH_OUT_INT_Pin | TOP_COVER_HALL_SWITCH_OUT_INT_Pin, GPIO_PIN_RESET);

    // Her okuma sonrasi kapatildi ayaklar
    /// kapak acikken (miknatis yokken) 1 cikisi veriyor, kapak kapaliyken (miknatsi goruyorsa) 0 cikisi veriyor.
    /// halleffectalarmstatus'te sagdaki bit U8 (TOP_COVER), Soldaki bit U9 (BATTERY_COVER)'u temsil ediyor.
}

_io void UltrasonicSensorInitialProc(void)
{
    if (UL_UltrasonicSensorInitial(&g_sUsrSystemUltrasonicParameters))
    {
        g_UsrSystemUltrasonicSensorInitialFlag = true;
        UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, enableUltrasonicSensor);
    }
    else
        g_UsrSystemUltrasonicSensorInitialFlag = false;
}

_io void AdcSensorsInitialProc(void)
{
    UL_AdcPeripheral(&g_sUsrSystemAdcParameters, enableAdcPeripheral);
    UL_NtcInitial(&g_sUsrSystemNtcParameters);
    UL_BatteryInitial(&g_sUsrSystemBatteryParameters);
}

_io void _AdcSensorsInitialProc(void)
{
    if (UL_AdcInitial(&g_sUsrSystemAdcParameters))
    {
        g_UsrSystemAdcSensorInitialFlag = true;
        UL_AdcPeripheral(&g_sUsrSystemAdcParameters, enableAdcPeripheral);

        if (UL_NtcInitial(&g_sUsrSystemNtcParameters))
        {
            UL_NtcPeripheral(&g_sUsrSystemNtcParameters, enableNtcPeripheral);
            g_UsrSystemTemperatureSensorInitialFlag = true;
        }
        if (UL_BatteryInitial(&g_sUsrSystemBatteryParameters))
        {
            UL_BatteryPeripheral(&g_sUsrSystemBatteryParameters, enableBatteryPeripheral);
            g_UsrSystemBatterySensorInitialFlag = true;
        }
    }
    else
    {
        g_UsrSystemAdcSensorInitialFlag = false;
    }
}

_io float CalculateBatteryVoltageProc(void)
{
    uint8_t WrongOne = 0;
    m_BatterySum = 0;

    for (uint8_t i = 0; i < _USR_SENSOR_BATTERY_SAMPLE; i++)
    {
        float factor = (3 * (*(VREF_ADD))) / ((float)g_sUsrSystemAdcRawParameters.rawvreftempvalue);
        uint32_t rawBatteryValue = g_sUsrSystemAdcRawParameters.rawbatteryhighvalue - g_sUsrSystemAdcRawParameters.rawbatterylowvalue;
        float m_BatteryVoltage = ((factor * (float)rawBatteryValue) / USR_ADC_RESOLUTION) * VBAT_ADC_CALIBRATION_VALUE;

        if (_USR_SENSOR_BATTERY_VOLTAGE_ERROR_CONTROL(m_BatteryVoltage))
        {
            WrongOne++;
        }
        else
        {
            m_BatterySum += m_BatteryVoltage;
        }
    }

    if ((WrongOne * 2) >= _USR_SENSOR_BATTERY_SAMPLE) // orneklerin yarisindan fazlasi hataliysa
    {
        g_BatteryErrorFlag = true;
        g_sAllSensorValues.batteryvoltageerrorflag = g_BatteryErrorFlag;
        _USR_SENSOR_BATTERY_SENSOR_ON_OFF(0);
        UL_BatteryPeripheral(&g_sUsrSystemBatteryParameters, disableBatteryPeripheral);
    }
    else // hata yok
    {
        g_BatteryErrorFlag = false;
        g_sAllSensorValues.batteryvoltageerrorflag = g_BatteryErrorFlag;
        m_BatterySum /= (_USR_SENSOR_BATTERY_SAMPLE - WrongOne);
        return m_BatterySum;
    }
    return 0;
    ////// ( Vref ) * ( (raw_adc_high - raw_adc_low) / 4095 ) * 32/10
}

_io uint8_t CalculateBatteryVoltagePercentageProc(float f_battery)
{
    uint8_t batteryPercentage = 0;

    if (f_battery >= 6.95)
    {
        batteryPercentage = 100;
    }
    else if (f_battery >= 6.85)
    {
        batteryPercentage = 60;
    }
    else if (f_battery >= 6.70)
    {
        batteryPercentage = 50;
    }
    else if (f_battery >= 6.50)
    {
        batteryPercentage = 30;
    }
    else if (f_battery >= 6.30)
    {
        batteryPercentage = 10;
    }
    else
    {
        batteryPercentage = 0;
    }
    return batteryPercentage;
}

_io int CalculateDistanceSensorValueProc(void)
{
    // m_DistanceSum = 0;
    // uint8_t WrongOne = 0;

    // for (uint8_t i = 0; i < _USR_SENSOR_DISTANCE_SAMPLE; i++) // _USR_SENSOR_DISTANCE_SAMPLE kez olcum alindi, hatasiz olcumlerin ortalamasi alinacak.
    // {
    //     m_UltrasonicRawDistance = UL_UltrasonicSensorGetValue(100);
    //     HAL_Delay(1); // BU OLMASSA OLCUMLERIN BUYUK KISMI YAMULUYOR

    //     if (m_UltrasonicRawDistance != _USR_SENSOR_DISTANCE_ERROR_VALUE)
    //     {
    //         m_DistanceValues[i] = m_UltrasonicRawDistance;
    //     }
    //     else
    //     {
    //         WrongOne++;
    //         m_DistanceValues[i] = 0;
    //     }
    //     m_DistanceSum += m_DistanceValues[i];
    // }

    // if ((WrongOne * 2) >= _USR_SENSOR_DISTANCE_SAMPLE) // olcumlerin yarisindan fazlasi hataliysa (BURAYİ OLDUGU GİBİ COPY PASTE YAPTİM )
    // {
    //     g_MeasurementErrorFlag = true;
    //     g_sAllSensorValues.distancesensorerrorflag = g_MeasurementErrorFlag;
    //     g_UsrSystemUltrasonicSensorInitialFlag = false;
    //     _USR_SENSOR_DISTANCE_SENSOR_ON_OFF(0);
    //     UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, disableUltrasonicSensor);
    // }
    // else // kabul edilebilirse tamamdır
    // {
    //     g_MeasurementErrorFlag = false;
    //     g_sAllSensorValues.distancesensorerrorflag = g_MeasurementErrorFlag;
    //     HAL_UART_Abort_IT(g_sUsrSystemUltrasonicParameters.pUart); // İsim bitti, bosuna beni interrupt'a cekmesin (durduk yere biseler gonderiyo bu gri olan)
    //     m_DistanceSum /= (_USR_SENSOR_DISTANCE_SAMPLE - WrongOne);
    //     if (m_DistanceSum <= 400)
    //     {
    //         return g_sAllSensorValues.distancevalue; /// 300'un altindaysan, son anlamli degeri geri dondur dendi !!!
    //     }
    //     return m_DistanceSum;
    // }
    // return -100;

    m_DistanceSum = 0;
    uint8_t sampleCount = 0;
    for (uint8_t i = 0; i < _USR_SENSOR_DISTANCE_SAMPLE; i++) // _USR_SENSOR_DISTANCE_SAMPLE kez olcum alindi, hatasiz olcumlerin ortalamasi alinacak.
    {
        m_UltrasonicRawDistance = UL_UltrasonicSensorGetValue(100);
        HAL_Delay(1); // BU OLMASSA OLCUMLERIN BUYUK KISMI YAMULUYOR

        if (m_UltrasonicRawDistance != _USR_SENSOR_DISTANCE_ERROR_VALUE)
        {
            sampleCount++;
            m_DistanceSum += m_UltrasonicRawDistance;
        }
    }

    if (sampleCount <= 7)
        return -100;
    else
        return m_DistanceSum / sampleCount;
}

_io float CalculateNtcTempValueProc(void)
{
    uint8_t WrongOne = 0;
    m_NtcSum = 0;

    for (uint8_t i = 0; i < _USR_SENSOR_NTC_SAMPLE; i++)
    {
        m_NtcRawTemp = UL_NtcGetValue(g_sUsrSystemAdcRawParameters.rawtempvalue);
        if (m_NtcRawTemp != _USR_SENSOR_ADC_ERROR_VALUE)
        {
            m_NtcSum += m_NtcRawTemp;
        }
        else
        {
            WrongOne++;
        }
    }

    if ((WrongOne * 2) >= _USR_SENSOR_NTC_SAMPLE) // orneklerin yarisindan fazlasi hataliysa (EREN ABİDEN KOPYALADIM YİNE OLDUGU GİBİ)
    {

        g_TemperatureErrorFlag = true;
        g_sAllSensorValues.temperatureSensorErrorFlag = g_TemperatureErrorFlag;
        _USR_SENSOR_NTC_SENSOR_ON_OFF(0);
        UL_NtcPeripheral(&g_sUsrSystemNtcParameters, disableNtcPeripheral);
    }
    else // hata yok
    {
        g_TemperatureErrorFlag = false;
        g_sAllSensorValues.temperatureSensorErrorFlag = g_TemperatureErrorFlag;
        m_NtcSum /= (_USR_SENSOR_NTC_SAMPLE - WrongOne);
        return m_NtcSum;
    }
    return -100;
}

_io void AllPeripheralDisableProc(void)
{
    UL_UltrasonicSensorPeripheral(&g_sUsrSystemUltrasonicParameters, disableUltrasonicSensor);
    UL_AdcPeripheral(&g_sUsrSystemAdcParameters, disableAdcPeripheral);
    UL_NtcPeripheral(&g_sUsrSystemNtcParameters, disableNtcPeripheral);
    UL_BatteryPeripheral(&g_sUsrSystemBatteryParameters, disableBatteryPeripheral);
}
