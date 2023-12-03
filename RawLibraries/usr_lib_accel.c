#include "usr_lib_accel.h"

#define _io static
#define _iov static volatile

_io S_ACC_PARAMETERS m_sAccelometerParameters;

extern bool g_UsrSystemAccelometerChipOkFlag;
extern bool g_UsrSystemAccelometerInterruptDetectedFlag;
extern bool g_UsrSystemAcceleometerSensorIsreadyFlag;

float sensitivityFactor = 0;

uint8_t g_accParamBuf[11] =
    {
        0x90, // 1001000 : ilk bit 1->pull-up disconnect to SDO/SA0 pin
        0x5F, // 0101111 : Normal Mode 100Hz
        0x00, // no filter
        0x40, // 01000000 : Click interrupt hariç bütün interruptlar aktif
        0x30, // 00110000 : set +-16g resolution   (00: ±2g; 01: ±4g; 10: ±8g; 11: ±16g)
        0x00, // 00001000 : latch interrupt for int1
        0x00, // 00000000 : active high interrupt
        0x7F, // 01111111 : all axis interrupt enable
        0x00, // 11111111 : interrupt1 source register read only register. vazgeçtim hepsi 0x00 olsun daha geç interrupta geçiyor
        0x14, // 0x0F de düşünülebilir
        0x02, //
        /*
            0x90, // disable internal pullup over SA0
            0x5F, // 100hz data rate LPen=1 , Zen=1 , Yen=1 , Xen=1 (axes enabled)
            0x30, // set +-16g resulotion
            0x00, // interrupt will NOT be latched
            0x00, // active high interrupt
            0x7F, // Or combination of all axes interrupt
            0x14, // 3,7g treshold for interrupt 1
            0x02, // 20ms duration for interrupt
            0x40, // root the int1 function to int1 pin
        */
};

_io const uint8_t g_accParamRegAddr[11] =
    {
        LIS2DE12_CTRL_REG0,
        LIS2DE12_CTRL_REG1,
        LIS2DE12_CTRL_REG2,
        LIS2DE12_CTRL_REG3,
        LIS2DE12_CTRL_REG4,
        LIS2DE12_CTRL_REG5,
        LIS2DE12_CTRL_REG6,
        LISD2E12_INT1_CFG,
        LISD2E12_INT1_SRC,
        LISD2E12_INT1_THS,
        LISD2E12_INT1_DURATION,
        /*
            0x1E, // CTRL_REG0
            0x20, // CTRL_REG1
            0x23, // CTRL_REG4
            0x24, // CTRL_REG5
            0x25, // CTRL_REG6
            0x30, // INT1_CFG
            0x32, // INT1_THS
            0x33, // INT1_DURATION
            0x22, // CTRL_REG3
        */
};

_io bool WriteByteProc(const uint8_t f_add, const uint8_t f_data)
{
    uint8_t buf[2] = {f_add, f_data};
    if (HAL_I2C_Master_Transmit(&ACC_I2C_CHANNEL, (LISDE12_ADDRESS << 1), buf, 2, 1000) != HAL_OK)
        return false;
    return true;
}

_io bool ReadByteProc(const uint16_t f_add, uint8_t *f_ptr)
{
    if (HAL_I2C_Mem_Read(&ACC_I2C_CHANNEL, (LISDE12_ADDRESS << 1), f_add, 1, f_ptr, 1, 1000) != HAL_OK)
        return false;
    return true;
}

_io void AccInitialParamProc(S_ACC_PARAMETERS *f_pParameter)
{
    m_sAccelometerParameters = *f_pParameter;
    HAL_GPIO_WritePin(m_sAccelometerParameters.accelpowerport, m_sAccelometerParameters.accelpin, (GPIO_PinState)m_sAccelometerParameters.accelpowerstatus);
}

bool UL_AccelInitial(S_ACC_PARAMETERS *f_pParameter)
{
    m_sAccelometerParameters = *f_pParameter;
    AccInitialParamProc(&m_sAccelometerParameters); // active accelometer

    // HAL_Delay(50);
    // for(uint8_t i = 0; i < 12; i++)
    // {
    //     WriteByteProc(g_accParamRegAddr[i], g_accParamBuf[i]);
    // }
    // sensitivityFactor = 0.0312f;

    return true;
}

bool UL_AccelCheckChip(void)
{
    uint8_t checkvalue = 0;

    for (uint8_t i = 0; i < 5; i++)
        if (ReadByteProc(LIS2DE12_WHO_AM_I, &checkvalue) != HAL_OK)
        {
            if (checkvalue == 0x33)
                return true;
        }
    return false;
}

bool UL_AccelCheckParam(void)
{
    uint8_t f_RegValBuf = 0;

    for (int i = 0; i < 12; i++)
    {
        ReadByteProc(g_accParamRegAddr[i], &f_RegValBuf);
        if (f_RegValBuf != g_accParamBuf[i])
            return false;
    }
    return true;
}

bool UL_AccelChipOk(void)
{
    if (!UL_AccelCheckChip())
        return false;
    if (!UL_AccelCheckParam())
        return false;
    return true;
}

bool UL_AccelFabrication(void)
{
    if (g_UsrSystemAccelometerChipOkFlag)
    {
        for (uint8_t i = 0; i < 12; i++)
        {
            WriteByteProc(g_accParamRegAddr[i], g_accParamBuf[i]);
        }
    }
    return true;
}

uint8_t UL_ReadAccelData(float *x, float *y, float *z)
{
    float data[3];
    uint8_t rawdata[3]; // geçici dizi raw değerleri tutsun

    if (ReadByteProc(LIS2DE12_OUT_X, &rawdata[0]) == true)
    {
        data[0] = rawdata[0] * sensitivityFactor;
        *x = data[0];
    }

    if (ReadByteProc(LIS2DE12_OUT_Y, &rawdata[1]) == true)
    {
        data[1] = rawdata[1] * sensitivityFactor;
        *y = data[1];
    }

    if (ReadByteProc(LIS2DE12_OUT_Z, &rawdata[2]) == true)
    {
        data[2] = rawdata[2] * sensitivityFactor;
        *z = data[2];
    }

    return 1;
}

void UL_AccelometerClearFlag(void)
{
    if (g_UsrSystemAccelometerInterruptDetectedFlag)
    {
        uint8_t val = 0;
        ReadByteProc(0x31, &val);
        g_UsrSystemAccelometerInterruptDetectedFlag = false;
    }
}

void UsrI2CScan(void)
{
    uint8_t address_buf[16] = {0};
    uint8_t address = 0;

    for (address = 0; address < 128; address++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(address << 1), 1, 10) == HAL_OK)
        {
            sprintf((char *)address_buf, "iste adres: 0x%X\r\n", address);
        }
    }
}
