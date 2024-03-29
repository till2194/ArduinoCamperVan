/*
  Arduino project for a camper van. Includes serveral sensors and a display to show of informations.
  Created by T. Sepke, 2022

  --------------------------------------------
  Licensed under "MIT" License.

  Arduino Camper Van

  Copyright © 2022 till2194

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
  and associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute,
  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
  is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copiesor
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  --------------------------------------------
  List of sensors:
    - Gyroscope and accelerometer MPU6050 GY-521 (I2C)
        - VCC 3.3V
        - GND
        - SCL
        - SDA
        - XCL
        - XDA
        - AD0 -> VCC 3.3V (high for different I2C ID active)
        - INT
    - Real time clock module RTC DS3231 (3.3V I2C)
        - GND (brown)
        - VCC 3.3V (red)
        - SDA (green)
        - SCL (orange)
        - SQW
        - 32K
    - 1.3" OLED display SH1106 128x64 (I2C)
        - VDD 3.3V (black)
        - GND (white)
        - SCK (inside: blue; cable: grey)
        - SDA (inside: green; cable: purple)
    - Rotary encoder KY-040
        - GND (brown)
        - +/VCC 5V (inside: red; cable: white)
        - SW interrupt (inside: orange; cable: green)
        - DT digital (inside: yellow; cable: brown)
        - CLK digital (inside: green; cable: yellow)
    - Temperature and humidity sensor DHT11
        - GND, right pin (brown)
        - VCC 5V (orange)
        - Data digtal, left pin (red)
    - Current sensor ACS712 30A
        - VCC 5V (yellow)
        - OUT analog (blue)
        - GND (black)
    - Voltage sensor <25V
        - -/GND (black)
        - +/VCC 5V (yellow)
        - S/Data analog (green)
    - Water level switch grey water
        - VCC (white)
        - GND (black)
    - Water level switch fresh water
        - VCC (purple)
        - GND (grey)
    - LED (VCC: digital)

  --------------------------------------------
  Arduino pin layout:
        3.3V <-> 3.3V distribution
          5V <-> 5V distribution
         GND <-> GND distribution
         GND
         Vin
          A0 <-> Analog voltage sensor (green)
          A1 <-> Analog current sensor (blue)
          A2
          A3
    (SDA) A4 <-> Display I2C data (green)
    (SCL) A5 <-> Display I2C clock (blue)

         SCL <-> RTC I2C clock (orange)
         SDA <-> RTC I2C data (green)
        AREF
         GND
          13
          12
    (PWM) 11
    (PWM) 10
    (PWM)  9 <-> LED greywater full front (blue)
           8 <-> LED freshwater empty back (red)
           7 <-> DHT11 data (red)
    (PWM)  6 <-> Rotary encoder clock (green)
    (PWM)  5 <-> Rotary encoder data (yellow)
           4 <-> Water switch for grey water (black)
    (PWM)  3 <-> Water switch for fresh water (violet)
    (INT)  2 <-> Rotary encoder switch (orange)
    (TX)   1
    (RX)   0
*/

// ---------------------- Libraries ---------------------
#include "Arduino.h"          // Library: main arduino
#include "DS3231_minimal.h"   // Library: Real time device (minimal Version by me)
#include "MPU6050_minimal.h"  // Library: MPU accelerometer & gyrosope (minimal Version by me)
#include "Rotary.h"           // Library: Encoder https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
#include "dht_nonblocking.h"  // Library: DHT sensor
#include "display.h"          // Display based on lcdgfx library https://github.com/lexus2k/lcdgfx
#include "LookupTable1D.h"    // 1D lookup Class to interpolate data

// ---------------------- Settings ----------------------
// #define ROTARY_INTERRUPT      // active: interrupt mode of rotary switch; not active: polling mode
#define RTC_RESET_TIME false  // true: set time for RTC.

// --------------------- Debug Mode ---------------------
// #define DEBUG    // switch to (de)activate serial debug output
// #define PLOTTER  // switch to (de)activate serial plotter output

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(F(x))
#define DEBUG_PRINTLN(x) Serial.println(F(x))
#define DEBUG_PRINTVAR(x) Serial.print(x)
#define DEBUG_PRINTVARLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTVAR(x)
#define DEBUG_PRINTVARLN(x)
#endif

// ---------------------- Defines -----------------------
#define MPU_I2C_ADDR 0x69     // I2C address of the MPU sensor (0x68 for AD0=LOW, 0x69 for AD0=HIGH)
#define DHT_TYPE DHT_TYPE_11  // Type of DHT sensor
#define ROTARY_PIN_SW 2       // Rotary switch pin (interrupt/poll)
#define FRESH_WATER_PIN 3     // Water (fresh) switch pin (digital)
#define GREY_WATER_PIN 4      // Water (grey) switch pin (digital)
#define ROTARY_PIN_DT 5       // Rotary data pin (digital)
#define ROTARY_PIN_CLK 6      // Rotary clock pin (digital)
#define DHT_PIN 7             // DHT data pin (digital)
#define FRESH_WATER_LED_PIN 8 // LED pin for fresh water empty (digital)
#define GREY_WATER_LED_PIN 9  // LED pin for grey water full (digital)
#define VOLTAGE_PIN A0        // Voltage read pin (analog)
#define CURRENT_PIN A1        // Current read pin (analog)

#define DHT_HISTORY_COUNT 24  // Number of DHT history data
#define MPU_HISTORY_COUNT 10  // Number of MPU history data
#define DC_ENERGY_COUNT 24    // Number of DC energy history data
#define STANDBY_DELAY 60      // Time till standby (in s)

// --------------------- Data struct types ---------------------
struct DHTDataType  // DHT data type as a struct
{
    float temperature;  // Temperature value
    float humidity;     // Humidity value
};

struct DHTHistoryType  // DHT history data type as a struct
{
    int8_t hour[DHT_HISTORY_COUNT];         // Hour array
    int8_t temperature[DHT_HISTORY_COUNT];  // Temperature array
    int8_t humidity[DHT_HISTORY_COUNT];     // Humidity array
};

struct MPUHistoryType  // MPU history data type as a struct
{
    float phiX[MPU_HISTORY_COUNT];  // PhiX array
    float phiY[MPU_HISTORY_COUNT];  // PhiY array
};

struct WaterDataType  // Water data type as struct
{
    bool fresh;  // true, when freshwater is not empty
    bool grey;   // true, when greywater is full
};

struct DCDataType  // Data type for DC information
{
    float voltage;                      // Voltage value in V
    float current;                      // Current value in A
    float power;                        // Power value in W
    float energy;                       // Energy accumulation Ah of the last hour
    int soc;                            // State of charge of the battery
    float energy24[DC_ENERGY_COUNT];    // Energy data in Ah of the last 24 hours
};

double voltageMap[] = {11.51, 11.66, 11.81, 11.96, 12.10, 12.24, 12.37, 12.50, 12.62, 12.73};   // Battery voltage data
double socMap[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};                                    // Battery soc data

// --------------- Classes & Data structs ---------------
MPU6050 MPU_device = MPU6050(MPU_I2C_ADDR);             // MPU6050 accelerometer & gyrosope device
MPUHistoryType MPUHistory;                              // MPU History data struct with phix and phiy
DS3231 RTC_device;                                      // DS3231 clock device
DHT_nonblocking dht_sensor(DHT_PIN, DHT_TYPE);          // DHT class (pin, sensor_type)
DHTDataType DHTData;                                    // DHT struct from the DHT sensor
DHTHistoryType DHTHistory;                              // DHT struct for DHT sensor history
Rotary rotary = Rotary(ROTARY_PIN_DT, ROTARY_PIN_CLK);  // Rotary Definition als Poll
WaterDataType WaterData = {true, false};                // Water struct for freshwater and greywater sensors
DCDataType DCData = {0, 0, 0, 0, 0};                    // DC struct for current, voltage and power
displayOscar display(-1);                               // Display class inherited from lcdgfx
RTCDateTime RTCSettings;                                // Date time to set a new time for RTC device
// LookupTable1D batteryLookup(voltageMap, socMap, sizeof(voltageMap) / sizeof(voltageMap[0]));    // 1D Loopup for battery map

// ------------------ Global Variables ------------------
unsigned long timestampIdle = 0;            // Timestamp since last user action
unsigned long timestampSensors = 0;         // Timestamp since last MPU read
unsigned long timestampDisplay = 0;         // Timestamp since last display refresh
unsigned long timestampInterrupt = 0;       // Timestamp since last interrupt
unsigned long timestampFreshWaterLED = 0;   // Timestamp since the LED turned on

// --------------------- Main Setup ---------------------
void setup() {
#if defined(DEBUG) || defined(PLOTTER)
    Serial.begin(9600);
    int sizeTimestamps = sizeof(timestampIdle) + sizeof(timestampDisplay) + sizeof(timestampInterrupt) + sizeof(timestampSensors);
#endif
    DEBUG_PRINTLN("Byte sizes of:");
    DEBUG_PRINT("dht_sensor: ");
    DEBUG_PRINTVARLN((int)sizeof(dht_sensor));
    DEBUG_PRINT("DHTData: ");
    DEBUG_PRINTVARLN((int)sizeof(DHTData));
    DEBUG_PRINT("DHTHistory: ");
    DEBUG_PRINTVARLN((int)sizeof(DHTHistory));
    DEBUG_PRINT("MPU: ");
    DEBUG_PRINTVARLN((int)sizeof(MPU_device));
    DEBUG_PRINT("MPUHistory: ");
    DEBUG_PRINTVARLN((int)sizeof(MPUHistory));
    DEBUG_PRINT("RTC: ");
    DEBUG_PRINTVARLN((int)sizeof(RTC_device));
    DEBUG_PRINT("rotary: ");
    DEBUG_PRINTVARLN((int)sizeof(rotary));
    DEBUG_PRINT("display: ");
    DEBUG_PRINTVARLN((int)sizeof(display));
    DEBUG_PRINT("WaterData: ");
    DEBUG_PRINTVARLN((int)sizeof(WaterData));
    DEBUG_PRINT("timestamps: ");
    DEBUG_PRINTVARLN(sizeTimestamps);

    DEBUG_PRINTLN("------ Setup begin ------");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    DEBUG_PRINTLN("- LED Setup completed");
    RTC_setup(RTC_RESET_TIME);
    DEBUG_PRINTLN("- RTC Setup completed");
    MPU_setup();
    DEBUG_PRINTLN("- MPU Setup completed");
    rotary_setup();
    DEBUG_PRINTLN("- Rotary Setup completed");
    waterLevel_setup();
    DEBUG_PRINTLN("- WaterLevel Setup completed");
    display.initialize();
    DEBUG_PRINTLN("- Display Setup completed");
    timestampIdle = millis();
    DEBUG_PRINTLN("------ Setup ended ------");
}

// ---------------------- Main Loop ---------------------
void loop() {
    // Rotary reading: -> check process -> activate input turn with the direction
    unsigned char result = rotary.process();
    if (result == DIR_CW) {
        DEBUG_PRINTLN("Rotary was turned CW");
        rotary_turn(1);
    } else if (result == DIR_CCW) {
        DEBUG_PRINTLN("Rotary was turned CCW");
        rotary_turn(-1);
    }

    #ifndef ROTARY_INTERRUPT
    if (digitalRead(ROTARY_PIN_SW) == LOW) {
        rotary_interrupt();
    }
    #endif

    // Sensor readings:
    unsigned long dtSensor = millis() - timestampSensors;
    if (dtSensor > 500) {
        // DEBUG_PRINTLN("Reading sensor data...");
        timestampSensors = millis();
        MPU_device.getData();
        DCData = DC_getData(dtSensor);
        WaterData = getWaterData();
        pushFloatArray(MPUHistory.phiX, MPU_device.data.phiX, MPU_HISTORY_COUNT);
        pushFloatArray(MPUHistory.phiY, MPU_device.data.phiY, MPU_HISTORY_COUNT);
        DEBUG_PLOTTER();
    }

    // DHT reading: -> Try to get new DHT data
    if (DHT_read(&DHTData.temperature, &DHTData.humidity) == true) {
        // DEBUG_PRINTLN("Reading DHT sensor...");
    }

    // Get new time data
    RTC_device.getDateTime();

    // Every full hour: -> Save data to history arrays
    if (RTC_device.isAlarm1()) {
        RTC_device.getDateTime();  // update datetime, to prevent offsync between alarm timing and datetime update.
        pushFloatArray(DCData.energy24, DCData.energy, DC_ENERGY_COUNT);
        DCData.energy = 0;
        pushInt8Array(DHTHistory.hour, RTC_device.t.hour, DHT_HISTORY_COUNT);
        pushInt8Array(DHTHistory.temperature, DHTData.temperature, DHT_HISTORY_COUNT);
        pushInt8Array(DHTHistory.humidity, DHTData.humidity, DHT_HISTORY_COUNT);
    }

    // display refresh at 4 fps (main menu needs ~ 30ms, leave some time to poll rotary reading etc.)
    if (millis() - timestampDisplay > 250) {
        timestampDisplay = millis();
        display_refresh();
    }

    // Enter standby after certain time under certain conditions
    bool check1 = millis() - timestampIdle > (unsigned long)STANDBY_DELAY * 1000;
    bool check2 = display.getDisplayState() != STANDBY;
    bool check3 = !WaterData.grey;
    bool check4 = display.getMenuItem() <= 0;
    if (check1 && check2 && check3 && check4) {
        DEBUG_PRINTLN("Entering standby...");
        display.setDisplayState(STANDBY);
        display.clear();
        display_refresh();
    }
}

// ----------------------- Setups -----------------------

void (*restartFunc)(void) = 0;  // declare restart function @ address 0

/*
 * Initialize the RTC device with 2 alarms and get the first time data.
 * @param setTime set true to update the saved time on the RTC device.
 */
void RTC_setup(bool setTime) {
    RTC_device.begin();

    // Send sketch compiling time to Arduino (delayed by ~6 seconds!)
    if (setTime) {
        RTC_device.setDateTime(__DATE__, __TIME__);
        DEBUG_PRINTLN("Updated RTC time!");
    }
    RTC_device.setAlarm1(0, 0, 0, 0, DS3231_MATCH_M_S, false);  // match every hour
    // RTC_device.setAlarm2(0, 0, 0, DS3231_EVERY_MINUTE, false);  // match every minute
    RTC_device.getDateTime();
}

/*
 * Initialize the MPU device and active the I2C Bypass. Also test the connection.
 */
void MPU_setup() {
    if (MPU_device.testConnection()) {
        MPU_device.initialize();
        MPU_device.setBypass();
        MPU_device.getData();
        DEBUG_PRINTLN("MPU6050 connected successfully!");
    } else {
        DEBUG_PRINTLN("MPU6050 not connected!");
    }
}

/*
 * Setup for the water level switches.
 * Defines the pins and the modes.
 */
void waterLevel_setup() {
    pinMode(GREY_WATER_PIN, INPUT_PULLUP);
    pinMode(FRESH_WATER_PIN, INPUT_PULLUP);
    pinMode(FRESH_WATER_LED_PIN, OUTPUT);
    digitalWrite(FRESH_WATER_LED_PIN, LOW);
    pinMode(GREY_WATER_LED_PIN, OUTPUT);
    digitalWrite(GREY_WATER_LED_PIN, LOW);
}

/*
 * Setup for the rotary switch.
 * Select the pin mode and attatch the interrupt handler to the interrupt pin in interrupt mode.
 */
void rotary_setup() {
    pinMode(ROTARY_PIN_SW, INPUT);                                                    // Definition of the interrupt pin and handler
    digitalWrite(ROTARY_PIN_SW, HIGH);                                                // Activate input pullup

    #ifdef ROTARY_INTERRUPT
    attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_SW), rotary_interrupt, RISING);  // Attach Interrupt function at rising input (time when switch is depressed)
    #endif
}

// ------------------------ Reads -----------------------

/*
 * Read analog values from the DC pins.
 * Calculate power and energy consumption.
 * @return DC struct
 * @param dt time since last update in ms
 */
DCDataType DC_getData(unsigned long dt) {
    int VVraw = analogRead(VOLTAGE_PIN);                                // Raw voltage value at the voltage sensor pin
    float VVout = analogRead(VOLTAGE_PIN) * 5.0 / 1024.0;               // Voltage value in V of the voltage sensor (1024: 10bit resolution)
    float voltageRaw = VVout / 0.2;                                     // Input voltage in V of the voltage sensor Vout = Vin / (R2/(R1+R2)); R1=30k, R2=7.5k
    int VIraw = analogRead(CURRENT_PIN);                                // Raw voltage value at the current sensor pin
    float VIout = (analogRead(CURRENT_PIN) * 5000.0 / 1024.0);          // Voltage value in mV of the current sensor (1024: 10bit resolution)
    float currentRaw = -((VIout - 2500.0) / 66.2);                      // Current value in A of the current sensor (2500 mV offset, 66 A/mV)
    
    // filter current and voltage with exponential moving average
    float alpha = 0.3;                                                  // weight factor (0 < alpha < 1); alpha = 0.3 => tau = 1.4 sec
    float current = alpha * currentRaw + (1 - alpha) * DCData.current;  // EMA formula
    float voltage = alpha * voltageRaw + (1 - alpha) * DCData.voltage;  // EMA formula

    float power = voltage * current;                                        // Power value in W
    float energy = DCData.energy + (current * dt / 1000.0 / 60.0 / 60.0);   // Accumulate the used energy in Ah (dt in ms)
    
    // Update SoC by voltage only if there is no load
    int soc = DCData.soc;
    if (current <= 1.0) {
        // Simple linear function for SoC estimation
        float socRaw = 66.576 * voltage - 762.22;

        // 1D lookup interpolation
        // float socRaw = batteryLookup.interpolate(voltage);

        // Limits and rounding
        if (socRaw <= 0.0) {
            soc = 0.0;
        } else if (socRaw >= 100.0) {
            soc = 100.0;
        } else {
            // round to nearest 5% value
            soc = (((int) (socRaw + 2.5) ) / 5) * 5;
        }
    }
    return {voltage, current, power, energy, soc};
}

/*
 * Reads the digital pins for the two switches.
 * If a new state is noticed, the display wakes up from standby.
 * @return Returns the new water data
 */
WaterDataType getWaterData() {
    WaterDataType newWaterData;
    newWaterData.grey = getWaterLevel(GREY_WATER_PIN);
    newWaterData.fresh = getWaterLevel(FRESH_WATER_PIN);

    if (newWaterData.grey != WaterData.grey) {
        if (newWaterData.grey) {
            DEBUG_PRINTLN("Greywater maximum reached!");
            digitalWrite(GREY_WATER_LED_PIN, HIGH);  // turn on LED
            timestampIdle = millis();
            if (display.getDisplayState() == STANDBY) {
                display_wake_up();
            }
        } else {
            digitalWrite(GREY_WATER_LED_PIN, LOW);  // turn off LED
        }
    }

    if (newWaterData.fresh != WaterData.fresh) {
        // state change of the switch
        if (!newWaterData.fresh) {
            // Fresh water gets empty
            DEBUG_PRINTLN("Freshwater minimum reached!");
            digitalWrite(FRESH_WATER_LED_PIN, HIGH);  // turn on LED
            timestampIdle = millis();
            timestampFreshWaterLED = millis();
            if (display.getDisplayState() == STANDBY) {
                display_wake_up();
            }
        } else {
            // Fresh water has been refilled
            digitalWrite(FRESH_WATER_LED_PIN, LOW);  // turn off LED
        }
    }

    if (!newWaterData.fresh & millis() - timestampFreshWaterLED >= 10 * 60 * 1000) {
        digitalWrite(FRESH_WATER_LED_PIN, LOW);  // turn off LED
    }

    return newWaterData;
}

/*
 * Reads the digital pin of a water switch.
 * @return Returns true if the water level is reached.
 */
bool getWaterLevel(uint8_t PIN) {
    int pinRead = digitalRead(PIN);
    if (pinRead == HIGH) {
        // water level not reached
        return false;
    } else {
        // water level is reached
        return true;
    };
}

/*
 * Reads the digital pin of the DHT sensor. Acts as a statemachine to prevent polling.
 * @return Returns true if a new value was available.
 */
static bool DHT_read(float *temperatureMes, float *humidityMes) {
    static unsigned long timestampDHT = millis();

    // Measure once every four seconds.
    if (millis() - timestampDHT > 3000ul || millis() < 4000) {
        if (dht_sensor.measure(temperatureMes, humidityMes) == true) {
            timestampDHT = millis();
            return (true);
        }
    }
    return (false);
}

// --------------------- User Inputs --------------------

/*
 * User input by interrupt switch. Enter or leave the menu.
 */
void rotary_interrupt() {
    if (millis() - timestampInterrupt > 500) {
        DEBUG_PRINTLN("Switch has been pressed");
        timestampInterrupt = millis();
        timestampIdle = millis();
        DISPLAY_STATE displayState = display.getDisplayState();
        uint8_t menuItem = display.getMenuItem();
        switch (displayState) {
            case STANDBY:
                display_wake_up();
                break;
            case MENU_DHT:
                switch (menuItem) {
                    case 0:  // enter scrolling
                        display.setMenuItem(1);
                        break;
                    default:
                        // leave scrolling
                        display.setMenuItem(0);
                        break;
                }
                break;
            case MENU_CLOCK:
                // go through all clock items (i.e. hour, minute, day, month, year)
                switch (menuItem) {
                    case 5:     // last item (year)
                        display.setMenuItem(0);
                        RTC_device.getDateTime();
                        // update time on RTC device (but use the old seconds)
                        RTC_device.setDateTime(RTCSettings.year, RTCSettings.month, RTCSettings.day, RTCSettings.hour, RTCSettings.minute, RTC_device.t.second);
                        break;
                    default:    // select next item
                        display.setMenuItem(counter(menuItem, 1, 0, 5, false));
                        break;
                }
                break;
            case MENU_RESTART:
                switch (menuItem) {
                    case 0:
                        display.setMenuItem(1);
                        break;
                    case 1:  // no
                        display.setMenuItem(0);
                        break;
                    case 2:  // yes
                        restartFunc();
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
}

/*
 * User input by rotary turn. If not in menu change display state.
 * @direction direction of the rotary turn (1 for right/CW; -1 for left/CCW)
 */
void rotary_turn(int8_t direction) {
    timestampIdle = millis();
    DISPLAY_STATE displayState = display.getDisplayState();
    uint8_t menuItem = display.getMenuItem();

    if (menuItem == 0) {
        // change display state
        if (displayState == STANDBY) {
            // wake up from standy
            display_wake_up();
        } else {
            // scroll through menus
            display.setDisplayState(static_cast<DISPLAY_STATE>(counter(displayState, direction, 1, COUNT - 1, false)));
            display.clear();
        }
    } else {
        // change menuItem
        switch (displayState) {
            case MENU_DHT:
                // scroll through the DHT history data (use menuItem-1 as start point of array to print)
                display.setMenuItem(counter(menuItem, direction, 1, max(1, DHT_HISTORY_COUNT - 5), false));
                break;
            case MENU_CLOCK:
                // change the clock items and store them in the settings struct
                switch (menuItem) {
                    case 1:     // hour
                        RTCSettings.hour = counter(RTCSettings.hour, direction, 0, 23, true);
                        break;
                    case 2:     // minute
                        RTCSettings.minute = counter(RTCSettings.minute, direction, 0, 59, true);
                        break;
                    case 3:     // day
                        RTCSettings.day = counter(RTCSettings.day, direction, 1, 31, true);
                        break;
                    case 4:     // month
                        RTCSettings.month = counter(RTCSettings.month, direction, 1, 12, true);
                        break;
                    case 5:     // year
                        RTCSettings.year = counter16(RTCSettings.year, direction, 0, 65535, true);
                        break;
                    default:
                        break;
                }
                break;
            case MENU_RESTART:
                // change yes/no selection
                display.setMenuItem(counter(menuItem, direction, 1, 2, false));
                break;
            default:
                break;
        }
    }
}

// ----------------------- Display ----------------------

/*
 * Refreshes the display based on the display state.
 */
void display_refresh() {
    unsigned long dt = millis();
    switch (display.getDisplayState()) {
        case STANDBY:
            break;
        case MENU_MAIN:
            display_menu_main();
            break;
        case MENU_BATTERY:
            display_menu_battery();
            break;
        case MENU_DHT:
            display_menu_DHT();
            break;
        case MENU_CLOCK:
            display_menu_clock();
            break;
        case MENU_RESTART:
            display_menu_restart();
            break;
        default:
            display_menu_main();
    }
    dt = millis() - dt;
    // DEBUG_PRINT("dt (ms): ");
    // DEBUG_PRINTVARLN(dt);
}

/*
 * Wake up the display from standby (set state to main menu)
 */
void display_wake_up() {
    DEBUG_PRINTLN("Waking up from standby...");
    display.setDisplayState(MENU_MAIN);
}

/*
 * Render the header
 */
void display_render_header() {
    display.renderHeadline(1);
    display.renderTime(RTC_device.t.hour, RTC_device.t.minute);
    display.renderPageNr(0, 19);
}

/*
 * Render the footer
 */
void display_render_footer() {
    display.renderFootline(6);
    display.renderDate(RTC_device.t.day, RTC_device.t.month, RTC_device.t.year, 7, 0);
    display.renderTemperature(DHTData.temperature, 7, 18);
}

/*
 * Render the main menu
 */
void display_menu_main() {
    float phiX_mean = floatAverage(MPUHistory.phiX, MPU_HISTORY_COUNT);
    float phiY_mean = floatAverage(MPUHistory.phiY, MPU_HISTORY_COUNT);
    display_render_header();
    display_render_footer();

    display.renderHumidity(DHTData.humidity, 2);
    display.renderFreshWater(WaterData.fresh, 3);
    display.renderGreyWater(WaterData.grey, 4);
    display.renderAngles(phiX_mean, phiY_mean, 5);
}

/*
 * Render the menu with battery data.
 */
void display_menu_battery() {
    display_render_header();
    char buffer[25];
    sprintf(buffer, "Batteriewerte:");
    display.renderText(buffer, 0, 2);

    display.renderBatterySOC(DCData.soc, DCData.voltage, 3);
    display.renderBatteryVoltage(DCData.voltage, 4);
    display.renderBatteryCurrent(DCData.current, 5);
    display.renderBatteryPower(DCData.power, 6);
    display.renderBatteryEnergy(DCData.energy24, DC_ENERGY_COUNT, 7);
}

/*
 * Render the menu with DHT history data.
 */
void display_menu_DHT() {
    display_render_header();
    char buffer[25];
    if (display.getMenuItem() >= 1) {
        // show scrolling
        sprintf(buffer, "Vergangene Werte:<->");
    } else {
        sprintf(buffer, "Vergangene Werte:   ");
    }
    display.renderText(buffer, 0, 2);

    uint8_t start = display.getMenuItem() == 0 ? 0 : display.getMenuItem() - 1;
    uint8_t end = min(start + 5, DHT_HISTORY_COUNT - 1);  // only 6 items fit on the display (0-5)
    char label[2];
    sprintf(label, "h");
    display.renderInt8Array(DHTHistory.hour, start, end, label, 4);
    sprintf(label, "T");
    display.renderInt8Array(DHTHistory.temperature, start, end, label, 5);
    sprintf(label, "H");
    display.renderInt8Array(DHTHistory.humidity, start, end, label, 6);
    sprintf(label, "B");
    display.renderFloatIntArray(DCData.energy24, start, end, label, 7);
}

/*
 * Render the menu to change clock data.
 */
void display_menu_clock() {
    uint8_t menuItem = display.getMenuItem();

    display_render_header();
    char buffer[25];
    char clearBuffer[25];
    sprintf(clearBuffer, "                   ");

    sprintf(buffer, "Uhr Einstellungen:");
    display.renderText(buffer, 0, 2);

    display.renderTime(RTCSettings.hour, RTCSettings.minute, 5, 9);
    display.renderDate(RTCSettings.day, RTCSettings.month, RTCSettings.year, 7, 4);

    if (menuItem == 0) {
        RTCSettings = RTC_device.t;     // get current time
        // clear selector
        display.renderText(clearBuffer, 0, 4);
        display.renderText(clearBuffer, 0, 6);
    } else {
        // show selected item
        switch (menuItem) {
            case 1:     // hour
                sprintf(buffer, "__   ");
                display.renderText(buffer, 9, 4);
                display.renderText(clearBuffer, 0, 6);
                break;
            case 2:     // minute
                sprintf(buffer, "   __");
                display.renderText(buffer, 9, 4);
                display.renderText(clearBuffer, 0, 6);
                break;
            case 3:     // day
                sprintf(buffer, "__        ");
                display.renderText(clearBuffer, 0, 4);
                display.renderText(buffer, 4, 6);
                break;
            case 4:     // month
                sprintf(buffer, "   __     ");
                display.renderText(clearBuffer, 0, 4);
                display.renderText(buffer, 4, 6);
                break;
            case 5:     // year
                sprintf(buffer, "      ____");
                display.renderText(clearBuffer, 0, 4);
                display.renderText(buffer, 4, 6);
                break;
            default:
                break;
        }
    }
}

/*
 * Render the menu to restart the device.
 */
void display_menu_restart() {
    display_render_header();

    char buffer[25];
    sprintf(buffer, "Neustarten?");
    display.renderText(buffer, 5, 3);
    switch (display.getMenuItem()) {
        case 1:
            display.renderYesNo(false, 5);
            break;
        case 2:
            display.renderYesNo(true, 5);
            break;
        default:
            display.clearLine(5);
            break;
    }
}

// ------------------- Helper Functions -----------------

/*
 * Count up and down through a list of items defined by min and max.
 * @param oldValue Old list item.
 * @param direction +1/-1 to go up or down the list.
 * @param minValue Minimum item of the list.
 * @param maxValue Maximum item of the list.
 * @param reset Set true to allow to go from max to min item.
 */
int counter(int oldValue, int direction, int minValue, int maxValue, bool reset) {
    int newValue;
    if (reset) {
        if (oldValue == minValue && direction < 0) {
            newValue = maxValue;
        } else if (oldValue == maxValue && direction > 0) {
            newValue = minValue;
        } else {
            newValue = oldValue + direction;
        }
    } else {
        newValue = max(oldValue + direction, minValue);
        newValue = min(newValue, maxValue);
    }
    return newValue;
}

/*
 * Count up and down through a list of items defined by min and max for uint16.
 * @param oldValue Old list item.
 * @param direction +1/-1 to go up or down the list.
 * @param minValue Minimum item of the list.
 * @param maxValue Maximum item of the list.
 * @param reset Set true to allow to go from max to min item.
 */
int counter16(uint16_t oldValue, uint16_t direction, uint16_t minValue, uint16_t maxValue, bool reset) {
    int newValue;
    if (reset) {
        if (oldValue == minValue && direction < 0) {
            newValue = maxValue;
        } else if (oldValue == maxValue && direction > 0) {
            newValue = minValue;
        } else {
            newValue = oldValue + direction;
        }
    } else {
        newValue = max(oldValue + direction, minValue);
        newValue = min(newValue, maxValue);
    }
    return newValue;
}

/*
 * Calculate average of a float array.
 * @param array Float array to calculate the average of.
 * @param count Number of items in the array.
 * @return Average of the array
 */
float floatAverage(float *array, int count) {
    double sum = 0;
    for (int i = 0; i < count; i++) {
        sum += array[i];
    }
    return ((float)sum) / count;
}

/*
 * Pushes a int value to an array at index 0.
 * @param array Integer array to push a value to.
 * @param val Value to push.
 * @param count Number of items in the array.
 */
void pushInt8Array(int8_t *array, int8_t val, uint8_t count) {
    for (int i = count - 1; i >= 0; i--) {
        if (i == 0) {
            array[i] = val;
        } else {
            array[i] = array[i - 1];
        }
    }
    DEBUG_PRINT_INT8_ARRAY(array, count);
}

/*
 * Pushes a float value to an array at index 0.
 * @param array Float array to push a value to.
 * @param val Value to push.
 * @param count Number of items in the array.
 */
void pushFloatArray(float *array, float val, uint8_t count) {
    for (int i = count - 1; i >= 0; i--) {
        if (i == 0) {
            array[i] = val;
        } else {
            array[i] = array[i - 1];
        }
    }
    // DEBUG_PRINT_FLOAT_ARRAY(array, count);
}

// ------------------- Debug Functions ------------------

/*
 * Prints a integer array to the serial.
 * @param array Integer array to print.
 * @param count Number of items in the array.
 */
void DEBUG_PRINT_INT8_ARRAY(int8_t *array, uint8_t count) {
#ifdef DEBUG
    DEBUG_PRINT("Array: ");
    for (uint8_t i = 0; i < count; i++) {
        DEBUG_PRINTVAR(array[i]);
        if (i < count - 1) {
            DEBUG_PRINT(" | ");
        }
    }
    DEBUG_PRINTLN("");
#endif
}

/*
 * Prints a float array to the serial.
 * @param array Float array to print.
 * @param count Number of items in the array.
 */
void DEBUG_PRINT_FLOAT_ARRAY(float *array, uint8_t count) {
#ifdef DEBUG
    DEBUG_PRINT("Array: ");
    for (uint8_t i = 0; i < count; i++) {
        DEBUG_PRINTVAR(array[i]);
        if (i < count - 1) {
            DEBUG_PRINT(" | ");
        }
    }
    DEBUG_PRINTLN("");
#endif
}

void DEBUG_PLOTTER() {
#ifdef PLOTTER
    Serial.print(F("Time:"));
    Serial.print(RTC_device.t.hour);
    Serial.print(F(":"));
    Serial.print(RTC_device.t.minute);
    Serial.print(F(":"));
    Serial.print(RTC_device.t.second);
    Serial.print(F(","));
    Serial.print(F("Temperature:"));
    Serial.print(DHTData.temperature);
    Serial.print(F(","));
    Serial.print(F("Humidity:"));
    Serial.print(DHTData.humidity);
    Serial.print(F(","));
    Serial.print(F("AcX:"));
    Serial.print(MPU_device.data.AcX);
    Serial.print(F(","));
    Serial.print(F("AcY:"));
    Serial.print(MPU_device.data.AcY);
    Serial.print(F(","));
    Serial.print(F("AcZ:"));
    Serial.print(MPU_device.data.AcZ);
    Serial.print(F(","));
    Serial.print(F("GyX:"));
    Serial.print(MPU_device.data.GyX);
    Serial.print(F(","));
    Serial.print(F("GyY:"));
    Serial.print(MPU_device.data.GyY);
    Serial.print(F(","));
    Serial.print(F("GyZ:"));
    Serial.print(MPU_device.data.GyZ);
    Serial.print(F(","));
    Serial.print(F("MPU-Temp:"));
    Serial.print(MPU_device.data.Temp);
    Serial.print(F(","));
    Serial.print(F("phiX:"));
    Serial.print(MPU_device.data.phiX);
    Serial.print(F(","));
    Serial.print(F("phiY:"));
    Serial.print(MPU_device.data.phiY);
    Serial.print(F(","));
    Serial.print(F("Voltage:"));
    Serial.print(DCData.voltage);
    Serial.print(F(","));
    Serial.print(F("Current:"));
    Serial.print(DCData.current);
    Serial.print(F(","));
    Serial.println(F(""));
#endif
}