#include <ESP32Servo.h> // https://github.com/madhephaestus/ESP32Servo

// ref to https://github.com/madhephaestus/ESP32Servo/blob/master/examples/Multiple-Servo-Example-Arduino/Multiple-Servo-Example-Arduino.ino

// create four servo objects 
Servo servo1;

// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;

// These are all GPIO pins on the ESP32
// Recommended pins include 2,4,12-19,21-23,25-27,32-33 
int servo1Pin = 25;

int pos = 0;      // position in degrees

void setup() {
  servo1.setPeriodHertz(50);      // Standard 50hz servo
  servo1.attach(servo1Pin, minUs, maxUs);
}

void loop() {
  for (pos = 0; pos <= 180; pos += 1) { // sweep from 0 degrees to 180 degrees
    // in steps of 1 degree
    servo1.write(pos);
    delay(15);             // waits 15ms for the servo to reach the position
  }
  delay(100);
  
  for (pos = 180; pos >= 0; pos -= 1) { // sweep from 180 degrees to 0 degrees
    // in steps of 1 degree
    servo1.write(pos);
    delay(15);             // waits 15ms for the servo to reach the position
  }
  delay(100);
}
