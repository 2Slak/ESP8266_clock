// ESP8266 clock using NTP
// Author: Jernej Slak
// Date: 13.11.2021
/* References I used:
https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
*/

#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"

//#define DEBUG
#define PIN_DIO  D6   // define DIO pin (any digital pin)
#define PIN_CLK  D5   // define CLK pin (any digital pin)

// WiFi creds
const char *ssid     = "zrihtejteDvigalo";
const char *password = "f#z,e8e_lj";

// Time offset
const long utcOffsetInSeconds = 3600;
int localHours;
int localMinutes;
int localSeconds;

// ntp initialization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// Display init
SevenSegmentExtended    display(PIN_CLK, PIN_DIO, 20);


// Data storage for deep sleep
uint32_t calculateCRC32(const uint8_t *data, size_t length);
// Structure which will be stored in RTC memory.
struct {
  uint32_t crc32;
  uint8_t storedData[3];
  //  0 - loopCounter
  //  1 - previousHours
  //  2 - previousMinutes
} rtcData;



void setup() {

  
  #ifdef DEBUG
    Serial.begin(115200);
    delay(1000);
  #endif

  //Check CRC of data in rtc memory
  ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData));
  uint32_t crcOfData = calculateCRC32((uint8_t*) &rtcData.storedData[0], sizeof(rtcData.storedData)); 
  if (crcOfData != rtcData.crc32) {
      //data doesn't match, reset loop counter to 0
      for (size_t i = 0; i < sizeof(rtcData.storedData); i++) {
        rtcData.storedData[i] = 0;
      }
  } 



  // Every time iteration counter goes to 0 connect to wifi and get ntp time
  if(rtcData.storedData[0] <= 4){
    // Wifi initialization
    WiFi.begin(ssid, password);
    Serial.print("Connecting");
    while ( WiFi.status() != WL_CONNECTED ) {
      
      delay ( 50 );
      #ifdef DEBUG
      Serial.print ( "." );
    }
    Serial.println("Connected.");
    delay(1000);
    {
    #endif
    }

    timeClient.begin();
    // Update time
    timeClient.update();
    rtcData.storedData[1] = timeClient.getHours();
    rtcData.storedData[2] = timeClient.getMinutes();
    localSeconds = timeClient.getSeconds();
    

    display.begin();              // initializes the display
    display.setBacklight(10);     // set the brightness to level 2 (max 8)
    display.setColonOn(true);
    display.printTime(rtcData.storedData[1], rtcData.storedData[2], false); 

    delay((60-timeClient.getSeconds())*1000);

    timeClient.update();
    rtcData.storedData[1] = timeClient.getHours();
    rtcData.storedData[2] = timeClient.getMinutes();
  
    display.printTime(rtcData.storedData[1], rtcData.storedData[2], false); 
  }
  else{
    //increment minutes, if min == 60, increment clock, reset minutes
    rtcData.storedData[2] = rtcData.storedData[2] + 1;
    if(rtcData.storedData[2] == 60){
      rtcData.storedData[2] = 0;
      rtcData.storedData[1] = rtcData.storedData[1] + 1;
      if( rtcData.storedData[1] == 24){
        rtcData.storedData[1] = 0;
      }
    }
    display.begin();              // initializes the display
    display.setBacklight(10);     // set the brightness to level 2 (max 8)
    display.setColonOn(true);
    display.printTime(rtcData.storedData[1], rtcData.storedData[2], false); 
  }
  
  


  // Increase loop counter, if it is >=61, reset it to 0
  if(rtcData.storedData[0] >= 61){
    rtcData.storedData[0] = 0;
  }
  else{
    rtcData.storedData[0]=rtcData.storedData[0] + 1;
  }
  // Update CRC32 of data
  rtcData.crc32 = calculateCRC32((uint8_t*) &rtcData.storedData[0], sizeof(rtcData.storedData));
  // Write struct to RTC memory
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData));

  #ifdef DEBUG
  Serial.println("Iteration counter: ");
  Serial.print(rtcData.storedData[0]);
  Serial.print(rtcData.storedData[1]);
  Serial.print(rtcData.storedData[2]);
  delay(500);
  #endif

  // Sleep for 1 minute
  ESP.deepSleep(60e6);

}

void loop() {


}


uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}