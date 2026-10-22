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
#include <WiFi.h>
#include <MQTT.h>

WiFiClient net;
MQTTClient client;

#include <ArduinoJson.h>
bool dumpFlag=true; //flag to trigger an export in loop()
bool saveFlag=false; //flag to control save to prefernces from loop()

// Time Globals
time_t now;  // this are the seconds since Epoch (1970) - UTC
tm tm;       // the structure tm holds time information in a more convenient way *
uint8_t hh, mm, ss;
uint8_t dy, mt;  // day and month
uint16_t yr;     // year

unsigned long ms = millis();
int hourDirectCalc = 0;                           //0-23 global hour data gets updated every second, used for AutoDim

// ----------------------------------------------------------------
// LED
#include "Adafruit_NeoPixel.h"

//# define PIXEL_BRIGHTNESS 127 //10
uint8_t PIXEL_BRIGHTNESS=20;

#define DINPIN 2
#define NPIXELS 60
//#define OFFSET 29
uint8_t OFFSET = 0;

uint32_t handleMinuteColor = 0x0000ff00; //0x00646400; //(uint32_t)strtoul("00646400", NULL, 16); //100, 100, 0;
//   printf("value = %u 0x%08x\n", handleMinuteColor, handleMinuteColor);
uint32_t handleHourColor=0x00ff0000;// 0x00c83200;
uint32_t handleSecondColor=0x0000001f; //64
uint32_t tickColor = 0x00323232; //pixels.Color(50, 50, 50); //0x00323232;

bool dimOn=true;
int hourStartDim = 20; //0 to 23
int hourStopDim = 6;
int valueDim=10;
bool flagIsAutoDimmed=false; // publish autocheck dim is on

Adafruit_NeoPixel pixels(NPIXELS, DINPIN, NEO_GRB + NEO_KHZ800);

Preferences preferences;

int cnt=0;  //used inside updateDisplay 

unsigned long lastMillis; //used inside loop()

/// convert r,g,b,w values to rgbw 32bit
uint32_t rgb_to_int(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return r | (g << 8) | (b << 16) | (w << 24);
}

/// convert rgbw 32bit to r,g,b,w values
void rgb_from_int(uint32_t rgbw, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) {
    b = (rgbw & 0x000000FF);
    g = (rgbw & 0x0000FF00) >> 8;
    r = (rgbw & 0x00FF0000) >> 16;
    w = (rgbw & 0xFF000000) >> 24;
}

// return a dimmed (or brighter) wrgb value, dim gives percent of change
uint32_t rgb_dim(uint32_t rgbw, uint8_t dim) {
    uint8_t b = (rgbw & 0x000000FF);
    uint8_t g = (rgbw & 0x0000FF00) >> 8;
    uint8_t r = (rgbw & 0x00FF0000) >> 16;
    uint8_t w = (rgbw & 0xFF000000) >> 24;
    b = b/100 * dim;
    g = g/100 * dim;
    r = r/100 * dim;
    uint32_t newWRGB = rgb_to_int(r,g,b,w);
    return newWRGB;
}

///mqtt client connect
void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("esp32_clock", "", "")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  //client.subscribe("/esp32_clock/brightness");
  client.subscribe("/esp32_clock/#");
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
  // start network
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
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
  return (i + OFFSET) % NPIXELS; 
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

bool checkAutoDim(int h, int m){
  getHourMin(h, m); //update global vars, hourDirectCalc is the hour (0..23)
 if(dimOn)
  {
    if((hourDirectCalc > hourStartDim) || (hourDirectCalc < hourStopDim)){
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
  setMinutes(m);
  setTicks(m);
  int hourPos = getHourMin(h, m); // use the minutes with hour to calculate a pos on the circle, ie 11:30 gives 57.5 instead of simply 55
  //setHours(h);
  setHours(hourPos);
  setSeconds(s);
  pixels.show();
  if(checkAutoDim(h, m)){
    pixels.setBrightness(valueDim);
  }else{
    pixels.setBrightness(PIXEL_BRIGHTNESS);
  }
  //printTime(h, m, s);
}

void updateDisplay() {
// Only update the screen once every second and at the start
  if (millis() - ms >= 1000) {
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

  client.publish("/esp32_clock/settings", mybuf, sizeof(mybuf));
  dumpFlag=false;
}

// publishes current time is within AutoDim timespan
void publishAutoDimmed(){
  String payload;
  if(checkAutoDim(hh, mm))
    payload="on";
  else
    payload="off";
//  char mybuf[payload.length()]; //initialized with \0, no need to add +1
//  payload.toCharArray(mybuf, sizeof(mybuf));
  client.publish("/esp32_clock/isAutoDimmed", (char*) payload.c_str(), sizeof(mybuf));
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

  setupWiFi();
  
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
}
