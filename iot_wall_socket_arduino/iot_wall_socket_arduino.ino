#include <IRremote.h>
#include <EEPROM.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX_AS.h>    // Core graphics library
#include "Adafruit_ILI9341_AS.h" // Hardware-specific library
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <UTouch.h>
#include "EmonLib.h"                   // Include Emon Library

#define PIN_PIR_SENSOR              0      // Motion detector sensor
#define PIN_DHT22                   1      // Temperature and humidity sensor
#define PIN_IR_RECEIVER             3      // IR receiver
#define PIN_BUZZER                  13     // Active buzzer
#define PIN_LEDS                    14     // WS2812B LED data pin
#define PIN_SD_CS                   21
#define PIN_LIGHT_RELAY             29
#define PIN_FAN_RELAY1              30 
#define PIN_FAN_RELAY2              31
#define PIN_FAN_TRIAC1              25
#define PIN_FAN_TRIAC2              26
#define PIN_FAN_TRIAC3              27
#define PIN_FAN_TRIAC4              28
#define PIN_LIGHT_ZERO_DETECTOR     10
#define PIN_FAN_ZERO_DETECTOR       11
#define PIN_CURRENT_TRANSFORMER     A0

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

#define TFT_DC             23
#define TFT_CS             4
#define TFT_BACKLIGHT      15
Adafruit_ILI9341_AS tft = Adafruit_ILI9341_AS(TFT_CS, TFT_DC, 22);

//UTouch  myTouch( 20, 12, 16, 15, 2);
UTouch  myTouch( 20, 12, 17, 16, 2);
extern uint8_t SmallFont[];

#define LED_COUNT      45
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, PIN_LEDS, NEO_GRB + NEO_KHZ800);

IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

EnergyMonitor emon1;                   // Create an instance

DHT dht(PIN_DHT22, DHT22);

float temperatureC=0, temperatureF=0, humidity=0;
float heatIndexC=0, heatIndexF=0;
float powerCurrent = 0, powerWatts = 0;

int lightCommand=0, fanCommand=0;
int fan_speed=4, prev_fan_speed=0;

boolean bLightOn=false, bFanOn=false, bPrevFanOn=false;
boolean bLightRelayActivated=false, bFanRelay1Activated=false, bFanRelay2Activated=false;
volatile boolean bLightZeroCrossingDetected = false, bFanZeroCrossingDetected = false;


enum ledModes
{
  LedModeDefault=0,
  LedModeRainbow,
  LedModeCycle,
  LedModeCycle2,
  LedModeCycle3,
  LedModeCycle4,
  LedModeRandom,
  LedModeRandom2,
  LedModeRandomCycle,
  LedModeRandomCycle2
};
ledModes ledMode = LedModeRainbow;
int ledCommand=0;
boolean bLedOn = false;
boolean bColorChanged = true;
int colorRed=0,colorGreen=0,colorBlue=255;
int J=0,K=0,L=0,x=0,y=0,z=0, STEP=0;
struct rgb
{
  int r;
  int g;
  int b;
};
rgb leds[LED_COUNT];

int count = 0;
boolean bFirstTime = true;
unsigned long last_ir_cmd_received;
int pir = 0, last_pir = 0;
unsigned long noMotionDetectedTime      = 0;
boolean bBacklightOn = true;

unsigned long weatherUpdateInterval     = 30000;
unsigned long zeroCrossCheckInterval    = 5000;
unsigned long pirSensorCheckInterval    = 2000;
unsigned long sendDataToESP8266Interval = 60000;
unsigned long checkPowerUsageInterval   = 30000;
unsigned long checkAlarmInterval        = 20000;
unsigned long updateAlertInterval       = 600000;
unsigned long checkFanInterval          = 600000;
unsigned long checkLightInterval        = 10000;
unsigned long ledUpdateInterval         = 500;
unsigned long updateVarWindowInterval   = 20000;
unsigned long updateClockInterval       = 40000;
unsigned long updateBigClockInterval    = 10000;

unsigned long weatherUpdateTime     = 0;
unsigned long zeroCrossCheckTime    = 0;
unsigned long pirSensorCheckTime    = 0;
unsigned long sendDataToESP8266Time = 0;
unsigned long checkPowerUsageTime   = 0;
unsigned long checkAlarmTime        = 0;
unsigned long updateAlertTime       = 0;
unsigned long checkFanTime          = 0;
unsigned long checkLightTime        = 0;
unsigned long ledUpdateTime         = 0;
unsigned long updateVarWindowTime   = 0;
unsigned long updateClockTime       = 0;
unsigned long updateBigClockTime    = 0;

unsigned long previousMillis = 0, currentMillis = 0, elapsedMillis = 0;
unsigned long previousMillisAlarm = 0, currentMillisAlarm = 0, elapsedMillisAlarm = 0;
unsigned long previousMillisIntruderAlert = 0, currentMillisIntruderAlert = 0, elapsedMillisIntruderAlert = 0;
unsigned long previousMillisFan = 0, currentMillisFan = 0, elapsedMillisFan = 0;
unsigned long previousMillisMotion = 0, currentMillisMotion = 0, elapsedMillisMotion = 0;

enum screens
{
  HomeScreen,
  FanOptionsScreen,
  LightOptionsScreen,
  PinInputScreen,
  TimeOptionsScreen,
  BigClockScreen
};
screens screenSelected = HomeScreen;
boolean bInitializeHomeScreen = true;

enum fanOptions
{
  Decrease1StepFor1DegreeDrop,
  Decrease1StepFor2DegreesDrop,
  Decrease1StepEvery90Mins,
  Decrease1StepEvery120Mins,
  StopIfBelow,
  noFanOptionSelected
};
fanOptions fanOptionSelected = noFanOptionSelected;
float fanStopBelowTemperature = 25;
int fanStartingSpeed = 0;
float fanStartingTemperature = 0;
unsigned long fanRunTime = 0;

enum lightOptions
{
  LightFullyAutomatic,
  LightSwitchOffIfNoMotionFor2Mins,
  noLightOptionSelected
};
lightOptions lightOptionSelected = noLightOptionSelected;

enum alarmStates
{
  AlarmUnarmed,
  AlarmArming,
  AlarmArmed,
  AlarmTriggered
};
alarmStates alarmCurrentState = AlarmUnarmed;
alarmStates alarmLastState = AlarmUnarmed;
unsigned long alarmTimeElapsedAfterArming = 0;
unsigned long alarmActivateDelay = 60000; // 1min
//unsigned long alarmActivateDelay = 5000; // 5 secs
unsigned long alarmTimeElapsedAfterTriggering = 0;
unsigned long alarmPinInputDelay = 60000; // 1 min
//unsigned long alarmPinInputDelay = 5000; // 5 secs
boolean bAlarmIntruderAlertSent = false;

enum timeFanOptions
{
  TimeAlarmOptionFanStop,
  TimeAlarmOptionFanMax,
  TimeAlarmOptionFanNone
};

timeFanOptions timeFanOptionSelected = TimeAlarmOptionFanNone;

enum timeLightOptions
{
  TimeAlarmOptionLightOn,
  TimeAlarmOptionLightNone
};

timeLightOptions timeLightOptionSelected = TimeAlarmOptionLightNone;

int PIN = 1234;
int currentPIN = 0;

int intTimeHour                         = 0;
int intTimeMinute                       = 0;
int intTimePM                           = 0;
int intTimeAlarmHour                    = 0;
int intTimeAlarmMinute                  = 0;
boolean bTimeAlarmEnabled               = false;
boolean bTimeAlarmTriggered             = false;
boolean bTimeAlarmTriggeredExtraActions = false;
boolean bBigClockShow                   = false;

typedef enum 
{
  DebugVariables,
  DebugMQTTReceived,
  DebugMQTTSent,
  DebugIR,
  DebugTouch,
  DebugAction,
  DebugError
}
debugTypes_t;

boolean bDebugOn = false;

void buzz(long frequency, long length) {
  long delayValue = 1000000/frequency/2; // calculate the delay value between transitions
  //// 1 second's worth of microseconds, divided by the frequency, then split in half since
  //// there are two phases to each cycle
  long numCycles = frequency * length/ 1000; // calculate the number of cycles for proper timing
  //// multiply frequency, which is really cycles per second, by the number of seconds to 
  //// get the total number of cycles to produce
  for (long i=0; i < numCycles; i++){ // for the calculated length of time...
    digitalWrite(PIN_BUZZER,HIGH); // write the buzzer pin high to push out the diaphram
    delayMicroseconds(delayValue); // wait for the calculated delay value
    digitalWrite(PIN_BUZZER, LOW); // write the buzzer pin low to pull back the diaphram
    delayMicroseconds(delayValue); // wait againf or the calculated delay value
  }
}

#define BUFFPIXEL 20

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.
void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  //  Serial.println();
  //  Serial.print(F("Loading image '"));
  //  Serial.print(filename);
  //  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    //    Serial.print(F("File size: ")); 
    //    Serial.println(read32(bmpFile));
    (void)read32(bmpFile);
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    //    Serial.print(F("Image Offset: ")); 
    //    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    //    Serial.print(F("Header size: ")); 
    //    Serial.println(read32(bmpFile));
    (void)read32(bmpFile);
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      //      Serial.print(F("Bit Depth: ")); 
      //      Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        //        Serial.print(F("Image size: "));
        //        Serial.print(bmpWidth);
        //        Serial.print('x');
        //        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
          pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        //        Serial.print(F("Loaded in "));
        //        Serial.print(millis() - startTime);
        //        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void clearDisp()
{
  tft.fillScreen(ILI9341_BLACK);
} 

void drawLightOn()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("lighton.bmp", 255, 30);
  }
}

void drawLightOff()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("lightoff.bmp", 255, 30);
  }
}

void drawFanOn()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("fanon.bmp", 235, 109);
  }
}

void drawFanOff()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("fanoff.bmp", 235, 109);
  }
}

void drawSlowFanSpeedButton()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("slow.bmp", 197, 160);
  }
}

void drawFastFanSpeedButton()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("fast.bmp", 255, 160);
  }
}

void drawFanSpeed()
{
  if (screenSelected == HomeScreen)
  {
    tft.fillRect(197,109,35,51,BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_RED);  
    tft.drawNumber(fan_speed, 198, 111, 7);
  }
}

void drawMotionDetectedImage(int pir_check)
{
  if (screenSelected == HomeScreen)
  {
    if(pir_check == 1)
    {
      bmpDraw("motion.bmp", 193, 35);
    }
    else
    {
      tft.fillRect(193,35,59,53,ILI9341_BLUE);
    }
  }
}

void drawWeatherImages()
{
  if (screenSelected == HomeScreen)
  {
    tft.fillRect(0,0,190,110,BLACK);
    bmpDraw("thermo3.bmp", 0, 0);
    tft.fillRect(0,110,120,40,BLACK);
    bmpDraw("humid2.bmp", 0, 110);
  }
}

void updateWeatherData()
{
  if (screenSelected == HomeScreen)
  {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Compute heat index
    // Must send in temp in Fahrenheit!
    heatIndexF = dht.computeHeatIndex(f, h);
    heatIndexC = (heatIndexF - 32)*5/9;

    temperatureC = t;
    temperatureF = f;
    humidity     = h;

    /*
    Serial.print("Celsius=");
    Serial.println(temperatureC);
    Serial.print("Fahrenheit=");
    Serial.println(temperatureF);
    Serial.print("Humidity=");
    Serial.println(humidity);
    Serial.print("HeatIndexC=");
    Serial.println(heatIndexC);
    Serial.print("HeatIndexF=");
    Serial.println(heatIndexF);
    */

    tft.setTextSize(1);
    tft.fillRect(40,0,150,110,BLACK);
    tft.fillRect(40,110,80,40,BLACK);
    tft.setTextColor(ILI9341_RED);  
    tft.drawFloat(temperatureC, 2, 40, 0, 7);
    tft.setTextColor(ILI9341_YELLOW);  
    tft.drawFloat(heatIndexC, 2, 40, 55, 7);
    tft.setTextColor(ILI9341_BLUE);  
    tft.drawNumber((int)humidity, 55, 110, 6);
  }
} 

void updateClock()
{
  char buf[15];

  if (screenSelected != HomeScreen)
  {
    return;
  }

  if (intTimePM)
  {  
    tft.fillRect(230,0,120,30,ILI9341_BLACK);
  }
  else
  {  
    tft.fillRect(230,0,120,30,ILI9341_WHITE);
  }

  tft.setTextColor(ILI9341_RED);  
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%2d:%02d", intTimeHour, intTimeMinute);
  tft.setTextColor(ILI9341_RED);  
  tft.drawString(buf, 249, 5, 4);
  updateTimeAlarmState();
}

void showBigClock()
{
  char buf[15];
  screenSelected=BigClockScreen;

  updateBigClockTime = 0;

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);  

  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%2d:%02d", intTimeHour, intTimeMinute);
  tft.setTextSize(2);  
  if (intTimeHour >= 10)
  {
    tft.drawString(buf, 0, 77, 7);
  }
  else
  {
    tft.drawString(buf, 20, 77, 7);
  }

  bBigClockShow = true;
}

void updateTimeAlarmState()
{
  int intHour = 0;

  if (intTimePM == 0)
  {
    if(intTimeHour == 12)
    {
      intHour = 0;
    }
    else
    {
      intHour = intTimeHour;
    }
  }
  else
  {
    if (intTimeHour == 12)
    {
      intHour = 0;
    }
    else
    {
      intHour = intTimeHour + 12;
    }

  }
  if (bTimeAlarmEnabled)
  {
    bmpDraw("alarm2.bmp", 200, 0);

    if((intHour == intTimeAlarmHour) && (intTimeMinute == intTimeAlarmMinute))
    {
      bTimeAlarmTriggered             = true;
      bTimeAlarmTriggeredExtraActions = true;
    }
  }
}

void drawPowerImage()
{
  if (screenSelected == HomeScreen)
  {
    bmpDraw("power.bmp", 0, 153);
  }
}

void updatePowerData()
{
  if (screenSelected == HomeScreen)
  {
    tft.setTextSize(1);
    tft.fillRect(40,153,94,40,BLACK);
    tft.setTextColor(ILI9341_RED);  
    tft.drawFloat(powerWatts, 2, 50, 162, 4);
  }
} 

void drawAlarmImage()
{
  bmpDraw("alarm.bmp", 137, 115);
}

void updateAlarmStatus()
{
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK);  

  switch (alarmCurrentState)
  {
  case AlarmUnarmed:
    tft.drawString("Unarmed", 138, 135, 2);
    break;
  case AlarmArming:
    tft.drawString("Arming", 138, 135, 2);
    break;
  case AlarmArmed:
    tft.drawString("Armed", 138, 135, 2);
    break;
  }
} 

void updatePinInputScreen()
{
  if (currentPIN == 0)
  {
    tft.fillRect(86,5,30,30,WHITE);
    tft.fillRect(125,5,30,30,WHITE);
    tft.fillRect(164,5,30,30,WHITE);
    tft.fillRect(203,5,30,30,WHITE);
  }
  if((currentPIN > 0) && (currentPIN<= 9))
  {
    bmpDraw("asterisk.bmp", 86, 5);
    tft.fillRect(125,5,30,30,WHITE);
    tft.fillRect(164,5,30,30,WHITE);
    tft.fillRect(203,5,30,30,WHITE);
  }
  else if((currentPIN > 9) && (currentPIN<= 99))
  {
    bmpDraw("asterisk.bmp", 86, 5);
    bmpDraw("asterisk.bmp", 125,5);
    tft.fillRect(164,5,30,30,WHITE);
    tft.fillRect(203,5,30,30,WHITE);
  }
  else if((currentPIN > 99) && (currentPIN<= 999))
  {
    bmpDraw("asterisk.bmp", 86, 5);
    bmpDraw("asterisk.bmp", 125,5);
    bmpDraw("asterisk.bmp", 164,5);
    tft.fillRect(203,5,30,30,WHITE);
  }
  else if((currentPIN > 99) && (currentPIN<= 999))
  {
    bmpDraw("asterisk.bmp", 86, 5);
    bmpDraw("asterisk.bmp", 125,5);
    bmpDraw("asterisk.bmp", 164,5);
    bmpDraw("asterisk.bmp", 203,5);
  }
}

void drawLedStripImage(boolean state)
{
  if (screenSelected != HomeScreen)
  {
    return;
  }
  
  if (state)
  {
    bmpDraw("ledon.bmp", 137, 173);
  }
  else
  {
    bmpDraw("ledoff.bmp", 137, 173);
  }
}

void setHomeScreen()
{
  screenSelected = HomeScreen;
  bmpDraw("bgroundL.bmp", 0, 0);
  drawWeatherImages();
  drawSlowFanSpeedButton();
  drawFastFanSpeedButton();
  drawFanSpeed();
  drawPowerImage();
  drawAlarmImage();
  updateAlarmStatus();
  drawLedStripImage(bLedOn);
}

void setFanOptionsScreen()
{
  screenSelected = FanOptionsScreen;
  tft.fillScreen(ILI9341_WHITE);
  tft.fillRect(0,0,320,30,ILI9341_CYAN);
  tft.setTextColor(ILI9341_BLACK);
  tft.drawString("Fan Options", 90, 5, 4);

  tft.setTextColor(ILI9341_MAGENTA);  
  tft.fillRect(10,40,20,20,ILI9341_BLACK);
  tft.drawString("Dec speed by 1 step for one degree drop", 37, 42, 2);
  tft.fillRect(10,70,20,20,ILI9341_BLACK);
  tft.drawString("Dec speed by 1 step for two degrees drop", 37, 72, 2);
  tft.fillRect(10,100,20,20,ILI9341_BLACK);
  tft.drawString("Dec speed by 1 step every 90 minutes", 37, 102, 2);
  tft.fillRect(10,130,20,20,ILI9341_BLACK);
  tft.drawString("Dec speed by 1 step every 120 minutes", 37, 132, 2);
  tft.fillRect(10,160,20,20,ILI9341_BLACK);
  tft.drawString("Turn off if temp below", 37, 162, 2);
  tft.fillRect(210,160,43,20,ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE);  
  tft.drawFloat(fanStopBelowTemperature, 2, 212, 162, 2);

  switch (fanOptionSelected)
  {
  case Decrease1StepFor1DegreeDrop:
    bmpDraw("tick.bmp", 10, 40);
    break;
  case Decrease1StepFor2DegreesDrop:
    bmpDraw("tick.bmp", 10, 70);
    break;
  case Decrease1StepEvery90Mins:
    bmpDraw("tick.bmp", 10, 100);
    break;
  case Decrease1StepEvery120Mins:
    bmpDraw("tick.bmp", 10, 130);
    break;
  case StopIfBelow:
    bmpDraw("tick.bmp", 10, 160);
    bmpDraw("minus.bmp", 188, 160);
    bmpDraw("plus.bmp", 255, 160);
    break;
  case noFanOptionSelected:
    break;
  }

  bmpDraw("back.bmp", 120, 198);
}

void setLightOptionsScreen()
{
  screenSelected = LightOptionsScreen;
  tft.fillScreen(ILI9341_WHITE);
  tft.fillRect(0,0,320,30,ILI9341_CYAN);
  tft.setTextColor(ILI9341_BLACK);
  tft.drawString("Light Options", 90, 5, 4);

  tft.setTextColor(ILI9341_MAGENTA);  
  tft.fillRect(10,40,20,20,ILI9341_BLACK);
  tft.drawString("Fully automatic", 37, 42, 2);
  tft.fillRect(10,70,20,20,ILI9341_BLACK);
  tft.drawString("Switch off if no movement for 2 mins", 37, 72, 2);

  switch (lightOptionSelected)
  {
  case LightFullyAutomatic:
    bmpDraw("tick.bmp", 10, 40);
    break;
  case LightSwitchOffIfNoMotionFor2Mins:
    bmpDraw("tick.bmp", 10, 70);
    break;
  case noLightOptionSelected:
    break;
  }

  bmpDraw("back.bmp", 120, 198);
}


void setTimeOptionsScreen()
{
  char buf1[3], buf2[3];
  screenSelected = TimeOptionsScreen;
  tft.fillScreen(ILI9341_WHITE);
  tft.fillRect(0,0,320,30,ILI9341_CYAN);
  tft.setTextColor(ILI9341_BLACK);
  tft.drawString("Time Options", 90, 5, 4);

  tft.setTextColor(ILI9341_MAGENTA);  
  tft.fillRect(10,40,20,20,ILI9341_BLACK);
  tft.drawString("Stop fan when alarm is triggered", 37, 42, 2);
  tft.fillRect(10,70,20,20,ILI9341_BLACK);
  tft.drawString("Increase fan speed to max", 37, 72, 2);
  tft.fillRect(10,100,20,20,ILI9341_BLACK);
  tft.drawString("Switch on light", 37, 102, 2);

  sprintf(buf1, "%2d", intTimeAlarmHour);
  sprintf(buf2, "%02d", intTimeAlarmMinute);

  tft.setTextColor(ILI9341_RED);  
  bmpDraw("alarm3.bmp", 0, 135);
  bmpDraw("plus.bmp", 120, 130);
  bmpDraw("plus.bmp", 190, 130);
  tft.fillRect(100,160,65,50,ILI9341_BLACK);
  tft.fillRect(170,160,65,50,ILI9341_BLACK);
  tft.drawString(buf1, 101, 162, 7);
  tft.drawString(buf2, 171, 162, 7);
  bmpDraw("minus.bmp", 120, 220);
  bmpDraw("minus.bmp", 190, 220);

  switch (timeFanOptionSelected)
  {
  case TimeAlarmOptionFanStop:
    bmpDraw("tick.bmp", 10, 40);
    break;
  case TimeAlarmOptionFanMax:
    bmpDraw("tick.bmp", 10, 70);
    break;
  }

  if (timeLightOptionSelected == TimeAlarmOptionLightOn)
  {
    bmpDraw("tick.bmp", 10, 100);
  }

  bmpDraw("back.bmp", 235, 198);
}

void setPinInputScreen()
{
  screenSelected = PinInputScreen;
  tft.fillScreen(ILI9341_BLACK);
  bmpDraw("keypad.bmp", 77, 40);
  tft.setTextColor(ILI9341_RED);  
  tft.drawString("Enter code", 100, 210, 4);
  currentPIN = 0;
  updatePinInputScreen();
}

void updateDebugWindow(int dtype, char *str)
{
  char buffer[256] = "", buffer2[256] = "";
  char line1[256] = "";
  char *line2Start, line2[256] = "";
  str[strlen(str)] = '\0';

  if (!bDebugOn)
  {
    return;
  }
  
  memset(buffer, 0, sizeof(buffer));
  memset(buffer2, 0, sizeof(buffer2));
  memset(line1, 0, sizeof(line1));
  memset(line2, 0, sizeof(line2));

  if(screenSelected != HomeScreen)
  {
    return;
  }

  switch(dtype)
  {
  case DebugVariables:
    char strfanStartingTemperature[10];
    tft.fillRect(0,210,320,30,ILI9341_BLUE);
    tft.setTextColor(ILI9341_WHITE);
    dtostrf(fanStartingTemperature, 5, 2, strfanStartingTemperature);
    sprintf(buffer2, "NoMvmnt=%lu alrmArmng=%lu fanSSpd=%d  fanSTmp=%s  fanRTime=%lu ledMd=%d R=%d G=%d B=%d ", 
    noMotionDetectedTime, alarmTimeElapsedAfterArming/1000, fanStartingSpeed, strfanStartingTemperature, fanRunTime/1000, ledMode, colorRed, colorGreen, colorBlue);
    strncpy(buffer, buffer2, strlen(buffer2));
    break;
  case DebugMQTTReceived:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_GREEN);
    tft.setTextColor(ILI9341_BLACK);
    break;
  case DebugMQTTSent:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_YELLOW);
    tft.setTextColor(ILI9341_BLACK);
    break;
  case DebugIR:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_CYAN);
    tft.setTextColor(ILI9341_BLACK);
    break;
  case DebugTouch:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_WHITE);
    tft.setTextColor(ILI9341_BLACK);
    break;
  case DebugAction:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_MAGENTA);
    tft.setTextColor(ILI9341_WHITE);
    break;
  case DebugError:
    strncpy(buffer, str, strlen(str));
    tft.fillRect(0,210,320,30,ILI9341_RED);
    tft.setTextColor(ILI9341_BLACK);
    break;
  }

  if(strlen(buffer) < 50)
  {
    tft.drawString(buffer, 5, 212, 2);
  }
  else
  {
    strncpy(line1, buffer, 50);
    line1[strlen(line1)] = '\0';
    line2Start = buffer + 50;
    strncpy(line2, line2Start, strlen(buffer) - 50);
    line2[strlen(line2)] = '\0';
    tft.drawString(line1, 5, 212, 2);
    tft.drawString(line2, 5, 226, 2);      
  }
}

void light_zero_cross_detect()
{
  bLightZeroCrossingDetected = true;
}

void fan_zero_cross_detect()
{
  bFanZeroCrossingDetected = true;
}

// Draw a red frame while a button is touched
void waitForIt(int x, int y, int w, int h)  
{
  buzz(1000, 50);
  tft.drawRect(x, y, w, h, RED);
  tft.drawRect(x+1, y+1, w-1, h-1, RED);
  tft.drawRect(x+2, y+2, w-2, h-2, RED);
  while (myTouch.dataAvailable())
    myTouch.read();
}

// Just wait for button release without hightlighting the button pressed.
void waitForItNoHighlight(int x, int y, int w, int h)  
{
  buzz(1000, 50);
  while (myTouch.dataAvailable())
    myTouch.read();
}


void waitForItIncrementDecrement(int x, int y, int w, int h, int field, int action)  
{
  char buf1[15];
  unsigned long startMillis = millis();
  unsigned long nowMillis = 0;
  unsigned long prevMillis = 0;
  unsigned long millisElapsedFromLastUpdate = 0;
  unsigned long millisElapsedFromStart = 0;

  buzz(1000, 50);
  tft.drawRect(x, y, w, h, RED);
  tft.drawRect(x+1, y+1, w-1, h-1, RED);
  tft.drawRect(x+2, y+2, w-2, h-2, RED);


  while (myTouch.dataAvailable())
  {
    nowMillis = millis();

    if (prevMillis == 0)
    {
      prevMillis = nowMillis;
    }

    millisElapsedFromStart = nowMillis - startMillis;
    millisElapsedFromLastUpdate = millisElapsedFromLastUpdate + nowMillis - prevMillis;
    prevMillis = nowMillis;

    if (field == 0)
    {
      if (millisElapsedFromStart > 1000)
      {
        if (millisElapsedFromLastUpdate > 500)
        {
          if (action == 0)
          {
            intTimeAlarmHour--;
            if (intTimeAlarmHour  < 0)
            {
              intTimeAlarmHour = 23;
            }
            memset(buf1, 0, sizeof(buf1));
            sprintf(buf1, "%2d", intTimeAlarmHour);
            tft.fillRect(100,160,65,50,ILI9341_BLACK);
            tft.drawString(buf1, 101, 162, 7);
          }
          else if (action == 1)
          {
            intTimeAlarmHour++;
            if (intTimeAlarmHour  > 23)
            {
              intTimeAlarmHour = 0;
            }
            memset(buf1, 0, sizeof(buf1));
            sprintf(buf1, "%2d", intTimeAlarmHour);
            tft.fillRect(100,160,65,50,ILI9341_BLACK);
            tft.drawString(buf1, 101, 162, 7);
          }
          millisElapsedFromLastUpdate = 0;
        }
      }
    }
    else if (field == 1)
    {
      if (millisElapsedFromStart > 1000)
      {
        if (millisElapsedFromLastUpdate > 500)
        {
          if (action == 0)
          {
            intTimeAlarmMinute--;
            if (intTimeAlarmMinute  < 0)
            {
              intTimeAlarmMinute = 59;
            }

            memset(buf1, 0, sizeof(buf1));
            sprintf(buf1, "%02d", intTimeAlarmMinute);
            tft.fillRect(170,160,65,50,ILI9341_BLACK);
            tft.drawString(buf1, 171, 162, 7);
          }
          else if (action == 1)
          {
            intTimeAlarmMinute++;
            if (intTimeAlarmMinute  > 59)
            {
              intTimeAlarmMinute = 0;
            }

            memset(buf1, 0, sizeof(buf1));
            sprintf(buf1, "%02d", intTimeAlarmMinute);
            tft.fillRect(170,160,65,50,ILI9341_BLACK);
            tft.drawString(buf1, 171, 162, 7);
          }
          millisElapsedFromLastUpdate = 0;
        }
      }
    }
    myTouch.read();
  }


}

void toggleLight()
{
  if (bLightRelayActivated)
  {
    bLightRelayActivated = false;
    digitalWrite(PIN_LIGHT_RELAY, LOW);
  }
  else
  {
    bLightRelayActivated = true;
    digitalWrite(PIN_LIGHT_RELAY, HIGH);
  }

  delay(200);
  bLightZeroCrossingDetected = false;
  bFanZeroCrossingDetected = false;
  delay(30);

  if (bLightZeroCrossingDetected)
  {
    bLightOn = true;
    drawLightOn();
  }
  else
  {
    bLightOn = false;
    drawLightOff();
  }

  sendLightStateToESP8266();
}

void turnLightOn()
{
  if(!bLightOn)
  {
    toggleLight();
  }
}

void turnLightOff()
{
  if(bLightOn)
  {
    toggleLight();
  }
}

void setFanOn()
{
  bFanOn = true;
  drawFanOn();
  fanRunTime = 0;
  fanStartingTemperature = temperatureC;
  fanStartingSpeed = 0;
}

void setFanOff()
{
  bFanOn = false;
  drawFanOff();
}

void setFanSpeed()
{
  switch (fan_speed)
  {
  case 0:
    digitalWrite(PIN_FAN_TRIAC1, LOW);
    digitalWrite(PIN_FAN_TRIAC2, LOW);
    digitalWrite(PIN_FAN_TRIAC3, LOW);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 1:
    digitalWrite(PIN_FAN_TRIAC1, HIGH);
    digitalWrite(PIN_FAN_TRIAC2, LOW);
    digitalWrite(PIN_FAN_TRIAC3, LOW);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 2:
    digitalWrite(PIN_FAN_TRIAC1, LOW);
    digitalWrite(PIN_FAN_TRIAC2, HIGH);
    digitalWrite(PIN_FAN_TRIAC3, LOW);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 3:
    digitalWrite(PIN_FAN_TRIAC1, LOW);
    digitalWrite(PIN_FAN_TRIAC2, LOW);
    digitalWrite(PIN_FAN_TRIAC3, HIGH);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 4:
    digitalWrite(PIN_FAN_TRIAC1, HIGH);
    digitalWrite(PIN_FAN_TRIAC2, LOW);
    digitalWrite(PIN_FAN_TRIAC3, HIGH);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 5:
    digitalWrite(PIN_FAN_TRIAC1, LOW);
    digitalWrite(PIN_FAN_TRIAC2, HIGH);
    digitalWrite(PIN_FAN_TRIAC3, HIGH);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 6:
    digitalWrite(PIN_FAN_TRIAC1, HIGH);
    digitalWrite(PIN_FAN_TRIAC2, HIGH);
    digitalWrite(PIN_FAN_TRIAC3, HIGH);
    digitalWrite(PIN_FAN_TRIAC4, LOW);                
    break;
  case 7:
    digitalWrite(PIN_FAN_TRIAC1, LOW);
    digitalWrite(PIN_FAN_TRIAC2, LOW);
    digitalWrite(PIN_FAN_TRIAC3, LOW);
    digitalWrite(PIN_FAN_TRIAC4, HIGH);                
    break;
  }
  drawFanSpeed();
  
  // Send fan state info only if there is a change
  if ((fan_speed != prev_fan_speed) || (bFanOn != bPrevFanOn))
  {
    prev_fan_speed = fan_speed;
    bPrevFanOn     = bFanOn;
    sendFanStateToESP8266();
  }
}

void increaseFanSpeed()
{
  if (fan_speed < 7)
  {
    fan_speed++;
    setFanSpeed();
  }
  drawFastFanSpeedButton();
}

void decreaseFanSpeed()
{
  if (fan_speed > 1)
  {
    fan_speed--;
    setFanSpeed();
  }
  drawSlowFanSpeedButton();
}

void toggleFan()
{
  if (bFanRelay1Activated)
  {
    bFanRelay1Activated = false;
    digitalWrite(PIN_FAN_RELAY1, LOW);
  }
  else
  {
    bFanRelay1Activated = true;
    digitalWrite(PIN_FAN_RELAY1, HIGH);
  }

  delay(200);
  bLightZeroCrossingDetected = false;
  bFanZeroCrossingDetected = false;
  delay(30);

  if (bFanZeroCrossingDetected)
  {
    setFanOn();
  }
  else
  {
    setFanOff();
  }
  setFanSpeed();
}

void turnFanOn()
{
  if(!bFanOn)
  {
    toggleFan();
  }
}

void turnFanOff()
{
  if(bFanOn)
  {
    toggleFan();
  }
}

void turnOffLeds()
{
  for(int i=0; i<strip.numPixels(); i++) 
  {
    leds[i].r=0;
    leds[i].g=0;
    leds[i].b=0;
    strip.setPixelColor(i, 0);
  }
  strip.show();
}  

void toggleLed()
{
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  if (bLedOn)
  {
    bLedOn = false;
    turnOffLeds();
  }
  else
  {
    bLedOn = true;
    bColorChanged = true;
  }

  drawLedStripImage(bLedOn);

  // Send to ESP8266
  root["LedState"] = (bLedOn)?1:0;
  root2["EnvData"] = root;
  root2.printTo(Serial);
  Serial.println();
  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

void turnLedOn()
{
  if(!bLedOn)
  {
    toggleLed();
  }
}

void turnLedOff()
{
  if(bLedOn)
  {
    toggleLed();
  }
}

void turnOffTimeAlarm()
{
  if (bTimeAlarmEnabled && bTimeAlarmTriggered)
  {
    bTimeAlarmEnabled = false;
    bTimeAlarmTriggered = false;
  }
}

void sendDataToESP8266()
{
  char temperatureStr[10], humidityStr[10], wattsStr[10];
  char buffer[256] = "";

  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();

  dtostrf(temperatureC, 5, 2, temperatureStr);
  root["Temperature"] = temperatureStr;
  dtostrf(humidity, 5, 2, humidityStr);
  root["Humidity"] = humidityStr;
  //    root["LightState"] = (bLightOn)?1:0;
  //    root["FanState"] = (bFanOn)?1:0;
  dtostrf(powerWatts, 5, 2, wattsStr);
  root["Watts"]    = wattsStr;

  root2["EnvData"] = root;

  root2.printTo(Serial);
  Serial.println();
  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

void sendLightStateToESP8266()
{
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  root["LightState"] = (bLightOn)?1:0;
  root2["EnvData"] = root;
  root2.printTo(Serial);
  Serial.println();
  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

void sendFanStateToESP8266()
{
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  root["FanState"] = (bFanOn)?1:0;
  root["FanSpeed"] = fan_speed;
  root2["EnvData"] = root;
  root2.printTo(Serial);
  Serial.println();
  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void turnLcdBacklightOn()
{
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  noMotionDetectedTime = 0;
  if (!bBacklightOn)
  {
    // Dont want LCD screen at full brightness in night - fan on/light off
    if ((noMotionDetectedTime > 600000) && bFanOn && !bLightOn)
    {
      analogWrite(TFT_BACKLIGHT, 100);
    }
    else
    {
      digitalWrite(TFT_BACKLIGHT, HIGH);
    }

    root["Backlight"] = 1;
    root2["EnvData"] = root;
    root2.printTo(Serial);
    Serial.println();
    root2.printTo(buffer, 256);
    updateDebugWindow(DebugMQTTSent, buffer);
    
    bBacklightOn = true;
  }
}

void turnLcdBacklightOff()
{
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<200> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";
  
  digitalWrite(TFT_BACKLIGHT, LOW);
  bBacklightOn = false;

  root["Backlight"] = 0;
  root2["EnvData"] = root;
  root2.printTo(Serial);
  Serial.println();
  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

void setup()
{
  Serial.begin(115200);

  emon1.current(0, 47.61);  // Second parameter is calibration value which is no of turns in current transformer divided by burden resistor ... 2000/42

  //    tft.begin();
  tft.init();
  tft.setRotation(3);
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  clearDisp();    

  myTouch.InitTouch();

  irrecv.enableIRIn(); // Starts the receiver

  pinMode(PIN_LIGHT_RELAY, OUTPUT);
  pinMode(PIN_FAN_RELAY1, OUTPUT);    
  pinMode(PIN_FAN_RELAY2, OUTPUT);    
  pinMode(PIN_FAN_TRIAC1, OUTPUT);    
  pinMode(PIN_FAN_TRIAC2, OUTPUT);    
  pinMode(PIN_FAN_TRIAC3, OUTPUT);    
  pinMode(PIN_FAN_TRIAC4, OUTPUT);    

  pinMode(PIN_PIR_SENSOR, INPUT);

  //  pinMode(PIN_LIGHT_ZERO_DETECTOR, INPUT);
  //  pinMode(PIN_FAN_ZERO_DETECTOR,   INPUT);
  //  digitalWrite(PIN_LIGHT_ZERO_DETECTOR, HIGH);
  //  digitalWrite(PIN_FAN_ZERO_DETECTOR,   HIGH);


  attachInterrupt(0, light_zero_cross_detect, FALLING);
  attachInterrupt(1, fan_zero_cross_detect, FALLING);

  // This will make the fan speed circuit active
  digitalWrite(PIN_FAN_RELAY2, HIGH);

  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.setCursor(110,205);
  tft.println("Initializing...");
  Serial.println("Initializing...");

  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("failed!");
  }
  Serial.println("OK!");

  bmpDraw("arduino.bmp", 104, 40);
  bmpDraw("esp8266.bmp", 104, 120);

  dht.begin();

  digitalWrite(PIN_BUZZER,HIGH);
  delay(2000);
  digitalWrite(PIN_BUZZER,LOW);

  setHomeScreen();
  setFanSpeed();
  bInitializeHomeScreen = true;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  rainbow(20);
  turnOffLeds(); // Initialize all pixels to 'off'
  
  randomSeed(analogRead(0));
  
  // Read settings from EEPROM
  if(EEPROM.read(0) != 255)
  {
    fanOptionSelected = (fanOptions)EEPROM.read(0);
  }
  if(EEPROM.read(1) != 255)
  {
    lightOptionSelected = (lightOptions)EEPROM.read(1);
  }
  if(EEPROM.read(2) != 255)
  {
    timeFanOptionSelected = (timeFanOptions)EEPROM.read(2);
  }
  if(EEPROM.read(3) != 255)
  {
    timeLightOptionSelected = (timeLightOptions)EEPROM.read(3);
  }
  
}


void loop()
{
  int i=0, c=0, opening_brace=0, pos=0;
  char str[256] = "",buf[256] = "";
  int x,y;
  unsigned long keyPressTime = 0;

  // Calculate how many milliseconds have elapsed since previous iteration
  currentMillis = millis();
  elapsedMillis = currentMillis - previousMillis;
  previousMillis = currentMillis;

  weatherUpdateTime     += elapsedMillis;
  zeroCrossCheckTime    += elapsedMillis;
  pirSensorCheckTime    += elapsedMillis;
  sendDataToESP8266Time += elapsedMillis;
  checkPowerUsageTime   += elapsedMillis;
  checkAlarmTime        += elapsedMillis;
  checkFanTime          += elapsedMillis;
  checkLightTime        += elapsedMillis;
  ledUpdateTime         += elapsedMillis;
  updateVarWindowTime   += elapsedMillis;
  updateClockTime       += elapsedMillis;
  updateBigClockTime    += elapsedMillis;

  if (pirSensorCheckTime > pirSensorCheckInterval)
  {
    pirSensorCheckTime = 0;

    // Calculate how many milliseconds have elapsed since the last time we were here
    currentMillisMotion = currentMillis;
    if (previousMillisMotion == 0)
    {
      previousMillisMotion = currentMillisMotion;
    }
    elapsedMillisMotion = currentMillisMotion - previousMillisMotion;
    previousMillisMotion = currentMillisMotion;

    pir = digitalRead(PIN_PIR_SENSOR);
    if (pir == 1)
    {
      // If movement is detected after a long time, say 10 mins, display 
      // big clock.  For instance, when waking up after sleep, display the
      // clock first.
      if ((noMotionDetectedTime > 600000) && bFanOn && !bLightOn)
      {
        showBigClock();
      } 

      turnLcdBacklightOn();
    }
    else
    {
      noMotionDetectedTime += elapsedMillisMotion;
      // Turn off LCD backlight if no one is there
      if (noMotionDetectedTime > 60000) 
      {
        turnLcdBacklightOff();
      }
    }

    if (pir != last_pir)
    {
      drawMotionDetectedImage(pir);
      last_pir = pir;
    }
  }

  if (myTouch.dataAvailable())
  {
    myTouch.read();
    x=myTouch.getX();
    y=myTouch.getY();

    turnLcdBacklightOn();

    //    Serial.print("x=");
    //    Serial.println(x);
    //    Serial.print("y=");
    //    Serial.println(y);      
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "Touch:   x=%d   y=%d", x, y);
    updateDebugWindow(DebugTouch, buf);

    turnOffTimeAlarm();

    if (screenSelected == HomeScreen)
    {
      // Light switch is pressed
      if ( (x>=255) && (x<=301) && (y>=30) && (y<=108) )
      {
        waitForIt(255, 30, 46, 78);
        keyPressTime = millis() - currentMillis;

        // If fan is pressed fro more than 2 seconds, open the fan options
        // screen instead of toggling power to it.
        if (keyPressTime > 2000)
        {
          setLightOptionsScreen();
        }
        else
        {
          toggleLight();
        }
      }        
      // Fan switch is pressed
      else if ( (x>=235) && (x<=310) && (y>=109) && (y<=160) )
      {
        waitForIt(235, 109, 75, 51);
        keyPressTime = millis() - currentMillis;

        // If fan is pressed fro more than 2 seconds, open the fan options
        // screen instead of toggling power to it.
        if (keyPressTime > 2000)
        {
          setFanOptionsScreen();
        }
        else
        {
          toggleFan();
        }
      }
      // Fan slow button is pressed
      else if ( (x>=197) && (x<=242) && (y>=160) && (y<=205) )
      {
        waitForIt(197, 160, 45, 45);
        decreaseFanSpeed();
      }        
      // Fan fast button is pressed
      else if ( (x>=255) && (x<=300) && (y>=160) && (y<=205) )
      {
        waitForIt(255, 160, 45, 45);
        increaseFanSpeed();
      }        
      // LED button is pressed
      else if ( (x>=137) && (x<=193) && (y>=173) && (y<=203) )
      {
        waitForIt(137, 173, 56, 30);
        toggleLed();
      }        
      // Alarm button is pressed
      else if ( (x>=137) && (x<=193) && (y>=115) && (y<=173) )
      {
        StaticJsonBuffer<200> jsonBuffer;
        StaticJsonBuffer<200> jsonBuffer2;
        JsonObject& root = jsonBuffer.createObject();
        JsonObject& root2 = jsonBuffer2.createObject();

        waitForIt(137, 115, 56, 56);
        keyPressTime = millis() - currentMillis;

        // If fan is pressed for more than 2 seconds, open the fan options
        // screen instead of toggling power to it.
        if (keyPressTime > 2000)
        {
          if (alarmCurrentState == AlarmUnarmed)
          {
            alarmCurrentState = AlarmArming;
            bAlarmIntruderAlertSent = false;
            updateAlarmStatus();
            checkAlarmTime = checkAlarmInterval;
            
            // Send to ESP8266
            root["AlarmState"] = "AlarmArming";
            root2["EnvData"] = root;
            root2.printTo(Serial);
            Serial.println();
            root2.printTo(buf, 256);
            updateDebugWindow(DebugMQTTSent, buf);
          }
          else if (alarmCurrentState == AlarmArming)
          {
            setPinInputScreen();
          }
        }
        else
        {
          drawAlarmImage();
          updateAlarmStatus();
        }
      }        
      // Time alarm (clock) options  is pressed
      else if ( (x>=200) && (x<=319) && (y>=0) && (y<=30) )
      {
        waitForIt(200, 0, 120, 30);
        keyPressTime = millis() - currentMillis;

        // If clock is pressed for more than 2 seconds, open the time options
        // screen instead of toggling alarm state.
        if (keyPressTime > 2000)
        {
          setTimeOptionsScreen();
        }
        else
        {
          if (bTimeAlarmEnabled)
          {
            bTimeAlarmEnabled = false;
          }
          else
          {
            bTimeAlarmEnabled = true;
          }
          updateTimeAlarmState();
        }
      }
      // debug window is pressed
      else if ( (x>=0) && (x<=319) && (y>=210) && (y<=240) )
      {
        waitForIt(0, 210, 320, 30);
        keyPressTime = millis() - currentMillis;

        if (keyPressTime > 2000)
        {
          if (bDebugOn)
          {
            bDebugOn = false;
            setHomeScreen();
            bInitializeHomeScreen = true;
          }
          else
          {
            bDebugOn = true;
          }
        }
      }
    }   
    else if (screenSelected == FanOptionsScreen)
    {
      // + & - are active only when StopIfBelow option is selected
      if (fanOptionSelected == StopIfBelow)
      {
        if ( (x>=188) && (x<=208) && (y>=160) && (y<=180) )
        {
          fanStopBelowTemperature = fanStopBelowTemperature - 0.25;
          waitForIt(188, 160, 20, 20);
          setFanOptionsScreen();
        }
        else if ( (x>=255) && (x<=275) && (y>=160) && (y<=180) )
        {
          fanStopBelowTemperature = fanStopBelowTemperature + 0.25;
          waitForIt(255, 160, 20, 20);
          setFanOptionsScreen();
        }
      } 

      if ( (x>=10) && (x<=30) && (y>=40) && (y<=60) )
      {
        if (fanOptionSelected == Decrease1StepFor1DegreeDrop)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepFor1DegreeDrop;
        }

        waitForIt(10, 40, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      } 
      else if ( (x>=10) && (x<=30) && (y>=70) && (y<=90) )
      {
        if (fanOptionSelected == Decrease1StepFor2DegreesDrop)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepFor2DegreesDrop;
        }

        waitForIt(10, 70, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      }        
      else if ( (x>=10) && (x<=30) && (y>=100) && (y<=120) )
      {
        if (fanOptionSelected == Decrease1StepEvery90Mins)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepEvery90Mins;
        }

        waitForIt(10, 100, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      }        
      else if ( (x>=10) && (x<=30) && (y>=130) && (y<=150) )
      {
        if (fanOptionSelected == Decrease1StepEvery120Mins)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepEvery120Mins;
        }

        waitForIt(10, 130, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      }        
      else if ( (x>=10) && (x<=30) && (y>=160) && (y<=180) )
      {
        if (fanOptionSelected == StopIfBelow)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = StopIfBelow;
        }

        waitForIt(10, 160, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      }       
      // Back button is pressed 
      else  if ( (x>=120) && (x<=200) && (y>=198) && (y<=240) )
      {
        waitForIt(120, 198, 80, 40);
        setHomeScreen();
        bInitializeHomeScreen = true;
      }        
    }
    else if (screenSelected == LightOptionsScreen)
    {

      if ( (x>=10) && (x<=30) && (y>=40) && (y<=60) )
      {
        if (lightOptionSelected == LightFullyAutomatic)
        {
          lightOptionSelected = noLightOptionSelected;
        }
        else
        {
          lightOptionSelected = LightFullyAutomatic;
        }

        waitForIt(10, 40, 20, 20);
        setLightOptionsScreen();
        EEPROM.write(1, lightOptionSelected);
      } 
      else if ( (x>=10) && (x<=30) && (y>=70) && (y<=90) )
      {
        if (lightOptionSelected == LightSwitchOffIfNoMotionFor2Mins)
        {
          lightOptionSelected = noLightOptionSelected;
        }
        else
        {
          lightOptionSelected = LightSwitchOffIfNoMotionFor2Mins;
        }

        waitForIt(10, 70, 20, 20);
        setLightOptionsScreen();
        EEPROM.write(1, lightOptionSelected);
      }        
      // Back button is pressed 
      else  if ( (x>=120) && (x<=200) && (y>=198) && (y<=240) )
      {
        waitForIt(120, 198, 80, 40);
        setHomeScreen();
        bInitializeHomeScreen = true;
      }        
    }
    else if (screenSelected == PinInputScreen)
    {
      StaticJsonBuffer<200> jsonBuffer;
      StaticJsonBuffer<200> jsonBuffer2;
      JsonObject& root = jsonBuffer.createObject();
      JsonObject& root2 = jsonBuffer2.createObject();


      // 1
      if ( (x>=81) && (x<=124) && (y>=44) && (y<=73) )
      {
        waitForItNoHighlight(81, 44, 43, 29);
        currentPIN = currentPIN*10 + 1;
      }
      // 2
      else if ( (x>=139) && (x<=182) && (y>=44) && (y<=73) )
      {
        waitForItNoHighlight(139, 44, 43, 29);
        currentPIN = currentPIN*10 + 2;
      }
      // 3
      else if ( (x>=195) && (x<=238) && (y>=44) && (y<=73) )
      {
        waitForItNoHighlight(195, 44, 43, 29);
        currentPIN = currentPIN*10 + 3;
      }
      // 4
      if ( (x>=81) && (x<=124) && (y>=85) && (y<=114) )
      {
        waitForItNoHighlight(81, 85, 43, 29);
        currentPIN = currentPIN*10 + 4;
      }
      // 5
      else if ( (x>=139) && (x<=182) && (y>=85) && (y<=114) )
      {
        waitForItNoHighlight(139, 85, 43, 29);
        currentPIN = currentPIN*10 + 5;
      }
      // 6
      else if ( (x>=195) && (x<=238) && (y>=85) && (y<=114) )
      {
        waitForItNoHighlight(195, 85, 43, 29);
        currentPIN = currentPIN*10 + 6;
      }
      // 7
      if ( (x>=81) && (x<=124) && (y>=127) && (y<=156) )
      {
        waitForItNoHighlight(81, 127, 43, 29);
        currentPIN = currentPIN*10 + 7;
      }
      // 8
      else if ( (x>=139) && (x<=182) && (y>=127) && (y<=156) )
      {
        waitForItNoHighlight(139, 127, 43, 29);
        currentPIN = currentPIN*10 + 8;
      }
      // 9
      else if ( (x>=195) && (x<=238) && (y>=127) && (y<=156) )
      {
        waitForItNoHighlight(195, 127, 43, 29);
        currentPIN = currentPIN*10 + 9;
      }
      // 0
      else if ( (x>=139) && (x<=182) && (y>=168) && (y<=197) )
      {
        waitForItNoHighlight(139, 168, 43, 29);
        currentPIN = currentPIN*10 + 0;
      }
      // C (Clear)
      else if ( (x>=195) && (x<=238) && (y>=168) && (y<=197) )
      {
        waitForItNoHighlight(195, 168, 43, 29);
        currentPIN = 0;
      }


      // Check if all digits of the PIN have been entered.
      if (currentPIN == PIN)
      {

        alarmCurrentState = AlarmUnarmed;
        alarmLastState    = AlarmUnarmed;
        setHomeScreen();
        bInitializeHomeScreen = true;
        digitalWrite(PIN_BUZZER,LOW);

        // Send to ESP8266
        root["AlarmState"] = "AlarmUnarmed";
        root2["EnvData"] = root;
        root2.printTo(Serial);
        Serial.println();
        root2.printTo(buf, 256);
        updateDebugWindow(DebugMQTTSent, buf);
      }
      else if (currentPIN>999)
      {
        currentPIN = 0;

        // Send to ESP8266
        root["AlarmState"] = "AlarmWrongPinEntered";
        root2["EnvData"] = root;
        root2.printTo(Serial);
        Serial.println();
        root2.printTo(buf, 256);
        updateDebugWindow(DebugMQTTSent, buf);
      }
      updatePinInputScreen();
    }
    else if (screenSelected == TimeOptionsScreen)
    {
      if ( (x>=10) && (x<=30) && (y>=40) && (y<=60) )
      {
        if (timeFanOptionSelected == TimeAlarmOptionFanStop)
        {
          timeFanOptionSelected = TimeAlarmOptionFanNone;
        }
        else
        {
          timeFanOptionSelected = TimeAlarmOptionFanStop;
        }

        waitForIt(10, 40, 20, 20);
        setTimeOptionsScreen();
        EEPROM.write(2, timeFanOptionSelected);
      } 
      else if ( (x>=10) && (x<=30) && (y>=70) && (y<=90) )
      {
        if (timeFanOptionSelected == TimeAlarmOptionFanMax)
        {
          timeFanOptionSelected = TimeAlarmOptionFanNone;
        }
        else
        {
          timeFanOptionSelected = TimeAlarmOptionFanMax;
        }

        waitForIt(10, 70, 20, 20);
        setTimeOptionsScreen();
        EEPROM.write(3, timeLightOptionSelected);
      }        
      else if ( (x>=10) && (x<=30) && (y>=100) && (y<=120) )
      {
        if (timeLightOptionSelected == TimeAlarmOptionLightOn)
        {
          timeLightOptionSelected = TimeAlarmOptionLightNone;
        }
        else
        {
          timeLightOptionSelected = TimeAlarmOptionLightOn;
        }

        waitForIt(10, 100, 20, 20);
        setTimeOptionsScreen();
      }           
      // Hour plus button is pressed 
      else  if ( (x>=120) && (x<=140) && (y>=130) && (y<=150) )
      {
        waitForItIncrementDecrement(120, 130, 20, 20, 0, 1);
      }
      // Minute plus button is pressed 
      else  if ( (x>=190) && (x<=210) && (y>=130) && (y<=150) )
      {
        waitForItIncrementDecrement(190, 130, 20, 20, 1, 1);
      }
      // Hour minus button is pressed 
      else  if ( (x>=120) && (x<=140) && (y>=220) && (y<=240) )
      {
        waitForItIncrementDecrement(120, 220, 20, 20, 0, 0);
      }
      // Minute minus button is pressed 
      else  if ( (x>=190) && (x<=210) && (y>=220) && (y<=240) )
      {
        waitForItIncrementDecrement(190, 220, 20, 20, 1, 0);
      }
      // Back button is pressed 
      else  if ( (x>=240) && (x<=319) && (y>=198) && (y<=240) )
      {
        waitForIt(240, 198, 80, 40);
        setHomeScreen();
        bInitializeHomeScreen = true;
      }        
    }
  }

  if(Serial.available())
  {
    while (Serial.available())
    {
      c = (int) Serial.read();
      if (c == 123)
      {
        delay(5);
        break;
      }
    }

    if (c == 123) // {
    {
      str[pos++] = (char)c;
      opening_brace++;

      while(Serial.available())
      {        
        c = (int) Serial.read();
        if ((c >= 32) && (c <= 126))
        {
          str[pos++] = (char)c;
        }

        if (c == 125)  // }
        {
          opening_brace--;
        }

        if (opening_brace == 0 )
        {
          str[pos++] = 0;

          //          Serial.print("Received from ESP8266: ");
          //          Serial.println(str);
          updateDebugWindow(DebugMQTTReceived, str);


          StaticJsonBuffer<200> jsonBuffer;
          //          DynamicJsonBuffer jsonBuffer;
          JsonObject& root = jsonBuffer.parseObject(str);

          if (root.success())
          {
            turnLcdBacklightOn();
            if(strstr(str,"LightCommand"))
            {
              lightCommand = root["LightCommand"];

              if (lightCommand == 0)
              {
                turnLightOff();
              }
              else if(lightCommand == 1)
              {
                turnLightOn();
              }
            }
            else if(strstr(str,"FanCommand"))
            {
              fanCommand = root["FanCommand"];

              if (fanCommand == 0)
              {
                turnFanOff();
              }
              else if(fanCommand == 1)
              {
                turnFanOn();
              }
            }
            else if(strstr(str,"FanSpeedCommand"))
            {
              fan_speed = root["FanSpeedCommand"];

              turnFanOn();
              setFanSpeed();
            }
            else if(strstr(str,"LedCommand"))
            {
              ledCommand = root["LedCommand"];

              if (ledCommand == 0)
              {
                turnLedOff();
              }
              else if(ledCommand == 1)
              {
                turnLedOn();
              }
            } 
            else if(strstr(str,"LedModeCommand"))
            {
              if (root["LedModeCommand"] == 0)
              {
                ledMode = LedModeDefault;
              }
              else if (root["LedModeCommand"] ==1)
              {
                ledMode = LedModeRainbow;
              }
              else if (root["LedModeCommand"] ==2)
              {
                ledMode = LedModeCycle;
              }
              else if (root["LedModeCommand"] ==3)
              {
                ledMode = LedModeCycle2;
              }
              else if (root["LedModeCommand"] ==4)
              {
                ledMode = LedModeCycle3;
              }
              else if (root["LedModeCommand"] ==5)
              {
                ledMode = LedModeCycle4;
              }
              else if (root["LedModeCommand"] ==6)
              {
                ledMode = LedModeRandom;
              }
              else if (root["LedModeCommand"] ==7)
              {
                ledMode = LedModeRandom2;
              }
              else if (root["LedModeCommand"] ==8)
              {
                ledMode = LedModeRandomCycle;
              }
              else if (root["LedModeCommand"] ==9)
              {
                ledMode = LedModeRandomCycle2;
              }
              turnOffLeds();
//              turnLedOn();
              bColorChanged = true;
              ledUpdateInterval = 0;
            } 
            else if(strstr(str,"LedColor"))
            {
              colorRed   = root["LedColorRed"];
              colorGreen = root["LedColorGreen"];
              colorBlue  = root["LedColorBlue"];
              turnOffLeds();
              turnLedOn();
              ledUpdateInterval = 0;
              bColorChanged = true;
              
              if ((ledMode != LedModeCycle) && (ledMode != LedModeCycle2) && (ledMode != LedModeCycle2) && (ledMode != LedModeCycle3) && (ledMode != LedModeCycle4))
              {
                ledMode = LedModeDefault;
              }
            }
            else if(strstr(str,"Timestamp"))
            {
              intTimeHour   = root["TimestampHour"];
              intTimeMinute = root["TimestampMinute"];
              intTimePM     = root["TimestampPM"];

              updateClock();
            }
          }
          else
          {
            updateDebugWindow(DebugError, "Error in processing serial input...");
          }

          break;
        }
      }
    }
  }    


  if(checkAlarmTime >= checkAlarmInterval)
  {
    checkAlarmTime = 0;
    StaticJsonBuffer<200> jsonBuffer;
    StaticJsonBuffer<200> jsonBuffer2;
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& root2 = jsonBuffer2.createObject();

    // Calculate how many milliseconds have elapsed since previous time we were here
    currentMillisAlarm = currentMillis;
    if (previousMillisAlarm == 0)
    {
      previousMillisAlarm = currentMillisAlarm;
    }
    elapsedMillisAlarm = currentMillisAlarm - previousMillisAlarm;
    previousMillisAlarm = currentMillisAlarm;

    if ((alarmLastState == AlarmUnarmed) && (alarmCurrentState == AlarmArming))
    {
      alarmLastState = AlarmArming;
      alarmTimeElapsedAfterArming = 0;
      previousMillisIntruderAlert = 0;
    }
    else if ((alarmLastState == AlarmArming) && (alarmCurrentState == AlarmArming))
    {
      alarmTimeElapsedAfterArming += elapsedMillisAlarm;
      buzz(2500, 50);

      // Arm the alarm after a delay
      if (alarmTimeElapsedAfterArming > alarmActivateDelay)
      {
        alarmCurrentState = AlarmArmed;
        alarmTimeElapsedAfterTriggering = 0;

        // Send to ESP8266
        root["AlarmState"] = "AlarmArmed";
        root2["EnvData"] = root;
        root2.printTo(Serial);
        Serial.println();
        root2.printTo(buf, 256);
        updateDebugWindow(DebugMQTTSent, buf);
      }

      // If the alarm needs to be disarmed while it is arming, then
      // pin input screen will be displayed and hence dont update
      // alarm status
      if (screenSelected == HomeScreen)
      {
        drawAlarmImage();
        updateAlarmStatus();
      }
    }
    else if (alarmCurrentState == AlarmArmed)
    {
      // Check if any movement is detected
      if(pir == 1)
      {
        alarmCurrentState = AlarmTriggered;
        setPinInputScreen();

        // Send to ESP8266
        root["AlarmState"] = "AlarmTriggered";
        root2["EnvData"] = root;
        root2.printTo(Serial);
        Serial.println();
        root2.printTo(buf, 256);
        updateDebugWindow(DebugMQTTSent, buf);
      }
    }
    else if (alarmCurrentState == AlarmTriggered)
    {
      alarmTimeElapsedAfterTriggering += elapsedMillisAlarm;

      if (alarmTimeElapsedAfterTriggering > alarmPinInputDelay)
      {
        // Send alert 
        digitalWrite(PIN_BUZZER,LOW);
        delay(1);
        digitalWrite(PIN_BUZZER,HIGH);

        // Calculate how many milliseconds have elapsed since previous time we were here
        currentMillisIntruderAlert = currentMillis;

        if (previousMillisIntruderAlert == 0)
        {
          previousMillisIntruderAlert = currentMillisIntruderAlert;
        }

        elapsedMillisIntruderAlert = currentMillisIntruderAlert - previousMillisIntruderAlert;
        previousMillisIntruderAlert = currentMillisIntruderAlert;

        updateAlertTime += elapsedMillisIntruderAlert;

        if (!bAlarmIntruderAlertSent || (updateAlertTime > updateAlertInterval))
        {
          updateAlertTime = 0;
          bAlarmIntruderAlertSent = true;

          // Send to ESP8266
          root["AlarmState"] = "AlarmIntruderAlert";
          root2["EnvData"] = root;
          root2.printTo(Serial);
          Serial.println();
          root2.printTo(buf, 256);
          updateDebugWindow(DebugMQTTSent, buf);
        }
      }
      else
      {
        buzz(10000,100);
      }
    }          
  }

  if(checkFanTime >= checkFanInterval)
  {
    checkFanTime = 0;

    // Calculate how many milliseconds have elapsed since previous time we were here
    currentMillisFan = currentMillis;
    if (previousMillisFan == 0)
    {
      previousMillisFan = currentMillisFan;
    }
    elapsedMillisFan = currentMillisFan - previousMillisFan;
    previousMillisFan = currentMillisFan;

    fanRunTime += elapsedMillisFan;

    if (bFanOn)
    {
      float tempDiff = 0;

      // Set fanStartingSpeed after 10 minutes of fan running
      if (fanStartingSpeed == 0)
      {
        if (fanRunTime > 600000)
        {
          fanStartingSpeed = fan_speed;
        }
      }
      else
      {
        switch (fanOptionSelected)
        {
        case Decrease1StepFor1DegreeDrop:
          tempDiff = fanStartingTemperature - temperatureC;         
          if (tempDiff > 0)
          {
            fan_speed = fanStartingSpeed - (int)tempDiff;
            if(fan_speed<1)
            {
              fan_speed = 1;
              turnFanOff();
            }
            setFanSpeed();
          }
          break;
        case Decrease1StepFor2DegreesDrop:
          tempDiff = fanStartingTemperature - temperatureC;         
          if (tempDiff > 0)
          {
            fan_speed = fanStartingSpeed - (int)tempDiff/2;
            if(fan_speed<1)
            {
              fan_speed = 1;
              turnFanOff();
            }
            setFanSpeed();
          }
          break;
        case Decrease1StepEvery90Mins:
          fan_speed = fanStartingSpeed - (int)(fanRunTime/5400000);
          if(fan_speed<1)
          {
            fan_speed = 1;
            turnFanOff();
          }
          setFanSpeed();
          break;
        case Decrease1StepEvery120Mins:
          fan_speed = fanStartingSpeed - (int)(fanRunTime/7200000);
          if(fan_speed<1)
          {
            fan_speed = 1;
            turnFanOff();
          }
          setFanSpeed();
          break;
        case StopIfBelow:
          if (temperatureC < fanStopBelowTemperature)
          {
            turnFanOff();
          }
          break;
        case noFanOptionSelected:
          break;
        }
      }
    }

  }

  if(checkLightTime >= checkLightInterval)
  {
    checkLightTime = 0;
    switch (lightOptionSelected)
    {
      case LightFullyAutomatic:
        if(!bLightOn)
        {
          if (pir == 1)
          {
            turnLightOn();
          }
        }
        else
        {
          if (noMotionDetectedTime > 120000)  // 2 mins
          {
            turnLightOff();
          }
        }
      break;
      case LightSwitchOffIfNoMotionFor2Mins:
        if(bLightOn)
        {
          if (noMotionDetectedTime > 120000)  // 2 mins
          {
            turnLightOff();
          }
        }
      break;
      case noLightOptionSelected:
      break;
    }
  }


  if (irrecv.decode(&results))
  {
    unsigned long value = results.value;
    last_ir_cmd_received = value;

    /*     
     Serial.print("IR value=");
     Serial.print(value, HEX);
     Serial.print(":decode_type=");
     Serial.print(results.decode_type);
     Serial.print(":bits=");
     Serial.print(results.bits);
     Serial.print(":rawlen=");
     Serial.println(results.rawlen);
     Serial.print("rawbuf=");
     Serial.println(results.rawbuf);
     */
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "IR command received=%lu", value);
    updateDebugWindow(DebugIR, buf);

    switch (value)
    {
    case 16738455: // 0
      break;
    case 16724175: // 1 
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 1;
      turnFanOn();
      setFanSpeed();
      break;
    case 16718055: // 2 
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 2;
      turnFanOn();
      setFanSpeed();
      break;
    case 16743045: // 3
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 3;
      turnFanOn();
      setFanSpeed();
      break;
    case 16716015: // 4
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 4;
      turnFanOn();
      setFanSpeed();
      break;
    case 16726215: // 5
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 5;
      turnFanOn();
      setFanSpeed();
      break;
    case 16734885: // 6
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 6;
      turnFanOn();
      setFanSpeed();
      break;
    case 16728765: // 7
      buzz(2500, 50);
      turnLcdBacklightOn();
      fan_speed = 7;
      turnFanOn();
      setFanSpeed();
      break;
    case 16748655: // + 
      buzz(2500, 50);
      turnLcdBacklightOn();
      increaseFanSpeed();
      break;
    case 16754775: // -
      buzz(2500, 50);
      turnLcdBacklightOn();
      decreaseFanSpeed();
      break;
    case 16753245: // "Power"
      buzz(2500, 50);
      turnLcdBacklightOn();
      toggleLight();
      turnOffTimeAlarm();
      break;
    case 16769565: // "Mute"
      buzz(2500, 50);
      turnLcdBacklightOn();
      toggleFan();
      turnOffTimeAlarm();
      break;
    case 16736925: // "Mode"
      buzz(2500, 50);
      turnLcdBacklightOn();
      turnOffLeds();
      toggleLed();
      bColorChanged = true;
      break;
    case 16712445: //  |<< (rewind)
      buzz(2500, 50);
      turnLcdBacklightOn();
      if (ledMode == LedModeDefault)
      {
        ledMode = LedModeRandomCycle2;
      }
      else
      {
        ledMode = (ledModes)(ledMode-1);
      }
      ledUpdateInterval = 0;
      turnOffLeds();
      turnLedOn();
      bColorChanged = true;
      break;    
    case 16761405: //  >>| (forward)
      buzz(2500, 50);
      turnLcdBacklightOn();
      if (ledMode == LedModeRandomCycle2)
      {
        ledMode = LedModeDefault;
      }
      else
      {
        ledMode = (ledModes)(ledMode+1);
      }

      ledUpdateInterval = 0;
      turnOffLeds();
      turnLedOn();
      bColorChanged = true;
      break;
    case 16720605: // >||
    case 16769055: // EQ
      buzz(2500, 50);
      turnLcdBacklightOn();
      showBigClock();
      break;
    }
    irrecv.resume(); // Receives the next value from the button you press
  }

  if (weatherUpdateTime >= weatherUpdateInterval)
  {
    weatherUpdateTime = 0;
    updateWeatherData();
  }

  if (sendDataToESP8266Time > sendDataToESP8266Interval)
  {
    sendDataToESP8266Time = 0;
    sendDataToESP8266();
  }

  if (bInitializeHomeScreen)
  {    
    bInitializeHomeScreen = false;
    bLightZeroCrossingDetected = false;
    bFanZeroCrossingDetected = false;

    delay(30);

    if (bLightZeroCrossingDetected)
    {
      bLightOn = true;
      drawLightOn();
    }
    else
    {
      bLightOn = false;
      drawLightOff();
    }

    if (bFanZeroCrossingDetected)
    {
      setFanOn();
    }
    else
    {
      setFanOff();
    }
  }

  if (zeroCrossCheckTime > zeroCrossCheckInterval)
  {    
    zeroCrossCheckTime = 0;
    bLightZeroCrossingDetected = false;
    bFanZeroCrossingDetected = false;

    delay(30);

    if (bLightZeroCrossingDetected)
    {
      if (!bLightOn)
      {
        bLightOn = true;
        drawLightOn();
        sendLightStateToESP8266();
      }
    }
    else
    {
      if (bLightOn)
      {
        bLightOn = false;
        drawLightOff();
        sendLightStateToESP8266();
      }
    }

    if (bFanZeroCrossingDetected)
    {
      if (!bFanOn)
      {
        setFanOn();
        sendFanStateToESP8266();
      }
    }
    else
    {
      if (bFanOn)
      {
        setFanOff();
        sendFanStateToESP8266();
      }
    }

  }

  if (ledUpdateTime > ledUpdateInterval)
  {
    long randr = 0, randg = 0, randb = 0;
    ledUpdateTime = 0;
    
    if(bLedOn)
    {
      // Turn off LEDs if alarm is not in unarmed state     
      if (alarmCurrentState != AlarmUnarmed)
      {
        turnLedOff();
      }

      switch (ledMode)
      {
      case LedModeDefault:
        ledUpdateInterval = 3000;
        if (bColorChanged)
        {
          for(i=0; i<strip.numPixels(); i++) 
          {
            strip.setPixelColor(i, colorRed, colorGreen, colorBlue);
          }
          strip.show();
          bColorChanged = false;      
        }
        break;
      case LedModeRainbow:      
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }
        if(J++ > 255)
        {
          J = 0;
        }    

        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, Wheel((i+J) & 255));
        }
        strip.show();
        break;
      case LedModeCycle:      
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }
        if(K==0)
        {
          K = 1;
        }    
        else if(K==1)
        {
          K = 0;
        }    

        for(i=K; i<strip.numPixels(); i=i+2) 
        {
          strip.setPixelColor(i, 0);
          strip.setPixelColor(i+1, colorRed, colorGreen, colorBlue);
        }
        strip.show();
        break;
      case LedModeCycle2:      
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }

        L++;
        if (L==LED_COUNT)
        {
          L=0;
        }

        for(i=0;i<LED_COUNT;i++)
        {        
          if (i==L)
          {
            strip.setPixelColor(i, colorRed, colorGreen, colorBlue);
          }
          else
          {
            strip.setPixelColor(i, 0, 0, 0);
          }
        }
        strip.show();
        break;
      case LedModeCycle3:      
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }

        L--;
        if (L==-1)
        {
          L=LED_COUNT-1;
        }

        for(i=LED_COUNT-1;i>=0;i--)
        {        
          if (i==L)
          {
            strip.setPixelColor(i, colorRed, colorGreen, colorBlue);
          }
          else
          {
            strip.setPixelColor(i, 0, 0, 0);
          }
        }
        strip.show();
        break;
      case LedModeCycle4:      
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }

        switch(STEP)
        {
          case 0:
            L++;
            if (L >= (LED_COUNT-1))
            {
              L=0;
              STEP++;
            }

            for(i=0;i<=L;i++)
            {        
              strip.setPixelColor(i, colorRed, colorGreen, colorBlue);
            }
          break;
          case 1:
            L++;
            if (L >= (LED_COUNT-1))
            {
              L=0;
              STEP++;
            }

            for(i=0;i<=L;i++)
            {        
              strip.setPixelColor(i, 0, 0, 0);
            }
          break;
        }
        
        if (STEP>1)
        {
          STEP=0;
        }
        strip.show();
        break;
      case LedModeRandom:  
        ledUpdateInterval += 500;
        if(ledUpdateInterval > 2000)
        {
          ledUpdateInterval = 0;
        }
        randr = random(256);    
        randg = random(256);    
        randb = random(256);    
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, randr, randg, randb);
        }
        strip.show();
        break;
      case LedModeRandom2:  
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }
        x = random(LED_COUNT);

        for(i=0; i<strip.numPixels(); i++) 
        {
          if(i==x)
          {
            strip.setPixelColor(i, random(256), random(256), random(256));
          }
          else
          {
            strip.setPixelColor(i, 0, 0, 0);
          }
        }
        strip.show();
        break;
      case LedModeRandomCycle:
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, random(256), random(256), random(256));
        }
        strip.show();
        break;
      case LedModeRandomCycle2:
        ledUpdateInterval += 100;
        if(ledUpdateInterval > 300)
        {
          ledUpdateInterval = 100;
        }
        for(i=strip.numPixels()-1; i>0; i--) 
        {
         leds[i] = leds[i-1];
         strip.setPixelColor(i, leds[i].r, leds[i].g, leds[i].b);
        }
        leds[0].r = random(256);
        leds[0].g = random(256);
        leds[0].b = random(256);
        strip.setPixelColor(0, leds[0].r, leds[0].g, leds[0].b);
        strip.show();
        break;
      }
    }
  }

  if(checkPowerUsageTime > checkPowerUsageInterval)
  {
    checkPowerUsageTime = 0;
    double Irms = emon1.calcIrms(2000);  // Calculate Irms only

    powerCurrent = Irms;
    powerWatts   = Irms * 230.0;
    updatePowerData();
  }

  if (updateVarWindowTime > updateVarWindowInterval)
  {
    updateVarWindowTime = 0;
    if(bDebugOn)
    {
      updateDebugWindow(DebugVariables, "");
    }
  }

  if (updateClockTime > updateClockInterval)
  {
    updateClockTime = 0;
    updateClock();

    if (bTimeAlarmEnabled && bTimeAlarmTriggered)
    {
      digitalWrite(PIN_BUZZER, LOW);
      delay(1);
      digitalWrite(PIN_BUZZER, HIGH);

      if(bTimeAlarmTriggeredExtraActions)
      {
        bTimeAlarmTriggeredExtraActions = false;

        switch (timeFanOptionSelected)
        {
        case TimeAlarmOptionFanStop:
          turnFanOff();
          break;
        case TimeAlarmOptionFanMax:
          fan_speed = 7;
          turnFanOn();
          setFanSpeed();
          break;
        }

        if (timeLightOptionSelected == TimeAlarmOptionLightOn)
        {
          turnLightOn();
        }
      }
    }
  }    

  if (bBigClockShow)
  {
    if (updateBigClockTime > updateBigClockInterval)
    {
      updateBigClockTime = 0;
      bBigClockShow = false;
      setHomeScreen();
      bInitializeHomeScreen = true;
    }
  }

  delay(1);
}

