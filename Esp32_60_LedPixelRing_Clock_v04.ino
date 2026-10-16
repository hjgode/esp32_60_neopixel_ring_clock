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
  ofsset      0-59
  handleMinuteColor
  handleHourColor
  handleSecondColor
  tickColor

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

MQTT

  // ✅ Correct - use std::string directly
  std::string topic = "mytopic";
  std::string payload = "Hello World";
  mqttClient.publish(topic, payload, 0, false);

  // ✅ Convert from Arduino String
  String arduinoTopic = "mytopic";
  String arduinoPayload = "Hello World";
  mqttClient.publish(arduinoTopic.c_str(), arduinoPayload.c_str(), 0, false);

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

#include <time.h>
#include <WiFi.h>
#include <MQTT.h>

WiFiClient net;
MQTTClient client;

// Time Globals
time_t now;  // this are the seconds since Epoch (1970) - UTC
tm tm;       // the structure tm holds time information in a more convenient way *
uint8_t hh, mm, ss;
uint8_t dy, mt;  // day and month
uint16_t yr;     // year

unsigned long ms = millis();

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

void setupNtpTime() {
  // Obtain current time and set variables for the Second, Minute and Hour
  configTime(0, 0, MY_NTP_SERVER);  // 0, 0 because we will use TZ in the next line
  setenv("TZ", MY_TIMEZONE, 1);     // Set environment variable with your time zone
  tzset();
}

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
uint32_t handleSecondColor=0x0000001f;
uint32_t tickColor = 0x00323232; //pixels.Color(50, 50, 50); //0x00323232;

bool dimOn=true;
int hourStartDim = 20; //0 to 23
int hourStopDim = 6;
int valueDim=10;

void printTime(int h, int m, int s) {
  Serial.printf("h: %2d m: %2d s: %2d\n", h, m, s);
}

Adafruit_NeoPixel pixels(NPIXELS, DINPIN, NEO_GRB + NEO_KHZ800);

int index(int i) {
  return (i + OFFSET) % NPIXELS;
}

void setPixel(int i, uint32_t color) {
  pixels.setPixelColor(index(i), color);
}

uint32_t getPixel(int i) {
  return pixels.getPixelColor(index(i));
}

void setTicks(int m) {
  //uint32_t tickColor = pixels.Color(tickColor); // pixels.Color(50, 50, 50); //0x00323232
  for (int h = 0; h < 12; h++) {
    setPixel(h * 5, tickColor);
  }
}

void setHours(int h) {
  uint32_t hourColor = 0x00c83200; //pixels.Color(200, 50, 0); //0x00c83200
  setPixel(h * 5, handleHourColor);  // h: 0..11 = 12 Hours
}

void setMinutes(int m) {
  uint32_t minColor = handleMinuteColor; //pixels.Color(100, 100, 0);
/*
  for (int i = 0; i <= m; i++) {
    setPixel(i, minColor);
  }
*/
  setPixel(m, handleMinuteColor);
}

void setSeconds(int s) {
//  uint32_t color = getPixel(s);
//  setPixel(s, color + 0x373737);
  setPixel(s, handleSecondColor); //pixels.Color(200,0,0)); //0x00c80000
}

int hourDirectCalc = 0;                           //0-23 global hour data gets updated every second
//-------------------------------------------------------------------------------
/*  
 *  HourCalc will calculate the hour indication on a 60 minute scale.
 */
//-------------------------------------------------------------------------------
int hourCalc(void ){
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int hourTemp;
  int minTemp;
  
  hourTemp = timeinfo.tm_hour;
  hourDirectCalc = hourTemp;
  minTemp = timeinfo.tm_min;

  if (hourTemp < 12){
    hourTemp  = hourTemp * 5;
  }
  else{
    hourTemp  = hourTemp - 12;
    hourTemp  = hourTemp * 5;
  }
  
  minTemp = minTemp / 12;
  hourTemp = hourTemp + minTemp;

  return hourTemp;
}

//-------------------------------------------------------------------------------
/*  
 *  CheckAutoDim checks if all requirements are met to start auto dimming.
 *  Auto Dimming is diming the total brightness for a predefined time from a predefined start time, 
 *  for example at night the clock does not need to be very bright.
 */
//-------------------------------------------------------------------------------

bool checkAutoDim(void){
  hourCalc(); //update global vars
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
  setHours(h);
  setSeconds(s);
  pixels.show();
  if(checkAutoDim()){
    pixels.setBrightness(valueDim);
  }else{
    pixels.setBrightness(PIXEL_BRIGHTNESS);
  }
  //printTime(h, m, s);
}

int cnt=0;

void updateDisplay() {
// Only update the screen once every second and at the start
  if (millis() - ms >= 1000) {
    hh = tm.tm_hour;
    // 12 hours clock
    if (hh > 11) hh = hh - 12;
    
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


void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  /*
  offset
  brightness
  handleMinuteColor
  handleHourColor
  handleSecondColor
  tickColor
  */
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
  
}

unsigned long lastMillis;

void loop() {
  client.loop();

  if (!client.connected()) {
    connect();
  }
  
  updateDisplay();
  delay(100);
/*  if (millis() - lastMillis >= 2*60*1000UL) 
  {
    lastMillis = millis();  //get ready for the next iteration
    printCurrentTime();  
  }
*/
}
