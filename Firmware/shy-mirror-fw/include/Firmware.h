#pragma once
#include <Arduino.h>

#include "Config.h"

void initMotor();
void runMotor();
bool motorBusy();
void stopMotor();
void testMotor();
void motorHoming();
int readHallSensor();

void initSensors();
void readSensors();
void testSensors();
void detectPeopleZones();
void printSensorOrder();



bool motorHomed();
void calibrateMotor();
void moveOppositePosition(uint8_t position, uint16_t distanceMm);

void updatePersonPresence(bool detected);
