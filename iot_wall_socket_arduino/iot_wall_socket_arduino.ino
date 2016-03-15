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

//UTouch myTouch(TCLK, TCS, TDIN, TDOUT, IRQ);
//UTouch  myTouch( 20, 12, 16, 15, 2);
UTouch  myTouch( 20, 12, 17, 16, 2);
extern uint8_t SmallFont[];

#define LED_COUNT      45
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, PIN_LEDS, NEO_GRB + NEO_KHZ800);

IRrecv irrecv(PIN_IR_RECEIVER);
decode_results results;

EnergyMonitor emon1;                   // Create an instance

DHT dht(PIN_DHT22, DHT22);


int intLightCommand=0, intFanCommand=0;
int intFanSpeed=4, prev_intFanSpeed=0;

boolean bLightOn=false, bFanOn=false;
boolean bLightRelayActivated=false, bFanRelay1Activated=false, bFanRelay2Activated=false;
volatile boolean bLightZeroCrossingDetected = false, bFanZeroCrossingDetected = false;

float fanStopBelowTemperature = 25;
int fanStartingSpeed = 0;
float fanStartingTemperature = 0;
unsigned long fanRunTime = 0;

float floatTemperatureC=0, floatTemperatureF=0, floatHumidity=0;
float floatHeatIndexC=0, floatHeatIndexF=0;
float floatPowerCurrent = 0, floatPowerWatts = 0;

int ledCommand=0;
boolean bLedOn = false;
boolean bColorChanged = true;
int intLedMode = 0;
int intLedSpeed=0;
int colorRed=0,colorGreen=0,colorBlue=255;
int randr=0,randg=0,randb=0;
int J=0,K=0,L=0,x=0,y=0,z=0, STEP=0, STEP2=0;

boolean prev_bLedOn = false;
int prev_intLedMode = 0;
int prev_intLedSpeed = 0;
int prev_colorRed=0,prev_colorGreen=0,prev_colorBlue=0;

int count = 0;
boolean bFirstTime = true;
unsigned long last_ir_cmd_received;
int pir = 0, last_pir = 0;
unsigned long noMotionDetectedTime      = 0;
boolean bBacklightOn = true;
boolean bBacklightEnabled = true;
boolean bInitializeHomeScreen = true;

unsigned long weatherUpdateInterval     = 30000;
unsigned long zeroCrossCheckInterval    = 5000;
unsigned long pirSensorCheckInterval    = 2000;
unsigned long sendStatusInterval        = 6000;
unsigned long checkPowerUsageInterval   = 30000;
unsigned long checkAlarmInterval        = 5000;
unsigned long updateAlertInterval       = 600000;
unsigned long checkFanInterval          = 600000;
unsigned long checkLightInterval        = 10000;
unsigned long ledUpdateInterval         = 500;
unsigned long updateVarWindowInterval   = 20000;
unsigned long updateClockInterval       = 40000;
unsigned long updateBigClockInterval    = 30000;
unsigned long saveEEPROMInterval        = 60000;

unsigned long weatherUpdateTime     = 0;
unsigned long zeroCrossCheckTime    = 0;
unsigned long pirSensorCheckTime    = 0;
unsigned long sendStatusTime        = 0;
unsigned long checkPowerUsageTime   = 0;
unsigned long checkAlarmTime        = 0;
unsigned long updateAlertTime       = 0;
unsigned long checkFanTime          = 0;
unsigned long checkLightTime        = 0;
unsigned long ledUpdateTime         = 0;
unsigned long updateVarWindowTime   = 0;
unsigned long updateClockTime       = 0;
unsigned long updateBigClockTime    = 0;
unsigned long saveEEPROMTime        = 0;

unsigned long previousMillis = 0, currentMillis = 0, elapsedMillis = 0;
unsigned long previousMillisAlarm = 0, currentMillisAlarm = 0, elapsedMillisAlarm = 0;
unsigned long previousMillisIntruderAlert = 0, currentMillisIntruderAlert = 0, elapsedMillisIntruderAlert = 0;
unsigned long previousMillisFan = 0, currentMillisFan = 0, elapsedMillisFan = 0;
unsigned long previousMillisMotion = 0, currentMillisMotion = 0, elapsedMillisMotion = 0;


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

boolean bDebugOn       = false;
boolean bStarted       = false;
int intStatusField     = 1;


enum ledModes
{
  LedModeDefault=0,
  LedModeNegative,
  LedModeFade,
  LedModeRainbow,
  LedModeCycle,
  LedModeCycle2,    // 5
  LedModeCycle3,
  LedModeCycle4,
  LedModeRandom,
  LedModeRandom2,
  LedModeRandomCycle,  // 10
  LedModeRandomCycle2,
  LedModeStrobe,
  LedModeStrobeRandom,
  LedModeStrobeStep,
  LedModeStrobeStep2,       // 15
  LedModeStrobeStepRandom, 
  LedModeStrobeStepRandom2,
  LedModeLarsonScanner,
  LedModeLarsonScannerRandom  //19
};

ledModes ledMode = LedModeDefault;

struct rgb
{
  int r;
  int g;
  int b;
};

rgb leds[LED_COUNT];

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

enum fanOptions
{
  Decrease1StepFor1DegreeDrop,
  Decrease1StepFor2DegreesDrop,
  Decrease1StepEvery120Mins,
  Decrease1StepEvery180Mins,
  StopIfBelow,
  noFanOptionSelected
};

fanOptions fanOptionSelected = noFanOptionSelected;

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

void initGlobals()
{
  randr=random(256);
  randg=random(256);
  randb=random(256);
  J=0;
  K=0;
  L=0;
  x=0;
  y=0;
  z=0;
  STEP=0;
  STEP2=0;
}


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
    tft.drawNumber(intFanSpeed, 198, 111, 7);
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
    floatHeatIndexF = dht.computeHeatIndex(f, h);
    floatHeatIndexC = (floatHeatIndexF - 32)*5/9;

    floatTemperatureC = t;
    floatTemperatureF = f;
    floatHumidity     = h;

    /*
    Serial.print("Celsius=");
     Serial.println(floatTemperatureC);
     Serial.print("Fahrenheit=");
     Serial.println(floatTemperatureF);
     Serial.print("Humidity=");
     Serial.println(floatHumidity);
     Serial.print("HeatIndexC=");
     Serial.println(floatHeatIndexC);
     Serial.print("HeatIndexF=");
     Serial.println(floatHeatIndexF);
     */

    tft.setTextSize(1);
    tft.fillRect(40,0,150,110,BLACK);
    tft.fillRect(40,110,80,40,BLACK);
    tft.setTextColor(ILI9341_RED);  
    tft.drawFloat(floatTemperatureC, 2, 40, 0, 7);
    tft.setTextColor(ILI9341_YELLOW);  
    tft.drawFloat(floatHeatIndexC, 2, 40, 55, 7);
    tft.setTextColor(ILI9341_BLUE);  
    tft.drawNumber((int)floatHumidity, 55, 110, 6);
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

  if ((intTimePM && (intTimeHour >= 6))
    || (!intTimePM && (intTimeHour < 6))
    || (!intTimePM && (intTimeHour == 12)))
  {
    tft.setTextColor(ILI9341_BLUE);  
  }
  else
  {
    tft.setTextColor(ILI9341_WHITE);  
  }

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
    tft.drawFloat(floatPowerWatts, 2, 50, 162, 4);
  }
} 

void updateAlarmStatus()
{
  switch (alarmCurrentState)
  {
  case AlarmUnarmed:
    bmpDraw("alarm.bmp", 137, 115);
    break;
  case AlarmArming:
    bmpDraw("arming.bmp", 137, 115);
    break;
  case AlarmArmed:
    bmpDraw("armed.bmp", 137, 115);
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
  tft.drawString("Dec speed by 1 step every 120 minutes", 37, 102, 2);
  tft.fillRect(10,130,20,20,ILI9341_BLACK);
  tft.drawString("Dec speed by 1 step every 180 minutes", 37, 132, 2);
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
  case Decrease1StepEvery120Mins:
    bmpDraw("tick.bmp", 10, 100);
    break;
  case Decrease1StepEvery180Mins:
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
    EEPROM.write(4, 0);
    digitalWrite(PIN_LIGHT_RELAY, LOW);
  }
  else
  {
    bLightRelayActivated = true;
    EEPROM.write(4, 1);
    digitalWrite(PIN_LIGHT_RELAY, HIGH);
  }
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
  fanStartingTemperature = floatTemperatureC;
  fanStartingSpeed = 0;
}

void setFanOff()
{
  bFanOn = false;
  drawFanOff();
}

void setFanSpeed()
{
  if (prev_intFanSpeed != intFanSpeed)
  {
    prev_intFanSpeed = intFanSpeed;
    EEPROM.write(6, intFanSpeed);
  }

  switch (intFanSpeed)
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
}

void increaseFanSpeed()
{
  char str[10];
  if (intFanSpeed < 7)
  {
    intFanSpeed++;
    sendIntToESP8266("ChangedFanSpeed", intFanSpeed); 
    setFanSpeed();
  }
  drawFastFanSpeedButton();
}

void decreaseFanSpeed()
{
  char str[10];
  if (intFanSpeed > 1)
  {
    intFanSpeed--;
    sendIntToESP8266("ChangedFanSpeed", intFanSpeed); 
    setFanSpeed();
  }
  drawSlowFanSpeedButton();
}

void toggleFan()
{
  if (bFanRelay1Activated)
  {
    bFanRelay1Activated = false;
    EEPROM.write(5, 0);
    digitalWrite(PIN_FAN_RELAY1, LOW);
  }
  else
  {
    bFanRelay1Activated = true;
    EEPROM.write(5, 1);
    digitalWrite(PIN_FAN_RELAY1, HIGH);
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

void turnOnLed()
{
  if (!bLedOn)
  {
    bLedOn = true;
    bColorChanged = true;
    sendIntToESP8266("ChangedLed", bLedOn?1:0);
  }
}

void turnOffLed()
{
  if (bLedOn)
  {
    bLedOn = false;
    sendIntToESP8266("ChangedLed", bLedOn?1:0);
  }

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
  if (bLedOn)
  {
    bLedOn = false;
    turnOffLed();
    sendIntToESP8266("ChangedLed", 0);
  }
  else
  {
    bLedOn = true;
    bColorChanged = true;
    sendIntToESP8266("ChangedLed", 1);
  }

  drawLedStripImage(bLedOn);
}

void turnOffTimeAlarm()
{
  if (bTimeAlarmEnabled && bTimeAlarmTriggered)
  {
    bTimeAlarmEnabled = false;
    bTimeAlarmTriggered = false;
  }
}

void sendRGBStateToESP8266()
{
  StaticJsonBuffer<100> jsonBuffer;
  StaticJsonBuffer<120> jsonBuffer2;
  //  StaticJsonBuffer<140> jsonBuffer3;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  //  JsonObject& root3 = jsonBuffer3.createObject();

  root["ChangedLedColorRed"] = colorRed;
  root["ChangedLedColorGreen"] = colorGreen;
  root["ChangedLedColorBlue"] = colorBlue;

  //  root2["ChangedLedColor"] = root;
  root2["socket1"] = root;
  root2.printTo(Serial);
  Serial.println();
}

void sendIntToESP8266(char *type, int value)
{
  StaticJsonBuffer<50> jsonBuffer;
  StaticJsonBuffer<70> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  // Send to ESP8266
  root[type] = value;
  root2["socket1"] = root;
  root2.printTo(Serial);
  Serial.println();

  root2.printTo(buffer, 256);
  updateDebugWindow(DebugMQTTSent, buffer);
}

void sendAllStateToESP8266()
{
  StaticJsonBuffer<80> jsonBuffer;
  StaticJsonBuffer<60> jsonBuffer2;
//  StaticJsonBuffer<140> jsonBuffer3;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
//  JsonObject& root3 = jsonBuffer3.createObject();
  char fstr[10];

  switch(intStatusField)
  {
    case 0: 
      dtostrf(floatTemperatureC, 5, 2, fstr);
      root["StateTemperature"]   = fstr;
      break;
    case 1: 
      dtostrf(floatHumidity, 5, 2, fstr);
      root["StateHumidity"]      = fstr;
      break;
    case 2: 
      dtostrf(floatPowerWatts, 5, 2, fstr);
      root["StateWatts"]         = fstr;
      break;
    case 3: 
      root["StateLight"]         = bLightOn?1:0;
      break;
    case 4: 
      root["StateFan"]           = bFanOn?1:0;
      break;
    case 5: 
      root["StateFanSpeed"]      = intFanSpeed;
      break;
    case 6: 
      root["StateLedMode"]       = intLedMode;
      break;
    case 7: 
      root["StateLedSpeed"]      = intLedSpeed;
      break;
    case 8: 
      root["StateLed"]           = bLedOn?1:0;
      break;
    case 9: 
      root["StateLedColorRed"]   = colorRed;
      break;
    case 10: 
      root["StateLedColorGreen"] = colorGreen;
      break;
    case 11: 
      root["StateLedColorBlue"]  = colorBlue;
      break;
  }
  
  if(++intStatusField > 11)
  {
    intStatusField = 0;
  }
  
  root2["socket1"] = root;
  root2.printTo(Serial);
  Serial.println();
}

void sendStringToESP8266(char *type, char *value)
{
  StaticJsonBuffer<50> jsonBuffer;
  StaticJsonBuffer<70> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& root2 = jsonBuffer2.createObject();
  char buffer[256] = "";

  // Send to ESP8266
  root[type] = value;
  root2["socket1"] = root;
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
  noMotionDetectedTime = 0;

  if (!bBacklightEnabled)
  {
    return;
  }

  if (!bBacklightOn)
  {
    bBacklightOn = true;
    // Dont want LCD screen at full brightness in night - fan on/light off
    if ((noMotionDetectedTime > 600000) && bFanOn && !bLightOn)
    {
      analogWrite(TFT_BACKLIGHT, 100);
    }
    else
    {
      digitalWrite(TFT_BACKLIGHT, HIGH);
    }
    sendIntToESP8266("StateBacklight", 1);
  }
}

void turnLcdBacklightOff()
{
  if (bBacklightOn)
  {
    digitalWrite(TFT_BACKLIGHT, LOW);
    bBacklightOn = false;
  }

  sendIntToESP8266("StateBacklight", 0);
}

void updateStateFromEEPROM()
{
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

  if(EEPROM.read(4) != 255)
  {
    if(EEPROM.read(4) == 0)
    {
      digitalWrite(PIN_LIGHT_RELAY, LOW);
    }
    else if(EEPROM.read(4) == 1)
    {
      digitalWrite(PIN_LIGHT_RELAY, HIGH);
    }
  }

  if(EEPROM.read(5) != 255)
  {
    if(EEPROM.read(5) == 0)
    {
      digitalWrite(PIN_FAN_RELAY1, LOW);
    }
    else if(EEPROM.read(5) == 1)
    {
      digitalWrite(PIN_FAN_RELAY1, HIGH);
    }
  }

  if(EEPROM.read(7) != 255)
  {
    if(EEPROM.read(7) == 0)
    {
      bLedOn = false;
    }
    else if(EEPROM.read(7) == 1)
    {
      bLedOn = true;
    }
    prev_bLedOn = bLedOn;
  }

  if(EEPROM.read(8) != 255)
  {
    intLedMode = EEPROM.read(8);
    prev_intLedMode = intLedMode;
    ledMode = (ledModes) intLedMode;
  }

  if(EEPROM.read(9) != 255)
  {
    intLedSpeed = EEPROM.read(9);
    prev_intLedSpeed = intLedSpeed;
    ledUpdateInterval = (unsigned long)(pow(2,(12-intLedSpeed)));
  }
  
  colorRed   = EEPROM.read(10);
  colorGreen = EEPROM.read(11);
  colorBlue  = EEPROM.read(12);

  prev_colorRed   = colorRed;
  prev_colorGreen = colorGreen;
  prev_colorBlue  = colorBlue;
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

  updateStateFromEEPROM();

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

  buzz(2500, 50);
  
  setHomeScreen();
  bInitializeHomeScreen = true;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  rainbow(5);
  turnOffLed(); // Initialize all pixels to 'off'

  if(EEPROM.read(6) != 255)
  {
    intFanSpeed = EEPROM.read(6);
    prev_intFanSpeed = intFanSpeed;
    setFanSpeed();
  }

  randomSeed(analogRead(0));
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
  sendStatusTime        += elapsedMillis;
  checkPowerUsageTime   += elapsedMillis;
  checkAlarmTime        += elapsedMillis;
  checkFanTime          += elapsedMillis;
  checkLightTime        += elapsedMillis;
  ledUpdateTime         += elapsedMillis;
  updateVarWindowTime   += elapsedMillis;
  updateClockTime       += elapsedMillis;
  updateBigClockTime    += elapsedMillis;
  saveEEPROMTime        += elapsedMillis;

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
      //      // If movement is detected after a long time, say 10 mins, display 
      //      // big clock.  For instance, when waking up after sleep, display the
      //      // clock first.
      //      if ((noMotionDetectedTime > 600000) && bFanOn && !bLightOn)
      //      {
      //        showBigClock();
      //      } 

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

    if (bBigClockShow)
    {
      bBigClockShow = false;
      setHomeScreen();
      bInitializeHomeScreen = true;
    }

    if (screenSelected == HomeScreen)
    {
      // Light switch is pressed
      if ( (x>=255) && (x<=301) && (y>=30) && (y<=108) )
      {
        waitForIt(255, 30, 46, 78);
        keyPressTime = millis() - currentMillis;

        // If fan is pressed fro more than 2 seconds, open the fan options
        // screen instead of toggling power to it.
        if (keyPressTime > 1000)
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
        if (keyPressTime > 1000)
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
        waitForIt(137, 115, 56, 56);
        keyPressTime = millis() - currentMillis;

        if (keyPressTime > 1000)
        {
          if (alarmCurrentState == AlarmUnarmed)
          {
            alarmCurrentState = AlarmArming;
            bAlarmIntruderAlertSent = false;
            updateAlarmStatus();
            checkAlarmTime = checkAlarmInterval;
            sendStringToESP8266("StateAlarm", "AlarmArming");
          }
          else if (alarmCurrentState == AlarmArming)
          {
            setPinInputScreen();
          }
        }
        else
        {
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
        if (keyPressTime > 1000)
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

        if (keyPressTime > 1000)
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
        if (fanOptionSelected == Decrease1StepEvery120Mins)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepEvery120Mins;
        }

        waitForIt(10, 100, 20, 20);
        setFanOptionsScreen();
        EEPROM.write(0, fanOptionSelected);
      }        
      else if ( (x>=10) && (x<=30) && (y>=130) && (y<=150) )
      {
        if (fanOptionSelected == Decrease1StepEvery180Mins)
        {
          fanOptionSelected = noFanOptionSelected;
        }
        else
        {
          fanOptionSelected = Decrease1StepEvery180Mins;
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
        sendStringToESP8266("StateAlarm", "AlarmUnarmed");
      }
      else if (currentPIN>999)
      {
        currentPIN = 0;
        sendStringToESP8266("StateAlarm", "AlarmWrongPinEntered");
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

        if (pos > 100)
        {
          buzz(10000, 50);
          break;
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
              intLightCommand = root["LightCommand"];

              if (intLightCommand == 0)
              {
                turnLightOff();
              }
              else if(intLightCommand == 1)
              {
                turnLightOn();
              }
            }
            else if(strstr(str,"FanCommand"))
            {
              intFanCommand = root["FanCommand"];

              if (intFanCommand == 0)
              {
                turnFanOff();
              }
              else if(intFanCommand == 1)
              {
                turnFanOn();
              }
            }
            else if(strstr(str,"FanSpeedCommand"))
            {
              intFanSpeed = root["FanSpeedCommand"];
              sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
              turnFanOn();
              setFanSpeed();
            }
            else if(strstr(str,"LedCommand"))
            {
              ledCommand = root["LedCommand"];

              if (ledCommand == 0)
              {
                turnOffLed();
              }
              else if(ledCommand == 1)
              {
                turnOnLed();
              }
            } 
            else if(strstr(str,"LedSpeedCommand"))
            {
              intLedSpeed = root["LedSpeedCommand"];
              sendIntToESP8266("ChangedLedSpeed", intLedSpeed);
              ledUpdateInterval = (unsigned long)(pow(2,(12-intLedSpeed)));
            } 
            else if(strstr(str,"LedModeCommand"))
            {
              intLedMode = root["LedModeCommand"];
              sendIntToESP8266("ChangedLedMode", intLedMode);
              ledMode = (ledModes) intLedMode;
              initGlobals();
              turnOnLed();
              bColorChanged = true;
            } 
            else if(strstr(str,"LedColor"))
            {
              colorRed   = root["LedColorRed"];
              colorGreen = root["LedColorGreen"];
              colorBlue  = root["LedColorBlue"];
              turnOnLed();
              bColorChanged = true;

              //              sendIntToESP8266("ChangedLedColorRed",   colorRed);
              //              sendIntToESP8266("ChangedLedColorGreen", colorGreen);
              //              sendIntToESP8266("ChangedLedColorBlue",  colorBlue);
              sendRGBStateToESP8266();

              if ((ledMode != LedModeDefault) && 
                (ledMode != LedModeNegative) && 
                (ledMode != LedModeFade) && 
                (ledMode != LedModeCycle) && 
                (ledMode != LedModeCycle2) && 
                (ledMode != LedModeCycle2) && 
                (ledMode != LedModeCycle3) && 
                (ledMode != LedModeCycle4) && 
                (ledMode != LedModeStrobe) &&
                (ledMode != LedModeStrobeStep) &&
                (ledMode != LedModeStrobeStep2) &&
                (ledMode != LedModeLarsonScanner))
              {
                intLedMode = 0;
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

        sendStringToESP8266("StateAlarm", "AlarmArmed");
      }

      // If the alarm needs to be disarmed while it is arming, then
      // pin input screen will be displayed and hence dont update
      // alarm status
      if (screenSelected == HomeScreen)
      {
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
        sendStringToESP8266("StateAlarm", "AlarmTriggered");
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
          sendStringToESP8266("StateAlarm", "AlarmIntruderAlert");
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
          fanStartingSpeed = intFanSpeed;
        }
      }
      else
      {
        switch (fanOptionSelected)
        {
        case Decrease1StepFor1DegreeDrop:
          tempDiff = fanStartingTemperature - floatTemperatureC;         
          if (tempDiff > 0)
          {
            intFanSpeed = fanStartingSpeed - (int)tempDiff;
            if(intFanSpeed<1)
            {
              intFanSpeed = 1;
              turnFanOff();
            }
            setFanSpeed();
          }
          break;
        case Decrease1StepFor2DegreesDrop:
          tempDiff = fanStartingTemperature - floatTemperatureC;         
          if (tempDiff > 0)
          {
            intFanSpeed = fanStartingSpeed - (int)tempDiff/2;
            if(intFanSpeed<1)
            {
              intFanSpeed = 1;
              turnFanOff();
            }
            setFanSpeed();
          }
          break;
        case Decrease1StepEvery120Mins:
          intFanSpeed = fanStartingSpeed - (int)(fanRunTime/7200000);
          if(intFanSpeed<1)
          {
            intFanSpeed = 1;
            turnFanOff();
          }
          setFanSpeed();
          break;
        case Decrease1StepEvery180Mins:
          intFanSpeed = fanStartingSpeed - (int)(fanRunTime/10800000);
          if(intFanSpeed<1)
          {
            intFanSpeed = 1;
            turnFanOff();
          }
          setFanSpeed();
          break;
        case StopIfBelow:
          if (floatTemperatureC < fanStopBelowTemperature)
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
      intFanSpeed = 1;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16718055: // 2 
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 2;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16743045: // 3
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 3;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16716015: // 4
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 4;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16726215: // 5
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 5;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16734885: // 6
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 6;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16728765: // 7
      buzz(2500, 50);
      turnLcdBacklightOn();
      intFanSpeed = 7;
      turnFanOn();
      setFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      sendIntToESP8266("ChangedFanSpeed", intFanSpeed);
      break;
    case 16748655: // + 
      buzz(2500, 50);
      turnLcdBacklightOn();
      increaseFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
      break;
    case 16754775: // -
      buzz(2500, 50);
      turnLcdBacklightOn();
      decreaseFanSpeed();
      fanStartingSpeed = 0;
      fanRunTime = 0;
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
      toggleLed();
      bColorChanged = true;
      break;
    case 16712445: //  |<< (rewind)
      buzz(2500, 50);
      turnLcdBacklightOn();

      if (--intLedMode <0)
      {
        intLedMode = 19;
      }
      ledMode = (ledModes)intLedMode;

      turnOnLed();
      bColorChanged = true;
      break;    
    case 16761405: //  >>| (forward)
      buzz(2500, 50);
      turnLcdBacklightOn();
      
      if (++intLedMode > 19)
      {
        intLedMode = 0;
      }
      ledMode = (ledModes)intLedMode;

      turnOnLed();
      bColorChanged = true;
    case 16769055: // EQ
      if (bBacklightEnabled)
      {
        bBacklightEnabled = false;
        turnLcdBacklightOff();
      }
      else
      {
        bBacklightEnabled = true;
        turnLcdBacklightOn();
      }

      break;
    case 16720605: // >||
      buzz(2500, 50);
      bBacklightEnabled = true;
      turnLcdBacklightOn();
      if (bBigClockShow)
      {
        bBigClockShow = false;
        setHomeScreen();
        bInitializeHomeScreen = true;
      }
      else
      {
        showBigClock();
        sendIntToESP8266("StateBacklight", 0);
      }
      break;
    }
    irrecv.resume(); // Receives the next value from the button you press
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
        sendIntToESP8266("ChangedLight", 1);
      }
    }
    else
    {
      if (bLightOn)
      {
        bLightOn = false;
        drawLightOff();
        sendIntToESP8266("ChangedLight", 0);
      }
    }

    if (bFanZeroCrossingDetected)
    {
      if (!bFanOn)
      {
        setFanOn();
        sendIntToESP8266("ChangedFan", 1);
      }
    }
    else
    {
      if (bFanOn)
      {
        setFanOff();
        sendIntToESP8266("ChangedFan", 0);
      }
    }

  }

  if (ledUpdateTime >= ledUpdateInterval)
  {
    //    long randr = 0, randg = 0, randb = 0;
    ledUpdateTime = 0;

    if(bLedOn)
    {
      switch (ledMode)
      {
      case LedModeDefault:
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
      case LedModeNegative:
        if (bColorChanged)
        {
          for(i=0; i<strip.numPixels(); i++) 
          {
            strip.setPixelColor(i, 255 - colorRed, 255 - colorGreen, 255 - colorBlue);
          }
          strip.show();
          bColorChanged = false;      
        }
        break;
      case LedModeFade:
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, (int)(colorRed * (100-STEP2)/100), (int)(colorGreen * (100-STEP2)/100), (int)(colorBlue * (100-STEP2)/100));
        }
        if(STEP == 0)
        {
          STEP2 = STEP2++;
          if (STEP2 > 99)
          {
            STEP = 1;
          }
        }
        else if(STEP == 1)
        {
          STEP2 = STEP2--;
          if (STEP2 < 1)
          {
            STEP = 0;
          }
        }
        strip.show();
        break;
      case LedModeRainbow:      
        if(J++ > 255)
        {
          J = 0;
        }    

        for(i=0; i<LED_COUNT; i++) 
        {
          strip.setPixelColor(i, Wheel((i+J) & 255));
        }
        strip.show();
        break;
      case LedModeCycle:      
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
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, random(256), random(256), random(256));
        }
        strip.show();
        break;
      case LedModeRandomCycle2:
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
      case LedModeStrobe:
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, colorRed, colorGreen, colorBlue);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        break;
      case LedModeStrobeRandom:
        randr = random(256);    
        randg = random(256);    
        randb = random(256);    
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, randr, randg, randb);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        break;
      case LedModeStrobeStep:
        for(i=0; i<9; i++) 
        {
          strip.setPixelColor(STEP*9 + i, colorRed, colorGreen, colorBlue);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        if (++STEP>5)
        {
          STEP=0;
        }
        strip.show();
        break;
      case LedModeStrobeStep2:
        for(i=0; i<9; i++) 
        {
          strip.setPixelColor(STEP2*9 + i, colorRed, colorGreen, colorBlue);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        if (STEP == 0)
        { 
          if(++STEP2 > 4)
          {
            STEP=1;
          }
        }
        else if (STEP == 1)
        { 
          if(--STEP2 < 1)
          {
            STEP=0;
          }
        }
        strip.show();
        break;
      case LedModeStrobeStepRandom:
        STEP = random(7);
        for(i=0; i<9; i++) 
        {
          strip.setPixelColor(STEP*9 + i, colorRed, colorGreen, colorBlue);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        break;
      case LedModeStrobeStepRandom2:
        STEP = random(7);
        randr = random(256);    
        randg = random(256);    
        randb = random(256);    

        for(i=0; i<9; i++) 
        {
          strip.setPixelColor(STEP*9 + i, randr, randg, randb);
        }
        strip.show();
        delay(5);
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        break;
      case LedModeLarsonScanner:
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }

        strip.setPixelColor(L -10, (int)(colorRed * 10/100), (int)(colorGreen * 10/100), (int)(colorBlue * 10/100));
        strip.setPixelColor(L - 9, (int)(colorRed * 30/100), (int)(colorGreen * 30/100), (int)(colorBlue * 30/100));
        strip.setPixelColor(L - 8, (int)(colorRed * 50/100), (int)(colorGreen * 50/100), (int)(colorBlue * 50/100));
        strip.setPixelColor(L - 7, (int)(colorRed * 70/100), (int)(colorGreen * 70/100), (int)(colorBlue * 70/100));
        strip.setPixelColor(L - 6, (int)(colorRed * 90/100), (int)(colorGreen * 90/100), (int)(colorBlue * 90/100));
        strip.setPixelColor(L - 5, colorRed,      colorGreen,      colorBlue);
        strip.setPixelColor(L - 4, (int)(colorRed * 90/100), (int)(colorGreen * 90/100), (int)(colorBlue * 90/100));
        strip.setPixelColor(L - 3, (int)(colorRed * 70/100), (int)(colorGreen * 70/100), (int)(colorBlue * 70/100));
        strip.setPixelColor(L - 2, (int)(colorRed * 50/100), (int)(colorGreen * 50/100), (int)(colorBlue * 50/100));
        strip.setPixelColor(L - 1, (int)(colorRed * 30/100), (int)(colorGreen * 30/100), (int)(colorBlue * 30/100));
        strip.setPixelColor(L,     (int)(colorRed * 10/100), (int)(colorGreen * 10/100), (int)(colorBlue * 10/100));

        if(STEP == 0)
        {
          if (++L > 55)
          {
            STEP = 1;
          }
        }
        else if(STEP == 1)
        {
          if (--L < 0)
          {
            STEP = 0;
          }
        }
        strip.show();
        break;
      case LedModeLarsonScannerRandom:
        for(i=0; i<strip.numPixels(); i++) 
        {
          strip.setPixelColor(i, 0, 0, 0);
        }

        strip.setPixelColor(L -10, (int)(randr * 10/100), (int)(randg * 10/100), (int)(randb * 10/100));
        strip.setPixelColor(L - 9, (int)(randr * 30/100), (int)(randg * 30/100), (int)(randb * 30/100));
        strip.setPixelColor(L - 8, (int)(randr * 50/100), (int)(randg * 50/100), (int)(randb * 50/100));
        strip.setPixelColor(L - 7, (int)(randr * 70/100), (int)(randg * 70/100), (int)(randb * 70/100));
        strip.setPixelColor(L - 6, (int)(randr * 90/100), (int)(randg * 90/100), (int)(randb * 90/100));
        strip.setPixelColor(L - 5, randr,      randg,      randb);
        strip.setPixelColor(L - 4, (int)(randr * 90/100), (int)(randg * 90/100), (int)(randb * 90/100));
        strip.setPixelColor(L - 3, (int)(randr * 70/100), (int)(randg * 70/100), (int)(randb * 70/100));
        strip.setPixelColor(L - 2, (int)(randr * 50/100), (int)(randg * 50/100), (int)(randb * 50/100));
        strip.setPixelColor(L - 1, (int)(randr * 30/100), (int)(randg * 30/100), (int)(randb * 30/100));
        strip.setPixelColor(L,     (int)(randr * 10/100), (int)(randg * 10/100), (int)(randb * 10/100));

        strip.show();

        if(STEP == 0)
        {
          if (++L > 55)
          {
            randr = random(256);    
            randg = random(256);    
            randb = random(256);    
            STEP = 1;
          }
        }
        else if(STEP == 1)
        {
          if (--L < 0)
          {
            randr = random(256);    
            randg = random(256);    
            randb = random(256);    
            STEP = 0;
          }
        }
        break;
      }
    }
  }

  if(checkPowerUsageTime > checkPowerUsageInterval)
  {
    checkPowerUsageTime = 0;
    double Irms = emon1.calcIrms(2000);  // Calculate Irms only

    floatPowerCurrent = Irms;
    floatPowerWatts   = Irms * 230.0;
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
          intFanSpeed = 7;
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

  if (updateBigClockTime > updateBigClockInterval)
  {
    updateBigClockTime = 0;
    if (bBigClockShow)
    {
      showBigClock();
    }
    else if(screenSelected == BigClockScreen)
    {
      setHomeScreen();
      bInitializeHomeScreen = true;
    }
  }

  if (saveEEPROMTime > saveEEPROMInterval)
  {
    saveEEPROMTime = 0;

    if (prev_bLedOn != bLedOn)
    {
      prev_bLedOn = bLedOn;
      EEPROM.write(7, bLedOn?1:0);
    }

    if (prev_intLedMode != intLedMode)
    {
      prev_intLedMode = intLedMode;
      EEPROM.write(8, intLedMode);
    }
    
    if (prev_intLedSpeed != intLedSpeed)
    {
      prev_intLedSpeed = intLedSpeed;
      EEPROM.write(9, intLedSpeed);
    }

    if (prev_colorRed != colorRed)
    {
      prev_colorRed = colorRed;
      EEPROM.write(10, colorRed);
    }

    if (prev_colorGreen != colorGreen)
    {
      prev_colorGreen = colorGreen;
      EEPROM.write(11, colorGreen);
    }

    if (prev_colorBlue != colorBlue)
    {
      prev_colorBlue = colorBlue;
      EEPROM.write(12, colorBlue);
    }
  }
  if (weatherUpdateTime >= weatherUpdateInterval)
  {
    weatherUpdateTime = 0;
    updateWeatherData();
  }

  if (sendStatusTime > sendStatusInterval)
  {
    sendStatusTime = 0;
  
    // Send a message that the system has started.  Only after 10 seconds so that
    // the ESP8266 module would have been initialized properly.
    if (!bStarted)
    {  
      bStarted = true;
      sendIntToESP8266("ArduinoStarting", 1);
    }

    sendAllStateToESP8266();
  }

  delay(1);
}


