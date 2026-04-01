#pragma once

#include <Arduino.h>

namespace app {

struct WifiConfig {
  const char* ssid;
  const char* password;
  const char* hostname;
};

struct DeviceConfig {
  const char* device_id;
  uint8_t servo_pin;
  uint16_t servo_min_pulse_us;
  uint16_t servo_max_pulse_us;
  uint8_t initial_angle;
  uint8_t max_angle;
  uint16_t default_move_duration_ms;
};

inline constexpr uint32_t kSerialBaudRate = 115200;

inline constexpr WifiConfig kWifiConfig = {
    "WIFI7_2.4G",
    "song123456",
    "hk-actuator-01",
};

inline constexpr DeviceConfig kDeviceConfig = {
    "hk-actuator-01",
    5,
    500,
    2400,
    0,
    180,
    600,
};

inline constexpr uint16_t kHttpPort = 80;
inline constexpr uint32_t kWifiReconnectIntervalMs = 5000;

}  // namespace app
