#include "SensorMap.h"
#include "MappingRelease.h"
#include "Config.h"
#include <string.h>
#include "Firmware.h"
#include <Wire.h>
#include <VL53L1X.h>
#include <FlashStorage.h>

const uint8_t sensorCount = 8;
const uint8_t xshutPins[sensorCount] = { A0, A1, A2, A3, 9, 10, 11, 12 };
const char * const pinNames[sensorCount] = { "A0", "A1", "A2", "A3", "9", "10", "11", "12" };
const uint16_t handNearMm = 200;
const uint16_t handReleaseMm = 350;
const uint32_t sampleMaxAgeMs = 250;
const uint32_t holdMs = 400;
const uint32_t mappingTimeoutMs = 60000;
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
uint8_t zoneToSensor[sensorCount] = {0,1,2,3,4,5,6,7};
FlashStorage(sensorMapStorage, SensorMapRecord);
bool mapping = false;
bool mapReady = false;
bool waitingForRelease = true;
uint8_t pendingOrder[sensorCount];
uint8_t mappedCount = 0;
int8_t candidate = -1;
uint32_t candidateSince = 0;
uint32_t mappingProgressAt = 0;
uint32_t lastMappingNotice = 0;

bool sensorDisabled(uint8_t id) {
  return (disabledSensorMask & (1u << id)) != 0;
}

uint8_t enabledSensorCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < sensorCount; ++i) if (!sensorDisabled(i)) ++count;
  return count;
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
          sensors[i].setMeasurementTimingBudget(50000)) {
        sensors[i].startContinuous(60);
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
  if (millis() - lastPoll < 10) return;
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
      else sensorReadMean[i] += (int(sensorReadRaw[i]) - sensorReadMean[i]) / 10;
      meanInitialized[i] = true;
    } else meanInitialized[i] = false;
  }
}

bool sensorFresh(uint8_t i) {
  return sensorPresent[i] && sensorHasSample[i] && millis() - sampleAt[i] <= sampleMaxAgeMs;
}

bool sensorNoTarget(uint8_t id) {
  return noTargetReading(sensorFresh(id) && sensorReadHealthy[id],
    sensors[id].ranging_data.range_status == VL53L1X::SignalFail);
}

void testSensors() {
  Serial.println("Sensor check: NO TARGET is normal in open space; use a card to verify distance.");
  for (uint8_t i = 0; i < sensorCount; ++i) {
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

void loadSensorMap() {
  SensorMapRecord record = sensorMapStorage.read();
  if (validSensorMap(record)) {
    memcpy(zoneToSensor, record.order, sensorCount);
    mapReady = true;
    Serial.println("Loaded saved sensor order.");
  } else Serial.println("No saved order; using wiring order. Type map to configure.");
}

void printSensorMap() {
  for (uint8_t slot = 0; slot < sensorCount; ++slot) {
    uint8_t id = zoneToSensor[slot];
    Serial.print("Physical position "); Serial.print(slot + 1);
    Serial.print(" -> ID "); Serial.print(id);
    if (sensorDisabled(id)) Serial.print(" DISABLED");
    Serial.print(" / XSHUT "); Serial.print(pinNames[id]);
    Serial.print(" / I2C 0x"); Serial.println(0x2A + id, HEX);
  }
}

void finishSensorMapping() {
  if (mappedCount != sensorCount) return;
  SensorMapRecord record = {};
  record.magic = sensorMapMagic;
  memcpy(record.order, pendingOrder, sensorCount);
  record.checksum = sensorMapChecksum(record);
  sensorMapStorage.write(record);
  SensorMapRecord saved = sensorMapStorage.read();
  if (validSensorMap(saved) && !memcmp(saved.order, pendingOrder, sensorCount)) {
    memcpy(zoneToSensor, pendingOrder, sensorCount);
    mapReady = true;
    Serial.println("Eight physical positions saved. Send home to enable automatic movement.");
    printSensorMap();
  } else Serial.println("Flash verification FAILED; previous map retained.");
  mapping = false;
}

void skipMappingPosition() {
  if (!mapping) { Serial.println("skip is only available during map."); return; }
  for (uint8_t id = 0; id < sensorCount; ++id) {
    if (!sensorDisabled(id)) continue;
    bool used = false;
    for (uint8_t slot = 0; slot < mappedCount; ++slot)
      if (pendingOrder[slot] == id) used = true;
    if (used) continue;
    pendingOrder[mappedCount++] = id;
    Serial.print("Skipped physical position "); Serial.println(mappedCount);
    candidate = -1;
    waitingForRelease = true;
    mappingProgressAt = millis();
    lastMappingNotice = millis();
    finishSensorMapping();
    return;
  }
  Serial.println("No disabled sensors left to assign; capture this position with your hand.");
}

bool sensorMappingActive() { return mapping; }

void startSensorMapping() {
  for (uint8_t i = 0; i < sensorCount; ++i) {
    if (sensorDisabled(i)) continue;
    if (!sensorFresh(i) || !sensorReadHealthy[i]) {
      Serial.println("Cannot map: all enabled sensors need fresh, healthy communication. Type sensors.");
      return;
    }
  }
  stopMotor();
  mapping = true;
  mappedCount = 0;
  candidate = -1;
  waitingForRelease = true;
  mappingProgressAt = millis();
  lastMappingNotice = millis();
  Serial.print("SETUP: remove hands, then cover enabled sensors in desired order. Count: ");
  Serial.println(enabledSensorCount());
  Serial.println("Hold hand 5-20 cm away for 0.4 s. After capture, remove your hand and wait for Ready.");
  Serial.println("Map physical positions 1-8 in order. Send skip at each missing/disabled position.");
  Serial.println("Open space / NO TARGET is OK. A valid close reading is needed only for capture.");
  Serial.println("One sensor at a time. cancel keeps the previous order.");
}

void cancelSensorMapping() {
  if (mapping) Serial.println("Setup cancelled; previous order retained.");
  mapping = false;
}

void updateSensorMapping() {
  if (!mapping) return;
  uint32_t now = millis();
  if (now - mappingProgressAt > mappingTimeoutMs) {
    Serial.println("Setup timed out.");
    cancelSensorMapping();
    return;
  }
  int8_t nearId = -1;
  uint8_t nearCount = 0;
  bool allReleaseClear = true;
  bool anyNoTarget = false;
  for (uint8_t i = 0; i < sensorCount; ++i) {
    if (sensorDisabled(i)) continue;
    if (!sensorFresh(i) || !sensorReadHealthy[i]) {
      Serial.print("Setup aborted: stale data or communication error, ID "); Serial.println(i);
      cancelSensorMapping();
      return;
    }
    bool noTarget = sensorNoTarget(i);
    anyNoTarget = anyNoTarget || noTarget;
    allReleaseClear = allReleaseClear && mappingReleaseClear(
      sensorFresh(i) && sensorReadHealthy[i], sensorValid[i],
      sensorReadRaw[i], noTarget, handReleaseMm);
    if (sensorValid[i] && sensorReadRaw[i] < (waitingForRelease ? handReleaseMm : handNearMm)) {
      nearId = i;
      ++nearCount;
    }
  }
  if (waitingForRelease) {
    // Empty space is acceptable for every enabled sensor, including the one
    // just captured. Never interpret SignalFail's raw millimetres as distance.
    bool released = allReleaseClear;
    uint32_t releaseHold = anyNoTarget ? 1000 : holdMs;
    if (!released) {
      candidate = -1;
      if (now - lastMappingNotice >= 2000) {
        lastMappingNotice = now;
        Serial.println("Waiting for hand removal / clear space:");
        for (uint8_t id = 0; id < sensorCount; ++id) {
          if (sensorDisabled(id) || sensorNoTarget(id)) continue;
          if (sensorValid[id]) {
            if (sensorReadRaw[id] >= handReleaseMm) continue;
            Serial.print("  ID "); Serial.print(id); Serial.print(" sees ");
            Serial.print(sensorReadRaw[id]); Serial.println(" mm (need >=350 mm).");
          } else {
            Serial.print("  ID "); Serial.print(id); Serial.print(": ");
            Serial.println(VL53L1X::rangeStatusToString(sensors[id].ranging_data.range_status));
            Serial.println("  Hold a card 40-60 cm away to check this range status.");
          }
        }
      }
      return;
    }
    if (candidate != -2) { candidate = -2; candidateSince = now; }
    if (now - candidateSince < releaseHold) return;
    waitingForRelease = false;
    candidate = -1;
    Serial.print("Ready for physical position "); Serial.println(mappedCount + 1);
    return;
  }
  if (nearCount != 1) { candidate = -1; return; }
  for (uint8_t zone = 0; zone < mappedCount; ++zone) {
    if (pendingOrder[zone] == nearId) { candidate = -1; return; }
  }
  if (candidate != nearId) { candidate = nearId; candidateSince = now; }
  if (now - candidateSince < holdMs) return;
  pendingOrder[mappedCount++] = nearId;
  Serial.print("Captured physical position "); Serial.print(mappedCount);
  Serial.print(" = ID "); Serial.print(nearId);
  Serial.print(" (XSHUT "); Serial.print(pinNames[nearId]); Serial.println("). Remove hand.");
  mappingProgressAt = now;
  lastMappingNotice = now;
  candidate = -1;
  waitingForRelease = true;
  finishSensorMapping();
}

void detectPeopleZones() {
  static uint32_t lastCheck = 0;
  static uint32_t candidateAt = 0;
  static uint8_t personCandidate = 0;
  if (!mapReady || !motorHomed()) { personCandidate = 0; return; }
  if (millis() - lastCheck < 60) return;
  lastCheck = millis();
  uint8_t nearest = 0;
  int nearestMm = 32767;
  for (uint8_t slot = 0; slot < sensorCount; ++slot) {
    uint8_t id = zoneToSensor[slot];
    if (sensorDisabled(id) || sensorNoTarget(id) || !sensorFresh(id) || !sensorValid[id]) continue;
    if (sensorReadMean[id] < lowerTreshold[slot] && sensorReadMean[id] < nearestMm) {
      nearest = slot + 1;
      nearestMm = sensorReadMean[id];
    }
  }
  if (nearest == 0) { personCandidate = 0; return; } // Hold last position when clear.
  if (nearest != personCandidate) {
    personCandidate = nearest;
    candidateAt = millis();
    return;
  }
  if (millis() - candidateAt < 350) return; // Reject passing noise and unstable targets.
  moveOppositePosition(nearest, nearestMm);
}
