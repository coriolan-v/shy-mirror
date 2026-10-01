#include "Firmware.h"
#include "Config.h"
// Adafruit ItsyBitsy M4 Express. Serial Monitor: 115200 baud, newline.
#include <string.h>

void printHelp();
void handleCommands();

void setup() {
  Serial.begin(115200);
  initMotor();
  initSensors();
  printSensorOrder();
  printHelp();
  if (homeOnStartup) motorHoming();
}

void loop() {
  handleCommands();
  runMotor();
  // The timer services STEP pulses during sensor reads and live retargeting.
  // Keep homing/test measurements isolated from sensor diagnostics.
  if (motorBusy() && !motorHomed()) return;
  readSensors();
  detectPeopleZones();
}

void printHelp() {
  Serial.println("Commands: order, sensors, motor, home, stop, calibrate, help");
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
        else if (!strcmp(command, "order") || !strcmp(command, "map")) printSensorOrder();
        else if (!strcmp(command, "cancel") || !strcmp(command, "stop")) {
          stopMotor();
        }
        else if (motorBusy())
          Serial.println("Motor moving. Use stop first.");
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


