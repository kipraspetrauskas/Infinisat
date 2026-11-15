# Cosmos Logic Integration for Infinisat

## Overview
This module integrates comprehensive orbital mechanics and N-body physics simulation into the Infinisat game. It includes:

- **Physics Engine**: Complete N-body gravitational simulation with RK4 integration
- **Celestial Entities**: Stars, planets, moons, asteroids, comets, black holes
- **Spacecraft System**: Probes, satellites, rockets with staging logic
- **3D Models**: 8 spacecraft models ready for import
- **Cosmos Database**: Full celestial body database with orbital parameters

## Structure

### Source Code (`Source/Infinisat/CosmosLogic/`)
```
├── Core/           — Entity definitions, system engine, world controller
├── Physics/        — Collision handling, motion solvers, N-body systems
├── Data/           — Material composition, trajectory data
├── Solver/         — Numerical integration methods (RK4, Verlet)
├── Modules/        — Component system for modular spacecraft
├── Collision/      — Collision detection and response
├── Extensions/     — Plugin architecture for extensions
├── Orbital/        — Orbital path calculations
├── Spacecraft/     — Spacecraft definitions and staging logic
├── Config/         — Simulation settings
├── Rendering/      — Visual templates for celestial bodies
├── Assets/         — Object template system
└── IO/             — Data loading utilities
```

### Content (`Content/CosmosLogic/`)
```
├── Models/         — 8 spacecraft .obj models
│   ├── Hubble.obj
│   ├── JWST.obj
│   ├── S-IVB-3.obj (Rocket Stage)
│   ├── Orion.obj
│   ├── Sputnik_1.obj
│   ├── juno.obj
│   ├── parker.obj
│   └── arrival_spacecraft.obj
└── Data/           — Cosmos database and manifests
    ├── Cosmos_Logic_Database.zip (SQLite database)
    ├── spacecraft_detailed.json
    ├── spacecraft_manifest.csv
    ├── rocket_models_manifest.json
    └── database_summary.json
```

## Key Classes

### ACosmosWorld
Main world manager that handles:
- N-body physics simulation
- Collision detection between celestial bodies
- Spacecraft trajectory updates
- Simulation time control

### ACelestialEntity
Base class for all celestial objects with:
- Gravitational force calculations
- Orbital mechanics (Keplerian elements)
- Mass and radius properties
- Collision response events

### ASpacecraft
Spacecraft system with:
- Multi-stage rocket support
- Thrust application
- Trajectory calculation
- Staging logic for separation

### FMotionSolver
Physics solver utilities:
- RK4 integration for accurate orbits
- Verlet integration for energy conservation
- N-body force calculations
- Orbital velocity helpers

## Usage

### Blueprint Integration
1. Place `ACosmosWorld` in your level
2. Add celestial entities using `AddCelestialEntity()`
3. Add spacecraft using `AddSpacecraft()`
4. Adjust `SimulationSpeed` for time dilation effects

### C++ Integration
```cpp
#include "CosmosLogic/CosmosWorld.h"
#include "CosmosLogic/CelestialEntity.h"
#include "CosmosLogic/Spacecraft.h"

// In your game mode or level blueprint:
ACosmosWorld* Cosmos = GetWorld()->SpawnActor<ACosmosWorld>(ACosmosWorld::StaticClass());

// Add a star
ACelestialEntity* Star = GetWorld()->SpawnActor<ACelestialEntity>();
Star->CelestialType = ECelestialType::Star;
Star->Mass = 1.989e30f; // Solar mass
Cosmos->AddCelestialEntity(Star);

// Add a spacecraft
ASpacecraft* Probe = GetWorld()->SpawnActor<ASpacecraft>();
Probe->SpacecraftType = ESpacecraftType::Probe;
Cosmos->AddSpacecraft(Probe);
```

## 3D Models
All models are in Wavefront OBJ format with real-world scale (1 unit = 1 meter):
- Origin at center of mass
- Y-axis up direction
- Ready for import into Unreal Engine or any 3D software

To import:
1. Drag .obj files into Content/CosmosLogic/Models/
2. Check "Import Mesh" and "Generate UVs"
3. Apply materials as needed

## Database
The SQLite database contains:
- **506 Galaxies** with morphological classifications
- **6 Black Holes** with spin parameters
- **13,873 Stars** with spectral data
- **6,334 Planets** with orbital elements
- **230 Moons** 
- **54,484 Solar System Objects** (asteroids, comets)
- **73 Spacecraft** with detailed specifications

## Physics Settings
Adjust in `ACosmosWorld`:
- `SimulationSpeed`: Time dilation multiplier
- `bUseNBodyPhysics`: Toggle N-body simulation vs Keplerian orbits

## Performance Tips
1. Use LOD for distant celestial bodies
2. Enable spatial partitioning for collision detection
3. Adjust integration timestep based on simulation speed
4. Use Verlet integration for long-term stability

## Credits
Cosmos Logic physics engine developed for Infinisat project.