#pragma once
#include <stdint.h>

const uint32_t sensorMapMagic = 0x534D0208; // SM, schema 2: eight physical slots including disabled positions
struct SensorMapRecord {
  uint32_t magic;
  uint8_t order[8];
  uint32_t checksum;
};

inline uint32_t sensorMapChecksum(const SensorMapRecord &record) {
  uint32_t hash = record.magic;
  for (uint8_t i = 0; i < 8; ++i) hash = (hash ^ record.order[i]) * 16777619UL;
  return hash;
}

inline bool validSensorMap(const SensorMapRecord &record) {
  if (record.magic != sensorMapMagic || record.checksum != sensorMapChecksum(record)) return false;
  uint8_t seen = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    if (record.order[i] >= 8 || (seen & (1u << record.order[i]))) return false;
    seen |= 1u << record.order[i];
  }
  return seen == 0xFF;
}

