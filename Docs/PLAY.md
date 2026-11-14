# Infinisat - playable build (Windows)

1. Download `Infinisat-Windows.zip` (Releases page, or this branch).
2. Unzip anywhere (e.g. Desktop). Keep the folder structure.
3. Run `Infinisat.exe`. If Windows SmartScreen appears: More info -> Run anyway.
   If it asks for the Visual C++ runtime / DirectX, run `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe` once.

Laptop without a graphics card? Pause (Esc) -> Settings -> Graphics -> **Low (low-poly)**.

## Controls
| Key | Action |
|---|---|
| WASD + mouse | move / look (first person) |
| Shift | sprint |
| Space / Ctrl | hover / crouch |
| E | interact (doors, parts, laptop, bench) |
| Q | scan for nearby parts |
| Esc | pause / settings (mouse sensitivity, FOV, graphics) |

## What to do
- **Prologue (CanSat at home):** find the ESP32, LoRa module, 18650 battery, DHT11 sensor, wires, parachute... around the house,
  build the CanSat on the garage bench, launch it to 1 km, then track it with the Yagi antenna and find it.
- **Missions:** pick a mission and a difficulty, 1-4 players (each with a role), design the satellite in the clean room,
  pick a launcher, fly the mission (time warp), answer the anomalies, read the debrief and report.

Reports are saved to `Documents/Infinisat Reports`, the leaderboard to `Saved/Infinisat/leaderboard.csv`.
