#pragma once

#include <Arduino.h>
#include <ESP32Servo.h>

#include "AppConfig.h"

namespace app {

struct ActuationTask {
  uint32_t request_id;
  uint8_t angle;
  uint16_t count;
  uint32_t move_duration_ms;
  bool return_home;
};

class ActuatorController {
 public:
  explicit ActuatorController(const DeviceConfig& config);

  void begin();
  void update();

  bool enqueueTask(uint8_t angle,
                   uint16_t count,
                   uint32_t move_duration_ms,
                   bool return_home,
                   uint32_t request_id,
                   String& error);
  bool isBusy() const;
  uint8_t currentAngle() const;
  String statusMessage() const;
  uint32_t lastCompletedRequestId() const;

 private:
  enum class MotionPhase {
    kIdle,
    kMovingToTarget,
    kMovingToHome,
  };

  void beginMotion(uint8_t target_angle, MotionPhase phase);
  bool advanceMotion(uint32_t now);
  void moveTo(uint8_t angle, bool log_change);
  void finishTask(const String& message);

  const DeviceConfig& config_;
  Servo servo_;
  MotionPhase phase_ = MotionPhase::kIdle;
  ActuationTask active_task_{0, 0, 0, 0, false};
  uint16_t remaining_cycles_ = 0;
  uint8_t current_angle_ = 255;
  uint8_t motion_start_angle_ = 0;
  uint8_t motion_target_angle_ = 0;
  uint32_t motion_started_ms_ = 0;
  uint32_t motion_duration_ms_ = 0;
  uint32_t last_completed_request_id_ = 0;
  String status_message_ = "idle";
};

}  // namespace app
