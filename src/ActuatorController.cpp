#include "ActuatorController.h"

namespace app {

ActuatorController::ActuatorController(const DeviceConfig& config) : config_(config) {}

void ActuatorController::begin() {
  servo_.setPeriodHertz(50);
  servo_.attach(config_.servo_pin, config_.servo_min_pulse_us, config_.servo_max_pulse_us);
  moveTo(config_.initial_angle, true);
  status_message_ = "ready";
  Serial.printf("Actuator ready on GPIO%d\n", config_.servo_pin);
}

void ActuatorController::update() {
  if (phase_ == MotionPhase::kIdle) {
    return;
  }

  const uint32_t now = millis();
  if (!advanceMotion(now)) {
    return;
  }

  if (phase_ == MotionPhase::kMovingToTarget) {
    if (!active_task_.return_home) {
      finishTask("positioned");
      return;
    }

    beginMotion(config_.initial_angle, MotionPhase::kMovingToHome);
    return;
  }

  if (remaining_cycles_ > 1) {
    --remaining_cycles_;
    beginMotion(active_task_.angle, MotionPhase::kMovingToTarget);
    return;
  }

  finishTask("completed");
}

bool ActuatorController::enqueueTask(uint8_t angle,
                                     uint16_t count,
                                     uint32_t move_duration_ms,
                                     bool return_home,
                                     uint32_t request_id,
                                     String& error) {
  if (isBusy()) {
    error = "actuator is busy";
    return false;
  }

  if (angle > config_.max_angle) {
    error = "angle out of range";
    return false;
  }

  if (count == 0) {
    error = "count must be greater than zero";
    return false;
  }

  if (!return_home && count != 1) {
    error = "position command supports count = 1 only";
    return false;
  }

  active_task_ = {request_id, angle, count, move_duration_ms, return_home};
  remaining_cycles_ = count;
  status_message_ = return_home ? "running" : "positioning";

  Serial.printf("Accepted request #%lu angle=%u count=%u durationMs=%lu returnHome=%s\n",
                static_cast<unsigned long>(request_id),
                angle,
                count,
                static_cast<unsigned long>(move_duration_ms),
                return_home ? "true" : "false");

  beginMotion(angle, MotionPhase::kMovingToTarget);
  return true;
}

bool ActuatorController::isBusy() const { return phase_ != MotionPhase::kIdle; }

uint8_t ActuatorController::currentAngle() const { return current_angle_; }

String ActuatorController::statusMessage() const { return status_message_; }

uint32_t ActuatorController::lastCompletedRequestId() const {
  return last_completed_request_id_;
}

void ActuatorController::beginMotion(uint8_t target_angle, MotionPhase phase) {
  motion_start_angle_ = current_angle_;
  motion_target_angle_ = target_angle;
  motion_started_ms_ = millis();
  motion_duration_ms_ = active_task_.move_duration_ms;
  phase_ = phase;

  if (motion_duration_ms_ == 0) {
    moveTo(target_angle, true);
  } else {
    Serial.printf("Servo motion started: %u -> %u in %lu ms\n",
                  motion_start_angle_,
                  motion_target_angle_,
                  static_cast<unsigned long>(motion_duration_ms_));
  }
}

bool ActuatorController::advanceMotion(uint32_t now) {
  if (motion_duration_ms_ == 0) {
    moveTo(motion_target_angle_, false);
    return true;
  }

  const uint32_t elapsed = now - motion_started_ms_;
  if (elapsed >= motion_duration_ms_) {
    moveTo(motion_target_angle_, true);
    return true;
  }

  const float progress =
      static_cast<float>(elapsed) / static_cast<float>(motion_duration_ms_);
  const float angle =
      static_cast<float>(motion_start_angle_) +
      (static_cast<float>(motion_target_angle_) - static_cast<float>(motion_start_angle_)) *
          progress;
  moveTo(static_cast<uint8_t>(roundf(angle)), false);
  return false;
}

void ActuatorController::moveTo(uint8_t angle, bool log_change) {
  if (current_angle_ == angle) {
    return;
  }

  current_angle_ = angle;
  servo_.write(angle);
  if (log_change) {
    Serial.printf("Servo moved to %u degrees\n", angle);
  }
}

void ActuatorController::finishTask(const String& message) {
  phase_ = MotionPhase::kIdle;
  last_completed_request_id_ = active_task_.request_id;
  status_message_ = message;
  active_task_ = {0, 0, 0, 0, false};
  remaining_cycles_ = 0;
  motion_start_angle_ = current_angle_;
  motion_target_angle_ = current_angle_;
  motion_started_ms_ = 0;
  motion_duration_ms_ = 0;
  Serial.printf("Request #%lu %s\n",
                static_cast<unsigned long>(last_completed_request_id_),
                message.c_str());
}

}  // namespace app
