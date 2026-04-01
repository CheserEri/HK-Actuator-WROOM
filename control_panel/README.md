# HK Actuator Flutter Web Control Panel

This directory contains the first Flutter Web control panel for the ESP32 actuator firmware.

## Planned usage

1. Update the ESP32 Wi-Fi settings in the firmware config.
2. Run the firmware on the target board.
3. Start this Flutter Web app and open it in a browser.
4. Enter the ESP32 device URL such as `http://192.168.1.100`.
5. Adjust `angle` and `cycles`, then trigger the actuator.

## Expected firmware endpoints

- `GET /status`
- `POST /actuate`

## Notes

- The firmware now includes permissive CORS headers so the web app can call it from a different origin inside the same LAN.
- The web app polls `/status` every two seconds.
- The current UI is a standalone control page and not yet packaged for production deployment.
