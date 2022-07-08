/*
  MPU6050_minimal.h - Minimal library to acces raw accelerometer and gyroscope data from MPU6050.
  Ability to return angles from acceleration data and to store offsets.
  Created by T. Sepke, June 16, 2022.
  Released into the public domain.

  Licensed under "MIT" License.
*/
#include "MPU6050_minimal.h"

#include "Arduino.h"
#include "Wire.h"

// PUBLIC

/*
 * Constructor function for the MPU6050
 * @param I2C_addr address of the I2C MPU6050 device.
 * Either 0x68 for AD0=LOW or 0x69 for AD0=HIGH.
 */
MPU6050::MPU6050(uint8_t I2C_addr) {
    devAddr = I2C_addr;    
}

/*
 * Reset and initialize the MPU6050. Fine scales for accelerometer & gyroscope.
 * 5 Hz filter to get best static data.
 */
void MPU6050::initialize(void) {
    writeBit(devAddr, MPU6050_RA_PWR_MGMT_1, 6, 0);        // PWR_MGMT_1: set SLEEP false (wakes up the MPU-6050)
    writeBit(devAddr, MPU6050_RA_PWR_MGMT_1, 7, 1);        // PWR_MGMT_1: set RESET true
    writeBits(devAddr, MPU6050_RA_PWR_MGMT_1, 2, 2, 1);    // PWR_MGMT_1: set CLKSEL 1 (clock source = X axis gyro ref)
    writeBits(devAddr, MPU6050_RA_GYRO_CONFIG, 4, 2, 0);   // GYRO_CONFIG: set FS_SEL 0 (full scale to 250 deg/s)
    writeBits(devAddr, MPU6050_RA_ACCEL_CONFIG, 4, 2, 0);  // ACCEL_CONFIG: set AFS_SEL (full scale to 2 g)
    writeBits(devAddr, MPU6050_RA_ACCEL_CONFIG, 4, 2, 0);  // CONFIG: set DLPF_CFG 6 (Low Pass Filter to 5Hz)
    writeBit(devAddr, MPU6050_RA_PWR_MGMT_1, 6, 0);        // PWR_MGMT_1: set SLEEP false (wakes up the MPU-6050)
}

/*
 * Test connection with WHO_AM_I register
 * @return true or false
 */
bool MPU6050::testConnection(void) {
    uint8_t readAddr = readByte(devAddr, MPU6050_RA_WHO_AM_I);
    return readAddr == MPU6050_SET_WHO_AM_I;
}

/*
 * Activate/Deactivate I2C Bypass of the device
 */
void MPU6050::setBypass(uint8_t enable) {
    writeBit(devAddr, MPU6050_RA_USER_CTRL, 5, 0);    // USER_CTRL: set I2C_MST_EN false (I2C Master Mode off)
    writeBit(devAddr, MPU6050_RA_INT_PIN_CFG, 1, 1);  // INT_PIN_CFG: set I2C_BYPASS_EN true (I2C Bypass active)
    writeBit(devAddr, MPU6050_RA_PWR_MGMT_1, 6, 0);   // PWR_MGMT_1: set SLEEP false (wakes up the MPU-6050)
}

MPUDataType MPU6050::getData(void) {
    getAcceleration(data.AcX, data.AcY, data.AcZ);
    getGyroscope(data.GyX, data.GyY, data.GyZ);
    getTemperature(data.Temp);
    getAngles(data.AcX, data.AcY, data.AcZ, data.phiX, data.phiY);
    return data;
}

/*
 * Get the accelerometer data (in g) from device register.
 * @param AcX x-data of the g-Vector
 * @param AcY y-data of the g-Vector
 * @param AcZ z-data of the g-Vector
 */
void MPU6050::getAcceleration(float &AcX, float &AcY, float &AcZ) {
    Wire.beginTransmission(devAddr);
    Wire.write(MPU6050_RA_ACCEL_XOUT_H);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(devAddr, (uint8_t)6, (uint8_t) true);  // request a total of 6 registers
    int16_t AcX_read = Wire.read() << 8 | Wire.read();      // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    int16_t AcY_read = Wire.read() << 8 | Wire.read();      // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    int16_t AcZ_read = Wire.read() << 8 | Wire.read();      // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)

    // remove offsets
    AcX_read += MPU6050_OFFSET_AcX;
    AcY_read += MPU6050_OFFSET_AcY;
    AcZ_read += MPU6050_OFFSET_AcZ;

    // convert raw data to g
    AcX = (float) AcX_read / (float) MPU6050_Ac_convert;
    AcY = (float) AcY_read / (float) MPU6050_Ac_convert;
    AcZ = (float) AcZ_read / (float) MPU6050_Ac_convert;
}

/*
 * Get the temperature sensor data (in degC) from device register.
 * @param T temperature value
 */
void MPU6050::getTemperature(float &T) {
    Wire.beginTransmission(devAddr);
    Wire.write(MPU6050_RA_TEMP_OUT_H);  // starting with register 0x43 (TEMP_OUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(devAddr, (uint8_t)2, (uint8_t) true);  // request a total of 2 registers
    int16_t T_read = Wire.read() << 8 | Wire.read();                // 0x3B (TEMP_OUT_H) & 0x3C (TEMP_OUT_L)

    T_read += MPU6050_OFFSET_temp;

    T = (float) T_read / (float) MPU6050_T_convert + 36.53;
}

/*
 * Get the gyroscope data (in deg/s) from device register.
 * @param GyX angular x-data
 * @param GyY angular y-data
 * @param GyZ angular z-data
 */
void MPU6050::getGyroscope(float &GyX, float &GyY, float &GyZ) {
    Wire.beginTransmission(devAddr);
    Wire.write(MPU6050_RA_GYRO_XOUT_H);  // starting with register 0x43 (GYRO_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(devAddr, (uint8_t)6, (uint8_t) true);  // request a total of 6 registers
    int16_t GyX_read = Wire.read() << 8 | Wire.read();                   // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    int16_t GyY_read = Wire.read() << 8 | Wire.read();                   // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    int16_t GyZ_read = Wire.read() << 8 | Wire.read();                   // 0x48 (GYRO_ZOUT_H) & 0x47 (GYRO_ZOUT_L)

    // remove offsets
    GyX_read += MPU6050_OFFSET_GyX;
    GyY_read += MPU6050_OFFSET_GyY;
    GyZ_read += MPU6050_OFFSET_GyZ;

    // convert raw data to deg/s
    GyX = (float) GyX_read / (float) MPU6050_Gy_convert;
    GyY = (float) GyY_read / (float) MPU6050_Gy_convert;
    GyZ = (float) GyZ_read / (float) MPU6050_Gy_convert;
}

// PRIVATE

/*
 * Calculates the angles from the acceleration data.
 * @param AcX x-data of the g-Vector
 * @param AcY y-data of the g-Vector
 * @param AcZ z-data of the g-Vector
 * @param phiX x-angle
 * @param phiY y-angle
 */
void MPU6050::getAngles(float AcX, float AcY, float AcZ, float &phiX, float &phiY) {
    float xVec[3] = { 1, 0, 0 };
    float yVec[3] = { 0, 1, 0 };
    float vec[3] = {AcX, AcY, AcZ};

    float phiYRaw = asin((xVec[0] * vec[0] + xVec[1] * vec[1] + xVec[2] * vec[2]) / (vecLength(vec) * vecLength(xVec))) * RAD_TO_DEG;
    float phiXRaw = asin((yVec[0] * vec[0] + yVec[1] * vec[1] + yVec[2] * vec[2]) / (vecLength(vec) * vecLength(xVec))) * RAD_TO_DEG;

    phiX = phiXRaw + MPU6050_OFFSET_phiX;
    phiY = phiYRaw + MPU6050_OFFSET_phiY;
}

/*
 * Get the length of a 1x3 vector.
 * @param vec Input vector
 * @return Length of the vector
 */
float MPU6050::vecLength(float vec[3]) {
  return sqrt(sq(vec[0]) + sq(vec[1]) + sq(vec[2]));
}

/*
 * Read a register from device
 * @param devAddr device I2C address
 * @param regAddr register address
 * @return Byte in that register
 */
uint8_t MPU6050::readByte(uint8_t devAddr, uint8_t regAddr) {
    Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    Wire.endTransmission(false);
    Wire.requestFrom(devAddr, (uint8_t)1, (uint8_t) true);
    uint8_t dataByte = Wire.read();
    return dataByte;
}

/*
 * Write a bit at a specific location to a register of a device.
 * Read register at the beginning to keep the other bits.
 * @param devAddr device I2C address
 * @param regAddr register address
 * @param byteToWrite byte to write into the register
 */
void MPU6050::writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t bitToWrite) {
    uint8_t byteToWrite = readByte(devAddr, regAddr);
    byteToWrite = (bitToWrite != 0) ? (bitToWrite | (1 << bitNum)) : (byteToWrite & ~(1 << bitNum));
    writeByte(devAddr, regAddr, byteToWrite);
}

/** Write multiple bits in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 */
void MPU6050::writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
    //      010 value to write
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    // 00011100 mask byte
    // 10101111 original value (sample)
    // 10100011 original & ~mask
    // 10101011 masked | value
    uint8_t b = readByte(devAddr, regAddr);
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1);  // shift data into correct position
    data &= mask;                      // zero all non-important bits in data
    b &= ~(mask);                      // zero all important bits in existing byte
    b |= data;                         // combine data with existing byte
    writeByte(devAddr, regAddr, b);
}

/*
 * Write a byte to a register of a device
 * @param devAddr device I2C address
 * @param regAddr register address
 * @param byteToWrite byte to write into the register
 */
void MPU6050::writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t byteToWrite) {
    Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    Wire.write(byteToWrite);
    Wire.endTransmission(true);
}