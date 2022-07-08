/*
DS3231.cpp - Class file for the DS3231 Real-Time Clock

Version: 1.0.1
(c) 2014 Korneliusz Jarzebski
www.jarzebski.pl

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DS3231_minimal.h"

#include <Wire.h>

#include "Arduino.h"

const uint8_t daysArray[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint8_t dowArray[] PROGMEM = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

// PUBLIC
/*
 * Initializing the RTC device.
 * @return true, if no errors occurred.
 */
bool DS3231::begin(void) {
    Wire.begin();

    setBattery(true, false);

    t.year = 2000;
    t.month = 1;
    t.day = 1;
    t.hour = 0;
    t.minute = 0;
    t.second = 0;
    t.dayOfWeek = 6;
    t.unixtime = 946681200;

    return true;
}

/*
 * Sets the internal time of the RTC device.
 * @param year Year
 * @param month Month
 * @param day Day
 * @param hour Hour
 * @param minute Minute
 * @param second Second
 */
void DS3231::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_TIME);

    Wire.write(dec2bcd(second));
    Wire.write(dec2bcd(minute));
    Wire.write(dec2bcd(hour));
    Wire.write(dec2bcd(dow(year, month, day)));
    Wire.write(dec2bcd(day));
    Wire.write(dec2bcd(month));
    Wire.write(dec2bcd(year - 2000));

    Wire.write(DS3231_REG_TIME);
    Wire.endTransmission();
}

/*
 * Sets the internal time of the RTC device.
 * @param t Time in seconds
 */
void DS3231::setDateTime(uint32_t t) {
    t -= 946681200;

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    second = t % 60;
    t /= 60;

    minute = t % 60;
    t /= 60;

    hour = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;

    for (year = 0;; ++year) {
        leap = year % 4 == 0;
        if (days < 365 + leap) {
            break;
        }
        days -= 365 + leap;
    }

    for (month = 1;; ++month) {
        uint8_t daysPerMonth = pgm_read_byte(daysArray + month - 1);

        if (leap && month == 2) {
            ++daysPerMonth;
        }

        if (days < daysPerMonth) {
            break;
        }
        days -= daysPerMonth;
    }

    day = days + 1;

    setDateTime(year + 2000, month, day, hour, minute, second);
}

/*
 * Sets the internal time of the RTC device.
 * @param date Date string in format of "Jun 16 2022"
 * @param time Time string in format of "19:07:10"
 */
void DS3231::setDateTime(const char* date, const char* time) {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    year = conv2d(date + 9);

    switch (date[0]) {
        case 'J':
            month = date[1] == 'a' ? 1 : month = date[2] == 'n' ? 6 : 7;
            break;
        case 'F':
            month = 2;
            break;
        case 'A':
            month = date[2] == 'r' ? 4 : 8;
            break;
        case 'M':
            month = date[2] == 'r' ? 3 : 5;
            break;
        case 'S':
            month = 9;
            break;
        case 'O':
            month = 10;
            break;
        case 'N':
            month = 11;
            break;
        case 'D':
            month = 12;
            break;
    }

    day = conv2d(date + 4);
    hour = conv2d(time);
    minute = conv2d(time + 3);
    second = conv2d(time + 6);

    setDateTime(year + 2000, month, day, hour, minute, second);
}

/*
 * Get the internal time of the RTC device.
 * @return Date time struct
 */
RTCDateTime DS3231::getDateTime(void) {
    int values[7];

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_TIME);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 7);

    while (!Wire.available()) {
    };

    for (int i = 6; i >= 0; i--) {
        values[i] = bcd2dec(Wire.read());
    }

    Wire.endTransmission();

    t.year = values[0] + 2000;
    t.month = values[1];
    t.day = values[2];
    t.dayOfWeek = values[3];
    t.hour = values[4];
    t.minute = values[5];
    t.second = values[6];
    t.unixtime = unixtime();

    return t;
}

/*
 * Is ready flag.
 */
uint8_t DS3231::isReady(void) {
    return true;
}

/*
 * INTCN of REG_CONTROL:
 * Enable Interrupt to be triggered by alarms.
 * The corresponding alarm interrupt must also be enabled.
 * @param enabled set to true or false
 */
void DS3231::setInterruptSetting(bool enabled) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    value &= 0b11111011;
    value |= (!enabled << 2);

    writeRegister8(DS3231_REG_CONTROL, value);
}

/*
 * INTCN of REG_CONTROL:
 * Get the Interrupt state.
 * @return true or false if the interrupt is enabled
 */
bool DS3231::getInterruptSetting(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    value &= 0b00000100;
    value >>= 2;

    return !value;
}

/*
 * Battery settings of REG_CONTROL:
 * @param timeBattery defines, if the time oscillator runs by battery (EOSC, default=true)
 * @param sqwBattery Enable square wave output at interrupt pin (INTCN must be zero).
 * Frequency can be adjusted by RS1 and RS2. (BBSQW, default=false)
 */
void DS3231::setBattery(bool timeBattery, bool sqwBattery) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    if (sqwBattery) {
        value |= 0b01000000;
    } else {
        value &= 0b10111111;
    }

    if (timeBattery) {
        value &= 0b01111011;
    } else {
        value |= 0b10000000;
    }

    writeRegister8(DS3231_REG_CONTROL, value);
}

/*
 * Set square wave frequency 1Hz up to 8kHz
 * @param DS3231_sqw_t Mode of the square wave frequency
 */
void DS3231::setSQWFrequency(DS3231_sqw_t mode) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    value &= 0b11100111;
    value |= (mode << 3);

    writeRegister8(DS3231_REG_CONTROL, value);
}

/*
 * Set the square wave frequency mode
 * @return selected mode (DS3231_sqw_t)
 */
DS3231_sqw_t DS3231::getSQWFrequency(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    value &= 0b00011000;
    value >>= 3;

    return (DS3231_sqw_t)value;
}

/*
 * Enable 32kHz square-wave pin output (EN32kHz of REG_STATUS)
 * @param enabled true to enable the pin output
 */
void DS3231::set32kHzPin(bool enabled) {
    uint8_t value;

    value = readRegister8(DS3231_REG_STATUS);

    value &= 0b11110111;
    value |= (enabled << 3);

    writeRegister8(DS3231_REG_STATUS, value);
}

/*
 * Get 32kHz square-wave pin output status (EN32kHz of REG_STATUS)
 * @return true or false if enabled
 */
bool DS3231::get32kHzPin(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_STATUS);

    value &= 0b00001000;
    value >>= 3;

    return value;
}

/*
 * Setting this bit to 1 forces the temperature sensor to convert the temperature into digital code
 */
void DS3231::forceTempConversion(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_CONTROL);

    value |= 0b00100000;

    writeRegister8(DS3231_REG_CONTROL, value);

    do {
    } while ((readRegister8(DS3231_REG_CONTROL) & 0b00100000) != 0);
}

/*
 * Read temperature value.
 * @return temperature value converted
 */
float DS3231::readTemperature(void) {
    uint8_t msb, lsb;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_TEMPERATURE);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 2);

    while (!Wire.available()) {
    };
    msb = Wire.read();
    lsb = Wire.read();

    return ((((short)msb << 8) | (short)lsb) >> 6) / 4.0f;
}

/*
 * Get Alarm 1 time.
 * @return alarm time struct
 */
RTCAlarmTime DS3231::getAlarm1(void) {
    uint8_t values[4];
    RTCAlarmTime a;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_1);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 4);

    while (!Wire.available()) {
    };

    for (int i = 3; i >= 0; i--) {
        values[i] = bcd2dec(Wire.read() & 0b01111111);
    }

    Wire.endTransmission();

    a.day = values[0];
    a.hour = values[1];
    a.minute = values[2];
    a.second = values[3];

    return a;
}

/*
 * Get Alarm 1 type.
 * @return alarm type
 */
DS3231_alarm1_t DS3231::getAlarmType1(void) {
    uint8_t values[4];
    uint8_t mode = 0;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_1);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 4);

    while (!Wire.available()) {
    };

    for (int i = 3; i >= 0; i--) {
        values[i] = bcd2dec(Wire.read());
    }

    Wire.endTransmission();

    mode |= ((values[3] & 0b01000000) >> 6);
    mode |= ((values[2] & 0b01000000) >> 5);
    mode |= ((values[1] & 0b01000000) >> 4);
    mode |= ((values[0] & 0b01000000) >> 3);
    mode |= ((values[0] & 0b00100000) >> 1);

    return (DS3231_alarm1_t)mode;
}

/*
 * Set Alarm 1 by time and type and if alarm interrupt pin should be enabled
 * @param dydw if enabled, bit 6 defines date of month match else it defines day of week match
 * @param hour Alarm hour
 * @param minute Alarm minute
 * @param second Alarm second
 * @param alarmMode alarm mode by alarm type (differs for alarm 1 and 2)
 * @param interruptEnable defines, if the interrupt pin should be linked to this alarm
 */
void DS3231::setAlarm1(uint8_t dydw, uint8_t hour, uint8_t minute, uint8_t second, DS3231_alarm1_t alarmMode, bool interruptEnable) {
    second = dec2bcd(second);
    minute = dec2bcd(minute);
    hour = dec2bcd(hour);
    dydw = dec2bcd(dydw);

    switch (alarmMode) {
        case DS3231_EVERY_SECOND:
            second |= 0b10000000;
            minute |= 0b10000000;
            hour |= 0b10000000;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_S:
            second &= 0b01111111;
            minute |= 0b10000000;
            hour |= 0b10000000;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_M_S:
            second &= 0b01111111;
            minute &= 0b01111111;
            hour |= 0b10000000;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_H_M_S:
            second &= 0b01111111;
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_DT_H_M_S:
            second &= 0b01111111;
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw &= 0b01111111;
            break;

        case DS3231_MATCH_DY_H_M_S:
            second &= 0b01111111;
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw &= 0b01111111;
            dydw |= 0b01000000;
            break;
    }

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_1);
    Wire.write(second);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(dydw);
    Wire.endTransmission();

    setInterruptAlarm1(interruptEnable);

    clearAlarm1();
}

/*
 * A1F of REG_STATUS: check if alarm 1 occured
 * @return true or false
 */
bool DS3231::isAlarm1(bool clear) {
    uint8_t alarm;

    alarm = readRegister8(DS3231_REG_STATUS);
    alarm &= 0b00000001;

    if (alarm && clear) {
        clearAlarm1();
    }

    return alarm;
}

/*
 * A1IE of REG_CONTROL: set alarm interrupt for alarm 1
 * @param enabled set interrupt true or false
 */
void DS3231::setInterruptAlarm1(bool armed) {
    uint8_t value;
    value = readRegister8(DS3231_REG_CONTROL);

    if (armed) {
        value |= 0b00000001;
    } else {
        value &= 0b11111110;
    }

    writeRegister8(DS3231_REG_CONTROL, value);
}

/*
 * A1IE of REG_CONTROL: get alarm interrupt setting for alarm 1
 * @return true or false if alarm interrupt is enabled
 */
bool DS3231::getInterruptAlarm1(void) {
    uint8_t value;
    value = readRegister8(DS3231_REG_CONTROL);
    value &= 0b00000001;
    return value;
}

/*
 * A1F of REG_STATUS: clear/quit alarm 1 flag
 */
void DS3231::clearAlarm1(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_STATUS);
    value &= 0b11111110;

    writeRegister8(DS3231_REG_STATUS, value);
}

/*
 * Get Alarm 2 time.
 * @return alarm time struct
 */
RTCAlarmTime DS3231::getAlarm2(void) {
    uint8_t values[3];
    RTCAlarmTime a;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_2);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 3);

    while (!Wire.available()) {
    };

    for (int i = 2; i >= 0; i--) {
        values[i] = bcd2dec(Wire.read() & 0b01111111);
    }

    Wire.endTransmission();

    a.day = values[0];
    a.hour = values[1];
    a.minute = values[2];
    a.second = 0;

    return a;
}

/*
 * Get Alarm 2 type.
 * @return alarm type
 */
DS3231_alarm2_t DS3231::getAlarmType2(void) {
    uint8_t values[3];
    uint8_t mode = 0;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_2);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 3);

    while (!Wire.available()) {
    };

    for (int i = 2; i >= 0; i--) {
        values[i] = bcd2dec(Wire.read());
    }

    Wire.endTransmission();

    mode |= ((values[2] & 0b01000000) >> 5);
    mode |= ((values[1] & 0b01000000) >> 4);
    mode |= ((values[0] & 0b01000000) >> 3);
    mode |= ((values[0] & 0b00100000) >> 1);

    return (DS3231_alarm2_t)mode;
}

/*
 * Set Alarm 2 by time and type and if alarm interrupt pin should be enabled
 * @param dydw if enabled, bit 6 defines date of month match else it defines day of week match
 * @param hour Alarm hour
 * @param minute Alarm minute
 * @param second Alarm second
 * @param alarmMode alarm mode by alarm type (differs for alarm 1 and 2)
 * @param interruptEnable defines, if the interrupt pin should be linked to this alarm
 */
void DS3231::setAlarm2(uint8_t dydw, uint8_t hour, uint8_t minute, DS3231_alarm2_t mode, bool interruptEnable) {
    minute = dec2bcd(minute);
    hour = dec2bcd(hour);
    dydw = dec2bcd(dydw);

    switch (mode) {
        case DS3231_EVERY_MINUTE:
            minute |= 0b10000000;
            hour |= 0b10000000;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_M:
            minute &= 0b01111111;
            hour |= 0b10000000;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_H_M:
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw |= 0b10000000;
            break;

        case DS3231_MATCH_DT_H_M:
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw &= 0b01111111;
            break;

        case DS3231_MATCH_DY_H_M:
            minute &= 0b01111111;
            hour &= 0b01111111;
            dydw &= 0b01111111;
            dydw |= 0b01000000;
            break;
    }

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_REG_ALARM_2);
    Wire.write(minute);
    Wire.write(hour);
    Wire.write(dydw);
    Wire.endTransmission();

    setInterruptAlarm2(interruptEnable);

    clearAlarm2();
}

/*
 * A1F of REG_STATUS: check if alarm 1 occured
 * @return true or false
 */
bool DS3231::isAlarm2(bool clear) {
    uint8_t alarm;

    alarm = readRegister8(DS3231_REG_STATUS);
    alarm &= 0b00000010;

    if (alarm && clear) {
        clearAlarm2();
    }

    return alarm;
}

/*
 * A2IE of REG_CONTROL: set alarm interrupt for alarm 2
 * @param enabled set interrupt true or false
 */
void DS3231::setInterruptAlarm2(bool armed) {
    uint8_t value;
    value = readRegister8(DS3231_REG_CONTROL);

    if (armed) {
        value |= 0b00000010;
    } else {
        value &= 0b11111101;
    }

    writeRegister8(DS3231_REG_CONTROL, value);
}

/*
 * A2IE of REG_CONTROL: get alarm interrupt setting for alarm 1
 * @return true or false if alarm interrupt is enabled
 */
bool DS3231::getInterruptAlarm2(void) {
    uint8_t value;
    value = readRegister8(DS3231_REG_CONTROL);
    value &= 0b00000010;
    value >>= 1;
    return value;
}

/*
 * A1F of REG_STATUS: clear/quit alarm 1 flag
 */
void DS3231::clearAlarm2(void) {
    uint8_t value;

    value = readRegister8(DS3231_REG_STATUS);
    value &= 0b11111101;

    writeRegister8(DS3231_REG_STATUS, value);
}

// PRIVATE

uint8_t DS3231::bcd2dec(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd % 16);
}

uint8_t DS3231::dec2bcd(uint8_t dec) {
    return ((dec / 10) * 16) + (dec % 10);
}

long DS3231::time2long(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    return ((days * 24L + hours) * 60 + minutes) * 60 + seconds;
}

uint16_t DS3231::date2days(uint16_t year, uint8_t month, uint8_t day) {
    year = year - 2000;

    uint16_t days16 = day;

    for (uint8_t i = 1; i < month; ++i) {
        days16 += pgm_read_byte(daysArray + i - 1);
    }

    if ((month == 2) && isLeapYear(year)) {
        ++days16;
    }

    return days16 + 365 * year + (year + 3) / 4 - 1;
}

uint8_t DS3231::daysInMonth(uint16_t year, uint8_t month) {
    uint8_t days;

    days = pgm_read_byte(daysArray + month - 1);

    if ((month == 2) && isLeapYear(year)) {
        ++days;
    }

    return days;
}

uint16_t DS3231::dayInYear(uint16_t year, uint8_t month, uint8_t day) {
    uint16_t fromDate;
    uint16_t toDate;

    fromDate = date2days(year, 1, 1);
    toDate = date2days(year, month, day);

    return (toDate - fromDate);
}

bool DS3231::isLeapYear(uint16_t year) {
    return (year % 4 == 0);
}

uint8_t DS3231::dow(uint16_t y, uint8_t m, uint8_t d) {
    uint8_t dow;

    y -= m < 3;
    dow = ((y + y / 4 - y / 100 + y / 400 + pgm_read_byte(dowArray + (m - 1)) + d) % 7);

    if (dow == 0) {
        return 7;
    }

    return dow;
}

uint32_t DS3231::unixtime(void) {
    uint32_t u;

    u = time2long(date2days(t.year, t.month, t.day), t.hour, t.minute, t.second);
    u += 946681200;

    return u;
}

uint8_t DS3231::conv2d(const char* p) {
    uint8_t v = 0;

    if ('0' <= *p && *p <= '9') {
        v = *p - '0';
    }

    return 10 * v + *++p - '0';
}

void DS3231::writeRegister8(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t DS3231::readRegister8(uint8_t reg) {
    uint8_t value;
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 1);
    while (!Wire.available()) {
    };
    value = Wire.read();
    Wire.endTransmission();

    return value;
}