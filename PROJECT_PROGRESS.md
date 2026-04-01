# HK Actuator Project Progress

## Project Goal

Build a LAN-controlled physical button actuator based on ESP32 and servo hardware, with a Flutter Web control interface for phone and desktop browsers.

## Current Stage

- Stage: firmware architecture bootstrap
- Date: 2026-03-31
- Focus: decouple actuator logic from LAN communication and prepare for multi-device growth

## Completed In This Execution

- Replaced the single-file servo demo with a modular firmware structure
- Added an actuator controller that accepts dynamic `angle` and `count` tasks
- Added automatic return-to-zero behavior after each trigger cycle
- Added a LAN control service with:
  - Wi-Fi station mode setup
  - automatic reconnect attempts
  - `GET /status` status endpoint
  - `GET/POST /actuate` command endpoint
- Added request validation for `deviceId`, `angle`, and `count`
- Reserved a stable `deviceId` field for future multi-device support
- Added central configuration for Wi-Fi and hardware parameters
- Added browser-friendly CORS handling on the firmware API
- Created a standalone Flutter Web control panel scaffold in `control_panel/`
- Added a responsive UI for device URL, angle, cycle count, command submit, and status polling
- Updated firmware Wi-Fi config to target the real network `WIFI7_2.4G`
- Corrected the upload serial port from Bluetooth `COM3` to USB serial `COM5`
- Successfully compiled and uploaded the latest firmware to the ESP32 board on `COM5`

## Firmware Progress Assessment

- Servo drive verification: done
- Parameterized task execution: done
- Automatic reset-to-zero: done
- Wi-Fi auto reconnect skeleton: done
- LAN command API: done
- Browser integration support: done
- Firmware upload to hardware: done
- Multi-device protocol reservation: started
- HomeKit-decoupled architecture: started
- Flutter Web control panel: started
- End-to-end hardware verification: in progress

## New Firmware Structure

- `include/AppConfig.h`: central app, Wi-Fi, and device configuration
- `include/ActuatorController.h`: business-layer actuator API
- `include/LanControlService.h`: LAN communication interface
- `src/ActuatorController.cpp`: non-blocking actuator task execution
- `src/LanControlService.cpp`: Wi-Fi and HTTP endpoint handling
- `src/main.cpp`: application bootstrap and loop integration
- `control_panel/lib/main.dart`: responsive Flutter Web dashboard
- `control_panel/pubspec.yaml`: Flutter app dependency definition

## API Draft

### `GET /status`

Returns current device status, including:

- `deviceId`
- `busy`
- `currentAngle`
- `lastCompletedRequestId`
- `statusMessage`
- `wifiConnected`
- `ip`
- `ssid`

### `POST /actuate`

Supported input styles:

- query parameters: `/actuate?angle=90&count=2`
- JSON body:

```json
{
  "deviceId": "hk-actuator-01",
  "angle": 90,
  "count": 2
}
```

Response behavior:

- `202` when task is accepted
- `400` when parameters are invalid
- `409` when the actuator is busy

## Pending Next Steps

1. Replace placeholder Wi-Fi credentials in `AppConfig.h`
2. Compile and flash on real ESP32 hardware
3. Verify servo timing, hold duration, and reset reliability
4. Run and verify the Flutter Web control page against real hardware
5. Add a stronger protocol layer such as explicit command status polling or task history
6. Consider a task queue if back-to-back commands are required
7. Add device discovery or saved device presets for multi-device usage

## Risks / Notes

- Firmware now includes the provided Wi-Fi credentials and should be treated as sensitive project configuration
- Local compilation could not be verified in this environment because `pio.exe` is unavailable
- Local Flutter execution could not be verified in this environment because Flutter tooling was not available
- Current request parser is intentionally lightweight to avoid extra dependencies
- Current firmware accepts one active task at a time and rejects new commands while busy
- Serial monitor did not return boot logs within the short observation window, so the device IP still needs to be confirmed
