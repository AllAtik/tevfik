#include "usr_general.h"

#define _c(x)            ((const char *)x)
#define _size(x)         strlen((const char *)x)
#define LOCALPOSITION    "UFS"

_io bool ModuleSendCommandAndGetResponseProc(const char *f_pcCommand, const char *f_pcResponse, uint8_t f_tryCount, uint32_t f_timeout);
_io bool ModuleSendCommandWithLenAndGetResponseProc(const uint8_t *f_pcCommand, uint16_t f_len, const char *f_pcResponse, uint8_t f_tryCount, uint32_t f_timeout);
_io bool MqttConnectionProc(const S_GSM_MQTT_CONNECTION_PARAMETERS *f_pGsmMqttConnectionParameters);
_io void UartParserProc(void);
_io void ClearUartProc(void);
_io bool TcpSendDataProc(const uint8_t *f_pData, uint16_t f_len);
_io int TcpGetRawDataProc(void);
_io int TcpGetDataProc(unsigned char *f_pData, int f_len);
_io void TcpGetStoredDataTriggerReadProc(uint32_t *f_pTimerCnt);
_io void TcpGetStoredDataProc(void);
_io void TcpConnectionStatus(void);
_io void MqttKeepaliveProc(void);
_io int ModuleListenResultProc(const char *f_pRes, uint32_t f_timeout);
_io void ModuleResetProc(void);
_io void UsrSleepGpioOutPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate);

_io S_GSM_PARAMETERS                  m_sGsmParameters;
//_io S_GSM_FTP                         m_sGsmFtpParameters;
_io S_GSM_MQTT_PARAMETERS             m_sGsmMqttParameters;
_io S_GSM_MQTT_CONNECTION_PARAMETERS  m_sGsmMqttConnectionParameters;
//_io S_GSM_APN_PARAMETERS              m_sGsmApnParameters;
_io S_GSM_MODULE_INFO                 m_sGsmModuleInfoParameters;

#define _USR_GSM_UART_RAW_CHANNEL     m_sGsmParameters.pUart->Instance

_io uint8_t m_receiveGsmBuf[_gsm_receive_buffer_size];         // esas deger _gsm_receive_buffer_size, 256 yaziliydi
uint8_t m_receiveGsmEndBuf[_gsm_receive_buffer_size];          // esas deger _gsm_receive_buffer_size, 256 yaziliydi
_io uint8_t m_transmitGsmBuf[_gsm_trasbuffer_buffer_size];     // esas deger _gsm_receive_buffer_size, 256 yaziliydi   // GSM'ye yolla
_io uint8_t m_tcpGsmBuf[_gsm_receive_buffer_size];

_io uint16_t m_receiveGsmUartCnt;
_io uint16_t m_receiveGsmUartCnt;
_io uint16_t m_tcpGsmUartCnt;
_io int m_tcpCurrentLen = 0;

bool m_eReceiveGsmDataCameOkFlg; // _io varadi
bool m_eReceiveGsmDataCameFlg;

_io bool m_eMqttConnectionOkFlg;
_io uint32_t m_mqttKeepAliveTime = 0;

void UL_GsmModuleInitial(S_GSM_PARAMETERS *f_pParam)
{
    m_sGsmParameters = *f_pParam;

    m_sGsmParameters.pUart->Instance->CR1 |= ((uint32_t)0x00000010); ////parity error, idle error
    m_sGsmParameters.pUart->Instance->CR3 |= ((uint32_t)0x00000001); ////FE , ORE , NF error

    HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);

    HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.powerKeyPin, (GPIO_PinState)m_sGsmParameters.powerKeyPinEnableStatus); 
    HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.resetPin,    (GPIO_PinState)m_sGsmParameters.powerPinEnableStatus);
    HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.gsmProcessMcuPin, (GPIO_PinState)m_sGsmParameters.gsmProcessMcuStatus);
    if (m_sGsmParameters.eModuleType != quectelM65GsmModule && m_sGsmParameters.eModuleType != quectelM66GsmModule)
        HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.resetPin, (GPIO_PinState)!m_sGsmParameters.resetPinEnableStatus); 

    _gsm_delay(500);
}


void UL_GsmModuleDeInitial(void)
{
    if (m_sGsmParameters.pUart != NULL)
    {
        HAL_UART_Abort_IT(m_sGsmParameters.pUart);
        HAL_UART_DeInit(m_sGsmParameters.pUart);
    }
    HAL_GPIO_WritePin(m_sGsmParameters.pPowerPort,    m_sGsmParameters.powerPin,    (GPIO_PinState)!m_sGsmParameters.powerPinEnableStatus);
    HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.powerKeyPin, (GPIO_PinState)!m_sGsmParameters.powerKeyPinEnableStatus);
    HAL_GPIO_WritePin(m_sGsmParameters.pResetPort,    m_sGsmParameters.resetPin,    (GPIO_PinState)!m_sGsmParameters.resetPinEnableStatus);
}


void UL_GsmModulePeripheral(EGsmPeripheral f_eControl)
{
    if(f_eControl == enableGsmPeripheral)
    {
        _USR_GSM_UART_RAW_INIT_FUNC();
        _USR_GSM_UART_RAW_CHANNEL->ICR =  0xFFFFFFFF;
        _USR_GSM_UART_RAW_CHANNEL->CR1 |= 0x00000010;
        _USR_GSM_UART_RAW_CHANNEL->CR3 |= 0x00000001;
        HAL_UART_AbortReceive_IT(&_USR_GSM_UART_CHANNEL);
        HAL_UART_Receive_IT(&_USR_GSM_UART_CHANNEL, m_receiveGsmBuf, 1024);
        _USR_GSM_MAIN_POWER_CONTROL(f_eControl);
        _gsm_delay(1000);
        _USR_GSM_POWER_CONTROL(f_eControl);
        _gsm_delay(250);
        _USR_GSM_POWERKEY_CONTROL(f_eControl);
    }
    else
    {
        _USR_GSM_UART_RAW_DEINIT_FUNC();              // _USR_GSM_UART_RAW_DEINIT_FUNC() yaziyordu
        _USR_GSM_MAIN_POWER_CONTROL(f_eControl);
        _USR_GSM_POWER_CONTROL(f_eControl);
        _USR_GSM_POWERKEY_CONTROL(f_eControl);
        UsrSleepGpioOutPins(GSM_RX_GPIO_Port, GSM_RX_Pin, GPIO_PIN_RESET);
        UsrSleepGpioOutPins(GSM_TX_GPIO_Port, GSM_TX_Pin, GPIO_PIN_RESET);
        UsrSleepGpioOutPins(SIM_DETECT_POWER_GPIO_Port, SIM_DETECT_POWER_Pin, GPIO_PIN_RESET);
        UsrSleepGpioOutPins(SIM_DETECT_GPIO_Port, SIM_DETECT_Pin, GPIO_PIN_RESET);
    }
    UsrSleepGpioOutPins(GSM_PROCESS_STATUS_MCU_GPIO_Port, GSM_PROCESS_STATUS_MCU_Pin, GPIO_PIN_RESET);
}


bool UL_GsmModuleCheck(void)
{
    if (!ModuleSendCommandAndGetResponseProc("AT\r", "\r\nOK\r\n", 20, 1000))   // 10, 1000
        return false;
    if (!ModuleSendCommandAndGetResponseProc("ATE0\r", "\r\nOK\r\n", 3, 1000))
        return false;

    return true;
}


bool UL_GsmModuleGetInfo(S_GSM_MODULE_INFO *f_pInfo)
{
    m_sGsmModuleInfoParameters = *f_pInfo;
    int tryCount = 0;
start_step:;
    if (++tryCount > 20)
    {
        return false;
    }
    
    bool checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+CSQ\r", "\r\nOK\r\n", 5, 300))
    {
        int val1, val2;
        if (sscanf(_c(m_receiveGsmEndBuf), "%*[^:]: %d,%d%*[^\r\n]", &val1, &val2) == 2)
        {
            if (val1 >= 0 && val1 <= 31)
            {
                f_pInfo->signal = val1;
                checkFlg = true;
            }
        }
    }
    else
    {
        checkFlg = false;
    }

    if (!checkFlg)
    {
        _gsm_delay(1000);
        goto start_step;
    }

    checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("ATI\r", "\r\nOK\r\n", 5, 3000))
    {
        const char *ptr = strstr(_c(m_receiveGsmEndBuf), "Revision");
        if (ptr != NULL)
        {
            memset((void *)f_pInfo->moduleInfoBuf, 0, 64);
            if (sscanf(_c(ptr), "Revision: %[^\r\n]", f_pInfo->moduleInfoBuf) == 1)
            {
                checkFlg = true;
            }
        }
    }
    else
    {
        checkFlg = false;
    }

    if (!checkFlg)
        goto start_step;

    checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+CGSN\r", "\r\nOK\r\n", 5, 3000))
    {
        memset((void *)f_pInfo->imeiBuf, 0, 32);
        if (sscanf(_c(m_receiveGsmEndBuf), "\r\n%[^\r\n]", f_pInfo->imeiBuf) == 1)
        {
            checkFlg = true;
        }
    }
    else
    {
        checkFlg = false;
    }

    if (!checkFlg)
        goto start_step;

    checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+QCCID\r", "\r\nOK\r\n", 5, 3000))
    {
        memset((void *)f_pInfo->iccidBuf, 0, 32);
        if (sscanf(_c(m_receiveGsmEndBuf), "\r\n%[^\r\n]", f_pInfo->iccidBuf) == 1)
        {
            checkFlg = true;
        }
    }
    else
    {
        checkFlg = false;
    }

    if (!checkFlg)
        goto start_step;
    

    return true;
}


bool UL_GsmModuleMqttInitial(const S_GSM_MQTT_CONNECTION_PARAMETERS *f_pcParameter)
{
    m_sGsmMqttConnectionParameters = *f_pcParameter;

    int tryCount = 0;
start_step:;
    if (++tryCount > 20)
    {
#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttInitial:Try limit error\n");
#endif
        return false;
    }

    bool checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+CREG?\r", "\r\nOK\r\n", 5, 1000))
    {
        int val1, val2;
        if (sscanf(_c(m_receiveGsmEndBuf), "%*[^:]: %d,%d%*[^\r\n]", &val1, &val2) == 2)
        {
            if (val2 == 1)
            {
                checkFlg = true;
            }
        }
    }
    if (!checkFlg)
    {
        _gsm_delay(1000);
        goto start_step;
    }

    if(m_sGsmParameters.eModuleType == cavliGsmModules)
    {
        sprintf((char *)m_transmitGsmBuf, "AT+CGDCONT=2,\"IP\",\"%s\"\r", f_pcParameter->sGsmApn.name);
        if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto start_step;
        }

        if (!ModuleSendCommandAndGetResponseProc("AT+CGACT=1,2\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto start_step;
        }

        tryCount = 0;
    second_step:;
        if (++tryCount > 20)
        {
#ifdef _gsm_debug
            __logsw("UL_GsmModuleMqttInitial:Second step try limit error\n");
#endif
            return false;
        }

        checkFlg = false;
        if (ModuleSendCommandAndGetResponseProc("AT+CGACT?\r", "\r\nOK\r\n", 5, 1000))
        {
            int val1, val2;
            if (sscanf((const char *)m_receiveGsmEndBuf, "%*[^: ]: %d,%d[^\r\n]", &val1, &val2) == 2)
            {
                if (val2 == 1)
                    checkFlg = true;
            }
        }

        if (!checkFlg)
        {
            _gsm_delay(1000);
            goto second_step;
        }

#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttInitial:Raw ip initial ok\n");
#endif        
    }

    tryCount = 0;
    bool tcpConnectFlg = false;
third_step:;
    if (++tryCount > 20)
    {
#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttInitial:Third step try limit error\n");
#endif
        return false;
    }

    if (m_sGsmParameters.eModuleType == cavliGsmModules)
    {
        if (!ModuleSendCommandAndGetResponseProc("AT+CIPMUX=0\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }

        if (!ModuleSendCommandAndGetResponseProc("AT+CIPHEAD=1\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }
        checkFlg = false;
        sprintf((char *)m_transmitGsmBuf, "AT+CIPSTART=\"TCP\",\"%s\",%d\r", f_pcParameter->sMqtt.urlBuf, f_pcParameter->sMqtt.port);
        if (ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 5000))
        {
            if (strstr((const char *)m_receiveGsmEndBuf, "CONNECT OK") != NULL)
                checkFlg = true;
            else if (strstr((const char *)m_receiveGsmEndBuf, "ALREADY CONNECT") != NULL)
                checkFlg = true;
        }

        if (!checkFlg)
        {
            _gsm_delay(1000);
            goto third_step;
        }

        if (!ModuleSendCommandAndGetResponseProc("AT+CIPRXGET=1\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }
    }
    else
    {
        //memset((void *)m_transmitGsmBuf, 0, 256);
        sprintf((char *)m_transmitGsmBuf, "AT+CGDCONT=3,\"IP\",\"%s\"\r", f_pcParameter->sGsmApn.name);
        _gsm_watchdog();   // Eren yazdı
        if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }
 
        if (!ModuleSendCommandAndGetResponseProc("AT+CGACT=1,1\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }

        if (!ModuleSendCommandAndGetResponseProc("AT+CGPADDR=1\r", "\r\nOK\r\n", 5, 1000))
        {
            _gsm_delay(1000);
            goto third_step;
        }

        if (!ModuleSendCommandAndGetResponseProc("AT+CGREG=1\r", "\r\nOK\r\n", 5, 3000))
        {
            _gsm_delay(1000);
            goto third_step;
        }    

        checkFlg = false;
        if (ModuleSendCommandAndGetResponseProc("AT+CGACT?\r", "\r\nOK\r\n", 5, 3000))
        {
            int val1, val2;
            if (sscanf((const char *)m_receiveGsmEndBuf, "%*[^: ]: %d,%d[^\r\n]", &val1, &val2) == 2)
            {
                if (val2 == 1)
                    checkFlg = true;
            }
        }

        if (!checkFlg)
        {
            _gsm_delay(1000);
            goto third_step;
        } 

        if(tcpConnectFlg)
        {
            if (!ModuleSendCommandAndGetResponseProc("AT+QICLOSE\r", "\r\nOK\r\n", 5, 3000))
            {
                _gsm_delay(1000);
                goto third_step;
            } 
            tcpConnectFlg = false;
        }
  

        if (!ModuleSendCommandAndGetResponseProc("AT+QINDI=1\r", "\r\nOK\r\n", 5, 3000))
        {
            _gsm_delay(1000);
            goto third_step;
        }
        _gsm_watchdog();    // Eren yazdi
        // memset((void *)m_transmitGsmBuf, 0, 256);
        sprintf((char *)m_transmitGsmBuf, "AT+QIOPEN=\"TCP\",\"%s\",%d\r", f_pcParameter->sMqtt.urlBuf, f_pcParameter->sMqtt.port);

        if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 1, 120000))
        {
    #ifdef _gsm_debug
            __logse("UL_GsmModuleMqttInitial:Tcp connection error step 1\n");
    #endif
            return false;
        }

        // if (!ModuleListenResultProc("\r\n+QIOPEN: 0,0\r\n", 30000))
        if (!ModuleListenResultProc("CONNECT", 30000))
        {
    #ifdef _gsm_debug
            __logse("UL_GsmModuleMqttInitial:Tcp connection error step 2\n");
    #endif
            return false;
        }
    }

#ifdef _gsm_debug
    __logsi("UL_GsmModuleMqttInitial:Tcp connection ok\n");
#endif

    if (!MqttConnectionProc(f_pcParameter))
    {
#ifdef _gsm_debug
        __logse("UL_GsmModuleMqttInitial:Mqtt connection error\n");
#endif
        return false;
    }

    m_eMqttConnectionOkFlg = true;
    UL_GsmModuleMqttConnectionStatusCallback(connectGsmMqttConnectionStatus);

    return true;
}


bool UL_GsmModuleMqttSubcribeTopic(const char *f_cpTopic, int f_qos)
{
    if (!m_eMqttConnectionOkFlg)
    {
#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttSubcribeTopic:Mqtt connection not ok\n");
#endif
        return false;
    }

    bool res = false;

    uint8_t *ptr = (uint8_t *)_gsm_malloc(_gsm_subcribe_buffer);   // esas deger 1024, 512 eren yazdi
    if (ptr != NULL)
    {
        MQTTString topicString = MQTTString_initializer;
        topicString.cstring = (char *)f_cpTopic;

        int len = MQTTSerialize_subscribe(ptr, _gsm_subcribe_buffer, 0, 1, 1, &topicString, &f_qos);    // esas deger 1024
        if (TcpSendDataProc((const uint8_t *)ptr, len))
        {
            int timeout = HAL_GetTick();
            while ((HAL_GetTick() - timeout) < 5000)
            {
                if (TcpGetRawDataProc() == -1)
                {
                    _gsm_free(ptr);
                    return false;
                }
                if (MQTTPacket_read(ptr, _gsm_subcribe_buffer, TcpGetDataProc) == SUBACK)   // esas deger 1024
                {
                    unsigned short submsgid;
                    int subcount;
                    int grantedQos;

                    MQTTDeserialize_suback(&submsgid, 1, &subcount, &grantedQos, ptr, _gsm_subcribe_buffer);     // esas deger 1024

                    if (grantedQos != 0)
                    {
#ifdef _gsm_debug
                        __logsw("UL_GsmModuleMqttSubcribeTopic:Subcribe ack error\n");
#endif                        
                        _gsm_free(ptr);
                        return false;
                    }
                    else
                    {
                        res = true;
                        break;
                    }
                }
            }
        }
        _gsm_free(ptr);
    }
    return res;
}


bool UL_GsmModuleMqttPublishTopic(const char *f_cpTopic, const char *f_cpData, int f_qos, int f_retain)
{
    if (!m_eMqttConnectionOkFlg)
    {
#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttPublishTopic:Mqtt connection not ok\n");
#endif
        return false;
    }

    bool res = false;
    uint8_t *ptr = (uint8_t *)_gsm_malloc(_gsm_subcribe_buffer);   // esas deger 1024
    if (ptr != NULL)
    {
        MQTTString topicString = MQTTString_initializer;
        topicString.cstring = (char *)f_cpTopic;
        int len = MQTTSerialize_publish(ptr, _gsm_subcribe_buffer, 0, f_qos, 0, 0, topicString, (unsigned char *)f_cpData, _size(f_cpData));
        if (TcpSendDataProc((const uint8_t *)ptr, len))
            res = true;
        _gsm_free(ptr);
    }
    return res;
}


void UL_GsmModuleMqttGeneral(void)
{
   if (!m_eMqttConnectionOkFlg)
        return;

    _io uint32_t _mqttCheckDataTimeout = 0;
    if (_mqttCheckDataTimeout == 0)
        _mqttCheckDataTimeout = HAL_GetTick();

    TcpGetRawDataProc();
    TcpGetStoredDataTriggerReadProc(&_mqttCheckDataTimeout);
    MqttKeepaliveProc();

    if (m_tcpGsmUartCnt != 0)
    {
        uint8_t *ptr = (uint8_t *)_gsm_malloc(128);                       // 128 Eren yazdi,   // esas yazan deger _gsm_mqtt_general_func_buffer
        if (ptr != NULL)
        {                                                                 // 128 den yukari reset yiyor
            memset((void *)ptr, 0, 128);                                  // 128 Eren yazdi    // esas yazan deger _gsm_mqtt_general_func_buffer
            if (MQTTPacket_read(ptr, 128, TcpGetDataProc) == PUBLISH)     // 128 Eren yazdi    // esas yazan deger _gsm_mqtt_general_func_buffer
            {
                MQTTString received;
                unsigned char dup;
                int qos;
                unsigned char retained;
                unsigned short msgid;
                int payloadLenIn;
                unsigned char *payloadIn;

                int res = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &received, &payloadIn, &payloadLenIn, ptr, _gsm_mqtt_general_func_buffer);
                if (res == 1)
                {
                    if (received.lenstring.len >= _gsm_mqtt_max_topic_size || payloadLenIn >= _gsm_mqtt_max_payoad_size)
                    {
                        m_tcpCurrentLen = 0;
                        m_tcpGsmUartCnt = 0;
                        TcpGetStoredDataProc();
                        _gsm_free(ptr);
                        return;
                    }
                    _mqttCheckDataTimeout = HAL_GetTick();
                    UL_GsmModuleMqttSubcribeDataCallback(received.lenstring.data, received.lenstring.len, (const char*)payloadIn, payloadLenIn);
                }
            }
            _gsm_free(ptr);
        }
    }
}


bool UL_GsmModuleGetResponse(const uint8_t *f_pcCommand, uint16_t f_len, const char *f_pcResponse, uint8_t f_tryCount, uint32_t f_timeout)
{
    bool res = false;
    int tryCount = 0;

start_step:;
    if (++tryCount > f_tryCount)
    {
#ifdef _gsm_debug
        __logsw("ModuleSendCommandWithLenAndGetResponseProc:At command error \n");
#endif
        return false;
    }
    // m_receiveGsmUartCnt = 0;
    ClearUartProc();
    // HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    // HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);

    m_eReceiveGsmDataCameOkFlg = false;
    // HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)f_pcCommand, f_len, f_len);

    int timeout = HAL_GetTick();

    while ((HAL_GetTick() - timeout) < f_timeout)
    {
        UartParserProc();

        if (m_eReceiveGsmDataCameOkFlg)
        {
            __logsw("Recevive : %s", m_receiveGsmEndBuf);
            if (strstr((const char *)m_receiveGsmEndBuf, f_pcResponse))
            {
                res = true;
                break;
            }
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }
    if (!res)
        goto start_step;
    return res;
}



void UL_GsmModuleMqttClosed(void)
{
    m_eMqttConnectionOkFlg = false;
    if(m_sGsmParameters.eModuleType == cavliGsmModules)
    {
        ModuleSendCommandAndGetResponseProc("AT+CIPCLOSE\r", "OK", 3, 1000);
    }
    else
    {
        ModuleSendCommandAndGetResponseProc("AT+QICLOSE\r", "\r\nOK\r\n", 3, 1000);
    }
}


void UL_GsmModuleUartInterruptCallback(void)
{
    if (m_sGsmParameters.pUart == NULL)
        return;
    if (m_sGsmParameters.pUart->Instance->ISR & ((uint32_t)0x1f))
    {
        if ((m_sGsmParameters.pUart->Instance->ISR & 0x10) == 0)
        {
            HAL_UART_Receive_IT(m_sGsmParameters.pUart, (uint8_t *)m_receiveGsmBuf, 1024);
        }
        m_sGsmParameters.pUart->Instance->ICR |= 0x1f;
        m_eReceiveGsmDataCameFlg = true;              // Bu flag UartParserProc fonksiyonuna gidecek
    }
}


void UL_GsmModuleHardwareReset(void)
{
    ModuleResetProc();
    _gsm_delay(5000);
}


int UL_GsmModuleReadFile(const char *f_cpFileName, uint32_t f_startIndex, uint32_t f_size, uint8_t *f_pData)
{
#ifdef _4GGSM
    int res = -1;

    sprintf(m_transmitGsmBuf, "AT+FSRDBLOCK=\"%s\",%d,%d\r", f_cpFileName, f_startIndex, f_size);
    ClearUartProc();
    HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)m_transmitGsmBuf, strlen((const char *)m_transmitGsmBuf), 5 * strlen((const char *)m_transmitGsmBuf));

    int timeout = HAL_GetTick();
    sprintf(m_transmitGsmBuf, "\r\n+FSRDBLOCK: %s,%%d,", f_cpFileName);
    while ((HAL_GetTick() - timeout) < 2000)
    {
        UartParserProc();
        int len;
        if (sscanf((const char *)m_receiveGsmEndBuf, (const char *)m_transmitGsmBuf, &len) == 1)
        {
            char tempBuf[64];
            sprintf(tempBuf, "\r\n+FSRDBLOCK: %s,%d,", f_cpFileName, len);
            int totalLenght = _size(tempBuf) + len + 8; //// \r\n\r\nOK\r\n
            if (m_receiveGsmUartCnt >= totalLenght)
            {
                char *ptr;
                if ((ptr = strstr(_c(m_receiveGsmEndBuf), _c(tempBuf))) != NULL)
                {
                    int tempBuflen = _size(tempBuf);
                    for (uint16_t i = 0; i < len; i++)
                        f_pData[i] = ptr[tempBuflen + i];
                    res = len;
                    break;
                }
            }
        }
    }

    return res;
#endif
    int val1, val2, val3, new_val1, new_val2;

#ifdef _gsm_debug
    __logsi("UL_GsmModuleReadFile:Read File  connection ok\n");
#endif

    sprintf((char*)m_transmitGsmBuf, "AT+QFOPEN=\"%s\"\r", f_cpFileName);
    if(ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 2, 3000))
    {
        if(sscanf(_c(m_receiveGsmEndBuf), "%*[^:]: %d%*[^\r\n]", &val1) == 1)
        {
            new_val1 = val1;
        }
    }

    // sprintf((char*)m_transmitGsmBuf, "AT+QFWRITE=%d,%d,10\r", new_val1, f_size);
    // if(ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nCONNECT\r\n", 5, 3000))
    // {
    //     memset((void *)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
    //     memcpy((void *)m_transmitGsmBuf, (const void*)f_pData, sizeof(m_transmitGsmBuf));
    //     if(!ModuleSendCommandWithLenAndGetResponseProc((const uint8_t*)m_transmitGsmBuf, strlen((const char*)m_transmitGsmBuf), "\r\nQFWRITE\r\n", 3, 1000))
    //         // return false;
        
    //     if(sscanf(_c(m_receiveGsmEndBuf), "%*[^:]: %d,%d%*[^\r\n]", &val2, &val3) == 2)
    //     {
    //         new_val2 = val3;
    //     }
    // }

    memset((void *)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
    sprintf((char*)m_transmitGsmBuf, "AT+QFSEEK=%d,%d,0\r", new_val1, f_startIndex);
    if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 3000))
      return false;
        
    sprintf((char*)m_transmitGsmBuf, "AT+QFREAD=%d\r", new_val1);
    if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 3000))
      return false;

    sprintf((char*)m_transmitGsmBuf, "AT+QFCLOSE=%d\r", new_val1);
    if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 3000))
      return false;


    return true;
}


int UL_GsmModuleGetFileTotalLen(const char *f_cpFileName)
{
#ifdef _4GGSM
    int res = -1;
    sprintf(m_transmitGsmBuf, "AT+FSLSTFILE=2,\"%s\"\r", f_cpFileName);
    ClearUartProc();
    HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)m_transmitGsmBuf, strlen((const char *)m_transmitGsmBuf), 5 * strlen((const char *)m_transmitGsmBuf));

    int timeout = HAL_GetTick();
    while ((HAL_GetTick() - timeout) < 2000)
    {
        int len;
        UartParserProc();
        if (sscanf((const char *)m_receiveGsmEndBuf, "\r\n+FSLSTFILE: %d\r\n\r\nOK\r\n", &len) == 1)
            return len;
    }
    return res;
#endif

    sprintf((char*)m_transmitGsmBuf, "AT+QFLDS=\"%s\"\r", LOCALPOSITION);
    if(!ModuleSendCommandAndGetResponseProc((const char*)m_transmitGsmBuf, "\r\nOK\r\n", 1, 3000));

    memset((void*)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
    sprintf((char*)m_transmitGsmBuf, "AT+QFLST\r");
    if(!ModuleSendCommandAndGetResponseProc((const char*)m_transmitGsmBuf, "\r\nOK\r\n", 1, 3000));

    return 1;
}


bool UL_GsmModuleFtpFileDownload(const S_GSM_FTP *f_cpFtp)
{
#ifdef _4GGSM
    int tryCount = 0;
start_step:;
    if (++tryCount > 20)
    {
// #ifdef _gsm_debug
//         __logsw("UL_GsmModuleMqttInitial:Try limit error\n");
// #endif
        return false;
    }

    if (!ModuleSendCommandAndGetResponseProc("AT+CEREG=0\r", "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

    bool checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+CREG?\r", "\r\nOK\r\n", 5, 1000))
    {
        if (m_sGsmParameters.eModuleType == cavliGsmModules)
        {
            int val1, val2;
            char buf1[16], buf2[16];
            if (sscanf((const char *)m_receiveGsmEndBuf, "%*[^: ]: %d,%d,\"%[^\"]\",\"%[^\"][^\r\n]", &val1, &val2, buf1, buf2) == 4)
            {
                if (val2 == 1)
                    checkFlg = true;
            }
        }
    }

    if (!checkFlg)
    {
        _gsm_delay(1000);
        goto start_step;
    }

    sprintf((char *)m_transmitGsmBuf, "AT+CGDCONT=2,\"IP\",\"%s\"\r", f_cpFtp->sGsmApn.name);
    if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

    if (!ModuleSendCommandAndGetResponseProc("AT+CGACT=1,2\r", "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

    sprintf(m_transmitGsmBuf, "AT^FTPOPEN=\"%s:%d\",\"%s\",\"%s\",1,180,0\r",
            f_cpFtp->urlBuf, f_cpFtp->port, f_cpFtp->userNameBuf, f_cpFtp->userPasswordBuf);

    if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 2, 30000))
        return false;

// #ifdef _gsm_debug
//     __logsw("UL_GsmModuleFtpFileDownload:Ftp connection ok\n");
// #endif

    sprintf(m_transmitGsmBuf, "AT^FTPDLSET=\"%s%s\"\r", f_cpFtp->filePathBuf, f_cpFtp->fileNameBuf);
    if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 2, 5000))
        return false;

    int tryCnt = 0;
try_step:;
    if (++tryCnt > 5)
        return false;

    UL_GsmModuleDeleteFile((const char *)f_cpFtp->fileNameBuf);
// #ifdef _gsm_debug
//     __logsw("UL_GsmModuleFtpFileDownload:Ftp start donwload\n");
// #endif

    bool downloadFlg = false;
    ClearUartProc();
    HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)"AT^FTPDL=1\r", strlen("AT^FTPDL=1\r"), 5 * strlen("AT^FTPDL=1\r"));
    int timeout = HAL_GetTick();
    int index = 0;
    do
    {
        if ((HAL_GetTick() - timeout) > 30000)
            goto try_step;

        UartParserProc();
        if (m_eReceiveGsmDataCameOkFlg)
        {
            if (strstr((const char *)m_receiveGsmEndBuf, "\r\n^FTPDL:2,0\r\n") != NULL)
            {
                if (index > 3)
                    break;
            }
            else if (strstr((const char *)m_receiveGsmEndBuf, "\r\n^FTPDL") != NULL)
            {
                index++;
// #ifdef _gsm_debug
//                 __logsw("UL_GsmModuleFtpFileDownload:Ftp packed donwload : %d\n", index);
// #endif
            }
            m_receiveGsmUartCnt = 0;
            m_eReceiveGsmDataCameOkFlg = false;
        }
    } while (!downloadFlg);

    return true;

#endif

#ifdef GSM_2G_MODULES
    m_sGsmFtpParameters = *f_cpFtp;

    int tryCount = 0;
start_step:;
    if (++tryCount > 20)
    {
#ifdef _gsm_debug
        __logsw("UL_GsmModuleMqttInitial:Try limit error\n");
#endif
        return false;
    }

    // _gsm_watchdog; // Eren yazdi
    if (!ModuleSendCommandAndGetResponseProc("AT+CGREG=1\r", "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;
    }    

    bool checkFlg = false;
    if (ModuleSendCommandAndGetResponseProc("AT+CREG?\r", "\r\nOK\r\n", 5, 1000))
    {
        int val1, val2;
        if (sscanf(_c(m_receiveGsmEndBuf), "%*[^:]: %d,%d%*[^\r\n]", &val1, &val2) == 2)
        {
            if (val2 == 1)
            {
                checkFlg = true;
            }
        }
    }

    if (!checkFlg)
    {
        _gsm_delay(1000);
        goto start_step;
    }   

    sprintf((char *)m_transmitGsmBuf, "AT+CGDCONT=3,\"IP\",\"%s\"\r", f_cpFtp->sGsmApn.name);
    if(!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;        
    }

    if (!ModuleSendCommandAndGetResponseProc("AT+CGACT=1,1\r", "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

    memset((void*)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
	sprintf((char*)m_transmitGsmBuf, "AT+QFTPUSER=\"%s\"\r", f_cpFtp->userNameBuf);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;        
    }

    memset((void*)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
    sprintf((char*)m_transmitGsmBuf, "AT+QFTPPASS=\"%s\"\r", f_cpFtp->userPasswordBuf);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 1000))
    {
        _gsm_delay(1000);
        goto start_step;         
    }

    memset((void*)m_transmitGsmBuf, 0, sizeof(m_transmitGsmBuf));
	sprintf((char*)m_transmitGsmBuf, "AT+QFTPOPEN=\"%s\",%d\r", f_cpFtp->urlBuf, f_cpFtp->port);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 2, 30000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

#ifdef _gsm_debug
    __logsi("UL_GsmModuleFtpFileDownload:Ftp connection ok\n");
#endif

	sprintf((char*)m_transmitGsmBuf, "AT+QFTPCFG=4,\"/UFS/\"\r");
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n",5 ,2000))
    {
        _gsm_delay(1000);
        goto start_step;          
    }

	sprintf((char*)m_transmitGsmBuf, "AT+QFTPPATH=\"%s\"\r", f_cpFtp->filePathBuf);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 2000))
    {
        _gsm_delay(1000);
        goto start_step;  
    }

    // UL_GsmModuleDeleteFile((const char *)f_cpFtp->fileNameBuf);
#ifdef _gsm_debug
    __logsi("UL_GsmModuleFtpFileDownload:Ftp start donwload\n");
#endif

	sprintf((char*)m_transmitGsmBuf, "AT+QFTPCFG=4,\"/UFS/\"\r");
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 2000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

	sprintf((char*)m_transmitGsmBuf, "AT+QFTPGET=\"%s\"\r", f_cpFtp->fileNameBuf);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 2000))
    {
        _gsm_delay(1000);
        goto start_step;
    }

    return true;
#endif
}


bool UL_GsmModuleDeleteFile(const char *f_cpFileName)
{
    if (m_sGsmParameters.eModuleType == cavliGsmModules)
        sprintf((char*)m_transmitGsmBuf, "AT+FSDELFILE=\"%s\"\r", f_cpFileName);
    else
        sprintf((char*)m_transmitGsmBuf, "AT+QFDEL=\"%s\"\r", f_cpFileName);
	if(!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "\r\nOK\r\n", 5, 1000))
        return false;
	
    return ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, "\r\nOK\r\n", 5, 1000);
}


_io bool ModuleSendCommandAndGetResponseProc(const char *f_pcCommand, const char *f_pcResponse, uint8_t f_tryCount, uint32_t f_timeout)
{
    bool res = false;
    int tryCount = 0;

start_step:;
    if (++tryCount > f_tryCount)
    {
#ifdef _gsm_debug
        __logsw("ModuleSendCommandAndGetResponseProc:At command error : %s\n", f_pcCommand);
#endif        
        return false;
    }
// m_receiveGsmUartCnt = 0;
    ClearUartProc();
    HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, 1024);
#ifdef _gsm_debug
    __logsw("ModuleSendCommandAndGetResponseProc:Transmit command [%d]: %s\n", strlen(f_pcCommand), f_pcCommand);
#endif
    m_eReceiveGsmDataCameOkFlg = false;
    memset((void *)m_receiveGsmEndBuf, 0, sizeof(m_receiveGsmEndBuf));
#ifdef _gsm_debug
    __logse("Command : %s", f_pcCommand);
#endif
    HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)f_pcCommand, strlen(f_pcCommand), 5 * strlen(f_pcCommand));
    HAL_UART_Receive_IT(m_sGsmParameters.pUart, (uint8_t *)m_receiveGsmEndBuf, sizeof(m_receiveGsmEndBuf));

    int timeout = HAL_GetTick();
    while ((HAL_GetTick() - timeout) < f_timeout)
    {
        UartParserProc();
        if (m_eReceiveGsmDataCameOkFlg)
        {
            #ifdef _gsm_debug
                __logsw("m_receiveGsmEndBuf: %s", m_receiveGsmEndBuf);
            #endif
            if (strstr((const char *)m_receiveGsmEndBuf, f_pcResponse))
            {
                res = true;
                break;
            }
            else if (strstr((const char *)m_receiveGsmEndBuf, "ERROR"))
            {
                res = false;
                break;
            }
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }
    if (!res)
    {
        goto start_step;
    }
    return res;
}


_io bool ModuleSendCommandWithLenAndGetResponseProc(const uint8_t *f_pcCommand, uint16_t f_len, const char *f_pcResponse, uint8_t f_tryCount, uint32_t f_timeout)
{
    bool res = false;
    int tryCount = 0;

start_step:;
    if (++tryCount > f_tryCount)
    {
#ifdef _gsm_debug
        __logsw("ModuleSendCommandWithLenAndGetResponseProc:At command error \n");
#endif
        return false;
    }
    // m_receiveGsmUartCnt = 0;
    ClearUartProc();
    // HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    // HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);

    m_eReceiveGsmDataCameOkFlg = false;
    HAL_UART_Transmit(m_sGsmParameters.pUart, (const uint8_t *)f_pcCommand, f_len, f_len);

    int timeout = HAL_GetTick();

    while ((HAL_GetTick() - timeout) < f_timeout)
    {
        UartParserProc();

        if (m_eReceiveGsmDataCameOkFlg)
        {
            #ifdef _gsm_debug
            __logsw("Recevive : %s", m_receiveGsmEndBuf);
            #endif
            if (strstr((const char *)m_receiveGsmEndBuf, f_pcResponse))
            {
                res = true;
                break;
            }
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }
    if (!res)
        goto start_step;
    return res;
}


_io bool MqttConnectionProc(const S_GSM_MQTT_CONNECTION_PARAMETERS *f_pcParameter)
{
    m_sGsmMqttConnectionParameters = *f_pcParameter;
    unsigned char buf[128] = {0};
    MQTTPacket_connectData mqtt_packet = MQTTPacket_connectData_initializer;
    mqtt_packet.MQTTVersion = 4;
    mqtt_packet.username.cstring = (char *)f_pcParameter->sMqtt.usernameBuf;
    mqtt_packet.password.cstring = (char *)f_pcParameter->sMqtt.passwordBuf;
    mqtt_packet.clientID.cstring = (char *)f_pcParameter->sMqtt.randomIdBuf;
    mqtt_packet.keepAliveInterval = f_pcParameter->sMqtt.keepAlive;
    mqtt_packet.cleansession = 1;
    m_mqttKeepAliveTime = mqtt_packet.keepAliveInterval;

    int len = MQTTSerialize_connect(buf, sizeof(buf), &mqtt_packet);

    if (TcpSendDataProc((const uint8_t *)buf, len))
    {
        int timeout = HAL_GetTick();
        while ((HAL_GetTick() - timeout) < 30000) // 30000
        {
            if (TcpGetRawDataProc() == -1)
            {
                return false;
            }
            if (MQTTPacket_read(buf, 128, TcpGetDataProc) == CONNACK)
            {
                unsigned char sessionResent, connack_rc;
                int res = MQTTDeserialize_connack(&sessionResent, &connack_rc, buf, 128);

                if (res != 1 || connack_rc != 0)
                {
#ifdef _gsm_debug
                    __logse("MqttConnectionProc:Mqtt connection ack error\n");
#endif
                    return false;
                }
                else
                    return true;
            }
        }
    }
    return false;
}


_io void ClearUartProc(void)
{
    m_eReceiveGsmDataCameFlg = false;
    m_eReceiveGsmDataCameOkFlg = false;
    m_receiveGsmUartCnt = 0;
    m_receiveGsmEndBuf[0] = '\0';
    HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, 1024);
}


_io void UartParserProc(void)
{
    if (m_eReceiveGsmDataCameFlg)
    {
        int currentUartCount = m_sGsmParameters.pUart->RxXferSize - m_sGsmParameters.pUart->RxXferCount;

        if ((m_receiveGsmUartCnt + currentUartCount) >= _gsm_receive_buffer_size)
        {
            m_receiveGsmUartCnt = 0;
#ifdef _gsm_debug
            __logsw("UartParserProc: Clear counter : %d-%d\n", m_receiveGsmUartCnt, currentUartCount);
#endif
            goto end_step;
        }
        memcpy((void *)&m_receiveGsmEndBuf[m_receiveGsmUartCnt], (const void *)m_receiveGsmBuf, currentUartCount);
        m_receiveGsmUartCnt += currentUartCount;
        m_receiveGsmEndBuf[m_receiveGsmUartCnt] = 0;           // Null ile sonlandır.
#ifdef _gsm_debug
        __logsw("Coming data : %s", m_receiveGsmEndBuf);
#endif        
        m_eReceiveGsmDataCameOkFlg = true;
    end_step:;
        HAL_UART_Abort_IT(m_sGsmParameters.pUart);
        HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);   // esas deger _gsm_receive_buffer_size, 1024 yaziliydi
        m_eReceiveGsmDataCameFlg = false;
    }
}


_io bool TcpSendDataProc(const uint8_t *f_pData, uint16_t f_len)
{
    if (m_sGsmParameters.eModuleType == cavliGsmModules)
        sprintf((char *)m_transmitGsmBuf, "AT+CIPSEND=%d\r", f_len);
    else
        sprintf((char *)m_transmitGsmBuf, "AT+QISEND=%d\r", f_len);
    if (!ModuleSendCommandAndGetResponseProc((const char *)m_transmitGsmBuf, ">", 1, 5000))
        return false;

    memcpy((void *)m_transmitGsmBuf, (const void *)f_pData, f_len);
    m_transmitGsmBuf[f_len] = 0x1a;
    if (!ModuleSendCommandWithLenAndGetResponseProc((const uint8_t *)m_transmitGsmBuf, f_len + 1, "SEND OK", 1, 5000))
    {
        #ifdef _gsm_debug
          __logse("usr_lib_gsm: TcpSendDataProc: send data error \n");
        #endif 
        return false;
    }
    return true;
}


_io int TcpGetRawDataProc(void)
{
    UartParserProc();
    if (m_receiveGsmUartCnt != 0)
    {
        if (m_sGsmParameters.eModuleType == cavliGsmModules)
        {
            if (strstr((const char *)&m_receiveGsmEndBuf, "\r\nCLOSED\r\n") != NULL)
            {
                m_eMqttConnectionOkFlg = false;
                UL_GsmModuleMqttConnectionStatusCallback(disconnectGsmMqttConnectionStatus);
                m_receiveGsmUartCnt = 0;
                return -1;
            }
            else if (strstr((const char *)&m_receiveGsmEndBuf, "\r\n+CIPRXGET\r\n") != NULL)
            {
                TcpGetStoredDataProc();
                m_receiveGsmUartCnt = 0;
                return 0;
            }            
        }
        else
        {
            if (strstr((const char *)&m_receiveGsmEndBuf, "\r\nCLOSED\r\n") != NULL)
            {
                m_eMqttConnectionOkFlg = false;
                UL_GsmModuleMqttConnectionStatusCallback(disconnectGsmMqttConnectionStatus);
                m_receiveGsmUartCnt = 0;
                return -1;
            }
            else if (strstr((const char *)&m_receiveGsmEndBuf, "+QIRDI") != NULL)
            {
                TcpGetStoredDataProc();
                m_receiveGsmUartCnt = 0;
                return 0;
            }
        }
    }
    return 0;
}


_io int TcpGetDataProc(unsigned char *f_pData, int f_len)
{
    uint8_t *ptr = (uint8_t *)f_pData;
    int r = 0;

    if (m_tcpGsmUartCnt != 0)
    {
        if (f_len >= m_tcpGsmUartCnt)
            f_len = m_tcpGsmUartCnt;

        r = f_len;
        for (int i = 0; i < r; i++)
            ptr[i] = m_tcpGsmBuf[m_tcpCurrentLen + i];

        m_tcpCurrentLen += r;

        if (m_tcpCurrentLen >= m_tcpGsmUartCnt)
        {
            m_tcpCurrentLen = 0;
            m_tcpGsmUartCnt = 0;
        }
    }
    return r;
}


_io void TcpGetStoredDataTriggerReadProc(uint32_t *f_pTimerCnt)
{
    if ((HAL_GetTick() - *f_pTimerCnt) > 30000)
    {
        *f_pTimerCnt = HAL_GetTick();
        TcpConnectionStatus();
        TcpGetStoredDataProc();
        m_receiveGsmUartCnt = 0;
    }
}


_io void TcpGetStoredDataProc(void)
{
    char *ptr;
    int val = 0;
    if (m_sGsmParameters.eModuleType == cavliGsmModules)
    {
        if (!ModuleSendCommandAndGetResponseProc((const char *)"AT+CIPRXGET=2,1024\r", "\r\n+IPD", 3, 1000))
            return;
        ptr = strstr((const char *)&m_receiveGsmEndBuf, "\r\n+IPD");
    }
    else
    {
        sprintf((char *)m_transmitGsmBuf, "AT+QIRD=0,1,0,1500\r");
        if (!ModuleSendCommandAndGetResponseProc(_c(m_transmitGsmBuf), "+QIRD:", 5, 3000))
            return;
        ptr = strstr((const char *)&m_receiveGsmEndBuf, "+QIRD:");
    }

    if (ptr != NULL)
    {
        if (m_sGsmParameters.eModuleType == cavliGsmModules)
        {
            if (sscanf((const char *)ptr, "%*[^,],%d:%*[]", &val) == 1)
            {
                if (val < _gsm_receive_buffer_size && val <= m_receiveGsmUartCnt && val != 0)
                {
                    char buf[32];
                    sprintf(buf, "\r\n+IPD,%d:", val);
                    ptr += strlen((const char *)buf);

                    if ((m_tcpGsmUartCnt + val) >= _gsm_receive_buffer_size)
                    {
                        m_tcpCurrentLen = 0;
                        m_tcpGsmUartCnt = 0;
                    }

                    for (int i = 0; i < val; i++)
                        m_tcpGsmBuf[m_tcpGsmUartCnt + i] = ptr[i];
                    m_tcpGsmUartCnt += val;
                }
                else
                {
                    m_tcpCurrentLen = 0;
                    m_tcpGsmUartCnt = 0;
                }
            }
            else
            {
                m_tcpCurrentLen = 0;
                m_tcpGsmUartCnt = 0;
            }            
        }
        else
        {
            if (sscanf((const char *)ptr, "%*[^,],TCP,%d\r\n", &val) == 1)
            {
                if (val < _gsm_receive_buffer_size && val <= m_receiveGsmUartCnt && val != 0)
                {
                    uint16_t m = 0;
                    for (; m <= m_receiveGsmUartCnt; m++)
                    {
                        if (*ptr == '\n')
                        {
                            ptr++;
                            break;
                        }
                        ptr++;
                    }
                    if (m == (m_receiveGsmUartCnt + 1))
                    {
                        m_tcpCurrentLen = 0;
                        m_tcpGsmUartCnt = 0;
                        return;
                    }
                    if ((m_tcpGsmUartCnt + val) >= _gsm_receive_buffer_size)
                    {
                        m_tcpCurrentLen = 0;
                        m_tcpGsmUartCnt = 0;
                    }

                    for (int i = 0; i < val; i++)
                        m_tcpGsmBuf[m_tcpGsmUartCnt + i] = ptr[i];
                    m_tcpGsmUartCnt += val;
                }
                else
                {
                    m_tcpCurrentLen = 0;
                    m_tcpGsmUartCnt = 0;
                }
            }
            else
            {
                m_tcpCurrentLen = 0;
                m_tcpGsmUartCnt = 0;
            }
        }

    }
}


_io void TcpConnectionStatus(void)
{
    _io uint8_t _connectionCheckCnt = 0;

    if (!m_eMqttConnectionOkFlg)
        return;

    if (_connectionCheckCnt > 10)
    {
        _connectionCheckCnt = 0;
        m_eMqttConnectionOkFlg = false;
        UL_GsmModuleMqttConnectionStatusCallback(disconnectGsmMqttConnectionStatus);
        return;
    }

    if (m_sGsmParameters.eModuleType == cavliGsmModules)
    {
        if (!ModuleSendCommandAndGetResponseProc((const char *)"AT+CIPSTATUS\r", "\r\nOK\r\n", 3, 1000))
        {
            ++_connectionCheckCnt;
            return;
        }

        if (strstr((const char *)m_receiveGsmEndBuf, "CONNECTED") != NULL)
        {
            _connectionCheckCnt = 0;
            return;
        }
        else if (strstr((const char *)m_receiveGsmEndBuf, "CLOSING") != NULL ||
                 strstr((const char *)m_receiveGsmEndBuf, "CLOSED") != NULL ||
                 strstr((const char *)m_receiveGsmEndBuf, "PDP DEACT") != NULL)
        {
            m_eMqttConnectionOkFlg = false;
            UL_GsmModuleMqttConnectionStatusCallback(disconnectGsmMqttConnectionStatus);
            m_receiveGsmUartCnt = 0;
        }
        else
            ++_connectionCheckCnt;
    }
    else
    {
        if (!ModuleSendCommandAndGetResponseProc((const char *)"AT+QISTAT\r", "\r\nOK\r\n", 3, 1000))
        {
            ++_connectionCheckCnt;
            return;
        }
// Bakılmalı !
        char *ptr = strstr((const char *)m_receiveGsmEndBuf, "TCP");
        if (ptr != NULL)
        {
            ptr += 6; // to show "TCP","
            int remotePort = 0, localePort = 0, conStat = 0;
            if (sscanf((const char *)ptr, "%*[\"]\",%d,%d,%d,%*[]", &remotePort, &localePort, &conStat) == 3)
            {
                if (conStat == 2)
                {
                    _connectionCheckCnt = 0;
                    return;
                }
                else if (conStat == 4)
                {
                    m_eMqttConnectionOkFlg = false;
                    UL_GsmModuleMqttConnectionStatusCallback(disconnectGsmMqttConnectionStatus);
                    m_receiveGsmUartCnt = 0;
                    return;
                }
            }
        }

        ++_connectionCheckCnt;
    }

}


_io int ModuleListenResultsProc(const char *f_pList, uint8_t f_totalNumber, uint32_t f_timeout)
{
    // m_receiveGsmUartCnt = 0;
    ClearUartProc();
    // HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    // HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);
    int timeout = HAL_GetTick();
    while ((HAL_GetTick() - timeout) < f_timeout)
    {
        UartParserProc();

        if (m_eReceiveGsmDataCameOkFlg)
        {
            for (uint8_t i = 0; i < f_totalNumber; i++)
            {
                if (strstr((const char *)m_receiveGsmEndBuf, (char *)f_pList[i]))
                    return i;
            }
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }

    return -1;
}


_io int ModuleListenResultProc(const char *f_pRes, uint32_t f_timeout)
{
    m_receiveGsmUartCnt = 0;
    HAL_UART_Abort_IT(m_sGsmParameters.pUart);
    HAL_UART_Receive_IT(m_sGsmParameters.pUart, m_receiveGsmBuf, _gsm_receive_buffer_size);
    int timeout = HAL_GetTick();
    while ((HAL_GetTick() - timeout) < f_timeout)
    {
        UartParserProc();

        if (m_eReceiveGsmDataCameOkFlg)
        {
            #ifdef _gsm_debug
            __logsw("Receive : %s", m_receiveGsmEndBuf);
            #endif
            if (strstr((const char *)m_receiveGsmEndBuf, (const char *)f_pRes))
                return true;
            m_eReceiveGsmDataCameOkFlg = false;
        }
    }
    return false;
}


_io void MqttKeepaliveProc(void)
{
    _io uint32_t _mqttKeepAliveTimer = 0;

    if (((HAL_GetTick() - _mqttKeepAliveTimer) / 1000) < (m_mqttKeepAliveTime / 2))
        return;

    _mqttKeepAliveTimer = HAL_GetTick();
    UL_GsmModuleMqttPublishTopic("keepalive", "a", 0, 0);
}


_io void ModuleResetProc(void)
{
    if (m_sGsmParameters.eModuleType != quectelM65GsmModule && m_sGsmParameters.eModuleType != quectelM66GsmModule)
    {
        HAL_GPIO_WritePin(m_sGsmParameters.pResetPort, m_sGsmParameters.resetPin, (GPIO_PinState)m_sGsmParameters.resetPinEnableStatus);
        _gsm_delay(500);
        HAL_GPIO_WritePin(m_sGsmParameters.pResetPort, m_sGsmParameters.resetPin, (GPIO_PinState)!m_sGsmParameters.resetPinEnableStatus);
        _gsm_delay(500);
    }
    else
    {
        HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.gsmProcessMcuPin ,(GPIO_PinState)!m_sGsmParameters.gsmProcessMcuStatus);
        _gsm_delay(500);
        HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.powerKeyPin,      (GPIO_PinState)!m_sGsmParameters.powerPinEnableStatus);
        _gsm_delay(500);
        HAL_GPIO_WritePin(m_sGsmParameters.pPowerkeyPort, m_sGsmParameters.powerKeyPin,      (GPIO_PinState)!m_sGsmParameters.powerKeyPinEnableStatus); 
        _gsm_delay(500);
    }
}


__attribute__((weak)) void UL_GsmModuleMqttConnectionStatusCallback(EGsmMqttConnectionStatus f_eStatus)
{
#ifdef _gsm_debug
    __logsw("UL_GsmModulePPPMqttConnectionStatusCallback:Mqtt connection status : %d\n", f_eStatus);
#endif
}

__attribute__((weak)) void UL_GsmModuleMqttSubcribeDataCallback(const char *f_pTopic, uint16_t f_topicLen, const char *f_pPayload, uint16_t f_payloadLen)
{
#ifdef _gsm_debug
    __logsw("UL_GsmModuleMqttSubcribeDataCallback: Toppic len : %d Data len : %d\n", f_topicLen, f_payloadLen);
    __logsw("UL_GsmModuleMqttSubcribeDataCallback:  Topic: %.*s  Payload: %.*s\n", f_topicLen, f_pTopic, f_payloadLen, f_pPayload);
#endif
}


_io void UsrSleepGpioOutPins(GPIO_TypeDef *f_pGpio, uint16_t f_pinGroup, GPIO_PinState f_ePinstate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    HAL_GPIO_WritePin(f_pGpio, f_pinGroup, f_ePinstate);
    GPIO_InitStruct.Pin = f_pinGroup;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(f_pGpio, &GPIO_InitStruct);
}





