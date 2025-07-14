
/* ===========================================================================
 *                              IMPLEMENTATION
 * ===========================================================================*/

#include "imu.h"
#include "usart.h"

/* ------------------------------------------------- */
/*  low‑level helpers                                */
/* ------------------------------------------------- */
static HAL_StatusTypeDef _write_byte(mpu9250_t *dev, uint8_t reg, uint8_t data)
{
    return HAL_I2C_Mem_Write(dev->hi2c, dev->addr, reg,
                              I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef _read_bytes(mpu9250_t *dev, uint8_t reg,
                                     uint8_t *buf, uint8_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c, dev->addr, reg,
                             I2C_MEMADD_SIZE_8BIT, buf, len, HAL_MAX_DELAY);
}

/* ------------------------------------------------- */
/*  Init                                             */
/* ------------------------------------------------- */
HAL_StatusTypeDef MPU9250_Init(mpu9250_t *dev,
                               I2C_HandleTypeDef *hi2c,
                               mpu9250_gyro_fs_t fs)
{
    dev->hi2c = hi2c;
    dev->addr = MPU9250_I2C_ADDR;

    /* 1. Wake device (clear SLEEP bit) */
    HAL_StatusTypeDef ret = _write_byte(dev, MPU9250_REG_PWR_MGMT_1, 0x01); /* clk = auto */
    if (ret != HAL_OK) return ret;

    /* 2. Set sample‑rate divider (0 ▸ sample rate = gyro ODR / (1+SMPLRT_DIV) ) */
    ret = _write_byte(dev, MPU9250_REG_SMPLRT_DIV, 0x00);
    if (ret != HAL_OK) return ret;

    /* 3. Set DLPF to 41 Hz for gyro (CONFIG, bits[2:0]=3) → cleaner data */
    ret = _write_byte(dev, MPU9250_REG_CONFIG, 0x03);
    if (ret != HAL_OK) return ret;

    /* 4. Configure gyro full‑scale range */
    uint8_t gcfg = (fs << 3);          /* bits 4:3 */
    ret = _write_byte(dev, MPU9250_REG_GYRO_CONFIG, gcfg);
    if (ret != HAL_OK) return ret;

    /* 5. Store scale factor for float conversion */
    switch (fs) {
        case MPU9250_GYRO_FS_250DPS:  dev->gyro_scale = 1.0f / 131.0f;  break;
        case MPU9250_GYRO_FS_500DPS:  dev->gyro_scale = 1.0f /  65.5f;  break;
        case MPU9250_GYRO_FS_1000DPS: dev->gyro_scale = 1.0f /  32.8f;  break;
        case MPU9250_GYRO_FS_2000DPS: dev->gyro_scale = 1.0f /  16.4f;  break;
        default:                      dev->gyro_scale = 1.0f / 131.0f;  break;
    }
    return HAL_OK;
}

/* ------------------------------------------------- */
/*  Raw gyro read (int16 counts)                     */
/* ------------------------------------------------- */
HAL_StatusTypeDef MPU9250_Gyro_ReadRaw(mpu9250_t *dev,
                                       int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];
    HAL_StatusTypeDef ret = _read_bytes(dev, MPU9250_REG_GYRO_XOUT_H, buf, 6);
    if (ret != HAL_OK) return ret;

    *gx = (int16_t)((buf[0] << 8) | buf[1]);
    *gy = (int16_t)((buf[2] << 8) | buf[3]);
    *gz = (int16_t)((buf[4] << 8) | buf[5]);
    return HAL_OK;
}

/* ------------------------------------------------- */
/*  Gyro read in °/s                                 */
/* ------------------------------------------------- */
HAL_StatusTypeDef MPU9250_Read_Angular_Velocity(mpu9250_t *dev,
                                       float *gx_dps, float *gy_dps, float *gz_dps)
{
    int16_t gx_raw, gy_raw, gz_raw;
    HAL_StatusTypeDef ret = MPU9250_Gyro_ReadRaw(dev, &gx_raw, &gy_raw, &gz_raw);
    if (ret != HAL_OK) return ret;

    *gx_dps = (float)gx_raw * dev->gyro_scale;
    *gy_dps = (float)gy_raw * dev->gyro_scale;
    *gz_dps = (float)gz_raw * dev->gyro_scale;
    return HAL_OK;
}
