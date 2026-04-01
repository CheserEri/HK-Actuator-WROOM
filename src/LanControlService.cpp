#include "LanControlService.h"

#include <ctype.h>

namespace app {

LanControlService::LanControlService(const WifiConfig& wifi_config,
                                     const DeviceConfig& device_config,
                                     ActuatorController& actuator)
    : wifi_config_(wifi_config),
      device_config_(device_config),
      actuator_(actuator),
      server_(kHttpPort) {}

void LanControlService::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(wifi_config_.hostname);

  configureRoutes();
  ensureWifiConnected();
  server_.begin();

  Serial.printf("LAN control service listening on port %u\n", kHttpPort);
}

void LanControlService::update() {
  ensureWifiConnected();
  server_.handleClient();
}

void LanControlService::ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - last_reconnect_attempt_ms_) < 0 &&
      last_reconnect_attempt_ms_ != 0) {
    return;
  }

  if (last_reconnect_attempt_ms_ != 0 &&
      now - last_reconnect_attempt_ms_ < kWifiReconnectIntervalMs) {
    return;
  }

  last_reconnect_attempt_ms_ = now;
  Serial.printf("Connecting to Wi-Fi SSID '%s'\n", wifi_config_.ssid);
  WiFi.begin(wifi_config_.ssid, wifi_config_.password);
}

void LanControlService::configureRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/actuate", HTTP_GET, [this]() { handleActuate(); });
  server_.on("/actuate", HTTP_POST, [this]() { handleActuate(); });
  server_.on("/position", HTTP_GET, [this]() { handlePosition(); });
  server_.on("/position", HTTP_POST, [this]() { handlePosition(); });
  server_.on("/press", HTTP_GET, [this]() { handlePress(); });
  server_.on("/press", HTTP_POST, [this]() { handlePress(); });
  server_.on("/actuate", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_.on("/position", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_.on("/press", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_.on("/status", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_.on("/", HTTP_OPTIONS, [this]() { handleOptions(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void LanControlService::sendCommonHeaders() {
  server_.sendHeader("Access-Control-Allow-Origin", "*");
  server_.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server_.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server_.sendHeader("Cache-Control", "no-store");
}

void LanControlService::handleRoot() {
  sendCommonHeaders();
  server_.send(200,
               "application/json",
               "{\"service\":\"hk-actuator\",\"message\":\"Use /status, /actuate, "
               "/position or /press\"}");
}

void LanControlService::handleStatus() {
  sendCommonHeaders();
  server_.send(200, "application/json", buildStatusJson(true));
}

void LanControlService::handleActuate() {
  sendCommonHeaders();
  CommandRequest request = parseCommandRequest(true, true);
  if (!request.valid) {
    server_.send(400,
                 "application/json",
                 "{\"accepted\":false,\"message\":\"" + jsonEscape(request.error) +
                     "\"}");
    return;
  }

  String error;
  const uint32_t request_id = next_request_id_++;
  if (!actuator_.enqueueTask(request.angle,
                             request.count,
                             request.move_duration_ms,
                             request.return_home,
                             request_id,
                             error)) {
    server_.send(409,
                 "application/json",
                 "{\"accepted\":false,\"requestId\":" + String(request_id) +
                     ",\"message\":\"" + jsonEscape(error) + "\"}");
    return;
  }

  String response = "{\"accepted\":true,\"requestId\":" + String(request_id) +
                    ",\"deviceId\":\"" + jsonEscape(device_config_.device_id) +
                    "\",\"message\":\"accepted\"}";
  server_.send(202, "application/json", response);
}

void LanControlService::handlePosition() {
  sendCommonHeaders();
  CommandRequest request = parseCommandRequest(false, false);
  if (!request.valid) {
    server_.send(400,
                 "application/json",
                 "{\"accepted\":false,\"message\":\"" + jsonEscape(request.error) +
                     "\"}");
    return;
  }

  String error;
  const uint32_t request_id = next_request_id_++;
  if (!actuator_.enqueueTask(request.angle,
                             1,
                             request.move_duration_ms,
                             request.return_home,
                             request_id,
                             error)) {
    server_.send(409,
                 "application/json",
                 "{\"accepted\":false,\"requestId\":" + String(request_id) +
                     ",\"message\":\"" + jsonEscape(error) + "\"}");
    return;
  }

  String response = "{\"accepted\":true,\"requestId\":" + String(request_id) +
                    ",\"deviceId\":\"" + jsonEscape(device_config_.device_id) +
                    "\",\"message\":\"accepted\"}";
  server_.send(202, "application/json", response);
}

void LanControlService::handlePress() {
  sendCommonHeaders();
  CommandRequest request = parseCommandRequest(false, true);
  if (!request.valid) {
    server_.send(400,
                 "application/json",
                 "{\"accepted\":false,\"message\":\"" + jsonEscape(request.error) +
                     "\"}");
    return;
  }

  String error;
  const uint32_t request_id = next_request_id_++;
  if (!actuator_.enqueueTask(request.angle,
                             1,
                             request.move_duration_ms,
                             true,
                             request_id,
                             error)) {
    server_.send(409,
                 "application/json",
                 "{\"accepted\":false,\"requestId\":" + String(request_id) +
                     ",\"message\":\"" + jsonEscape(error) + "\"}");
    return;
  }

  String response = "{\"accepted\":true,\"requestId\":" + String(request_id) +
                    ",\"deviceId\":\"" + jsonEscape(device_config_.device_id) +
                    "\",\"message\":\"accepted\"}";
  server_.send(202, "application/json", response);
}

void LanControlService::handleOptions() {
  sendCommonHeaders();
  server_.send(204, "text/plain", "");
}

void LanControlService::handleNotFound() {
  sendCommonHeaders();
  server_.send(404,
               "application/json",
               "{\"message\":\"not found\",\"path\":\"" +
                   jsonEscape(server_.uri()) + "\"}");
}

LanControlService::CommandRequest LanControlService::parseCommandRequest(bool require_count,
                                                                        bool return_home) {
  CommandRequest request;
  request.device_id = device_config_.device_id;
  request.return_home = return_home;
  request.move_duration_ms = device_config_.default_move_duration_ms;
  long parsed_int = 0;

  if (server_.hasArg("angle")) {
    if (!tryParseNonNegativeInteger(server_.arg("angle"), parsed_int)) {
      request.error = "angle must be a non-negative integer";
      return request;
    }
    request.has_angle = true;
    request.angle = static_cast<uint8_t>(parsed_int);
  }

  if (server_.hasArg("count")) {
    if (!tryParseNonNegativeInteger(server_.arg("count"), parsed_int)) {
      request.error = "count must be a positive integer";
      return request;
    }
    request.has_count = true;
    request.count = static_cast<uint16_t>(parsed_int);
  }

  if (server_.hasArg("moveDurationMs")) {
    if (!tryParseNonNegativeInteger(server_.arg("moveDurationMs"), parsed_int)) {
      request.error = "moveDurationMs must be a non-negative integer";
      return request;
    }
    request.has_move_duration_ms = true;
    request.move_duration_ms = static_cast<uint32_t>(parsed_int);
  }

  if (server_.hasArg("deviceId")) {
    request.device_id = server_.arg("deviceId");
  }

  const String payload = server_.arg("plain");
  String parsed_string;

  if (!request.has_count && extractIntField(payload, "count", parsed_int)) {
    request.has_count = true;
    request.count = static_cast<uint16_t>(parsed_int);
  }

  if (!request.has_angle && extractIntField(payload, "angle", parsed_int)) {
    request.has_angle = true;
    request.angle = static_cast<uint8_t>(parsed_int);
  }

  if (!request.has_move_duration_ms && extractIntField(payload, "moveDurationMs", parsed_int)) {
    request.has_move_duration_ms = true;
    request.move_duration_ms = static_cast<uint32_t>(parsed_int);
  }

  if (!server_.hasArg("deviceId") &&
      extractStringField(payload, "deviceId", parsed_string)) {
    request.device_id = parsed_string;
  }

  if (request.device_id.length() > 0 &&
      request.device_id != device_config_.device_id) {
    request.error = "deviceId mismatch";
    return request;
  }

  if (!request.has_angle) {
    request.error = "angle is required";
    return request;
  }

  if (require_count && !request.has_count) {
    request.error = "count is required";
    return request;
  }

  if (request.angle > device_config_.max_angle) {
    request.error = "angle must be between 0 and " + String(device_config_.max_angle);
    return request;
  }

  if (require_count && request.count == 0) {
    request.error = "count must be greater than zero";
    return request;
  }

  if (!require_count) {
    request.count = 1;
  }

  request.valid = true;
  return request;
}

String LanControlService::buildStatusJson(bool include_wifi_details) const {
  String response = "{\"deviceId\":\"" + jsonEscape(device_config_.device_id) + "\"";
  response += ",\"busy\":" + String(actuator_.isBusy() ? "true" : "false");
  response += ",\"currentAngle\":" + String(actuator_.currentAngle());
  response += ",\"lastCompletedRequestId\":" +
              String(actuator_.lastCompletedRequestId());
  response += ",\"statusMessage\":\"" + jsonEscape(actuator_.statusMessage()) + "\"";

  if (include_wifi_details) {
    response += ",\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    response += ",\"ip\":\"" + jsonEscape(WiFi.status() == WL_CONNECTED
                                              ? WiFi.localIP().toString()
                                              : String("")) +
                "\"";
    response += ",\"ssid\":\"" + jsonEscape(wifi_config_.ssid) + "\"";
  }

  response += "}";
  return response;
}

String LanControlService::jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 4);
  for (size_t i = 0; i < value.length(); ++i) {
    const char ch = value[i];
    if (ch == '\\' || ch == '"') {
      escaped += '\\';
    }
    escaped += ch;
  }
  return escaped;
}

bool LanControlService::extractIntField(const String& payload,
                                        const char* key,
                                        long& value) {
  const String pattern = "\"" + String(key) + "\"";
  const int key_index = payload.indexOf(pattern);
  if (key_index < 0) {
    return false;
  }

  const int colon_index = payload.indexOf(':', key_index + pattern.length());
  if (colon_index < 0) {
    return false;
  }

  int start = colon_index + 1;
  while (start < payload.length() && isspace(payload[start])) {
    ++start;
  }

  int end = start;
  while (end < payload.length() && (isDigit(payload[end]) || payload[end] == '-')) {
    ++end;
  }

  if (end == start) {
    return false;
  }

  value = payload.substring(start, end).toInt();
  return true;
}

bool LanControlService::extractStringField(const String& payload,
                                           const char* key,
                                           String& value) {
  const String pattern = "\"" + String(key) + "\"";
  const int key_index = payload.indexOf(pattern);
  if (key_index < 0) {
    return false;
  }

  const int colon_index = payload.indexOf(':', key_index + pattern.length());
  if (colon_index < 0) {
    return false;
  }

  int start = payload.indexOf('"', colon_index + 1);
  if (start < 0) {
    return false;
  }

  ++start;
  const int end = payload.indexOf('"', start);
  if (end < 0) {
    return false;
  }

  value = payload.substring(start, end);
  return true;
}

bool LanControlService::tryParseNonNegativeInteger(const String& raw, long& value) {
  if (raw.isEmpty()) {
    return false;
  }

  for (size_t i = 0; i < raw.length(); ++i) {
    if (!isDigit(raw[i])) {
      return false;
    }
  }

  value = raw.toInt();
  return true;
}

}  // namespace app
