/*
  This is a sketch for the tutorial 'LED Ring Clock with WS2812', published
  on https://www.makerguides.com/led-ring-clock/.

  It is using a regular ESP32 with an 60 LED ring with WS2812 LEDs.

  You can see hour marks (white dots) and the seconds running (moving white dot).
  The current hour is marked by the orange dot and the minutes by the yellow dots.

  The original project was using an api for getting the local timezone by the IP address:
  http://worldtimeapi.org/api/ip. Unfortunately, this service is no longer available.

  As a replacement, I'm using the ESP32 built-in time class that offers a local time
  adjustment based on a timezone string. For Germany/Europe it is:
  #define MY_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

  Please change the timezone string to your local place, see a complete list here:
  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv.

  As the project was developed by Stefan Maetschke, all credits go to him. I just made
  the changes to get a working clock.
  (2026) AndroidCrypto (Michael), https://medium.com/@androidcrypto
  https://github.com/AndroidCrypto/ESP32_60_LED-Pixel_Ring_Clock

  I'm using the most recent version of Arduino (2.3.7) and 
  esp32-Boards by Espressif Systems (3.3.7).

  possible mqtt values
  brightness  0-255
  offset      0-59
  handleMinuteColor as 32Bit Hex number, ie 001f3a40
  handleHourColor
  handleSecondColor
  tickColor
  dimOn
  valueDim
  hourStartDim
  hourStopDim
  dumpValues
  save_Pref
  load_Pref
  getAutoDimmed

  doc["offset"]=OFFSET;
  doc["brightness"]=PIXEL_BRIGHTNESS;
  doc["handleHourColor"]=handleHourColor;
  doc["handleMinuteColor"]=handleMinuteColor;
  doc["handleSecondColor"]=handleSecondColor;
  doc["tickColor"]=tickColor;
  doc["dimOn"]=dimOn;
  doc["valueDim"]=valueDim;
  doc["hourStartDim"]=hourStartDim;
  doc["hourStopDim"]=hourStopDim;
  getAutoDimmed

  out only:
  settings
  isAutoDimmed

  Color is a uint_32 adjusted with brightness, 32bit packed RGB value
    r,g,b 0-255
  alternativeley use

    @brief   Set a pixel's color using a 32-bit 'packed' RGB or RGBW value.
    @param   n  Pixel index, starting from 0.
    @param   c  32-bit color value. Most significant byte is white (for RGBW
                pixels) or ignored (for RGB pixels), next is red, then green,
                and least significant byte is blue.

    void Adafruit_NeoPixel::setPixelColor(uint16_t n, uint32_t c) {
        r = (r * brightness) >> 8;
        g = (g * brightness) >> 8;
        b = (b * brightness) >> 8;

*/

// ----------------------------------------------------------------
// Sketch and Board information
const char* PROGRAM_VERSION = "ESP32 60 LED Pixel Ring Clock V04";
const char* PROGRAM_TITLE = "ESP32 LED Clock";

// ----------------------------------------------------------------
// WiFi / NTP

const char* WIFI_SSID = "Horst1";
const char* WIFI_PASSWORD = "1234567890123";

// Time management
// Configuration of NTP
// choose the best fitting NTP server pool for your country
//#define MY_NTP_SERVER "at.pool.ntp.org"
#define MY_NTP_SERVER "pool.ntp.org"

// choose your time zone from this list
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
//#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#define MY_TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"  //germany

#include <Preferences.h>
#include <time.h>

//declarations
void pref_Save();
void pref_Load();

#include <WiFi.h> //see WebGUI
#include <MQTT.h>

WiFiClient net;
MQTTClient client;

#include <ArduinoJson.h>
bool dumpFlag=true; //flag to trigger an export in loop()
bool saveFlag=false; //flag to control save to prefernces from loop()

// Time Globals
time_t now;  // this are the seconds since Epoch (1970) - UTC
struct tm tm;       // the structure tm holds time information in a more convenient way *
uint8_t hh, mm, ss;
uint8_t dy, mt;  // day and month
uint16_t yr;     // year

unsigned long ms = millis();
int hourDirectCalc = 0;                           //0-23 global hour data gets updated every second, used for AutoDim

// ----------------------------------------------------------------
// LED
#include "Adafruit_NeoPixel.h"
#include <WebGUI.h>


//# define PIXEL_BRIGHTNESS 127 //10
uint8_t PIXEL_BRIGHTNESS=20;

#define DINPIN 2
#define NPIXELS 60
//#define OFFSET 29
uint8_t OFFSET = 0;

uint32_t handleMinuteColor = 0x0000ff00; //0x00646400; //(uint32_t)strtoul("00646400", NULL, 16); //100, 100, 0;
//   printf("value = %u 0x%08x\n", handleMinuteColor, handleMinuteColor);
uint32_t handleHourColor=0x00ff0000;// 0x00c83200;
uint32_t handleSecondColor=0x00ffff00; //64, yellow 00FFFF00
uint32_t tickColor = 0x00323232; //pixels.Color(50, 50, 50); //0x00323232;

bool dimOn=true;
int hourStartDim = 20; //0 to 23
int hourStopDim = 6;
int valueDim=10;
bool flagIsAutoDimmed=false; // publish autocheck dim is on
int lastCurrentHour=-1; //track autoDim check

Adafruit_NeoPixel pixels(NPIXELS, DINPIN, NEO_GRB + NEO_KHZ800);

Preferences preferences;

int cnt=0;  //used inside updateDisplay 

unsigned long lastMillis; //used inside loop()

//##### webgui stuff #########################################################
void setSlider(Slider &slider, int pos){
  slider.setValue(pos);
}
int getSlider(Slider &slider){
  return slider.getIntValue();
}

int TOGGLE_AUTODIM = 0;
// Network configuration - UPDATE THESE WITH YOUR WIFI CREDENTIALS
//const char* WIFI_SSID = "Horst1";        // Your WiFi network name
//const char* WIFI_PASSWORD = "1234567890123"; // Your WiFi password

// Web interface configuration
const char* PAGE_TITLE = "ESP32 Clock Control Panel";  // Browser tab title and page heading

const String htmlRedColor="#ff0000";
const String htmlGreenColor="#00ff00";
const String htmlBlueColor="#0000ff";
  Slider sliderBrightness("Brightness", 20, 50, 0, 255, PIXEL_BRIGHTNESS, 200);

  Slider sliderHourHandleR("Hour Red", 20, 50, 0, 255, 30, 200, "#ff0000");
  Slider sliderHourHandleG("Hour Green", 20 + 20 +200, 50, 0, 255, 30, 200, "#00ff00");
  Slider sliderHourHandleB("Hour Blue", 20+20+200+200, 50, 0, 255, 30, 200, "#0000ff");
  
  Slider sliderMinuteHandleR("Minute Red", 20, 50, 0, 255, 30, 200, "#ff0000");
  Slider sliderMinuteHandleG("Minute Green", 20 + 20 +200, 50, 0, 255, 30, 200, "#00ff00");
  Slider sliderMinuteHandleB("Minute Blue", 20+20+200+200, 50, 0, 255, 30, 200, "#0000ff");

  Slider sliderSecondHandleR("Second Red", 20, 50, 0, 255, 30, 200, htmlRedColor);
  Slider sliderSecondHandleG("Second Green", 20 + 20 +200, 50, 0, 255, 30, 200, htmlGreenColor);
  Slider sliderSecondHandleB("Second Blue", 20+20+200+200, 50, 0, 255, 30, 200, htmlBlueColor);

  Slider sliderTickColorR("Ticks Red", 20, 50, 0, 255, 30, 200, htmlRedColor);
  Slider sliderTickColorG("Ticks Green", 20 + 20 +200, 50, 0, 255, 30, 200, htmlGreenColor);
  Slider sliderTickColorB("Ticks Blue", 20+20+200+200, 50, 0, 255, 30, 200, htmlBlueColor);

  Slider sliderAutoDimStart("AutoDim Start", 20 + 20 +200, 50, 0, 23, hourStartDim, 200);
  Slider sliderAutoDimStop("AutoDim Stop", 20+20+200+200, 50, 0, 23, hourStopDim, 200);

  // Control elements - these create the web interface buttons and sliders
  Toggle toggleAutoDim("AutoDim Toggle", 20, 50, 200);     // Toggle: label, x, y, width

  // Slider constructor: Slider(label, x, y, min, max, default, width)
  Slider sliderAutoDim("AutoDim value", 20, 80, 0, 255, 90);       // Servo 1 position control

  // Button contructor
  Button saveButton("Save Settings", 20, 190, 150, 40);
  Button loadButton("Load Settings", 20, 190, 150, 40);

  // Variables to track system state
  int currentAutoDimValue = 30;     // Current brightness value (0-255)
  // Global variables to track servo states
  int sliderAutoDimValuePos = 10;     // Current position of slider

  SystemStatus systemStatus("SytemStatus", 20, 20, 350);

void startWebGUI(){
  
    Serial.println("Connecting to WiFi...");
  if (GUI.connectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("Connected to WiFi network: " + String(WIFI_SSID));
    Serial.println("Web interface: http://" + GUI.getIP());
  } else {
    Serial.println("WiFi connection failed - starting backup Access Point");
    GUI.startAP("Basic-Controller", "password123");
    Serial.println("Connect to WiFi: Basic-Controller");
    Serial.println("Web interface: http://192.168.4.1");
  }
  
  // Configure the web interface appearance
  GUI.setTitle(PAGE_TITLE);              // Browser tab title and page heading
  
  // Add all control elements to the web interface
  // The order you add them determines their order on the page
  GUI.addElement(&sliderBrightness);
  
  uint8_t R,G,B;
  //new_RGB = (blue << 16) | (green << 8) | red;
  R=(handleHourColor >> 16) & 0xff;
  G=(handleHourColor >> 8) & 0xff;
  B=handleHourColor & 0xff;
  GUI.addElement(&sliderHourHandleR);
  GUI.addElement(&sliderHourHandleG);
  GUI.addElement(&sliderHourHandleB);
  sliderHourHandleR.setValue(R);
  sliderHourHandleG.setValue(G);
  sliderHourHandleB.setValue(B);

  GUI.addElement(&sliderMinuteHandleR);
  GUI.addElement(&sliderMinuteHandleG);
  GUI.addElement(&sliderMinuteHandleB);
  R=(handleMinuteColor >> 16) & 0xff;
  G=(handleMinuteColor >> 8) & 0xff;
  B=handleMinuteColor & 0xff;
  setSlider(sliderMinuteHandleR, R);
  setSlider(sliderMinuteHandleG, G);
  setSlider(sliderMinuteHandleB, B);

  GUI.addElement(&sliderSecondHandleR);
  GUI.addElement(&sliderSecondHandleG);
  GUI.addElement(&sliderSecondHandleB);
  R=(handleSecondColor >> 16) & 0xff;
  G=(handleSecondColor >> 8) & 0xff;
  B=handleSecondColor & 0xff;
  setSlider(sliderSecondHandleR, R);
  setSlider(sliderSecondHandleG, G);
  setSlider(sliderSecondHandleB, B);

  GUI.addElement(&sliderTickColorR);
  GUI.addElement(&sliderTickColorG);
  GUI.addElement(&sliderTickColorB);
  R=(tickColor >> 16) & 0xff;
  G=(tickColor >> 8) & 0xff;
  B=tickColor & 0xff;
  setSlider(sliderTickColorR, R);
  setSlider(sliderTickColorG, G);
  setSlider(sliderTickColorB, B);

  GUI.addElement(&toggleAutoDim);     // Add LED toggle switch
  GUI.addElement(&sliderAutoDim);

  GUI.addElement(&sliderAutoDimStart);
  GUI.addElement(&sliderAutoDimStop);

  toggleAutoDim.setState(dimOn);
  sliderAutoDim.setValue(valueDim);

  GUI.addElement(&saveButton);
  GUI.addElement(&loadButton);

  GUI.addElement(&systemStatus);

  // Start the web server
  GUI.begin();

}

/*
in loop
  GUI.update();
  
  // Check if the LED toggle switch was changed
  if (ledToggle.wasToggled()) 
  {
    bool ledState = ledToggle.isOn();  // Get the current toggle state
    Serial.println("Toggle LED: " + String(ledState ? "ON" : "OFF"));
    
    // Control the built-in LED with simple on/off
    digitalWrite(TOGGLE_LED_PIN, ledState);
  }
*/

uint32_t oldRGB=0;

// call peridically in loop()
void updateWebGUI(){
  GUI.update();
  
  //brightness
  int bright=getSlider(sliderBrightness);
  PIXEL_BRIGHTNESS=bright;

  // Check if the LED toggle switch was changed
  if (toggleAutoDim.wasToggled()) 
  {
    bool autoDimState = toggleAutoDim.isOn();  // Get the current toggle state
    //TODO: apply changes
    dimOn=autoDimState;
  }
  //TODO: apply changes...
  valueDim=sliderAutoDim.getIntValue();

  int R,G,B;
  //hour handle
  R=getSlider(sliderHourHandleR);
  G=getSlider(sliderHourHandleG);
  B=getSlider(sliderHourHandleB);
  //get long int for this
  uint32_t RGB = (R << 16) | (G << 8) | B;
  handleHourColor=RGB;
  if(oldRGB != RGB){
    Serial.printf("hour handle color now: ");
    Serial.println(RGB);
    oldRGB=RGB;
  }

  //minute handle
  R=getSlider(sliderMinuteHandleR);
  G=getSlider(sliderMinuteHandleG);
  B=getSlider(sliderMinuteHandleB);
  //get long int for this
  RGB = (R << 16) | (G << 8) | B;
  handleMinuteColor=RGB;

  //second handle
  R=getSlider(sliderSecondHandleR);
  G=getSlider(sliderSecondHandleG);
  B=getSlider(sliderSecondHandleB);
  //get long int for this
  RGB = (R << 16) | (G << 8) | B;
  handleSecondColor=RGB;
  
  //tick color
  R=getSlider(sliderTickColorR);
  G=getSlider(sliderTickColorG);
  B=getSlider(sliderTickColorB);
  //get long int for this
  RGB = (R << 16) | (G << 8) | B;
  tickColor=RGB;

  //hour Start Dim
  int startDim=getSlider(sliderAutoDimStart);
  hourStartDim=startDim;
  //hour Stop Dim
  hourStopDim=getSlider(sliderAutoDimStop);

  // Check if save button was pressed
  if (saveButton.wasPressed()) {
    //TODO: save settings
    pref_Save();
  }
  if (loadButton.wasPressed()) {
    //TODO: save settings
    pref_Load();
  }
}
//##### end webgui


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
// gives _rmtGetBus(): ==>_rmtWrite():GPIO 2 is not attached to an RMT channel. in this app!
void rainbow(int wait) {
  pixels.setBrightness(80);
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    // strip.rainbow() can take a single argument (first pixel hue) or
    // optionally a few extras: number of rainbow repetitions (default 1),
    // saturation and value (brightness) (both 0-255, similar to the
    // ColorHSV() function, default 255), and a true/false flag for whether
    // to apply gamma correction to provide 'truer' colors (default true).
    pixels.rainbow(firstPixelHue);
    // Above line is equivalent to:
    // strip.rainbow(firstPixelHue, 1, 255, 255, true);
    pixels.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

/// convert r,g,b,w values to rgbw 32bit
uint32_t rgb_to_int(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  //get long int for this
  uint32_t RGB = (w << 24) | (r << 16) | (g << 8) | b;
  return RGB;
//    return r | (g << 8) | (b << 16) | (w << 24);
}

// return a dimmed (or brighter) wrgb value, dim gives percent of change
uint32_t rgb_dim(uint32_t rgbw, uint8_t dim) {
    uint8_t r=(rgbw >> 16) & 0xff;
    uint8_t g=(rgbw >> 8) & 0xff;
    uint8_t b=rgbw & 0xff;
    uint8_t w =(rgbw >> 24) & 0xff;

    b = b/100 * dim;
    g = g/100 * dim;
    r = r/100 * dim;
    uint32_t newWRGB = rgb_to_int(r,g,b,w);
    return newWRGB;
}

///mqtt client connect
void connect() {
//  rainbow(1); //gives _rmtGetBus(): ==>_rmtWrite():GPIO 2 is not attached to an RMT channel.
  Serial.print("checking wifi...");
  int max_attempts=5;
  int test=0;
  while ((WiFi.status() != WL_CONNECTED) && test<max_attempts ) {
    if (!GUI.connectWiFi(WIFI_SSID, WIFI_PASSWORD)){
      Serial.print(".");
      test++;
    }
    delay(1000);
    //rainbow(1);
  }

  Serial.print("\nconnecting...");
  //use different ID? if two clocks in net?
  while (!client.connect("esp32_clock1", "", "")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  //client.subscribe("esp32_clock/brightness");
  client.subscribe("esp32_clock/#");
  Serial.println("subscribe done");
  // client.unsubscribe("/hello");
}

/// setup ntp tim and timezone incl. DST
void setupNtpTime() {
  // Obtain current time and set variables for the Second, Minute and Hour
  configTime(0, 0, MY_NTP_SERVER);  // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TIMEZONE, 1);     // Set environment variable with your time zone
  tzset();
}

/// connect to WiFi
void setupWiFi() {
  startWebGUI();
  // start network
/*
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
*/
}

/// dump current time to serial
void printCurrentTime() {;
  time(&now);              // read the current time
  localtime_r(&now, &tm);  // update the structure tm with the current time
  char buf[100];
  char sDst[5];
  if(tm.tm_isdst==1)
    snprintf(sDst,5,"DST");
  else
    snprintf(sDst,5,"STD");

  size_t len = strftime(buf, 100, "%Y-%m-%d %H:%M:%S ", &tm);

  Serial.printf("%s %s", buf, sDst);
  
  Serial.println();
}

/// save values to NVS
void pref_Save(){
  size_t result=0;
  preferences.begin("esp32_clock", false);
  preferences.clear();
  //                           ....5....0....5
  result=preferences.putULong("handleHourColor", handleHourColor);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "handleHourColor");
  result=preferences.putULong("handleMinColor", handleMinuteColor);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "handleMinColor");
  result=preferences.putULong("handleSecColor", handleSecondColor);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "handleSecColor");

  result=preferences.putUInt("offset", OFFSET);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "offset");
  result=preferences.putUInt("brightness", PIXEL_BRIGHTNESS);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "brightness");

  result=preferences.putULong("tickColor", tickColor);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "tickColor");

  result=preferences.putBool("dimOn", dimOn);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "dimOn");
  result=preferences.putInt("valueDim", valueDim);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "valueDim");

  result=preferences.putInt("hourStartDim", hourStartDim);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "hourStartDim");
  result=preferences.putInt("hourStopDim", hourStopDim);
  if(result==0) Serial.printf("Error in pref_Save for %s\n", "hourStopDim");

  preferences.end();
  saveFlag=false;
  Serial.println("save_Pref done");
}

/// load values from NVS
void pref_Load(){
  preferences.begin("esp32_clock", true);
  unsigned long ul=0;
  uint32_t ulDefault=0;
  ulDefault=handleHourColor;
  handleHourColor=preferences.getULong("handleHourColor", ulDefault);
  ulDefault=handleMinuteColor;
  handleMinuteColor=preferences.getULong("handleMinColor", ulDefault);  
  ulDefault=handleSecondColor;
  handleSecondColor=  preferences.getULong("handleSecColor", ulDefault);

  uint uiDef=OFFSET;
  OFFSET=preferences.getUInt("offset", uiDef);
  uiDef=PIXEL_BRIGHTNESS;
  PIXEL_BRIGHTNESS =  preferences.getUInt("brightness", uiDef);

  ulDefault=tickColor;
  tickColor=preferences.getULong("tickColor", ulDefault);

  bool bDef=dimOn;
  dimOn=preferences.getBool("dimOn", bDef);

  int iDef=valueDim;
  valueDim=preferences.getInt("valueDim", iDef);
  iDef=hourStartDim;
  hourStartDim=preferences.getInt("hourStartDim", iDef);
  iDef=hourStopDim;
  hourStopDim=preferences.getInt("hourStopDim", iDef);

  preferences.end();
  Serial.println("load_Pref done");
}

void printTime(int h, int m, int s) {
  Serial.printf("h: %2d m: %2d s: %2d\n", h, m, s);
}

//give pixel number for givven pixel count and offset
int index(int i) {
  return (i + OFFSET) % NPIXELS; //ie 15 Seconds => (15 + 0) % 60 = 15
  // 5 seconds => 5 + 0 % 60 = 5
}

//set pixel on LED band regarding offset and num LEDs
void setPixel(int i, uint32_t color) {
  pixels.setPixelColor(index(i), color);
}

//set pixel and left and right neighbor pixel
void setPixelBlock(int i, uint32_t color) {
  //pixels.setPixelColor(index(i), color);
  pixels.setPixelColor(index(i-1), color);
  pixels.setPixelColor(index(i+1), color);
}

uint32_t getPixel(int i) {
  return pixels.getPixelColor(index(i));
}

//set pixel for each hour mark
void setTicks(int m) {
  //uint32_t tickColor = pixels.Color(tickColor); // pixels.Color(50, 50, 50); //0x00323232
  for (int h = 0; h < 12; h++) {
    setPixel(h * 5, tickColor);
  }
}

//show hour pixel on LED circle
void setHours(int h) {
  //uint32_t hourColor = 0x00c83200; //pixels.Color(200, 50, 0); //0x00c83200
  //already aligned in getHourMin
  setPixel(h, handleHourColor);
//  setPixel(h * 5, handleHourColor);  // h: 0..11 = 12 Hours, 5=NumPixels/12
  //use a dimmed color for the neighbor pixels
  uint32_t newRGBW = rgb_dim(handleHourColor, 50);
  setPixelBlock(h, newRGBW);
//  setPixelBlock(h*5, newRGBW);
}

void setMinutes(int m) {
  uint32_t minColor = handleMinuteColor; //pixels.Color(100, 100, 0);
/*
  for (int i = 0; i <= m; i++) {
    setPixel(i, minColor);
  }
*/
  setPixel(m, handleMinuteColor);
  uint32_t newRGBW = rgb_dim(handleMinuteColor, 50);
  setPixelBlock(m, newRGBW);
}

void setSeconds(int s) {
//  uint32_t color = getPixel(s);
//  setPixel(s, color + 0x373737);
  setPixel(s, handleSecondColor); //pixels.Color(200,0,0)); //0x00c80000
}

//-------------------------------------------------------------------------------
/*  
 *  HourCalc will calculate the hour indication on a 60 minute scale.
   06:15 gives 6 * 5 + 15/12 = 31,25
   11:30 gives 11 * 5 + 30/12 = 57.5 (2.5 Pixels before 12oclock)

 */
//-------------------------------------------------------------------------------
int getHourMin(int hourTemp, int minTemp){
/*
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int hourTemp;
  int minTemp;
  
  hourTemp = timeinfo.tm_hour;
  minTemp = timeinfo.tm_min;

*/
  hourDirectCalc = hourTemp;

  if (hourTemp < 12){
    hourTemp  = hourTemp * 5; //5 is Pixel_Num / 12 (a lcok shows up to 12 hours)
  }
  else{
    hourTemp  = hourTemp - 12;
    hourTemp  = hourTemp * 5;
  }
  
  minTemp = minTemp / 12; //minutes divided by 12 
  hourTemp = hourTemp + minTemp; 
  //11:30 gives 11 * 5 + 30/12 = 57.5 (2.5 Pixels before 12oclock)
  //06:15 gives 6 * 5 + 15/12 = 31,25
  return hourTemp;
}

//-------------------------------------------------------------------------------
/*  
 *  CheckAutoDim checks if all requirements are met to start auto dimming.
 *  Auto Dimming is diming the total brightness for a predefined time from a predefined start time, 
 *  for example at night the clock does not need to be very bright.
 */
//-------------------------------------------------------------------------------
bool checkAutoDim(int h){
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int hourTemp;
  int minTemp;
  hourTemp = timeinfo.tm_hour;
  //getHourMin(h, m); //update global vars, hourDirectCalc is the hour (0..23)
 if(dimOn)
  {
//    Serial.printf("hour=%i, StartDim=%i StopDim=%i", hourTemp, hourStartDim, hourStopDim);
    // start=20 stop=06
    // start=13 stop=15
    if((hourTemp >= hourStartDim) || (hourTemp < hourStopDim)){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}

void showTime(int h, int m, int s) {
  pixels.clear();
  setTicks(m);

  setMinutes(m);
  int hourPos = getHourMin(h, m); // use the minutes with hour to calculate a pos on the circle, ie 11:30 gives 57.5 instead of simply 55
  //setHours(h);
  setHours(hourPos);
  setSeconds(s);
  
  if(lastCurrentHour!=h){
    if(checkAutoDim(h)){
      //Serial.println("AutoDim on");
      pixels.setBrightness(valueDim);
    }else{
      //Serial.println("AutoDim OFF");
      pixels.setBrightness(PIXEL_BRIGHTNESS);
    }
    lastCurrentHour=h;
  }
  pixels.show();
  //printTime(h, m, s);
}

/// update display, only runs is a second has elapsed
void updateDisplay() {
// Only update the screen once every second and at the start
  if (millis() - ms >= 1000) {
    ms = millis(); //reset
    time(&now);              // read the current time
    localtime_r(&now, &tm);  // update the structure tm with the current time
    hh = tm.tm_hour;
    // 12 hours clock
    if (hh > 11) 
      hh = hh - 12;
    
    mm = tm.tm_min;
    // current measure mode, gives acurrent of 100 mA with brightness of 10
    // during WiFi update up to 170 mA
    // for brightness 30, the current increases to 150 mA / 217 mA
    // for brightness 5, the current decreases to 89 / 159 mA, but the LEDs are 
    // no longer clear to differentate, 10 is a good choice
    //mm = 59;

    ss = tm.tm_sec;
    showTime(hh, mm, ss);
    cnt++;
    if (cnt>30){
      printCurrentTime();
      cnt=0;
    }
  }
}

/// print all values to json and publish to mqtt
void dumpValues(){
  JsonDocument doc;
  doc["offset"]=OFFSET;
  doc["brightness"]=PIXEL_BRIGHTNESS;
  doc["handleHourColor"]=handleHourColor;
  doc["handleMinuteColor"]=handleMinuteColor;
  doc["handleSecondColor"]=handleSecondColor;
  doc["tickColor"]=tickColor;
  doc["dimOn"]=dimOn;
  doc["valueDim"]=valueDim;
  doc["hourStartDim"]=hourStartDim;
  doc["hourStopDim"]=hourStopDim;

  String payload;
  serializeJsonPretty(doc, payload);
  char mybuf[payload.length()+1];
  payload.toCharArray(mybuf, sizeof(mybuf));

  client.publish("esp32_clock/settings", mybuf, sizeof(mybuf));
  dumpFlag=false;
}

// publishes current time is within AutoDim timespan
void publishAutoDimmed(){
  String payload;
  if(checkAutoDim(hh))
    payload="on";
  else
    payload="off";
  char mybuf[payload.length()]; //initialized with \0, no need to add +1
  payload.toCharArray(mybuf, sizeof(mybuf));
  client.publish("esp32_clock/isAutoDimmed", (char*) payload.c_str(), sizeof(mybuf));
  flagIsAutoDimmed=false;
}

// mqtt subscribe handler
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  Serial.printf("received %s, %s\n", topic.c_str(), payload.c_str());
  String sTopic(topic);
  String sPayload(payload);
  if(sTopic.endsWith("offset")){
    OFFSET=(uint8_t)payload.toInt();
    Serial.printf("mqtt: new OFFSET=%i\n", OFFSET);
  }
  else if(topic.endsWith("brightness")){
    PIXEL_BRIGHTNESS=(uint8_t) payload.toInt();
      //will be done in next showTime  pixels.setBrightness(PIXEL_BRIGHTNESS);
    Serial.printf("mqtt: new PIXEL_BRIGHTNESS=%i\n", PIXEL_BRIGHTNESS);
  }
  else if(topic.endsWith("handleHourColor")){
    uint32_t newcolor = (uint32_t)strtoul(payload.c_str(), NULL, 16);
    handleHourColor = newcolor;
    Serial.printf("new handleHourColor = 0x%08x , %u\n", handleHourColor, handleHourColor);
  }
  else if(topic.endsWith("handleMinuteColor")){
    uint32_t newcolor = (uint32_t)strtoul(payload.c_str(), NULL, 16);
    handleMinuteColor = newcolor;
    Serial.printf("new handleMinuteColor = 0x%08x , %u\n", handleMinuteColor, handleMinuteColor);
  }
  else if(topic.endsWith("handleSecondColor")){
    uint32_t newcolor = (uint32_t)strtoul(payload.c_str(), NULL, 16);
    handleSecondColor = newcolor;
    Serial.printf("new handleSecondColor = 0x%08x , %u\n", handleSecondColor, handleSecondColor);
  }
  else if(topic.endsWith("tickColor")){
    uint32_t newcolor = (uint32_t)strtoul(payload.c_str(), NULL, 16);
    tickColor = newcolor;
    Serial.printf("new tickColor = 0x%08x , %u\n", tickColor, tickColor);
  }
  else if(topic.endsWith("dimOn")){
    dimOn= sPayload.toInt(); //0 or 1
    Serial.printf("new dimOn = %i \n", dimOn);
  }
  else if(topic.endsWith("valueDim")){
    valueDim= sPayload.toInt();
    Serial.printf("new valueDim = %i \n", valueDim);
  }
  else if(topic.endsWith("hourStartDim")){
    hourStartDim= sPayload.toInt();
    Serial.printf("new hourStartDim = %i \n", hourStartDim);
  }
  else if(topic.endsWith("hourStopDim")){
    hourStopDim= sPayload.toInt();
    Serial.printf("new hourStopDim = %i \n", hourStopDim);
  }
  else if (topic.endsWith("dumpValues")){
    dumpFlag=true;
  }
  else if (topic.endsWith("save_Pref")){
    saveFlag=true;
    pref_Save();
  }
  else if (topic.endsWith("load_Pref")){
    pref_Load();
  }
  else if (topic.endsWith("getAutoDimmed")){
    flagIsAutoDimmed=true;
  }

}


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("");
  Serial.println(PROGRAM_VERSION);

  setupNtpTime();

  setupWiFi();  // calls startWebGUI();

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin("192.168.0.40", 1883, net);
  client.onMessage(messageReceived);

  connect();

  pixels.begin();
  pixels.clear();
  pixels.setBrightness(PIXEL_BRIGHTNESS);
  pixels.show();
  
  pref_Load();

}

void loop() {
  client.loop();

  if (!client.connected()) {
    connect();
  }
  
  updateDisplay();
  delay(100);

  updateWebGUI();

  if(dumpFlag){
    dumpValues();
  }
  if(saveFlag){
    pref_Save();
    saveFlag=false;
  }
  if(flagIsAutoDimmed){
    publishAutoDimmed();

  }

/*  if (millis() - lastMillis >= 2*60*1000UL) 
  {
    lastMillis = millis();  //get ready for the next iteration
    printCurrentTime();  
  }
*/
  //refresh SystemInfo
  if (millis() - lastMillis >= 5*1000UL) 
  {
    lastMillis = millis();  //get ready for the next iteration
    systemStatus.setValue("");
  }
}
