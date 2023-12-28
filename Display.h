#ifndef Display
#define Display

#include "Arduino.h"
#include "lcdgfx.h"  // Bibliothek Display https://github.com/lexus2k/lcdgfx

// DisplaySH1106_128x64_I2C displaySH1106(-1);

typedef enum {
    STANDBY,
    MENU_MAIN,
    MENU_BATTERY,
    MENU_DHT,
    MENU_CLOCK,
    MENU_RESTART,
    COUNT
} DISPLAY_STATE;

class displayOscar : DisplaySH1106_128x64_I2C {
   public:
    using DisplaySH1106_128x64_I2C::DisplaySH1106_128x64_I2C;
    void initialize();
    void setDisplayState(DISPLAY_STATE newState);
    DISPLAY_STATE getDisplayState();
    void setMenuItem(uint8_t newItem);
    uint8_t getMenuItem();
    
    using DisplaySH1106_128x64_I2C::clear;

    void renderTime(uint8_t hour, uint8_t minute, uint8_t lineNr = 0, uint8_t charNr = 0);
    void renderDate(uint8_t day, uint8_t month, uint16_t year, uint8_t lineNr = 0, uint8_t charNr = 0);
    void renderTemperature(float temperature, uint8_t lineNr = 0, uint8_t charNr = 0);
    void renderPageNr(uint8_t lineNr = 0, uint8_t charNr = 0);
    void renderHumidity(float humidity, uint8_t lineNr = 0);
    void renderAngles(float phiX, float phiY, uint8_t lineNr = 0);
    void renderYesNo(bool yes, uint8_t lineNr = 0);
    void renderFreshWater(bool waterLevel, uint8_t lineNr = 0);
    void renderGreyWater(bool waterLevel, uint8_t lineNr = 0);
    void renderBatteryVoltage(float voltage, uint8_t lineNr = 0);
    void renderBatteryCurrent(float current, uint8_t lineNr = 0);
    void renderBatteryPower(float power, uint8_t lineNr = 0);
    void renderBatterySOC(int soc, uint8_t lineNr = 0);
    void renderBatteryEnergy(float* energy24, uint8_t count, uint8_t lineNr = 0);

    void renderInt8Array(int8_t* array, uint8_t startN, uint8_t endN, char* label, uint8_t lineNr = 0);
    void renderFloatIntArray(float* array, uint8_t startN, uint8_t endN, char* label, uint8_t lineNr = 0);
    void renderText(char text[], uint8_t charNr = 0, uint8_t lineNr  = 0);
    void clearLine(uint8_t lineNr = 0);
    void renderHeadline(uint8_t lineNr = 0);
    void renderFootline(uint8_t lineNr = 0);

   private:
    DISPLAY_STATE displayState;
    uint8_t menuItem;
    uint8_t calcCursorX(uint8_t charNr);
    uint8_t calcCursorY(uint8_t lineNr);
};

#endif