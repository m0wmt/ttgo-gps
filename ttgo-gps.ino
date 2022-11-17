/*
  GPS Speed Monitor

  Do not forget to check user_setup_select.h in TFT_eSPI folder to ensure is not using user_setup.h and using the right
  LCD, in our case <User_Setups/Setup25_TTGO_T_Display.h>

  Arduino IDE, board used: TTGO LoRa32-OLED

  GPS chip used: GT-U7

  Pin connections between GT-U7 and ESP32 TTGO T-Display
    GPS <-> ESP32 
    VCC <-> 5V
    GND <-> G
    RXD <-> Pin 26 
    TXD <-> Pin 25
    PPS - not used
*/
#include <Arduino.h>
#include <math.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include "satellite.h"

// Turn debug messages (Serial.print) on/off
//#define DEBUG

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite satcom = TFT_eSprite(&tft);

//static const int RXPin = 16, TXPin = 17;  ESP32 with no display
static const int RXPin = 25, TXPin = 26;
static const uint32_t GPSBaud = 9600;

SoftwareSerial ss(RXPin, TXPin);
TinyGPSPlus gps;

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);

    Serial.println("\n##################################");
    Serial.println(F("ESP32 Information:"));
    Serial.print(F("  ESP Chip Model:    ")); Serial.println(ESP.getChipModel()); 
    Serial.print(F("  CPU Freq:          ")); Serial.print(ESP.getCpuFreqMHz()); Serial.println(F(" MHz"));
    Serial.print(F("  SDK Version:       ")); Serial.println(ESP.getSdkVersion());
    Serial.print(F("  Heap Size:         ")); Serial.print(ESP.getHeapSize()); Serial.println(F(" bytes"));
    Serial.print(F("  Free Heap:         ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));
    Serial.print(F("  Used Heap:         ")); Serial.print(ESP.getHeapSize()-ESP.getFreeHeap()); Serial.println(F(" bytes"));
    Serial.print(F("  Sketch Size:       ")); Serial.print(ESP.getSketchSize()); Serial.println(F(" bytes"));
    Serial.print(F("  Free Sketch Space: ")); Serial.print(ESP.getFreeSketchSpace()); Serial.println(F(" bytes"));
    Serial.println(F(""));
    Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n", ESP.getHeapSize(), ESP.getHeapSize()-ESP.getFreeHeap(), ESP.getFreeHeap());
    Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
    Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
    Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
    Serial.println("##################################\n");
  #endif

  tft.init();
  tft.setRotation(3);
  tft.setSwapBytes(true);
  
  sprite.createSprite(240, 135);
  sprite.setSwapBytes(true);

  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE);
  sprite.setTextFont(4);
  sprite.drawString("Waiting on GPS fix", 15, 50);

  satcom.createSprite(20, 20);
  satcom.pushImage(0, 0, 20, 20, satellite);
  satcom.pushToSprite(&sprite, 5, 109, TFT_BLACK);
  sprite.setTextFont(2);
  sprite.drawString("0", 35, 115);

  sprite.pushSprite(0, 0);

  // Start listening to gps serial line
  ss.begin(9600);

  #ifdef DEBUG
    Serial.println(F("Initialisation complete."));
  #endif
}

void loop() {
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  if (gps.location.isUpdated()) {
    updateDisplay();
  }

  delay(100);
}

// Update the display with the new position, altitude etc.
void updateDisplay()
{
  // Default values
  int satellites = 0;
  int mph = 0;
  double lat = 0;
  double lng = 0;
  double altitude = 0;
  char data[50] = {};
  
  if (gps.satellites.isValid()) {
    satellites = gps.satellites.value();
  }

  if (gps.speed.isValid()) {
    // We're not interested in decimal points, get the nearest whole number
    // to display on the screen for mph.

    mph = lround(gps.speed.mph());

    #ifdef DEBUG
      Serial.print(F("SPEED      Fix Age="));
      Serial.print(gps.speed.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.speed.value());
      Serial.print(F(" MPH="));
      Serial.println(gps.speed.mph());
    #endif
  }

  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
  }

  if (gps.altitude.isValid()) {
    altitude = gps.altitude.feet();

    #ifdef DEBUG
      Serial.print(F("ALTITUDE   Fix Age="));
      Serial.print(gps.altitude.age());
      Serial.print(F("ms Raw="));
      Serial.print(gps.altitude.value());
      Serial.print(F(" Meters="));
      Serial.println(gps.altitude.meters());
    #endif
  }

  // Display lat/lng
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(TFT_WHITE);
  sprite.setTextFont(2);
  sprintf(data, "Lat: %.6lf  Lon: %.6lf", lat, lng);
  sprite.drawString(data, 5, 5);

  // Display altitude
  if (altitude > 0) {
    sprintf(data, "Alt: %.2f ft", altitude);
    sprite.drawString(data, 5, 23);
  } else {
    sprite.drawString("Alt: 0 ft", 5, 23);
  }

  // large font for MPH value
  sprite.setTextFont(8);
  if (mph > 0) {
    sprintf(data, "%d", mph);
    if (mph < 10) {
      sprite.drawString(data, 106, 55);
    } else {
      sprite.drawString(data, 51, 55);
    }
  } else {
    sprite.drawString("0", 106, 55);
  }  

  // Smaller font for the mph sign
  sprite.setTextFont(4);
  sprite.setTextColor(TFT_RED);
  sprite.drawString("mph", 163, 110);

  // Show number of satellites viewed by the GPS antenna
  sprite.setTextFont(2);
  if (satellites > 0) {
    sprintf(data, "%d", satellites);
    sprite.drawString(data, 35, 115);
  } else {
    sprite.drawString("0", 35, 115);
  }
  satcom.pushToSprite(&sprite, 5, 109, TFT_BLACK);

  // Display the information
  sprite.pushSprite(0, 0);

  #ifdef DEBUG
    if (gps.time.isValid()) {
      Serial.print(F(" "));
      if (gps.time.hour() < 10) Serial.print(F("0"));
      Serial.print(gps.time.hour());
      Serial.print(F(":"));
      if (gps.time.minute() < 10) Serial.print(F("0"));
      Serial.print(gps.time.minute());
      Serial.print(F(":"));
      if (gps.time.second() < 10) Serial.print(F("0"));
      Serial.print(gps.time.second());
      Serial.print(F("."));
      if (gps.time.centisecond() < 10) Serial.print(F("0"));
      Serial.print(gps.time.centisecond());

      Serial.println();
    }

    // sprintf(data, "-Satellites: %d\n-Latitude: %.6lf\n-Longitude: %.6lf\n-Altitude: %.2lf ft\n-Speed: %.1lf mph", satellites, lat, lng, altitude, mph);
    // Serial.println(data);
  #endif
}