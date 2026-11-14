# Infinisat

Educational satellite mission design game for the NASA Space Apps 2026 challenge
"Space Mission Design Game". You play BIT, a floating robot engineer: take orders,
build satellites part by part, launch them and operate them, from a back-yard CanSat
to an interstellar solar-sail probe. Every number in the game can be clicked to show
the formula behind it.

Design document: [Docs/The idea and point of the game.txt](Docs/The%20idea%20and%20point%20of%20the%20game.txt)

## Requirements (all free)

- Unreal Engine **5.8** (Epic Games Launcher)
- Visual Studio 2022 Build Tools (or VS 2022/2026) with **Desktop development with C++**
  and the **.NET Framework 4.8 SDK** component (Unreal's build tool needs it)
- Git + **Git LFS** (`git lfs install` once per PC)
- Optional, only for re-importing assets: Blender 5.x, Python 3.11

## Get the project

```sh
git lfs install
git clone https://github.com/kipraspetrauskas/Infinisat.git C:/Dev/Infinisat
```

Keep the project **outside OneDrive/Dropbox** folders: sync tools corrupt Unreal files.

## Build and run

```sh
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" InfinisatEditor Win64 Development -Project="C:/Dev/Infinisat/Infinisat.uproject" -WaitMutex
```

Then open `Infinisat.uproject` (editor) or run the game directly:

```sh
"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "C:/Dev/Infinisat/Infinisat.uproject" -game -windowed -ResX=1600 -ResY=900
```

The first launch compiles shaders (a few minutes).

Useful command-line shortcuts: `-ISWorld` (start in the open world), `-ISMission=2 -ISScreen=design|launch|ops|debrief`.

## Project layout

| Path | What |
|---|---|
| `Source/Infinisat/ISData.*` | Parts catalogue, launch vehicles, missions, quizzes (all the real-world numbers) |
| `Source/Infinisat/ISSim.*` | Budget evaluator (mass, power, link, delta-v, radiation, lifetime, cost) and operations simulator |
| `Source/Infinisat/ISWorld.*` | Open world: garage, house, launch field, sky; CanSat build, flight physics, radio tracking |
| `Source/Infinisat/ISBit.*` | BIT, the first-person player character (floating hands) |
| `Source/Infinisat/ISParts3D.*` | Procedural detailed part models (electronics, rocket, parachute, hands) |
| `Source/Infinisat/ISScene.*` | Space views: orbits, real satellite populations, planets |
| `Source/Infinisat/SISGameUI.*` | All UI (Slate, code only) |
| `Tools/` | Asset pipeline: NASA/Poly Haven download, Blender conversion, Unreal Python import |
| `Content/Infinisat/` | Imported assets (Git LFS) |

## Asset pipeline (only needed to rebuild `Content/`)

```sh
python -I Tools/polyhaven_dl.py _downloads/ph models:<ids> textures:<ids>
blender -b --factory-startup -P Tools/convert_models.py -- _downloads/raw _downloads/clean
blender -b --factory-startup -P Tools/convert_ph.py -- _downloads/ph/models _downloads/ph_clean
UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_assets.py
UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_ph.py
```

## Credits and licences

- 3D spacecraft models and planet textures: [NASA 3D Resources](https://github.com/nasa/NASA-3D-Resources) (public domain; NASA logos not used, no endorsement implied)
- Props, vegetation and textures: [Poly Haven](https://polyhaven.com) (CC0)
- Data: NASA, USGS (Landsat), NASA Earthdata (GNSS), Jonathan McDowell's planet4589.org (Starlink statistics)
- Hackathon prototype, not affiliated with NASA or ESA.
