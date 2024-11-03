// M4 express  itsy Bitsy

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  
  initMotor();

  motorHoming();

  initSensors();
}

void loop() {
  // put your main code here, to run repeatedly:
  readSensors();

  detectPeopleZones();
}
