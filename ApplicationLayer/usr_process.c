#include "usr_general.h"

#define _USR_PERCENTAGE_LIMIT 10

_io bool GsmProc(void);

extern S_SENSOR_ALL_VALUES g_sAllSensorValues;
// extern S_DEVICE_NVS_INFO g_sNvsDeviceInfo;

extern S_HALLEFFECT_PARAMETERS g_sUsrSystemHalleffectParameters;
extern S_LED_PARAMETERS g_sUsrSystemLedParameters;

extern S_GSM_PARAMETERS g_sUsrSystemGsmParameters;
extern S_GSM_MODULE_INFO g_sUsrSystemGsmModuleInfo;
extern S_GSM_MQTT_CONNECTION_PARAMETERS g_sUsrSystemGsmMqttInitialParameters;
extern S_GSM_FTP g_sUsrSystemGsmFtpParameters;

extern bool g_UsrSystemSendDecideDataFlag;
extern bool g_UsrSystemSendAlarmMissionFlag; // g_UsrSystemSendDecideDataFlag yerel degisken idi
extern bool g_UsrSystemSendDataMissionFlag;

extern bool g_UsrSystemGsmModuleInitialFlag;
extern bool g_UsrSystemGsmModuleMqttInitialFlag;
extern bool g_UsrSystemGsmErrorFlag;

bool g_UsrProcessFirstRunFlag = false;

extern bool g_UsrSystemLedInitialFlag;     // using for led
extern uint8_t g_UsrSystemCoverLastStatus; // halleffect last alarm control

extern EDeviceSendDataAlarmProcess g_DeviceSendType;

uint8_t m_AlarmGroupBits = 0;

// Sleep flags

// ayberk
bool isTankFullLast = 0;                          // tankın full'lük durumu degisti mi ? anlamak icin kullanildi
uint8_t fullnessLimitExdeed = 0;                  // fullness asildi mi ?
S_AYBERK_ALARM_TYPE g_sUsrProcessAyberkAlarmType; // durum flaglari burada yigildi.
extern bool g_UsrSensorMeasurementFinish;

extern bool g_UsrSystemADCandHallSensorsRoutineReadDoneFlag; /// ADC ve Hall sensörleri okuma tamamlandi flag'i
extern bool g_UsrSystemUltrasonicSensorRoutineReadDoneFlag;  /// USR Sensor rutin okuma tamamlandi flag'i

extern bool g_UsrSystemRoutineDataSendFlag;             /// Rutin data basma flag'i
extern uint32_t g_UsrSystemRoutineDataSendTime;         /// En son ne zaman bastım
extern uint32_t g_UsrSystemRoutineDataSendIntervalTime; /// Rutin data basma periyodum

extern bool g_UsrSystemAccelometerWakeUpFlag;

// using for temp alarm
_io uint8_t m_logTemperatureAlarmCount;
_io float m_logTemperatureAlarmBuf[_fire_alarm_log_count];

// Alarm ıntervals
extern uint8_t g_UsrsystemAlarmsIntervalTime;

// gsm check
extern bool g_UsrSystemDeviceStatusCheckMissionFlag;
extern bool m_eReceiveGsmDataCameOkFlg;
extern bool m_eReceiveGsmDataCameFlg;
extern uint8_t m_receiveGsmEndBuf[512];

// for mqtt payload
char m_JsonDataBuf[256];
char *json;
bool startParser = false; // request parser

// int counter = 0;
// int timestampp = 0;
// int sendType = 0;
char *mcu = "STM32L051";
// float temp, charge = 0;
char *version = "8.4.1";
uint8_t lat, lon = 0;
// int rawData = 0;

//
_io void ClearFlagProc(void);
_io char *PreparePublishJsonDataProc(void);
_io void PrepareRequsetJsonDataProc(void);
_io void AddDataToBufProc(void);

_io int16_t m_logRawDataBuf[144];
_io uint8_t m_logRawDataBufCnt;
_io uint32_t m_startTs;

void UsrProcessGsmModuleOpenProcess(void)
{
    if (g_UsrSystemDeviceStatusCheckMissionFlag || g_UsrSystemSendAlarmMissionFlag || g_UsrSystemSendDataMissionFlag)
    {
        /// GSM MODULU AKTIFLESTIRILIYOR !!  // Burada Gsm ve mqtt yi sürekli açıp kapamaya gerek yok sleep tarafına bak

        // UL_GsmModuleMqttGeneral();
        json = PreparePublishJsonDataProc();
        if (UL_GsmModuleMqttPublishTopic("topic1eren", json, 0, 0))
        {
            startParser = true;
        }

        /// GSM AKTIFLESTIRILDI !

        __logsi("GSM Aktif !");

        if (g_UsrSystemDeviceStatusCheckMissionFlag) /// GELIS SEBEBI : deviceStatus Check (BU VARSA DIGERLERI ZATEN 1 OLAMAZ)
        {
            g_UsrSystemDeviceStatusCheckMissionFlag = false;
            g_sleepFlag = false; /// check sırasında rutin işlemler atlanmıştı, bir kez de uyku atlanmalı ve döngü başa dönmeli. (iki kez ardarda uyur bu olmassa)
            /// device Statusun durumuna bakilmasi, g_sNvsDeviceInfo.deviceStatus'e deger atanmasi
            if (startParser)
            {
                startParser = false;
                // PrepareRequsetJsonDataProc();   // response bekleme yeri

                ////////// DEBUG SIRASINDA GECICI OLARAK KONULDU !!!!!!!!!!!!1
                g_sNvsDeviceInfo.deviceStatus = 1; // ilk reset anında buradan yollayacak, burada response bekleyecek en fazla 5dk, deviceStatus u soru için eepromunu güncelleyecek
                ////////// DEBUG SIRASINDA GECICI OLARAK KONULDU !!!!!!!!!!!!1
            }
        }
        if (g_UsrSystemSendDataMissionFlag) /// GELIS SEBEBI : DATA BASMAK (1 OLABİLİR)  // resetten gelen flag
        {
            g_UsrSystemSendDataMissionFlag = false; // burada aslında log basılan şeyler gsm den gönderilmiş olmalı (bayrak 1 iken, gsm gönderme fonk. burada olmalı)
            int XX = UL_RtcGetTs();
            __logsi("ts: %d, temp: %.3f, charge: %.3f, distance: %d\n", XX, g_sAllSensorValues.tempvalue, g_sAllSensorValues.batteryvoltage, g_sAllSensorValues.distancevalue);
            /// RUTİN DATA BASİLACAK
        }

        if (g_UsrSystemSendAlarmMissionFlag) /// GELIS SEBEBI : Alarm gonderimi (1 OLABİLİR)
        {
            g_UsrSystemSendAlarmMissionFlag = false;
            /// ALARM DURUMU VARMIS, BASILACAK !!!
        }
        // üç if in de en altında

        /// IS BITTI, GSM KAPATILACAK !!!
        /*
                if(!g_UsrSystemGsmModuleInitialFlag)
                {
                    UL_GsmModuleMqttPublishTopic("topic1eren", "GsmClosed", 0, 0);
                    UL_GsmModuleMqttClosed();
                    UL_GsmModulePeripheral(disableGsmPeripheral);
                    UL_GsmModuleHardwareReset();
                }
        */
    }

    /// KAPATILDI
}

void UsrProcess(void)
{
    _io bool _sendDataFlg = true;
    _io bool _isTankFullState = false;
    _io bool _fullnessLimitExdeed = false;
    _io bool _isCleaned = false;
    _io bool _fireAlarmFlag = false;
    _io uint8_t _coverAlarmFlag = 0;

    if (g_dailyResetTimer > _USR_SYSTEM_DAILY_RESET_TIME)
        HAL_NVIC_SystemReset();

    if (_sendDataFlg)
    {
        __logsw("DEVICE COME FROM RESET");
        g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_DEVICE_RESET;
        g_dataSendTs = UL_RtcGetTs();
    }

    if (!g_sensorsReadingFlag)
    {
        AddDataToBufProc();

        if (_coverAlarmFlag != g_sAllSensorValues.halleffectalarmstatus)
        {
            _coverAlarmFlag = g_sAllSensorValues.halleffectalarmstatus;
            __logse("Cover last status %02x", _coverAlarmFlag);
            g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_COVERS_ALARM;

            _sendDataFlg = true;
        }

        if (_fireAlarmFlag != g_fireAlarmFlag)
        {
            _fireAlarmFlag = g_fireAlarmFlag;
            g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_FIRE_ALARM;
            __logse("Fire alarm status was changed %d", (int)_fireAlarmFlag);
            _sendDataFlg = true;
        }

        if (g_sAllSensorValues.distancevalue != _USR_SENSOR_DISTANCE_ERROR_VALUE)
        {
            if (g_sAllSensorValues.distancevalue <= g_sNvsDeviceInfo.fullAlarmLimit)
            {
                if (!_isTankFullState)
                {
                    __logse("Enter full alarm");
                    g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_FULL_ALARM;
                    _sendDataFlg = true;
                }
                _isTankFullState = true;
            }
            else
            {
                if (_isTankFullState)
                {
                    __logsw("Exit full alarm");
                    g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_FULL_ALARM;
                    _sendDataFlg = true;
                }
                _isTankFullState = false;
            }

            if (g_sAllSensorValues.distancevalue <= g_sNvsDeviceInfo.fullnessAlarmLimit)
            {
                if (!_fullnessLimitExdeed)
                {
                    __logsw("Enter fullness alarm");
                }

                _fullnessLimitExdeed = true;
            }
            else if (_fullnessLimitExdeed &&
                     g_sAllSensorValues.distancevalue >= (g_sNvsDeviceInfo.depthAlarmLimit * ((100 - g_sNvsDeviceInfo.toleranceValue) / 100)))
            {
                __logsi("Tank has been emptied !");
                g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_FULLNESS_ALARM;
                _sendDataFlg = true;
                _isCleaned = true;
                _fullnessLimitExdeed = false;
            }
        }

        if ((UL_RtcGetTs() - g_dataSendTs) >= g_sNvsDeviceInfo.sendingDataInterval)
        {
            g_packageEventBits |= _USR_SYSTEM_EVENT_BITS_PERIODIC_DATA_SEND;
            __logsw("Dummy data send");
            _sendDataFlg = true;
        }
    }

    if (_sendDataFlg)
    {
        g_dataSendTs = UL_RtcGetTs();
        if (GsmProc())
        {
            m_logRawDataBufCnt = 0;
            g_packageEventBits = 0;
            _isCleaned = false;
        }
        //...
        _sendDataFlg = false;
    }

    g_sleepFlag = true;
}

void _UsrProcessAlarmProcess(void)
{
    /// distance (USR)
    if (g_UsrSystemUltrasonicSensorRoutineReadDoneFlag) // USR Sensor Okumasi Basariyla Tamamlandi
    {
        g_UsrSystemUltrasonicSensorRoutineReadDoneFlag = false;

        // Gelen veriyi diziye koydun, son eleman en sonda, yine sondaki elemana gore asagadileri yaptin, ama gonderebildin mi ? gonderebildiysen diziyi bosalt. gonderemediysen dizide biriksin. !!!

        // tank full mü
        isTankFullLast = g_sUsrProcessAyberkAlarmType.isTankFull;
        if (g_sAllSensorValues.distancevalue <= g_sNvsDeviceInfo.fullAlarmLimit) /// TANK DOLU
        {
            g_DeviceSendType = tankIsFullNowAlarmLimitSendDecideData;
            g_sAllSensorValues.alarmeventgroup |= 0x02;  // send mqtt for json file, ikinci bit fullalarmlimit
            g_sUsrProcessAyberkAlarmType.isTankFull = 1; // son ndurum kontrolü için
            m_AlarmGroupBits |= 0x02;                    // değişken ayarlaması
        }
        else /// TANK DOLU DEGIL
        {
            g_DeviceSendType = tankIsNotFullNowAlarmLimitSendDecideData;
            g_sAllSensorValues.alarmeventgroup &= ~(0x02); // send mqtt for json file, ikinci bit fullalarmlimit
            g_sUsrProcessAyberkAlarmType.isTankFull = 0;   // son ndurum kontrolü için
            m_AlarmGroupBits &= ~(0x02);                   // değişken ayarlaması
        }

        // son durum kontrolü
        if (isTankFullLast != g_sUsrProcessAyberkAlarmType.isTankFull) ///  DEGISIM OLURSA ALARM GITSIN
        {
            g_sUsrProcessAyberkAlarmType.isTankFullChanged = 1; // son durum değişti kontrolü
            g_UsrSystemSendDecideDataFlag = true;               // Mqtt den gönder

            if (g_sUsrProcessAyberkAlarmType.isTankFull) // Doluluk koşulu doğruysa
            {
                __logse("Tank is Full Now !");
            }
            else
            {
                __logse("Tank is Not Full !");
            }
        }
        else
        {
            g_sUsrProcessAyberkAlarmType.isTankFullChanged = 0;
        }

        /// tank boşaltıldı mı
        if (g_sAllSensorValues.distancevalue <= g_sNvsDeviceInfo.fullnessAlarmLimit) /// FULLNESS UZERINE CIKILDI
        {
            __logse("Enter fullness alarm");
            g_sUsrProcessAyberkAlarmType.isTankCleaned = 0;
            g_sAllSensorValues.alarmeventgroup &= ~(0x01); // send mqtt for json file, ilk bit fullnessAlarmLimit, boşaltıldı durumu
            m_AlarmGroupBits &= ~(0x01);                   // değişken ayarlaması
            fullnessLimitExdeed = 1;                       // fullness asildi mi ?
        }

        if (fullnessLimitExdeed)
        {
            if (g_sAllSensorValues.distancevalue > (g_sNvsDeviceInfo.depthAlarmLimit * ((100 - g_sNvsDeviceInfo.toleranceValue) / 100))) // SONRADAN ALTINA INILDI
            {
                __logse("Fullness alarm finished");
                g_sUsrProcessAyberkAlarmType.isTankCleaned = 1; // BİR SONRAKİ İTERASYONA KADAR KALACAK BU
                g_sAllSensorValues.alarmeventgroup |= (0x01);   // send mqtt for json file, ilk bit fullnessAlarmLimit, boşaltıldı durumu
                m_AlarmGroupBits |= (0x01);                     // değişken ayarlaması
                g_DeviceSendType = tankJustCleanedAlarmSendDecideData;
                g_UsrSystemSendDecideDataFlag = true; // Mqtt den gönder

                __logse("Tank has been emptied !");

                fullnessLimitExdeed = 0;
            }
        }
        else
        {
            g_sUsrProcessAyberkAlarmType.isTankCleaned = 0;
            m_AlarmGroupBits &= ~(0x01);
        }
    }

    if (g_UsrSystemADCandHallSensorsRoutineReadDoneFlag) // ADC okumasi basariyla tamamlandi, Hall Sensor ise acilmis olmali.
    {
        g_UsrSystemADCandHallSensorsRoutineReadDoneFlag = false;

        /// Kapaklar (Hall Effect)
        if (g_UsrSystemCoverLastStatus != g_sAllSensorValues.halleffectalarmstatus) // kapaklarda degisim olmussa
        {
            g_UsrSystemCoverLastStatus = g_sAllSensorValues.halleffectalarmstatus;

            if (g_sAllSensorValues.halleffectalarmstatus == 1) // sadece top cover acik
            {
                g_DeviceSendType = topCoverOpenAlarmSendDecideData;
                __logse("Top Cover Open, Battery Cover Closed");
            }
            else if (g_sAllSensorValues.halleffectalarmstatus == 2) // sadece bat cover acik
            {
                g_DeviceSendType = batteryCoverOpenAlarmSendDecideData;
                __logse("Top Cover Closed, Battery Cover Open");
            }
            else if (g_sAllSensorValues.halleffectalarmstatus == 3) // ikisi de acik
            {
                g_DeviceSendType = allCoversOpenAlarmSendDecideData;
                __logse("Top Cover and Battery Cover Open");
            }
            else
            {
                g_DeviceSendType = allCoversClosedAlarmSendDecideData; // hepsi kapali
                __logse("Top Cover and Battery Cover Closed");
            }
            g_UsrSystemSendDecideDataFlag = true; // Mqtt den gönder
        }

        /// Termal (NTC)
        if (!g_sAllSensorValues.temperatureSensorErrorFlag)
        {
            if (m_logTemperatureAlarmCount < _fire_alarm_log_count)
            {
                m_logTemperatureAlarmBuf[m_logTemperatureAlarmCount++] = g_sAllSensorValues.tempvalue;
            }
            else
            {
                for (int i = 0; i < (_fire_alarm_log_count - 1); i++)
                {
                    m_logTemperatureAlarmBuf[i] = m_logTemperatureAlarmBuf[i + 1];
                }
                m_logTemperatureAlarmBuf[_fire_alarm_log_count - 1] = g_sAllSensorValues.tempvalue;
                float diff = m_logTemperatureAlarmBuf[_fire_alarm_log_count - 1] - m_logTemperatureAlarmBuf[0];
                if (diff >= g_sNvsDeviceInfo.enterFireAlarmValue)
                {
                    if (!((g_sAllSensorValues.alarmeventgroup >> 3) & 1))
                    {
                        m_AlarmGroupBits |= (1 << 3);
                        g_sAllSensorValues.alarmeventgroup = m_AlarmGroupBits;
                        __logse("Fire alarm detected");
                        g_sAllSensorValues.eWakeuptype = wakeupFromFireAlarmStatus;
                        g_DeviceSendType = FireAlarmOn;
                        g_UsrSystemSendDecideDataFlag = true; // Mqtt den gönder
                    }
                }
                else if (diff < g_sNvsDeviceInfo.exitFireAlarmValue)
                {
                    if ((g_sAllSensorValues.alarmeventgroup >> 3) & 1)
                    {
                        m_AlarmGroupBits &= ~(1 << 3);
                        g_sAllSensorValues.alarmeventgroup = m_AlarmGroupBits;
                        __logse("Cancelled fire alarm");
                        g_sAllSensorValues.eWakeuptype = wakeupFromFireAlarmStatus;
                        g_DeviceSendType = FireAlarmOver;
                        g_UsrSystemSendDecideDataFlag = true; // Mqtt den gönder
                    }
                }
            }
        }
    }

    /// g_UsrSystemSendDataMissionFlag DATA basilmak uzere, GSM'e izin verilir.

    // Rutin Data Basilmasi Durumu
    uint32_t currentTime = UL_RtcGetTs();
    g_UsrSystemRoutineDataSendFlag = ((currentTime - g_UsrSystemRoutineDataSendTime) >= (g_UsrSystemRoutineDataSendIntervalTime - 1));
    if (g_UsrSystemRoutineDataSendFlag)
    {
        g_UsrSystemRoutineDataSendTime = currentTime;
        g_UsrSystemRoutineDataSendFlag = false;

        g_UsrSystemSendDataMissionFlag = true; // data basilacak (gsm'i aktiflestirme izni)
    }

    // Rutin olmayan (Alarm Durumu, ilk calisma yada akselometre) (Alarm Durumu Asagida)
    if (g_UsrSystemAccelometerWakeUpFlag || g_UsrProcessFirstRunFlag)
    {
        g_UsrSystemAccelometerWakeUpFlag = false;
        g_UsrProcessFirstRunFlag = false;

        g_UsrSystemSendDataMissionFlag = true; // data basilacak (gsm'i aktiflestirme izni) // reset anında buraya gel ve buradan da gsm'yi açıp dataları at
    }

    // Alarm Durumu (Hem Alarm Basmak icin Hem de Data yollamak icin izin verir)
    if (g_UsrSystemSendDecideDataFlag)
    {
        g_UsrSystemSendDecideDataFlag = false;
        g_sAllSensorValues.alarmeventgroup = m_AlarmGroupBits;

        g_UsrSystemSendAlarmMissionFlag = true; // alarm soz konusu (gsm'i aktiflestirme izni)
        g_UsrSystemSendDataMissionFlag = true;  // data basilacak (gsm'i aktiflestirme izni)
    }

    // ClearFlagProc(); KONUSUNU SENİNLE KONUSMALIYIZ EREN ABİ
}

void UsrProcessLedOpenAnimation(void)
{
    // device Status bayrağını düşün
    // if(UL_LedInitial(&g_sUsrSystemLedParameters))
    // {
    //     UL_LedPeripheral(enableLedPeripheral);
    //     g_UsrSystemLedInitialFlag = true;
    // }

    // if(g_UsrSystemLedInitialFlag)
    // {
    //     UL_LedOpenAnimation(250); // RGB blink with timeout
    //     g_UsrSystemLedInitialFlag = false;
    // }

    if (g_sNvsDeviceInfo.deviceStatus)
        UL_LedOpenAnimation(250); // RGB blink with timeout
    else
        UL_LedPassiveAnimation(250);
}

void UsrProcessDecideFirstState(void)
{
    if (g_sAllSensorValues.halleffectalarmstatus)
    {
        g_UsrSystemCoverLastStatus = g_sAllSensorValues.halleffectalarmstatus;
        g_sAllSensorValues.eWakeuptype = wakeupFromCoverAlarm;
    }
    else
    {
        g_sAllSensorValues.eWakeuptype = wakeupFromDisableStatus;
        g_sleepFlag = false;             // false      // usr_sleep.c  dosyası
        g_UsrProcessFirstRunFlag = true; // false      // usr_sensor.c dosyası
    }
    g_sensorsReadingFlag = true;
    // __logsi("ts: %d, cover:%d, wakeUpType: %d, sleepFlag: %d, sensorReadFlag: %d\n",g_sAllSensorValues.rtc ,g_UsrSystemCoverLastStatus, g_sAllSensorValues.eWakeuptype, g_sleepFlag, g_UsrProcessReadingStartFlag);
}

_io char *PreparePublishJsonDataProc(void)
{
    // sprintf((char*)m_JsonDataBuf, "{\"deviceReset\":%d,\"ts\":%d,\"sendType\":%d,\"mcu\":%s,\"module\":%s,\"sigq\":%d,\"ccid\":%s,\"temp\":%f,\"charge\":%f,\"fireAlarm\":%s,\"fullAlarm\":%s,\"coverAlarm\":%s,\"batteryCoverAlarm\":%s,\"fullnessAlarm\":%s,\"version\":%s,\"lat\":%d,\"lon\":%d,\"rawData\":%d}",
    // sprintf((char*)m_JsonDataBuf, "{\"deviceReset\":%d,\"ts\":%d,\"sendType\":%d,\"mcu\":%s,\"module\":%s,\"sigq\":%d,\"ccid\":%s,\"temp\":%.3f,\"charge\":%.3f,\"fireAlarm\":%s,\"fullAlarm\":%s,\"coverAlarm\":%s,\"batteryCoverAlarm\":%s,\"rawData\":%d}",
    sprintf((char *)m_JsonDataBuf, "{\"deviceReset\":%d,\"ts\":%d,\"sendType\":%d,\"mcu\":%s,\"module\":%s,\"sigq\":%d,\"ccid\":%s,\"temp\":%.3f,\"charge\":%.3f,\"fireAlarm\":%s,\"fullAlarm\":%s,\"coverAlarm\":%s,\"batteryCoverAlarm\":%s}",
            g_sNvsDeviceInfo.deviceStatus,
            g_sAllSensorValues.rtc,
            g_DeviceSendType,
            mcu,
            g_sUsrSystemGsmModuleInfo.moduleInfoBuf,
            g_sUsrSystemGsmModuleInfo.signal,
            g_sUsrSystemGsmModuleInfo.iccidBuf,
            g_sAllSensorValues.tempvalue,
            g_sAllSensorValues.batteryvoltage,
            ((g_sAllSensorValues.alarmeventgroup & 0x01) ? "true" : "false"),
            ((g_sAllSensorValues.alarmeventgroup & 0x02) ? "true" : "false"),
            ((g_sAllSensorValues.halleffectalarmstatus & 0x01) ? "true" : "false"),
            ((g_sAllSensorValues.halleffectalarmstatus & 0x02) ? "true" : "false"));
    //((g_sUsrSystemAlarmGroupParameters.fullnessSendFlag & 0x01) ? "true" : "false"),
    // version
    // lat,
    // lon,
    // g_sAllSensorValues.distancevalue);

    return m_JsonDataBuf;
}

_io void PrepareRequsetJsonDataProc(void)
{
    // m_eReceiveGsmDataCameOkFlg = false;
    int timeout = HAL_GetTick();
    while ((HAL_GetTick() - timeout) < 50000) // bu timeout un 5 dk'ya kadar yolu var
    {
        UsrSystemWatchdogRefresh();
        if (m_eReceiveGsmDataCameOkFlg)
        {
            //__logsi("%s", m_receiveGsmEndBuf);
            const char *deviceStatusKey = "#";
            //__logsw("Recevive : %s", m_receiveGsmEndBuf);
            const char *foundStatus = strstr((const char *)m_receiveGsmEndBuf, deviceStatusKey);
            if (foundStatus == NULL)
            {
                __logsi("deviceStatus not found\n");
            }
            else
            {
                __logsi("deviceStatus found\n");
            }
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }
}

_io void ClearFlagProc(void)
{
    // g_UsrSystemSendDecideDataFlag = false;
    // g_UsrSystemGsmModuleInitialFlag = false; // gsm flag
    g_UsrSystemGsmErrorFlag = false;
}

_io void AddDataToBufProc(void)
{
    _io int32_t _lastDistanceValue = 0;

    if (g_sAllSensorValues.distancevalue != _USR_SENSOR_DISTANCE_ERROR_VALUE && _lastDistanceValue != 0)
    {
        uint8_t tryCount = 0;
        uint8_t tryErrorCount = 0;
        int percentage = 0;
    start_step:;
        if (++tryCount > 3)
        {
            _lastDistanceValue = g_sAllSensorValues.distancevalue;
            goto reading_log_step;
        }
        float value = g_sAllSensorValues.distancevalue - _lastDistanceValue;
        if (value < 0)
            value *= -1;
        percentage = (int)((value * 100) / _lastDistanceValue);
        if (percentage >= _USR_PERCENTAGE_LIMIT)
        {
        sleep_step:;
            UsrSleepEnterSubSleep(6); // every wakeup time is ten second so waiting is 1 minute
            UsrSensorGetDistance();
            if (g_sAllSensorValues.distancevalue == _USR_SENSOR_DISTANCE_ERROR_VALUE)
            {
                if (++tryErrorCount > 3)
                    goto reading_log_step;
                else
                    goto sleep_step;
            }
            goto start_step;
        }
        else
            _lastDistanceValue = g_sAllSensorValues.distancevalue;
    }

reading_log_step:;
    if (m_logRawDataBufCnt == 0)
        m_startTs = UL_RtcGetTs();

    if (m_logRawDataBufCnt >= 144)
    {
        for (uint8_t i = 0; i < (m_logRawDataBufCnt - 1); i++)
            m_logRawDataBuf[i] = m_logRawDataBuf[i + 1];

        m_logRawDataBuf[143] = g_sAllSensorValues.distancevalue;
    }
    else
        m_logRawDataBuf[m_logRawDataBufCnt++] = g_sAllSensorValues.distancevalue;
}

_io bool GsmProc(void)
{
    __logsi("GSM SEND DATA OK!!!!");
    return true;
}