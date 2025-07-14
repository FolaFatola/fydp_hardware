/*
 * imu.h
 *
 *  Created on: Jul 3, 2025
 *      Author: folafatola
 */

#ifndef IMU_INC_IMU_H_
#define IMU_INC_IMU_H_

#include "stm32f4xx_hal.h"   /* or your device family header   */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Device & register addresses ---------- */
#define MPU9250_I2C_ADDR       (0x68 << 1)   /* AD0 = 0 ▸ 0x68, HAL wants 8‑bit */

#define MPU9250_REG_PWR_MGMT_1  0x6B
#define MPU9250_REG_SMPLRT_DIV  0x19
#define MPU9250_REG_CONFIG      0x1A
#define MPU9250_REG_GYRO_CONFIG 0x1B
#define MPU9250_REG_GYRO_XOUT_H 0x43   /* 6 bytes from here: X, Y, Z */

/* ---------- Gyro full‑scale enum ---------- */
typedef enum {
    MPU9250_GYRO_FS_250DPS  = 0,   /* 131  LSB/°/s */
    MPU9250_GYRO_FS_500DPS  = 1,   /* 65.5 LSB/°/s */
    MPU9250_GYRO_FS_1000DPS = 2,   /* 32.8 LSB/°/s */
    MPU9250_GYRO_FS_2000DPS = 3    /* 16.4 LSB/°/s */
} mpu9250_gyro_fs_t;

/* ---------- Opaque device handle ---------- */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t           addr;          /* 7‑bit <<1 form that HAL uses         */
    float              gyro_scale;    /* DPS/LSB for chosen full scale        */
} mpu9250_t;

/* ---------- API ---------- */
HAL_StatusTypeDef MPU9250_Init(mpu9250_t *dev,
                               I2C_HandleTypeDef *hi2c,
                               mpu9250_gyro_fs_t fs);

HAL_StatusTypeDef MPU9250_Gyro_ReadRaw(mpu9250_t *dev,
                                       int16_t *gx, int16_t *gy, int16_t *gz);

HAL_StatusTypeDef MPU9250_Read_Angular_Velocity(mpu9250_t *dev,
                                       float *gx_dps, float *gy_dps, float *gz_dps);

#ifdef __cplusplus
}
#endif

#endif /* MPU9250_DRIVER_H_ */

