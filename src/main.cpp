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

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include "Adafruit_PWMServoDriver.h"
#include <adaptive_agc.h>

BH1750 lightMeter1;
BH1750 lightMeter2;
BH1750 lightMeter3;
// BH1750 lightMeter4;

// Timing infrastructure for time-based parameters
unsigned long last_loop_time = 0;
float loop_time_ms = 0;
float avg_loop_time_ms = 0;  // Running average of actual loop time

// Time-based parameter configuration (in milliseconds)
// These define the response characteristics of the adaptive system
// Calibrated based on measured loop time of 1.75ms
#define FAST_RESPONSE_TIME_MS  26.25   // How quickly to track flicker changes (15 samples @ 1.75ms/sample)
#define SLOW_RESPONSE_TIME_MS  875.0   // How slowly to track ambient/baseline (500 samples @ 1.75ms/sample)
#define TREND_RESPONSE_TIME_MS 525.0   // Smoothing for rate-of-change detection (100 samples @ 1.75ms/sample)

// Convert time-based parameters to sample counts (calculated in setup)
long FAST_WINDOW;
long SLOW_WINDOW;
long TREND_WINDOW;

#define TREND_SENSE 0.5      // Sensitivity multiplier for trend detection
#define SETTLED_WINDOW 0.1   // Threshold for "settled" state
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

// Adaptive AGC instances - one per channel
AdaptiveAGC *agc1;
AdaptiveAGC *agc2;
AdaptiveAGC *agc3;

#define REPORT_RATE 50
int report = REPORT_RATE;

Adafruit_PWMServoDriver PCA9685 = Adafruit_PWMServoDriver(0x40, Wire);

#define I2C_SENSOR1 2
#define I2C_SENSOR2 3
#define I2C_SENSOR3 4

void TCA9548A(uint8_t bus){
    Wire.beginTransmission(0x70);  // TCA9548A address is 0x70
    Wire.write(1 << bus);          // send byte to select bus
    Wire.endTransmission();
    // Serial.print(bus);
}

void setup() {
  //Serial.begin(115200);

  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin();
  // On esp8266 you can select SCL and SDA pins using Wire.begin(D4, D3);
  // For Wemos / Lolin D1 Mini Pro and the Ambient Light shield use
  // Wire.begin(D2, D1);

  PCA9685.begin();
  PCA9685.setPWMFreq(PWM_FREQ); //1600);  // This is the maximum PWM frequency and suited to LED's
  // PCA9685.setOutputMode(true);

  TCA9548A(I2C_SENSOR1);
  lightMeter1.begin();
  float lux1 = lightMeter1.readLightLevel();

  TCA9548A(I2C_SENSOR2);
  lightMeter2.begin();
  float lux2 = lightMeter1.readLightLevel();

  TCA9548A(I2C_SENSOR3);
  lightMeter3.begin();
  float lux3 = lightMeter3.readLightLevel();

  // TCA9548A(3);
  // lightMeter4.begin();
  // float lux4 = lightMeter4.readLightLevel();

  // Calibrate loop timing by running several iterations
  Serial.println(F("Calibrating loop timing..."));
  unsigned long cal_start = millis();
  for(int i = 0; i < 20; i++) {
    TCA9548A(I2C_SENSOR1); lightMeter1.readLightLevel();
    TCA9548A(I2C_SENSOR2); lightMeter2.readLightLevel();
    TCA9548A(I2C_SENSOR3); lightMeter3.readLightLevel();
  }
  unsigned long cal_time = millis() - cal_start;
  avg_loop_time_ms = cal_time / 20.0;
  
  // Calculate window sizes in samples based on desired time and measured loop speed
  FAST_WINDOW = max(1L, (long)(FAST_RESPONSE_TIME_MS / avg_loop_time_ms));
  SLOW_WINDOW = max(1L, (long)(SLOW_RESPONSE_TIME_MS / avg_loop_time_ms));
  TREND_WINDOW = max(1L, (long)(TREND_RESPONSE_TIME_MS / avg_loop_time_ms));
  
  Serial.print(F("Loop time: "));
  Serial.print(avg_loop_time_ms);
  Serial.println(F(" ms"));
  Serial.print(F("Fast window: "));
  Serial.print(FAST_WINDOW);
  Serial.print(F(" samples ("));
  Serial.print(FAST_RESPONSE_TIME_MS);
  Serial.println(F(" ms)"));
  Serial.print(F("Slow window: "));
  Serial.print(SLOW_WINDOW);
  Serial.print(F(" samples ("));
  Serial.print(SLOW_RESPONSE_TIME_MS);
  Serial.println(F(" ms)"));
  Serial.print(F("Trend window: "));
  Serial.print(TREND_WINDOW);
  Serial.print(F(" samples ("));
  Serial.print(TREND_RESPONSE_TIME_MS);
  Serial.println(F(" ms)"));

  // Initialize adaptive AGC instances
  agc1 = new AdaptiveAGC(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW, 
                         TREND_SENSE, SETTLED_WINDOW, lux1,
                         RANGE_EXPANSION, PWM_MAX, BASE);
  
  agc2 = new AdaptiveAGC(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW,
                         TREND_SENSE, SETTLED_WINDOW, lux2,
                         RANGE_EXPANSION, PWM_MAX, BASE);
  
  agc3 = new AdaptiveAGC(FAST_WINDOW, SLOW_WINDOW, TREND_WINDOW,
                         TREND_SENSE, SETTLED_WINDOW, lux3,
                         RANGE_EXPANSION, PWM_MAX, BASE);

  pinMode(LED_LEVEL_1, OUTPUT);
  pinMode(LED_SETTLED_1, OUTPUT);
  pinMode(LED_LEVEL_2, OUTPUT);
  pinMode(LED_SETTLED_2, OUTPUT);
  pinMode(LED_LEVEL_3, OUTPUT);
  pinMode(LED_SETTLED_3, OUTPUT);
}

void loop() {
  // Measure actual loop timing
  unsigned long current_time = millis();
  if(last_loop_time > 0) {
    loop_time_ms = current_time - last_loop_time;
    // Update running average (slow smoothing)
    avg_loop_time_ms = avg_loop_time_ms * 0.99 + loop_time_ms * 0.01;
  }
  last_loop_time = current_time;

  // Read sensors
  TCA9548A(I2C_SENSOR1);
  float lux1 = lightMeter1.readLightLevel();

  TCA9548A(I2C_SENSOR2);
  float lux2 = lightMeter2.readLightLevel();

  TCA9548A(I2C_SENSOR3);
  float lux3 = lightMeter3.readLightLevel();

  // Process through AGC and output to PWM
  float pwm1 = agc1->process(lux1);
  PCA9685.setPWM(0, 0, pwm1);

  float pwm2 = agc2->process(lux2);
  PCA9685.setPWM(1, 0, pwm2);

  float pwm3 = agc3->process(lux3);
  PCA9685.setPWM(2, 0, pwm3);

  digitalWrite(LED_SETTLED_1, agc1->isSettled() ? HIGH : LOW);
  digitalWrite(LED_SETTLED_2, agc2->isSettled() ? HIGH : LOW);
  digitalWrite(LED_SETTLED_3, agc3->isSettled() ? HIGH : LOW);

  if(!--report){
    // Update settled indicator LEDs
    // digitalWrite(LED_SETTLED_1, agc1->isSettled() ? HIGH : LOW);
    // digitalWrite(LED_SETTLED_2, agc2->isSettled() ? HIGH : LOW);
    // digitalWrite(LED_SETTLED_3, agc3->isSettled() ? HIGH : LOW);

    // Report status
    Serial.print(F("Loop:"));
    Serial.print(avg_loop_time_ms, 1);
    Serial.print(F("ms R1:"));
    Serial.print(agc1->getRange(), 1);
    Serial.print(F(" R2:"));
    Serial.print(agc2->getRange(), 1);
    Serial.print(F(" R3:"));
    Serial.print(agc3->getRange(), 1);
    Serial.print(F(" PWM:"));
    Serial.print(agc1->getScaledValue(), 0);
    Serial.print(F(","));
    Serial.print(agc2->getScaledValue(), 0);
    Serial.print(F(","));
    Serial.println(agc3->getScaledValue(), 0);

    report = REPORT_RATE;
  }
}
