# AI Agent Instructions for ESP32 Wokwi Simulation Project

## What this project is

This repository contains an ESP32 firmware project that simulates an ultrasonic water tank system using Wokwi. The main firmware is in `src/main.cpp`, and the project uses PlatformIO with the Arduino framework on an ESP32-S3 board.

The repository also contains custom Wokwi chip sources (`custom-ultrasonic-sensor.c`, `custom-watertank-simulator.c`) and simulation files (`diagram.json`, `diagram_test.json`).

## Primary goals for AI assistance

- Help maintain and extend the ESP32 firmware logic in `src/main.cpp`.
- Preserve the PlatformIO build setup in `platformio.ini`.
- Respect the Wokwi simulation workflow: custom chips must compile to `.wasm` and the simulator is driven by JSON diagram files.
- Avoid exposing sensitive credentials from `src/include/secrets.h`.

## Useful files

- `README.md` — project purpose and run instructions.
- `platformio.ini` — build environment and library dependencies.
- `src/main.cpp` — core firmware, sensor logic, MQTT publish flow.
- `src/include/secrets.h` — local WiFi/MQTT credentials; not committed here.
- `custom-ultrasonic-sensor.c`, `custom-watertank-simulator.c` — custom Wokwi chip implementations.
- `diagram.json` / `diagram_test.json` — simulator circuit definitions.

## Build and run commands

Use PlatformIO for firmware builds and Wokwi CLI for chip compilation.

- Build firmware:
  ```powershell
  platformio run
  ```
- Compile each custom chip:
  ```powershell
  wokwi-cli chip compile custom-ultrasonic-sensor.c -o custom-ultrasonic-sensor.chip.wasm
  wokwi-cli chip compile custom-watertank-simulator.c -o custom-watertank-simulator.chip.wasm
  ```
- Start the simulator from `diagram.json` in VS Code via the Wokwi extension.

## Project conventions

- Firmware is Arduino-style C++ for ESP32.
- MQTT payloads are published to `sensors/group05/watertank/data`.
- The simulation is controlled by Wokwi JSON and compiled custom chips.
- Keep logic and configuration separate: update only `platformio.ini` for build/dependencies and `src/main.cpp` for runtime behavior.

## Important considerations

- `src/include/secrets.h` must provide `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_SERVER_IP`, and `MQTT_SERVER_PORT`. Do not hardcode credentials in shared files.
- Custom chip source changes require recompiling `.wasm` before the Wokwi simulator will reflect them.
- The Wokwi CLI may require a new terminal after installation.

## When in doubt

- Refer to `README.md` first for the run workflow.
- Prefer small, testable firmware changes rather than broad refactors.
- Keep the custom chip and diagram files aligned with the simulation expectations.
