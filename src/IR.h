#ifndef IR_H
#define IR_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ShelterSystem {
private:
    int _sensorPins[5];
    int _servoPin;
    Servo _shelterServo;
    
    unsigned long _simultaneousStartTime;
    bool _trackingActive;

    void shakeShelter();

public:
    ShelterSystem(const int pins[5], int servoPin);
    void begin();
    void update();
    void forceClockSync(); // Used to reset baseline millis post-shake
};

#endif // IR_H