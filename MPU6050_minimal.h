/*
  MPU6050_minimal.h - Minimal library to acces raw accelerometer and gyroscope data from MPU6050.
  Ability to return angles from acceleration data and to store offsets.
  Created by T. Sepke, June 16, 2022.
  Released into the public domain.

  Licensed under "MIT" License.
*/

#ifndef MPU6050_minimal
#define MPU6050_minimal

#include "Arduino.h"
#include "Wire.h"

// Register addresses
#define MPU6050_RA_PWR_MGMT_1 0x6B    // [7] DEVICE_RESET, [6] SLEEP, [5] CYCLE, [3] TEMP_DIS, [2:0] CLKSEL
#define MPU6050_RA_GYRO_CONFIG 0x1B   // [7] XG_ST, [6] YG_ST, [5] ZG_ST, [4:3] FS_SEL
#define MPU6050_RA_ACCEL_CONFIG 0x1C  // [7] XA_ST, [6] YA_ST, [5] ZA_ST, [4:3] AFS_SEL
#define MPU6050_RA_CONFIG 0x1A        // [5:3] EXT_SYNC_SET, [2:0] DLPF_CFG
#define MPU6050_RA_WHO_AM_I 0x75      // [6:1] WHO_AM_I
#define MPU6050_RA_USER_CTRL 0x6A     // [6] FIFO_EN, [5] I2C_MST_EN, [4] I2C_IF_DIS, [2] FIFO_RESET, [1] I2C_MST_RESET, [0] SIG_COND_RESET
#define MPU6050_RA_INT_PIN_CFG 0x37   // [7] INT_LEVEL, [6] INT_OPEN, [5] LATCH_INT_EN, [4] INT_RD_CLEAR, [3] FSYNC_INT_LEVEL, [2] FSYNC_INT_EN [1] I2C_BYPASS_EN
#define MPU6050_RA_ACCEL_XOUT_H 0x3B  // [7:0] ACCEL_?OUT (6 total register; goes from 0x3B to 0x40)
#define MPU6050_RA_TEMP_OUT_H 0x41    // [7:0] TEMP_OUT (2 total register; goes from 0x41 to 0x42)
#define MPU6050_RA_GYRO_XOUT_H 0x43   // [7:0] GYRO_?OUT (6 total register; goes from 0x43 to 0x48)

// WHO_AM_I
#define MPU6050_SET_WHO_AM_I 0b01101000

// Values
#define MPU6050_Ac_convert 16384  // LSB/g
#define MPU6050_Gy_convert 131    // LSB/deg/s
#define MPU6050_T_convert 340     // LSB/degC (offset 35degC = -521 LSB)

// Offsets
#define MPU6050_OFFSET_AcX 0
#define MPU6050_OFFSET_AcY 0
#define MPU6050_OFFSET_AcZ 1688 // factory: 1688
#define MPU6050_OFFSET_temp 0
#define MPU6050_OFFSET_GyX 0
#define MPU6050_OFFSET_GyY 0
#define MPU6050_OFFSET_GyZ 0
#define MPU6050_OFFSET_phiX 0
#define MPU6050_OFFSET_phiY 0

// MPU struct
#ifndef MPU6050_STRUCT
#define MPU6050_STRUCT
struct MPUDataType {
    float AcX;   // Acceleration in x in g
    float AcY;   // Acceleration in y in g
    float AcZ;   // Acceleration in z in g
    float GyX;   // Angular velocity around x in deg/s
    float GyY;   // Angular velocity around y in deg/s
    float GyZ;   // Angular velocity around z in deg/s
    float Temp;  // Temperature in T
    float phiX;  // Angle around x in deg
    float phiY;  // Angle around y in deg
};
#endif

class MPU6050 {
   public:
    MPU6050(uint8_t I2C_addr = 0x68);
    MPUDataType data;
    void initialize(void);
    bool testConnection(void);
    void setBypass(uint8_t enable = true);
    MPUDataType getData(void);
    void getAcceleration(float &AcX, float &AcY, float &AcZ);
    void getTemperature(float &T);
    void getGyroscope(float &GyX, float &GyY, float &GyZ);

   private:
    uint8_t devAddr;
    void getAngles(float AcX, float AcY, float AcZ, float &phiX, float &phiY);
    float vecLength(float vec[3]);
    uint8_t readByte(uint8_t devAddr, uint8_t regAddr);
    void writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data);
    void writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
    void writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t uint8_tToWrite);
};

#endif