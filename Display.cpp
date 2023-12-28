#include "Display.h"

#include "Arduino.h"
#include "lcdgfx.h"  // Bibliothek Display https://github.com/lexus2k/lcdgfx

// ------------ PUBLIC ------------

void displayOscar::initialize() {
    begin();
    setFixedFont(ssd1306xled_font6x8);
    clear();
    lcd_delay(1000);
    fill(0x00);

    displayState = MENU_MAIN;
    menuItem = 0;
}

/*
 * Set new display state by display state type
 * @param newState state to set (DISPLAY_STATE type)
 */
void displayOscar::setDisplayState(DISPLAY_STATE newState) {
    displayState = newState;
}

/*
 * Get display state as display state type
 * @return displayState (DISPLAY_STATE type)
 */
DISPLAY_STATE displayOscar::getDisplayState() {
    return displayState;
}

/*
 * Set new menu item
 * @param newItem Item value to set
 */
void displayOscar::setMenuItem(uint8_t newItem) {
    menuItem = newItem;
}

/*
 * Get menu item as an integer
 * @return menuItem
 */
uint8_t displayOscar::getMenuItem() {
    return menuItem;
}

/*
 * Render time in format "hh:mm" to a specific char position to display.
 * @param hour hour to be rendered.
 * @param minute minute to be rendered.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 * @param charNr Char position X (0-20)to render to. Default: 0
 */
void displayOscar::renderTime(uint8_t hour, uint8_t minute, uint8_t lineNr, uint8_t charNr) {
    char buffer[25];
    sprintf(buffer, "%02d:%02d", hour, minute);
    printFixed(calcCursorX(charNr), calcCursorY(lineNr), buffer);
}

/*
 * Render time in format "DD.MM.YYYY" to a specific char position to display.
 * @param day day to be rendered.
 * @param month month to be rendered.
 * @param year year to be rendered.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 * @param charNr Char position X (0-20) to render to. Default: 0
 */
void displayOscar::renderDate(uint8_t day, uint8_t month, uint16_t year, uint8_t lineNr, uint8_t charNr) {
    char buffer[25];
    sprintf(buffer, "%02d.%02d.%04d", day, month, year);
    printFixed(calcCursorX(charNr), calcCursorY(lineNr), buffer);
}

/*
 * Render temperature in format "12C" to a specific char position to display.
 * @param temperature Temperature to be rendered.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 * @param charNr Char position X (0-20) to render to. Default: 0
 */
void displayOscar::renderTemperature(float temperature, uint8_t lineNr, uint8_t charNr) {
    char buffer[25];
    char helper[25];
    dtostrf(temperature, 2, 0, helper);
    sprintf(buffer, "%2sC", helper);
    printFixed(calcCursorX(charNr), calcCursorY(lineNr), buffer);
}

/*
 * Render  in format "Neig:   -12.3 |   1.1" to a specific line position to display.
 * Fills the whole line.
 * @param phiX x-angle to be rendered.
 * @param phiY y-angle to be rendered.
 * @param charNr Char position X (0-20) to render to. Default: 0
 */
void displayOscar::renderHumidity(float humidity, uint8_t lineNr) {
    char buffer[25];
    char helper[25];
    dtostrf(humidity, 2, 0, helper);
    sprintf(buffer, "Luftfeucht:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%2s%%", helper);
    printFixed(calcCursorX(18), calcCursorY(lineNr), buffer);
}

void displayOscar::renderPageNr(uint8_t lineNr, uint8_t charNr) {
    char buffer[25];
    sprintf(buffer, "%2d", displayState);
    printFixed(calcCursorX(charNr), calcCursorY(lineNr), buffer);
}

/*
 * Render angles in format "Neig:   -12.3 |   1.1" to a specific line position to display.
 * Fills the whole line.
 * @param phiX x-angle to be rendered.
 * @param phiY y-angle to be rendered.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderAngles(float phiX, float phiY, uint8_t lineNr) {
    char buffer[25];
    char helper[25];
    char helper2[25];
    dtostrf(phiX, 5, 1, helper);
    dtostrf(phiY, 5, 1, helper2);
    sprintf(buffer, "Neigung:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s, %5s", helper, helper2);
    printFixed(calcCursorX(9), calcCursorY(lineNr), buffer);
}

/*
 * Displays a line with "Ja?" and "Nein?" and an arrow to indicate the current selection.
 * @param yes Indicicator to show the selection of yes or no.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderYesNo(bool yes, uint8_t lineNr) {
    char buffer[25];
    if (yes) {
        sprintf(buffer, "  Nein?");
        printFixed(calcCursorX(4), calcCursorY(lineNr), buffer);
        buffer[0] = 0;
        sprintf(buffer, "->Ja?");
        printFixed(calcCursorX(12), calcCursorY(lineNr), buffer);
    } else {
        sprintf(buffer, "->Nein?");
        printFixed(calcCursorX(4), calcCursorY(lineNr), buffer);
        buffer[0] = 0;
        sprintf(buffer, "  Ja?");
        printFixed(calcCursorX(12), calcCursorY(lineNr), buffer);
    }
}

/*
 * Render information about grey water to a specific line position to display.
 * Fills the whole line.
 * @param waterLevel true and not okay, if grey water is full.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderGreyWater(bool waterLevel, uint8_t lineNr) {
    char buffer[25];
    sprintf(buffer, "Abwasser:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s", waterLevel ? "voll!" : "okay");
    printFixed(calcCursorX(16), calcCursorY(lineNr), buffer);
}

/*
 * Render information about fresh water to a specific line position to display.
 * Fills the whole line.
 * @param waterLevel true and okay, if fresh water is not empty.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderFreshWater(bool waterLevel, uint8_t lineNr) {
    char buffer[25];
    sprintf(buffer, "Frischwasser:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s", waterLevel ? "okay" : "leer!");
    printFixed(calcCursorX(16), calcCursorY(lineNr), buffer);
}

/*
 * Render information about the battery state of charge.
 * Fills the whole line.
 * @param soc State of charge in %
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderBatterySOC(int soc, uint8_t lineNr = 0) {
    char buffer[25];
    sprintf(buffer, "Ladung:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%2d %%", soc);
    printFixed(calcCursorX(16), calcCursorY(lineNr), buffer);
}

/*
 * Render information about the battery voltage.
 * Fills the whole line.
 * @param voltage Voltage in V.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderBatteryVoltage(float voltage, uint8_t lineNr) {
    char buffer[25];
    char helper[25];
    dtostrf(voltage, 5, 2, helper);
    sprintf(buffer, "Spannung:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s V", helper);
    printFixed(calcCursorX(13), calcCursorY(lineNr), buffer);
}

/*
 * Render information about the battery current.
 * Fills the whole line.
 * @param current Current in A.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderBatteryCurrent(float current, uint8_t lineNr) {
    char buffer[25];
    char helper[25];
    dtostrf(current, 5, 2, helper);
    sprintf(buffer, "Staerke:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s A", helper);
    printFixed(calcCursorX(13), calcCursorY(lineNr), buffer);
}

/*
 * Render information about the battery power.
 * Fills the whole line.
 * @param power Power in W
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderBatteryPower(float power, uint8_t lineNr) {
    char buffer[25];
    char helper[25];
    dtostrf(power, 5, 1, helper);
    sprintf(buffer, "Leistung:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s W", helper);
    printFixed(calcCursorX(13), calcCursorY(lineNr), buffer);
}

/*
 * Render information about the battery energy consumption of the last hours.
 * Fills the whole line.
 * @param energy24 Energy array in Ah of the last count hours
 * @param count Number of entries in energy24
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderBatteryEnergy(float* energy24, uint8_t count, uint8_t lineNr) {
    // calculate the sum of energy entries
    float energy = 0;
    for (uint8_t i = 0; i <= count; i++) {
        energy += energy24[i];
    }

    char buffer[25];
    char helper[25];
    
    dtostrf(energy, 5, 1, helper);
    sprintf(buffer, "Verbrauch:");
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);
    buffer[0] = 0;
    sprintf(buffer, "%5s Ah", helper);
    printFixed(calcCursorX(13), calcCursorY(lineNr), buffer);
}

/*
 * Render a whole 8bit integer array with a one char label to the display.
 * Fills the whole line, not overflow protected, care length of array.
 * @param array Integer array to render.
 * @param startN First item to print.
 * @param endN Last item to print.
 * @param label One char to label the data.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderInt8Array(int8_t* array, uint8_t startN, uint8_t endN, char* label, uint8_t lineNr) {
    char buffer1[25];
    char buffer2[25];
    sprintf(buffer1, "%s: ", label);
    // printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);

    for (uint8_t i = startN; i <= endN; i++) {
        if (i < endN) {
            sprintf(buffer2, "%2d ", array[i]);
        } else {
            sprintf(buffer2, "%2d", array[i]);
        }
        strcat(buffer1, buffer2);
        buffer2[0] = 0;
    }
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer1);
}

/*
 * Render a whole float integer array which gets converted to int with a one char label to the display.
 * Fills the whole line, not overflow protected, care length of array.
 * @param array Integer array to render.
 * @param startN First item to print.
 * @param endN Last item to print.
 * @param label One char to label the data.
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderFloatIntArray(float* array, uint8_t startN, uint8_t endN, char* label, uint8_t lineNr) {
    char buffer1[25];
    char buffer2[25];
    sprintf(buffer1, "%s: ", label);
    // printFixed(calcCursorX(0), calcCursorY(lineNr), buffer);

    for (uint8_t i = startN; i <= endN; i++) {
        int value = array[i] + 0.5;
        if (i < endN) {
            sprintf(buffer2, "%2d ", value);
        } else {
            sprintf(buffer2, "%2d", value);
        }
        strcat(buffer1, buffer2);
        buffer2[0] = 0;
    }
    printFixed(calcCursorX(0), calcCursorY(lineNr), buffer1);
}

/*
 * Render text at a specific location on the display.
 * @param text Text to render.
 * @param charNr Char position X (0-20) to render to. Default: 0
 * @param lineNr Line position Y (0-7) to render to. Default: 0
 */
void displayOscar::renderText(char text[], uint8_t charNr, uint8_t lineNr) {
    printFixed(calcCursorX(charNr), calcCursorY(lineNr), text);
}

void displayOscar::clearLine(uint8_t lineNr) {
    setColor(0x00);
    fillRect(0, calcCursorY(lineNr), 127, calcCursorY(lineNr + 1) - 1);
    setColor(0xFF);
}

/*
 * Render a line at the top of a page line
 * @param lineNr Line position Y (0-7) to render the line to.
 */
void displayOscar::renderHeadline(uint8_t lineNr) {
    drawHLine(calcCursorX(0), calcCursorY(lineNr) + 2, 127);
}

/*
 * Render a line at the bottom of a page line
 * @param lineNr Line position Y (0-7) to render the line to.
 */
void displayOscar::renderFootline(uint8_t lineNr) {
    drawHLine(calcCursorX(0), calcCursorY(lineNr) + 5, 127);
}

// ------------ PRIVATE ------------

/*
 * Calculates the starting x-position of the cursor pixel for text.
 * @param charNr Char position X (0-20)
 */
uint8_t displayOscar::calcCursorX(uint8_t charNr) {
    return charNr * 6;
}

/*
 * Calculates the starting y-position of the cursor pixel for text.
 * @param charNr Char position Y (0-7)
 */
uint8_t displayOscar::calcCursorY(uint8_t lineNr) {
    return lineNr * 8;
}