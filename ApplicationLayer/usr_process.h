#ifndef __USR_PROCESS_H
#define __USR_PROCESS_H

#include "usr_general.h"

// yeni termal
#define _fire_alarm_log_count 6

void UsrProcess(void);
void UsrProcessGsmModuleOpenProcess(void);
void UsrProcessDecideFirstState(void);
void UsrProcessAlarmProcess(void);
void UsrProcessLedOpenAnimation(void);

/*
typedef enum
{
    enablePreFullnessAlarmSendDecideData,
    disablePreFullnessAlarmSendDecideData,
    enableFullAlarmLimitSendDecideData,
    disableFullAlarmLimitSendDecideData,
    enableSameHallEffectSensorSendDecideData,
    enableDifferentHallEffectSensorSendDecideData, 
}EDeviceSendDataAlarmProcess;
*/

typedef enum
{
    //enablePreFullnessAlarmSendDecideData,
    tankJustCleanedAlarmSendDecideData,
    tankIsFullNowAlarmLimitSendDecideData,
    tankIsNotFullNowAlarmLimitSendDecideData,
    //enableSameHallEffectSensorSendDecideData,
    //enableDifferentHallEffectSensorSendDecideData,
    allCoversOpenAlarmSendDecideData,
    allCoversClosedAlarmSendDecideData,
    batteryCoverOpenAlarmSendDecideData,
    topCoverOpenAlarmSendDecideData,
    FireAlarmOn,
    FireAlarmOver,
    deviceResetDeviceSendDataAlarmProcess,
    dummyDataSendDeviceSendDataAlarmProcess,
}EDeviceSendDataAlarmProcess;
/// butun alarm case'leri tek bir degiskende enum olarak yapilarak, aynı anda iki alarm sebebi varsa bunlardan sadece son check edilen gecerli alarm sebebinin gozukmesini goze aldik.
/// kullanilmayanlar kaldirilacak !! orn. enablePreFullnessAlarmSendDecideData


typedef struct S_AYBERK_ALARM_TYPE_TAG       // mevcut durumu ozetleyen flaglar, eren abinin isini kolaylastırmak icin bir araya toplandi
{
    bool isTankFull;                        // tank dolu mu
    bool isTankFullChanged;                 // tank'in full'lük durumu degistiginde 1 periyot boyunca 1 kalır. Bunun bir periyot boyunca 1 olması demek, isTankFull alarm olarak gidecek demektir.
    bool isTankCleaned;                     // tank bosaltildi mi (bosaltildiginda 1 periyot boyunca 1 kalır). Bunun bir periyot boyunca 1 olması demek, Çöp değişti alarmı gidecek demektir.          
    
}S_AYBERK_ALARM_TYPE;


#endif
