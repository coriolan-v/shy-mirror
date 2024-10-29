#include "Adafruit_VL53L0X.h"

#define NUMBER_OF_SENSORS 8

// address we will assign if dual sensor is present
// #define LOX1_ADDRESS 0x30
// #define LOX2_ADDRESS 0x31
// #define LOX3_ADDRESS 0x32
// #define LOX4_ADDRESS 0x33
// #define LOX5_ADDRESS 0x34
// #define LOX6_ADDRESS 0x35
// #define LOX7_ADDRESS 0x36
// #define LOX8_ADDRESS 0x37
int sensorAddresses[NUMBER_OF_SENSORS] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 };

//int sensor1, sensor2;

int sensorRanges[NUMBER_OF_SENSORS];

// set the pins to shutdown
// #define SHT_LOX1 1
// #define SHT_LOX2 2
// #define SHT_LOX3 3
// #define SHT_LOX4 4
// #define SHT_LOX5 5
// #define SHT_LOX6 6
// #define SHT_LOX7 7
// #define SHT_LOX8 8
int pins_SHT[NUMBER_OF_SENSORS] = { A1, A2, A3, A4, A5, SCK, MISO, MOSI };

// objects for the vl53l0x
// Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox2 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox3 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox4 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox5 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox6 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox7 = Adafruit_VL53L0X();
// Adafruit_VL53L0X lox8 = Adafruit_VL53L0X();

Adafruit_VL53L0X sensors[NUMBER_OF_SENSORS];

// this holds the measurement
// VL53L0X_RangingMeasurementData_t measure1;
// VL53L0X_RangingMeasurementData_t measure2;
VL53L0X_RangingMeasurementData_t measures[NUMBER_OF_SENSORS];

/*
    Reset all sensors by setting all of their XSHUT pins low for delay(10), then set all XSHUT high to bring out of reset
    Keep sensor #1 awake by keeping XSHUT pin high
    Put all other sensors into shutdown by pulling XSHUT pins low
    Initialize sensor #1 with lox.begin(new_i2c_address) Pick any number but 0x29 and it must be under 0x7F. Going with 0x30 to 0x3F is probably OK.
    Keep sensor #1 awake, and now bring sensor #2 out of reset by setting its XSHUT pin high.
    Initialize sensor #2 with lox.begin(new_i2c_address) Pick any number but 0x29 and whatever you set the first sensor to
 */
void setID() {

  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    sensors[i] = Adafruit_VL53L0X();
  }

  // all reset
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(pins_SHT[i], LOW);
  }
  delay(10);

  // digitalWrite(SHT_LOX1, LOW);
  // digitalWrite(SHT_LOX2, LOW);
  // delay(10);
  // all unreset
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(pins_SHT[i], HIGH);
  }
  delay(10);

  // digitalWrite(SHT_LOX1, HIGH);
  // digitalWrite(SHT_LOX2, HIGH);
  // delay(10);

  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(pins_SHT[i], HIGH);
  }

  // activating LOX1 and reseting LOX2
  digitalWrite(pins_SHT[0], HIGH);
  for(int i = 1; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX2, LOW);
  // digitalWrite(SHT_LOX3, LOW);
  // digitalWrite(SHT_LOX4, LOW);
  // digitalWrite(SHT_LOX5, LOW);
  // digitalWrite(SHT_LOX6, LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[0].begin(sensorAddresses[0])) {
    Serial.println(F("Failed to boot first VL53L0X"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 1");
  }
  delay(10);

  digitalWrite(pins_SHT[1], HIGH);
  for(int i = 2; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX3, LOW);
  // digitalWrite(SHT_LOX4, LOW);
  // digitalWrite(SHT_LOX5, LOW);
  // digitalWrite(SHT_LOX6, LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[1].begin(sensorAddresses[1])) {
    Serial.println(F("Failed to boot VL53L0X 2"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 2");
  }
  delay(10);

  digitalWrite(pins_SHT[2], HIGH);
  for(int i = 3; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX4, LOW);
  // digitalWrite(SHT_LOX5, LOW);
  // digitalWrite(SHT_LOX6, LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[2].begin(sensorAddresses[2])) {
    Serial.println(F("Failed to boot VL53L0X 3"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 3");
  }
  delay(10);

  digitalWrite(pins_SHT[3], HIGH);
  for(int i = 4; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX5, LOW);
  // digitalWrite(SHT_LOX6, LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[3].begin(sensorAddresses[3])) {
    Serial.println(F("Failed to boot VL53L0X 4"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 4");
  }
  delay(10);

  digitalWrite(pins_SHT[4], HIGH);
  for(int i = 5; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX6, LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[4].begin(sensorAddresses[4])) {
    Serial.println(F("Failed to boot VL53L0X 5"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 5");
  }
  delay(10);

  digitalWrite(pins_SHT[5], HIGH);
  for(int i = 6; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
  // digitalWrite(SHT_LOX7, LOW);
  // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[5].begin(sensorAddresses[5])) {
    Serial.println(F("Failed to boot VL53L0X 6"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 6");
  }
  delay(10);

  digitalWrite(pins_SHT[6], HIGH);
  for(int i = 7; i < NUMBER_OF_SENSORS; i++) digitalWrite(pins_SHT[i], LOW);
 // digitalWrite(SHT_LOX8, LOW);
  // initing LOX1
  if (!sensors[6].begin(sensorAddresses[6])) {
    Serial.println(F("Failed to boot VL53L0X 7"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 7");
  }
  delay(10);

  digitalWrite(pins_SHT[7], HIGH);
  // initing LOX1
  if (!sensors[7].begin(sensorAddresses[7])) {
    Serial.println(F("Failed to boot VL53L0X 8"));
    while (1)
      ;
  } else {
    Serial.println("Init OK sensor 8");
  }
  delay(10);
}

void readSensors() {
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    sensors[i].rangingTest(&measures[i], false);
  }

  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    Serial.print(i);
    Serial.print(": ");
    if (measures[i].RangeStatus != 4) {  // if not out of range
      sensorRanges[i] = measures[i].RangeMilliMeter;
      Serial.print(sensorRanges[i]);
      Serial.println("mm");
    }
  }
}

// void read_dual_sensors() {

//   lox1.rangingTest(&measure1, false);  // pass in 'true' to get debug data printout!
//   lox2.rangingTest(&measure2, false);  // pass in 'true' to get debug data printout!

//   // print sensor one reading
//   Serial.print("1: ");
//   if (measure1.RangeStatus != 4) {  // if not out of range
//     sensor1 = measure1.RangeMilliMeter;
//     Serial.print(sensor1);
//     Serial.print("mm");
//   } else {
//     Serial.print("Out of range");
//   }

//   Serial.print(" ");

//   // print sensor two reading
//   Serial.print("2: ");
//   if (measure2.RangeStatus != 4) {
//     sensor2 = measure2.RangeMilliMeter;
//     Serial.print(sensor2);
//     Serial.print("mm");
//   } else {
//     Serial.print("Out of range");
//   }

//   Serial.println();
// }

void initSensors() {
  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    pinMode(pins_SHT[i], OUTPUT);
  }

  Serial.println("Shutdown pins inited...");

  for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
    digitalWrite(pins_SHT[i], LOW);
  }

  Serial.println("All in reset mode...(pins are low)");


  Serial.println("Starting...");

  setID();


  // pinMode(SHT_LOX1, OUTPUT);
  // pinMode(SHT_LOX2, OUTPUT);

  // Serial.println("Shutdown pins inited...");

  // digitalWrite(SHT_LOX1, LOW);
  // digitalWrite(SHT_LOX2, LOW);

  //   Serial.println("Both in reset mode...(pins are low)");


  //   Serial.println("Starting...");
  //   setID();
}
