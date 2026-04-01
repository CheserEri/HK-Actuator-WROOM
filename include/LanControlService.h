#pragma once

#include <WebServer.h>
#include <WiFi.h>

#include "ActuatorController.h"
#include "AppConfig.h"

namespace app {

class LanControlService {
 public:
  LanControlService(const WifiConfig& wifi_config,
                    const DeviceConfig& device_config,
                    ActuatorController& actuator);

  void begin();
  void update();

 private:
  struct CommandRequest {
    bool valid = false;
    bool has_angle = false;
    bool has_count = false;
    bool has_move_duration_ms = false;
    uint8_t angle = 0;
    uint16_t count = 0;
    uint32_t move_duration_ms = 0;
    bool return_home = true;
    String device_id;
    String error;
  };

  void ensureWifiConnected();
  void configureRoutes();
  void sendCommonHeaders();

  void handleRoot();
  void handleStatus();
  void handleActuate();
  void handlePosition();
  void handlePress();
  void handleOptions();
  void handleNotFound();

  CommandRequest parseCommandRequest(bool require_count, bool return_home);
  String buildStatusJson(bool include_wifi_details) const;
  static String jsonEscape(const String& value);
  static bool extractIntField(const String& payload, const char* key, long& value);
  static bool extractStringField(const String& payload, const char* key, String& value);
  static bool tryParseNonNegativeInteger(const String& raw, long& value);

  const WifiConfig& wifi_config_;
  const DeviceConfig& device_config_;
  ActuatorController& actuator_;
  WebServer server_;
  uint32_t next_request_id_ = 1;
  uint32_t last_reconnect_attempt_ms_ = 0;
};

}  // namespace app
