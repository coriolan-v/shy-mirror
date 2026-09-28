#include "Firmware.h"
#include "Config.h"
// Adafruit ItsyBitsy M4 Express. Serial Monitor: 115200 baud, newline.
#include <string.h>
#include "SensorMap.h"

void printHelp();
void handleCommands();

void setup() {
  Serial.begin(115200);
  initMotor();
  initSensors();
  loadSensorMap();
  printHelp();
  if (homeOnStartup) motorHoming();
}

void loop() {
  handleCommands();
  runMotor();
  // I2C transactions can delay step pulses; diagnostics run separately.
  if (motorBusy()) return;
  readSensors();
  updateSensorMapping();
  if (!sensorMappingActive()) detectPeopleZones();
}

void printHelp() {
  Serial.println("Commands: map, order, cancel, sensors, motor, home, stop, calibrate, skip, help");
}

void handleCommands() {
  static char command[24];
  static uint8_t length = 0;
  static bool overflow = false;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (overflow) Serial.println("Command too long; ignored.");
      else if (length) {
        command[length] = '\0';
        if (!strcmp(command, "help")) printHelp();
        else if (!strcmp(command, "order")) printSensorMap();
        else if (!strcmp(command, "cancel") || !strcmp(command, "stop")) {
          cancelSensorMapping();
          stopMotor();
        }
        else if (!strcmp(command, "skip")) skipMappingPosition();
        else if (motorBusy() || sensorMappingActive())
          Serial.println("Test/setup running. Use cancel or stop first.");
        else if (!strcmp(command, "map")) startSensorMapping();
        else if (!strcmp(command, "sensors")) testSensors();
        else if (!strcmp(command, "motor")) testMotor();
        else if (!strcmp(command, "home")) motorHoming();
        else if (!strcmp(command, "calibrate")) calibrateMotor();
        else Serial.println("Unknown command. Type help.");
      }
      length = 0;
      overflow = false;
    } else if (length < sizeof(command) - 1) command[length++] = c;
    else overflow = true;
  }
}


