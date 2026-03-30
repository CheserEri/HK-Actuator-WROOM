#include <Arduino.h>
#include <ESP32Servo.h>

namespace {
constexpr uint32_t kSerialBaudRate = 115200;
constexpr int kServoPin = 5;
constexpr int kMinPulseWidthUs = 500;
constexpr int kMaxPulseWidthUs = 2400;
constexpr int kStartAngle = 0;
constexpr int kTargetAngle = 90;
constexpr uint32_t kPauseMs = 1000;

Servo servo_motor;

void moveServoTo(int angle) {
  servo_motor.write(angle);
  Serial.printf("Servo moved to %d degrees\n", angle);
}
}  // namespace

void setup() {
  Serial.begin(kSerialBaudRate);
  delay(1000);

  servo_motor.setPeriodHertz(50);
  servo_motor.attach(kServoPin, kMinPulseWidthUs, kMaxPulseWidthUs);

  Serial.println();
  Serial.println("SG90 servo sweep test started");
  Serial.println("Signal pin: GPIO5 (D5)");
  Serial.println("Servo will move between 0 and 90 degrees.");

  moveServoTo(kStartAngle);
  delay(kPauseMs);
}

void loop() {
  moveServoTo(kTargetAngle);
  delay(kPauseMs);

  moveServoTo(kStartAngle);
  delay(kPauseMs);
}
