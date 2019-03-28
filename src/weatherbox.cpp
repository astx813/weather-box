#define DEBUG true
// IFTTT/Adafruit IO/NeoPixel strio/7-segment display weather shadowbox by Becky Stern
// This program contains bits and pieces of various library sample codes:
/***************************************************
  Adafruit LED 7-Segment backpacks
  ----> http://www.adafruit.com/products/881
  ----> http://www.adafruit.com/products/880
  ----> http://www.adafruit.com/products/879
  ----> http://www.adafruit.com/products/878

  These displays use I2C to communicate, 2 pins are required to
  interface. There are multiple selectable I2C addresses. For backpacks
  with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
  with 3 Address Select pins: 0x70 thru 0x77

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  I2C LED Backpack Example written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution

// Adafruit IO Multiple Feed Example written by Todd Treece for Adafruit Industries
// Smart Toilet Light with ESP8266 written by Tony DiCola for Adafruit Industries
// Licenses: MIT (https://opensource.org/licenses/MIT)
 ****************************************************/

//#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_NeoPixel.h"
#include <ESP8266WiFi.h>
#include <AdafruitIO.h>
#include <Adafruit_MQTT.h>

#include "MegunoLink.h"
#include "Filter.h"

// Adafruit IO Subscription Example
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

// edit public.h tab and configure Adafruit IO & WIFI credentials
// and any other private data. Then rename to private.h!!!
#include "private.h"

// Configuration you can optionally change (but probably want to keep the same):
#define PIXEL_PIN       3                      // Pin connected to the NeoPixel data input.
#define PATTERN_PIN     16
#define PHOTO_PIN       A0
#define RED_PIN         0
#define BLUE_PIN        2
#define PIXEL_COUNT     11                      // Number of NeoPixels.
#define PIXEL_TYPE      NEO_GRBW + NEO_KHZ800   // Type of the NeoPixels (see strandtest example).

// before running this code, create feeds on Adafruit IO that match these names:
AdafruitIO_Feed *hightemp = io.feed("weather-high"); // set up the 'hightemp' feed
AdafruitIO_Feed *precipitation = io.feed("weather-precipitation"); // set up the 'precipitation' feed
AdafruitIO_Feed *lowtemp = io.feed("weather-low");
AdafruitIO_Feed *sunrise = io.feed("weather-sunrise");
AdafruitIO_Feed *sunset  = io.feed("weather-sunset");
AdafruitIO_Feed *control = io.feed("weather-control");
Adafruit_7segment highmatrix = Adafruit_7segment();  // create segment display object
Adafruit_7segment lowmatrix = Adafruit_7segment();
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE); // create NeoPixels object
ExponentialFilter<int> light(10,0);

int prevlight = -1;
int brightness = 255;
int photo;

void lightPixels(uint32_t);
void lightRange(int start, int end, uint32_t color, bool show);
void handleHigh(AdafruitIO_Data *data);
void handleLow(AdafruitIO_Data *data);
void handleSunrise(AdafruitIO_Data *data);
void handleSunset(AdafruitIO_Data *data);
void handleCondition(AdafruitIO_Data *data);
void handleControl(AdafruitIO_Data *data);

void setup() {
  int rnd=random(1,51);

  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);
  // start the serial connection
  Serial.begin(115200);

  highmatrix.begin(0x70); // initialize matrix displays
  lowmatrix.begin(0x71);
  highmatrix.clear();
  lowmatrix.clear();

  // Initialize NeoPixels.
  pixels.begin();
  lightRange(0,3,pixels.Color(rnd*5,rnd,rnd,0),false);
  lightRange(4,7,pixels.Color(rnd,rnd*5,rnd,0),false);
  lightRange(8,11,pixels.Color(rnd,rnd,rnd*5,0),true);

  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  hightemp->onMessage(handleHigh);
  lowtemp->onMessage(handleLow);
  sunrise->onMessage(handleSunrise);
  sunset->onMessage(handleSunset);
  precipitation->onMessage(handleCondition);

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  lightRange(4,7,pixels.Color(0,0,0,0),true);
  // we are connected
  if (DEBUG) {
    Serial.println();
    Serial.println(io.statusText());
    Serial.println("Loading initial vals");
  }
  hightemp->get();
  lowtemp->get();
  sunrise->get();
  sunset->get();
  precipitation->get();
  if (DEBUG) Serial.println("Setup complete.");
}

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.

  io.run();
  photo = analogRead(PHOTO_PIN);
  light.Filter(photo);
  brightness = map(light.Current(),1024,25,16,255);
  if(abs(brightness - prevlight)>8) {
    if(DEBUG) {
      Serial.print(brightness);
      Serial.print(" ");
      Serial.println(photo);
    }
    prevlight = brightness;
    pixels.setBrightness(brightness);
    highmatrix.setBrightness(map(light.Current(),1024,0,1,15));
    lowmatrix.setBrightness(map(light.Current(),1024,0,1,15));
    highmatrix.writeDisplay();
    lowmatrix.writeDisplay();
    pixels.show();
  }
  analogWrite(LED_BUILTIN, light.Current());
}

// these function are called whenever a feed message
// is received from Adafruit IO. They were attached to
// the feed in setup()
void handleHigh(AdafruitIO_Data *data) {
  Serial.print("high received <- ");
  Serial.println(data->value());  // print the temperature data to the serial monitor

  int todaysHigh = data->toInt(); // store the incoming temperature data as an integer
  highmatrix.print(todaysHigh, DEC);  // send the temperature value to the display
  highmatrix.writeDisplay();          // light up display
  delay(500);                     // wait half a second
}

void handleLow(AdafruitIO_Data *data) {
  Serial.print("low received <- ");
  Serial.println(data->value());  // print the temperature data to the serial monitor

  int todaysLow = data->toInt(); // store the incoming temperature data as an integer
  lowmatrix.print(todaysLow, DEC);  // send the temperature value to the display
  lowmatrix.writeDisplay();          // light up display
  delay(500);                     // wait half a second
}

void handleSunrise(AdafruitIO_Data *data) {
  Serial.print("Sunrise received <- ");
  Serial.println(data->value());

  //int sunrise = data->toInt();
  delay(500);
}

void handleSunset(AdafruitIO_Data *data) {
  Serial.print("Sunset received <- ");
  Serial.println(data->value());
  if(!data->isTrue()) {  // For manually turning off the lights
    lightRange(0, PIXEL_COUNT, 0, true);
  }
  //int sunset = data->toInt();
  delay(500);
}

void handleCondition(AdafruitIO_Data *data) {
  lightPixels(pixels.Color(0, 0, 0, 0)); // start with a clean slate

  String forecast = data->toString(); // store the incoming weather data in a string
  int slashIndex = forecast.indexOf("/");
  Serial.print("New forecast: ");
  Serial.print(forecast);
  if (forecast.startsWith("AM")) {
    forecast.remove(0,3);
    Serial.println(" (AM stripped)");
  } else if (forecast.startsWith("PM")) {
    forecast.remove(0,3);
    Serial.println(" (PM stripped)");
  }  else {
    Serial.println();
  }

  if (slashIndex >= 0) {
    forecast.remove(slashIndex,forecast.length()-slashIndex);
    Serial.println("Dropped second half");
  }

  //the following strings store the varous IFTTT weather report words I've discovered so far
  String rain = String("Rain");
  String lightrain = String("Light Rain");
  String rainshower = String ("Rain Shower");
  String showers = String("Showers");
  String snow = String("Snow");
  String cloudy = String("Cloudy");
  String mostlycloudy = String("Mostly Cloudy");
  String partlycloudy = String("Partly Cloudy");
  String clearsky = String("Clear");
  String fair = String("Fair");
  String sunny = String("Sunny");
  String mostlysunny = String("Mostly Sunny");
  String rainandsnow = String("Rain and Snow");
  String snowshower = String("Snow Showers");

  // These if statements compare the incoming weather variable to the stored conditions, and control the NeoPixels accordingly.

  // if there's rain in the forecast:
  //   bottom 4 blue, middle 4 white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(rain) ||
      forecast.equalsIgnoreCase(lightrain) ||
      forecast.equalsIgnoreCase(rainshower) ||
      forecast.equalsIgnoreCase(showers)) {
    Serial.println("precipitation in the forecast today");
    lightRange(0, 3, pixels.Color(0, 30, 200, 0), false);
    lightRange(4, 7, pixels.Color(0, 0, 0, 255), false);
  }

  // if there's snow in the forecast
  //   botom four whiteish blue, middle 4 white (but don't draw them yet)
  if (forecast.equalsIgnoreCase(snow) ||
      forecast.equalsIgnoreCase(rainandsnow) ||
      forecast.equalsIgnoreCase(snowshower)){
    Serial.println("precipitation in the forecast today");
    lightRange(0, 3, pixels.Color(0, 30, 200, 20), false);
    lightRange(4, 7, pixels.Color(0, 0, 0, 255), false);
  }

  // if there's sun in the forecast
  //   top four pixels yellow (but don't draw them yet)
  if (forecast.equalsIgnoreCase(clearsky) ||
      forecast.equalsIgnoreCase(fair) ||
      forecast.equalsIgnoreCase(sunny) ||
      forecast.equalsIgnoreCase(mostlysunny)){
    Serial.println("some kind of sun in the forecast today");
    lightRange(8, 11, pixels.Color(255, 150, 0, 0), false);
  }

  // if there're clouds in the forecast
  //   middle four white, top four pixels yellow (but don't draw them yet)
  if (forecast.equalsIgnoreCase(cloudy) ||
      forecast.equalsIgnoreCase(mostlycloudy) ||
      forecast.equalsIgnoreCase(partlycloudy)){
    Serial.println("cloudy sky in the forecast today");
    lightRange(4, 7, pixels.Color(0, 0, 0, 255), false);
    lightRange(8, 11, pixels.Color(255, 150, 0, 0), false);
   }

   pixels.show(); // light up the pixels
}

// Function to set all the NeoPixels to the specified color.
void lightPixels(uint32_t color) {
  for (int i=0; i<PIXEL_COUNT; ++i) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

void lightRange(int start, int end, uint32_t color, bool show) {
  for (int i=start; i<=end; i++) {
    pixels.setPixelColor(i, color);
  }
  if (show) pixels.show();
}

void doNothing() {

}
