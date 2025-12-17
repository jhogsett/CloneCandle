// CAREFUL CHANGING THESE PARAMETERS, THIS SEEMS TO BE WORKING VERY WELL WITH A TEA-LIGHT

/*

Example of BH1750 library usage.

This example initialises the BH1750 object using the default high resolution
continuous mode and then makes a light level reading every second.

Connections

  - VCC to 3V3 or 5V
  - GND to GND
  - SCL to SCL (A5 on Arduino Uno, Leonardo, etc or 21 on Mega and Due, on
    esp8266 free selectable)
  - SDA to SDA (A4 on Arduino Uno, Leonardo, etc or 20 on Mega and Due, on
    esp8266 free selectable)
  - ADD to (not connected) or GND

ADD pin is used to set sensor I2C address. If it has voltage greater or equal
to 0.7VCC voltage (e.g. you've connected it to VCC) the sensor address will be
0x5C. In other case (if ADD voltage less than 0.7 * VCC) the sensor address
will be 0x23 (by default).

*/

#include <Wire.h>
#include <BH1750.h>
#include "Adafruit_PWMServoDriver.h"
#include <trend_detector.h>

BH1750 lightMeter1;
BH1750 lightMeter2;
BH1750 lightMeter3;
// BH1750 lightMeter4;


#define FAST_WINDOW 15
#define SLOW_WINDOW 500
#define TREND_WINDOW 100
#define TREND_SENSE 0.25
#define SETTLED_WINDOW 0.25
// #define TREND_BOOST 2

// Nov 22, 2011 — PWM: 3, 5, 6, 9, 10, and 11. Provide 8-bit PWM output with the analogWrite() function. However, pin 3 is Reset.Read more

#define LED_LEVEL_1   9
#define LED_SETTLED_1 6
#define LED_LEVEL_2   10
#define LED_SETTLED_2 7
#define LED_LEVEL_3   11
#define LED_SETTLED_3 8

#define BASE 1
// #define BOOST 1

#define PWM_FREQ 96

#define PWM_MAX 4095.0 // 256.0
#define RANGE_EXPANSION 256.0 // Brightness

TrendDetector *trend_detector1_main;
TrendDetector *trend_detector1_high;
TrendDetector *trend_detector1_low;
TrendDetector *trend_detector2_main;
TrendDetector *trend_detector2_high;
TrendDetector *trend_detector2_low;
TrendDetector *trend_detector3_main;
TrendDetector *trend_detector3_high;
TrendDetector *trend_detector3_low;

#define REPORT_RATE 50
int report = REPORT_RATE;

Adafruit_PWMServoDriver PCA9685 = Adafruit_PWMServoDriver(0x40, Wire);

void TCA9548A(uint8_t bus){
    Wire.beginTransmission(0x70);  // TCA9548A address is 0x70
    Wire.write(1 << bus);          // send byte to select bus
    Wire.endTransmission();
    // Serial.print(bus);
}

void setup() {
  // Serial.begin(115200);

  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin();
  // On esp8266 you can select SCL and SDA pins using Wire.begin(D4, D3);
  // For Wemos / Lolin D1 Mini Pro and the Ambient Light shield use
  // Wire.begin(D2, D1);

  PCA9685.begin();
  PCA9685.setPWMFreq(PWM_FREQ); //1600);  // This is the maximum PWM frequency and suited to LED's
  // PCA9685.setOutputMode(true);

  TCA9548A(0);
  lightMeter1.begin();
  float lux1 = lightMeter1.readLightLevel();

  TCA9548A(1);
  lightMeter2.begin();
  float lux2 = lightMeter1.readLightLevel();

  TCA9548A(2);
  lightMeter3.begin();
  float lux3 = lightMeter3.readLightLevel();

  // TCA9548A(3);
  // lightMeter4.begin();
  // float lux4 = lightMeter4.readLightLevel();

  // Serial.println(F("BH1750 Test begin"));

  trend_detector1_main = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux1);
  trend_detector1_high = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux1);
  trend_detector1_low = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux1);

  trend_detector2_main = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux2);
  trend_detector2_high = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux2);
  trend_detector2_low = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux2);

  trend_detector3_main = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux3);
  trend_detector3_high = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux3);
  trend_detector3_low = new TrendDetector(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, TREND_SENSE, SETTLED_WINDOW, lux3);

  pinMode(LED_LEVEL_1, OUTPUT);
  pinMode(LED_SETTLED_1, OUTPUT);
  pinMode(LED_LEVEL_2, OUTPUT);
  pinMode(LED_SETTLED_2, OUTPUT);
  pinMode(LED_LEVEL_3, OUTPUT);
  pinMode(LED_SETTLED_3, OUTPUT);
}

void loop() {
  // Serial.println("--------");

  TCA9548A(0);
  float lux1 = lightMeter1.readLightLevel();
  // Serial.println(lux1);

  TCA9548A(1);
  float lux2 = lightMeter2.readLightLevel();
  // Serial.println(lux2);

  TCA9548A(2);
  float lux3 = lightMeter3.readLightLevel();
  // Serial.println(lux3);

  // TCA9548A(3);
  // float lux4 = lightMeter4.readLightLevel();
  // Serial.println(lux4);

  // delay(1000);
  // Serial.print(lux1);
  // Serial.print(",");
  // Serial.print(lux2);
  // Serial.print(",");
  // Serial.println(lux3);

  trend_detector1_main->sample(lux1);
  // float mean1 = trend_detector1_main->slow_mean();
  float mean1 = trend_detector1_main->fast_mean();

  if(lux1 > mean1){
    trend_detector1_high->sample(lux1);
  } else {
    trend_detector1_low->sample(lux1);
  }


  float range1 = trend_detector1_high->slow_mean() - trend_detector1_low->slow_mean();
  float base1 = trend_detector1_low->slow_mean();
  float val1 = trend_detector1_main->fast_mean() - base1;
  // float val1 = lux1 - base1;
  float percent1 = val1 / range1;

  float ival1 = val1 * RANGE_EXPANSION;
  int i1 = int(ival1);
  if(i1 < 0)
    i1 = 0;
  else if(i1 > PWM_MAX - BASE)
    i1 = PWM_MAX - BASE;
  PCA9685.setPWM(0, 0, BASE + i1);









  trend_detector2_main->sample(lux2);
  // float mean2 = trend_detector2_main->slow_mean();
  float mean2 = trend_detector2_main->fast_mean();

  if(lux2 > mean2){
    trend_detector2_high->sample(lux2);
  } else {
    trend_detector2_low->sample(lux2);
  }


  float range2 = trend_detector2_high->slow_mean() - trend_detector2_low->slow_mean();
  float base2 = trend_detector2_low->slow_mean();
  float val2 = trend_detector2_main->fast_mean() - base2;
  // float val2 = lux2 - base2;
  float percent2 = val2 / range2;

  float ival2 = val2 * RANGE_EXPANSION;
  int i2 = int(ival2);
  if(i2 < 0)
    i2 = 0;
  else if(i2 > PWM_MAX - BASE)
    i2 = PWM_MAX - BASE;
  PCA9685.setPWM(1, 0, BASE + i2);


  trend_detector3_main->sample(lux3);
  // float mean3 = trend_detector3_main->slow_mean();
  float mean3 = trend_detector3_main->fast_mean();

  if(lux3 > mean3){
    trend_detector3_high->sample(lux3);
  } else {
    trend_detector3_low->sample(lux3);
  }


  float range3 = trend_detector3_high->slow_mean() - trend_detector3_low->slow_mean();
  float base3 = trend_detector3_low->slow_mean();
  float val3 = trend_detector3_main->fast_mean() - base3;
  // float val3 = lux3 - base3;
  float percent3 = val3 / range3;

  float ival3 = val3 * RANGE_EXPANSION;
  int i3 = int(ival3);
  if(i3 < 0)
    i3 = 0;
  else if(i3 > PWM_MAX - BASE)
    i3 = PWM_MAX - BASE;
  PCA9685.setPWM(2, 0, BASE + i3);



  if(!--report){
    digitalWrite(LED_SETTLED_1, trend_detector1_main->settled() ? HIGH : LOW);
    digitalWrite(LED_SETTLED_2, trend_detector2_main->settled() ? HIGH : LOW);
    digitalWrite(LED_SETTLED_3, trend_detector3_main->settled() ? HIGH : LOW);

    Serial.print(range1);
    Serial.print(",");
    Serial.print(range2);
    Serial.print(",");
    Serial.print(range3);
    Serial.print(",");
    Serial.print(ival1);
    Serial.print(",");
    Serial.print(ival2);
    Serial.print(",");
    Serial.println(ival3);

    report = REPORT_RATE;
  }
}
