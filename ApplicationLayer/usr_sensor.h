#ifndef __USR_SENSOR_H
#define __USR_SENSOR_H

#include "usr_general.h"

#define _sensor_debug

#define _USR_SENSOR_DATA_READING_PERIOD (uint16_t)60
#define _USR_SESNOR_PERCENTAGE_LIMIT 10
#define _USR_SENSOR_ADC_ERROR_VALUE (float)-100
#define _USR_SENSOR_DISTANCE_ERROR_VALUE (int)-1
#define _USR_SENSOR_BATTERY_VOLTAGE_ERROR_CONTROL(x) (((float)x > 7.1) || ((float)x < 6.3))
#define _USR_SENSOR_DISTANCE_ERROR_VALUE -100

typedef struct S_SENSOR_ALL_VALUES_TAG
{
    int rtc;

    bool adcdataokflag; //// to show adc data ok flg

    float tempvalue;
    bool temperatureSensorErrorFlag;

    float batteryvoltage;
    uint8_t batteryvoltagepercentage;
    bool batteryvoltageerrorflag;

    int32_t distancevalue;
    bool distanceStatusFlag;
    bool distancedataokflag; //// to show distance data ok flag
    bool distancesensorerrorflag;

    uint8_t halleffectalarmstatus; // hall effect alarm status, battery cover and top cover
    uint8_t alarmeventgroup;       // ilk bit fullness alarm group, ikinci bit full alarm limit

    EWakeuptype eWakeuptype; //// to show device wake up from related source

    bool senddataflag; //// to show send gsm data ok flag
} S_SENSOR_ALL_VALUES;

void UsrSensorGetValues(void);
void UsrSensorGetAdcAndHalleffectValues(void);
int UsrSensorGetDistance(void);
void UsrSensorGetHalleffectStatusDirectly(void);
void UsrSensorHallEffectPinStatus(void);
void UsrSensorHallEffectGivePower(void);
void UsrSensorDistancePrepareMesaurement(void);
void UsrSensorAdcPrepareMeasurement(void);

extern bool g_sensorsReadingFlag;
extern S_SENSOR_ALL_VALUES g_sAllSensorValues;
extern uint32_t g_sensorReadTs;

#endif
