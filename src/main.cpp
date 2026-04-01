#include <Arduino.h>

#include "ActuatorController.h"
#include "AppConfig.h"
#include "LanControlService.h"

namespace {

app::ActuatorController actuator(app::kDeviceConfig);
app::LanControlService control_service(app::kWifiConfig, app::kDeviceConfig, actuator);

}  // namespace

void setup() {
  Serial.begin(app::kSerialBaudRate);
  delay(1000);

  Serial.println();
  Serial.println("HK actuator firmware booting");
  Serial.printf("Device ID: %s\n", app::kDeviceConfig.device_id);

  actuator.begin();
  control_service.begin();
}

void loop() {
  actuator.update();
  control_service.update();
  delay(5);
}
