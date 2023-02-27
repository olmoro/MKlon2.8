/*
    Дисплей TFT 1.8" SPI 128*160  VSPI
    2023.02.02
    OlMoro
*/
#include "TFT_eSPI.h" // Graphics and font library for ST7735 driver chip
#include "mdisplay.h"
#include <SPI.h>
#include <Arduino.h>
#include <iostream>
#include<string>
#include<sstream> // for using stringstream

#include <cstring>

using namespace std;

MDisplay::MDisplay()
{
    tft = new TFT_eSPI();         // Invoke library, pins defined in platformio.ini
    initTFT();
}

MDisplay::~MDisplay()   { delete tft; }


void MDisplay::initTFT()
{
  tft->init();
  tft->setRotation(2);                      // (0)  moro=2 поворот на 180°  (Alt+0176)
  tft->fillScreen(TFT_BLACK);                // Цвет фона вне круга
  tft->setTextColor(TFT_GREEN, TFT_BLACK);  // Adding a black background colour erases previous text automatically
}

void MDisplay::initLCD()
{
    initTFT();             //     // включая CLOCK
}

//void MDisplay::runDisplay(float u, float i, float celsius, int time, float ah, int percent, bool ap)
void MDisplay::runDisplay( float celsius, bool ap)
{
    primaryV ? displayVoltage() : displayPidI();
    primaryI ? displayCurrent() : displayPidV();
    displayMode();
    displayHelp();
    displayFulfill();
    displayDuration();
    displayAmphours();
    displayHeap();
    displayCelsius( celsius );
    displayLabel();
}

void MDisplay::showVolt( float _volt, uint8_t _places )
{
    volt    = _volt;
    placesV = _places;
    primaryV = true;
}

void MDisplay::displayVoltage()
{
    uint8_t xpos = MVoltage::po_x;
    uint8_t ypos = MVoltage::po_y;
    tft->setTextColor(MVoltage::fgcolor, MVoltage::bgcolor); // Set foreground and backgorund colour
    tft->drawString("    ", xpos, ypos, MVoltage::font);
    if(abs(volt) < 10) xpos += 14;
    if(volt < 0) xpos -= 8;
    xpos += tft->drawFloat(volt, placesV, xpos, ypos, MVoltage::font);
    xpos += tft->drawString(" V   ", xpos, ypos, MVoltage::font);
}

void MDisplay::showPidV(float _pid, uint8_t _places)    // show KP, KI, KD
{
    pid    = _pid;
    placesP = _places;
    primaryI = false;
}

void MDisplay::displayPidV()
{
    uint8_t xpos = MPidV::po_x;
    uint8_t ypos = MPidV::po_y;
    tft->setTextColor(MPidV::fgcolor, MPidV::bgcolor); // Set foreground and backgorund colour
    tft->drawString("    ", xpos, ypos, MPidV::font);
    if(abs(pid) < 10) xpos += 14;
    if(pid < 0) xpos -= 8;
    xpos += tft->drawFloat(pid, placesP, xpos, ypos, MPidV::font);
    xpos += tft->drawString("       ", xpos, ypos, MPidV::font);
}


void MDisplay::showAmp( float _amp, uint8_t _places )
{
    amp     = _amp;
    placesI = _places;
    primaryI = true;
}

void MDisplay::displayCurrent()
{
    uint8_t xpos = MCurrent::po_x;
    uint8_t ypos = MCurrent::po_y;
    tft->setTextColor(MCurrent::fgcolor, MCurrent::bgcolor); // Set foreground and backgorund colour
    tft->drawString("    ", xpos, ypos, MCurrent::font);
    if(abs(amp) < 10) xpos += 14;
    if(amp < 0) xpos -= 8;
    xpos += tft->drawFloat(amp, placesI, xpos, ypos, MCurrent::font);
    xpos += tft->drawString(" A   ", xpos, ypos, MCurrent::font);
}

void MDisplay::showPidI(float _pid, uint8_t _places)    // show KP, KI, KD
{
    pid    = _pid;
    placesP = _places;
    primaryV = false;
}

void MDisplay::displayPidI()
{
    uint8_t xpos = MPidI::po_x;
    uint8_t ypos = MPidI::po_y;
    tft->setTextColor(MPidI::fgcolor, MPidI::bgcolor); // Set foreground and backgorund colour
    tft->drawString("    ", xpos, ypos, MPidI::font);
    if(abs(pid) < 10) xpos += 14;
    if(pid < 0) xpos -= 8;
    xpos += tft->drawFloat(pid, placesP, xpos, ypos, MPidI::font);
    xpos += tft->drawString("       ", xpos, ypos, MPidI::font);
}






void MDisplay::showMode(char *s) { strcpy( modeString, s ); }

// https://www.programiz.com/cpp-programming/string-float-conversion 
// Example 4: float and double to string Using stringstream
void MDisplay::showMode(char *s, float val)
{
    strcpy( modeString, s );

  //  Serial.print("value="); Serial.println(val,2);

        // creating stringstream objects
    std::stringstream ss1;
        // assigning the value of num_float to ss1
    ss1 << val;
        // initializing string variable with the value of ss1
        // and converting it to string format with str() function
    std::string str1 = ss1.str();
    //std::cout << "Float to String = " << str1 << std::endl;
        // Get C string equivalent. 
        // https://cplusplus.com/reference/string/string/c_str/
        // Returns a pointer to an array that contains a 
        // null-terminated sequence of characters (i.e., a C-string) representing
        // the current value of the string object.
    char * cstr = new char [str1.length()+1];
    std::strcpy (cstr, str1.c_str());
        // cstr now contains a c-string copy of str1
    char * p = std::strtok (cstr," ");
    while (p!=0)
    {
    //    std::cout << p << '\n';
        p = std::strtok(NULL," ");
    }
    
    // Serial.print("left_string="); Serial.println(strlen(s));
    // Serial.print("right_string="); Serial.println(strlen(cstr));
    
    strcat(modeString, cstr);
    strcat(modeString, "    ");     // Чистит хвост строки. Не выходить за предел массива (21)

    delete[] cstr;
}


void MDisplay::displayMode()
{
    uint8_t xpos = MMode::po_x;
    uint8_t ypos = MMode::po_y;
    tft->setTextColor( MMode::fgcolor, MMode::bgcolor ); // Set foreground and backgorund colour
    //tft->drawCentreString( modeString, xpos, ypos, MMode::font ); // 16 chars only
    tft->drawString( modeString, xpos, ypos, MMode::font ); // 16 chars only
}

void MDisplay::showHelp( char *s ) { strcpy( helpString, s ); }

void MDisplay::displayHelp()
{
    uint8_t xpos = MHelp::po_x;
    uint8_t ypos = MHelp::po_y;
    tft->setTextColor( MHelp::fgcolor, MHelp::bgcolor ); // Set foreground and backgorund colour
    tft->drawString( helpString, xpos, ypos, MHelp::font );
}

void MDisplay::fulfill( int _val, uint16_t _color )
{
    percent = _val;
    color   = _color;
}

void MDisplay::displayFulfill()
{
    //    uint32_t color = TFT_RED;           // TEST
    //    percent = 50;

    int x = percent + MBar::min_x;
    int maxX = 127; //100 - percent;

    for (int i = 0; i <= 5; i++)
    {
        int y = MBar::po_y + i;

        tft->drawFastHLine( MBar::min_x, y, x, color );
        tft->drawFastHLine( x, y, maxX, TFT_BLACK );        //TFT_DARKGREY ); 
    } 

}

void MDisplay::showDuration( int duration, int _plan )
{
    upSeconds = duration;
    plan = _plan;                   // 0 - hhh:mm:ss,  1 - hhh
}

void MDisplay::displayDuration()
{
    uint8_t xpos = MDuration::po_x;
    uint8_t ypos = MDuration::po_y;
    tft->setTextColor( MDuration::fgcolor, MDuration::bgcolor ); // Set foreground and backgorund colour

    upSeconds = upSeconds % 86400;          // С учетом периода вызова задачи
    unsigned long hours = upSeconds / 3600;
    upSeconds = upSeconds % 3600;
    unsigned long minutes = upSeconds / 60;
    upSeconds = upSeconds % 60;
    
    if( plan == SEC )
    {
        // hours:minutes:seconds
        xpos += tft->drawNumber(     hours, xpos, ypos, MDuration::font );
        xpos += tft->drawChar(         ':', xpos, ypos, MDuration::font );
        if( minutes <= 9 ) 
            xpos += tft->drawChar(    '0', xpos, ypos, MDuration::font );
        xpos += tft->drawNumber(  minutes, xpos, ypos, MDuration::font );
        xpos += tft->drawChar(        ':', xpos, ypos, MDuration::font );
        if( upSeconds <= 9 ) 
            xpos += tft->drawChar(    '0', xpos, ypos, MDuration::font );
        xpos += tft->drawNumber(upSeconds, xpos, ypos, MDuration::font );
        xpos += tft->drawString(    "   ", xpos, ypos, MDuration::font );
    }
    else
    {
        // hours
        xpos += 5;
        xpos += tft->drawNumber(    hours, xpos, ypos, MDuration::font );
        xpos += tft->drawString( " hours", xpos, ypos, MDuration::font );
    }       
}

void MDisplay::showAh( float val ) { ah = val; } 

void MDisplay::displayAmphours()
{
    uint8_t xpos = MAmpHours::po_x;
    uint8_t ypos = MAmpHours::po_y;
    tft->setTextColor( MAmpHours::fgcolor, MAmpHours::bgcolor ); // Set foreground and backgorund colour
    xpos += tft->drawFloat(  ah, 1, xpos, ypos, MAmpHours::font );
    xpos += tft->drawString( " Ah", xpos, ypos, MAmpHours::font );
}

void MDisplay::displayHeap()
{
    uint8_t xpos = MHeap::po_x;
    uint8_t ypos = MHeap::po_y;
    tft->setTextColor( MHeap::fgcolor, MHeap::bgcolor ); // Set foreground and backgorund colour
    tft->drawNumber( ESP.getFreeHeap(), xpos, ypos, MAmpHours::font );
}

void MDisplay::displayCelsius( float celsius )
{
    uint8_t xpos = MCelsius::po_x;
    uint8_t ypos = MCelsius::po_y;
    tft->setTextColor( MCelsius::fgcolor, MCelsius::bgcolor ); // Set foreground and backgorund colour
    xpos += tft->drawFloat( celsius, 1, xpos, ypos, MCelsius::font );
    xpos += tft->drawString(      " C", xpos, ypos, MCelsius::font );
}

void MDisplay::showLabel( char *s ) { strcpy( labelString, s ); }

void MDisplay::displayLabel()
{
    uint8_t xpos = MLabel::po_x;
    uint8_t ypos = MLabel::po_y;
    tft->setTextColor( MLabel::fgcolor, MLabel::bgcolor ); // Set foreground and backgorund colour
    tft->drawCentreString( labelString, xpos, ypos, MLabel::font ); // 16 chars only
}

//void MDisplay::getTextLabel( char *s ) { strcpy( newLabelString, s ); }
void MDisplay::barOff() { fulfill( 0, TFT_DARKGREY ); }

void MDisplay::initBar( uint16_t color )
{
    static int x = 0;
    x += 4;
    if (x >= 100 ) x = 0;
    fulfill( x, color );
}

void MDisplay::barStop() 
{ 
    fulfill( 100, TFT_RED ); 
}
