#include "Config.h"
#include "Firmware.h"
#include <Wire.h>
#include <VL53L1X.h>

const uint8_t sensorCount = physicalZoneCount;
const uint8_t xshutPins[sensorCount] = { A0, A1, A2, A3, 9, 10, 11, 12 };
const char * const pinNames[sensorCount] = { "A0", "A1", "A2", "A3", "9", "10", "11", "12" };
const uint32_t sampleMaxAgeMs = sensorStaleMs;
VL53L1X sensors[sensorCount];
bool sensorPresent[sensorCount] = {};
bool sensorValid[sensorCount] = {};
bool sensorReadHealthy[sensorCount] = {};
bool sensorHasSample[sensorCount] = {};
bool meanInitialized[sensorCount] = {};
uint32_t sampleAt[sensorCount] = {};
uint16_t sensorReadRaw[sensorCount] = {};
int sensorReadMean[sensorCount] = {};
int lowerTreshold[sensorCount] = {1000,1000,1000,1000,1000,1000,1000,1000};
bool sensorDisabled(uint8_t id) {
  return (disabledSensorMask & (1u << id)) != 0;
}

void initSensors() {
  Wire.begin();
  Wire.setClock(400000);
  for (uint8_t i = 0; i < sensorCount; ++i) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }
  delay(10);
  for (uint8_t i = 0; i < sensorCount; ++i) {
    if (sensorDisabled(i)) continue; // Keep disabled sensors in reset.
    pinMode(xshutPins[i], INPUT); // Release XSHUT; do not drive it high.
    delay(10);
    sensors[i].setTimeout(100);
    if (sensors[i].init()) {
      sensors[i].setAddress(0x2A + i);
      Wire.beginTransmission(0x2A + i);
      if (Wire.endTransmission() == 0 &&
          sensors[i].setDistanceMode(VL53L1X::Short) &&
          sensors[i].setMeasurementTimingBudget(sensorTimingBudgetUs)) {
        sensors[i].startContinuous(sensorPeriodMs);
        sensorPresent[i] = sensors[i].last_status == 0;
      }
    }
    if (!sensorPresent[i]) {
      // Isolate failed devices to avoid address collisions at 0x29.
      pinMode(xshutPins[i], OUTPUT);
      digitalWrite(xshutPins[i], LOW);
      Serial.print("Sensor init FAIL, hardware ID "); Serial.println(i);
    }
  }
}

void readSensors() {
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 2) return;
  lastPoll = millis();
  for (uint8_t i = 0; i < sensorCount; ++i) {
    if (sensorDisabled(i) || !sensorPresent[i]) continue;
    bool ready = sensors[i].dataReady();
    if (sensors[i].last_status != 0) {
      sensorValid[i] = false;
      sensorReadHealthy[i] = false;
      meanInitialized[i] = false;
      continue;
    }
    if (!ready) continue;
    bool wasFresh = sensorHasSample[i] && millis() - sampleAt[i] <= sampleMaxAgeMs;
    sensorReadRaw[i] = sensors[i].read(false);
    sensorHasSample[i] = true;
    sampleAt[i] = millis();
    sensorReadHealthy[i] = !sensors[i].timeoutOccurred() && sensors[i].last_status == 0;
    sensorValid[i] = sensorReadHealthy[i] &&
      sensors[i].ranging_data.range_status == VL53L1X::RangeValid;
    if (sensorValid[i]) {
      if (!meanInitialized[i] || !wasFresh) sensorReadMean[i] = sensorReadRaw[i];
      else sensorReadMean[i] += (int(sensorReadRaw[i]) - sensorReadMean[i]) / sensorSmoothingDivisor;
      meanInitialized[i] = true;
    } else meanInitialized[i] = false;
  }
}

bool sensorFresh(uint8_t i) {
  return sensorPresent[i] && sensorHasSample[i] && millis() - sampleAt[i] <= sampleMaxAgeMs;
}

bool sensorNoTarget(uint8_t id) {
  return sensorFresh(id) && sensorReadHealthy[id] &&
    sensors[id].ranging_data.range_status == VL53L1X::SignalFail;
}

void testSensors() {
  Serial.println("Sensor check: NO TARGET is normal in open space; use a card to verify distance.");
  for (uint8_t slot = 0; slot < sensorCount; ++slot) {
    uint8_t i = sensorHardwareIdByPosition[slot];
    Serial.print("Sensor "); Serial.print(slot + 1); Serial.print(" / ");
    Serial.print("ID "); Serial.print(i); Serial.print(" XSHUT "); Serial.print(pinNames[i]);
    Serial.print(" addr 0x"); Serial.print(0x2A + i, HEX);
    if (sensorDisabled(i)) {
      Serial.println(" DISABLED in Config.h");
      continue;
    }
    Wire.beginTransmission(0x2A + i);
    bool ack = Wire.endTransmission() == 0;
    if (!sensorPresent[i] || !ack) Serial.println(" FAIL: missing/init/I2C");
    else if (!sensorFresh(i)) Serial.println(" FAIL: no fresh reading");
    else if (!sensorReadHealthy[i]) Serial.println(" FAIL: I2C/read error");
    else if (sensorNoTarget(i)) Serial.println(" OK: NO TARGET (open space / weak return)");
    else {
      Serial.print(sensorValid[i] ? " PASS: " : " CHECK TARGET: ");
      Serial.print(sensorReadRaw[i]); Serial.print(" mm, ");
      Serial.println(VL53L1X::rangeStatusToString(sensors[i].ranging_data.range_status));
    }
  }
  Serial.print("Hall input (LOW=active): "); Serial.println(readHallSensor());
}

void printSensorOrder() {
  for (uint8_t slot = 0; slot < sensorCount; ++slot) {
    uint8_t id = sensorHardwareIdByPosition[slot];
    Serial.print("Physical position "); Serial.print(slot + 1);
    Serial.print(" -> ID "); Serial.print(id);
    if (sensorDisabled(id)) Serial.print(" DISABLED");
    Serial.print(" / XSHUT "); Serial.print(pinNames[id]);
    Serial.print(" / I2C 0x"); Serial.println(0x2A + id, HEX);
  }
}

void detectPeopleZones() {
  static uint32_t lastCheck = 0;
  static uint32_t candidateAt = 0;
  static uint8_t personCandidate = 0;
  if (!motorHomed()) { personCandidate = 0; return; }
  if (millis() - lastCheck < 10) return;
  lastCheck = millis();
  uint8_t nearest = 0;
  int nearestMm = 32767;
  for (uint8_t slot = 0; slot < sensorCount; ++slot) {
    uint8_t id = sensorHardwareIdByPosition[slot];
    if (sensorDisabled(id) || sensorNoTarget(id) || !sensorFresh(id) || !sensorValid[id]) continue;
    if (sensorReadMean[id] < lowerTreshold[slot] && sensorReadMean[id] < nearestMm) {
      nearest = slot + 1;
      nearestMm = sensorReadMean[id];
    }
  }
  // Any close valid reading resets inactivity, even before zone confirmation.
  updatePersonPresence(nearest != 0);
  if (nearest == 0) { personCandidate = 0; return; }
  if (nearest != personCandidate) {
    personCandidate = nearest;
    candidateAt = millis();
    return;
  }
  if (millis() - candidateAt < personConfirmMs) return; // Reject passing noise and unstable targets.
  moveOppositePosition(nearest, nearestMm);
}
