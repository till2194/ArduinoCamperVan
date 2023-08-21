# ArduinoCamperVan
Arduino project for a camper van. Includes serveral sensors and a display to show of informations.

## List of hardware:
1. Arduino Uno R3
2. Gyroscope and accelerometer MPU6050 GY-521
3. Real time clock module RTC DS3231
4. 1.3" OLED display SH1106 128x64
5. Rotary encoder KY-040
6. Temperature and humidity sensor DHT11
7. Current sensor ACS712 30A
8. Voltage sensor 25V
9. 2x Water level switch
11. LED

## Used external libraries:
| Library | Description | Link |
| ------- | ----------- | ---- |
| lcdgfx  | lightweight display library | [Link](https://github.com/lexus2k/lcdgfx) |
| Rotary  | simple rotary encoder with debounce | [Link](https://github.com/buxtronix/arduino/tree/master/libraries/Rotary) |

## List of sensor pin connection:
1. Gyroscope and accelerometer MPU6050 GY-521 (3.3V I2C)
    - VCC 3.3V (red)
    - GND (brown)
    - SCL (orange)
    - SDA (yellow)
    - XCL
    - XDA
    - AD0 -> VCC 3.3V (high for different I2C ID active) (black)
    - INT
2. Real time clock module RTC DS3231 (3.3V I2C)
    - GND (brown)
    - VCC 3.3V (red)
    - SDA (green)
    - SCL (orange)
    - SQW
    - 32K
    - Out GND (brown)
    - Out VCC 3.3V (red)
    - Out SDA (yellow)
    - Out SCL (orange)
3. 1.3" OLED display SH1106 128x64 (I2C)
    - VDD 3.3V (black)
    - GND (white)
    - SCK (grey)
    - SDA (purple)
4. Rotary encoder KY-040 
    - GND (brown)
    - +/VCC 5V (red)
    - SW interrupt (orange)
    - DT digital (yellow)
    - CLK digital (green)
5. Temperature and humidity sensor DHT11
    - GND (brown, right pin)
    - VCC 5V (orange)
    - Data digtal (red, left pin)
6. Current sensor ACS712 30A
    - VCC 5V (yellow)
    - OUT analog (blue)
    - GND (black)
7. Voltage sensor <25V
    - -/GND (black)
    - +/VCC 5V (yellow)
    - S/Data analog (green)
8. Water level switch grey water
    - VCC 3.3V (white)
    - GND (black)
9. Water level switch fresh water
    - VCC 3.3V (purple)
    - GND (grey)
10. LED Back
    - VCC 5V (red)
    - GND (black)
11. LED Front
    - VCC 5V (orange)
    - GND (brown)
