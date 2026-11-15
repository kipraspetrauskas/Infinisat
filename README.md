# Cosmos Logic - Complete Simulation Package

## Overview
This package contains all the source code, physics engine logic, and simulation data needed to recreate a comprehensive orbital mechanics simulation with rocket physics.

## Contents

### 1. Physics Engine (C++)
Located in `CPP_Physics_Engine/`:

| File | Description | Lines |
|------|-------------|-------|
| `body.hpp` | Celestial body class definition | 65 |
| `celestial.hpp` | Celestial object base class | 69 |
| `ephemeris.hpp` | Orbital ephemeris calculations | 25,695 bytes |
| `integrators.cpp/hpp` | Numerical integration methods (RK4, etc.) | ~1,300 |
| `kepler_orbit.hpp` | Keplerian orbital mechanics | 5,464 bytes |
| `massive_body.hpp` | Massive body physics | 3,031 bytes |
| `methods.hpp` | Physics computation methods | 82,260 bytes |
| `ode.hpp` | Ordinary Differential Equations solver | 9,477 bytes |
| `plugin.cpp` | Main plugin interface (76KB) | ~2,000 |
| `vessel.hpp` | Vessel/rocket class definition | 21,466 bytes |
| `part.cpp/hpp` | **Rocket part physics** (mass, inertia, rigid body) | ~720 |
| `pile_up.cpp/hpp` | **Rocket pile-up mechanics** (staging, separation) | ~950 |

### 2. Simulation Library (C#)
Located in `CSharp_Game_Library/`:

| File | Description | Lines |
|------|-------------|-------|
| `Simulation.cs` | Main simulation loop | 2,429 |
| `Body.cs` | Celestial body implementation | 4,851 |
| `SandboxManager.cs` | Simulation world manager | ~3,000 |
| `Collision.cs` | Collision detection system | ~1,000 |
| `EquationsOfMotion.cs` | Physics equations | ~800 |
| `Integrator.cs` | Time integration | ~250 |
| `NBodyOptions.cs` | N-body simulation settings | ~350 |
| `Composition.cs` | Material composition | ~15 |
| `BodyComponent.cs` | **Rendering pipeline** - celestial body visualization | New |
| `PrefabAppearance.cs` | **Material system** - metallic/smoothness PBR materials | New |

### 3. Database & Models
- `spacecraft_manifest.csv` - All 73 HumanScaleObject entries with PrefabString references
- **12 Unique Spacecraft/Rocket Prefabs:**
  - JWST (James Webb Space Telescope)
  - Hubble (Hubble Space Telescope)
  - parker (Parker Solar Probe)
  - CassiniHuygens (Cassini-Huygens)
  - voyager_spacecraft (Voyager 1 & 2)
  - juno (Juno)
  - S-IVB-3 (Saturn V Stage III - **Rocket Stage!**)
  - NewHorizonSpacecraft (New Horizons)
  - Orion (Orion Spacecraft)
  - GenericSat_1 (Generic Satellite)
  - arrival_spacecraft (Arrival Heptapod Ship)
  - tesla_roadster (Tesla Roadster)
  - Sputnik_1 (Sputnik 1)

### 4. Asset Files
Located in the original extraction:
- `resources.assets` (759MB) - Main asset bundle with all 3D models
- `sharedassets0-4.assets` - Shared resources
- `level0-level4` - Level data
- `StreamingAssets/aa/StandaloneWindows64/defaultpersistent_assets_all.bundle` (257MB) - Contains spacecraft prefabs

## Extracting 3D Models

### Method 1: Using UABEA (Asset Bundle Extractor Archive)
1. Download UABEA from: https://github.com/AssetRipper/UABEA
2. Open `resources.assets` and `defaultpersistent_assets_all.bundle`
3. Search for prefabs by name: JWST, Hubble, parker, etc.
4. Export Mesh, Material, and Texture assets
5. Convert to .fbx or .obj format using AssetRipper

### Method 2: Using Python with unitypack.py
```python
# Example extraction script
import struct

def read_asset_bundle(filepath):
    with open(filepath, 'rb') as f:
        # Parse asset bundle header
        header = f.read(64)
        # Extract metadata and assets
        # ... (implementation depends on format version)
```

### Method 3: Using yt-dlp + ffmpeg for video references
For spacecraft videos or animations, check the GIF folder in StreamingAssets.

## Database Schema

The SQLite database (`ubox.db`) contains 8 tables:
- **Galaxies** - Galaxy data
- **BlackHoles** - Black hole parameters
- **Stars** - Stellar objects (13,873 entries)
- **Planets** - Planetary bodies (6,334 entries)
- **Moons** - Lunar bodies (230 entries)
- **SolarSystemObjects** - All solar system objects (54,484 entries)
- **HumanScaleObject** - Spacecraft/rockets (73 entries) ← **KEY TABLE**
- **GalacticCenterStar** - Galactic center data

## Rocket Physics Implementation

The rocket physics is implemented in:
1. `part.cpp/hpp` - Individual part properties (mass, inertia tensor, center of mass)
2. `pile_up.cpp/hpp` - Part stacking, staging logic, separation mechanics
3. `vessel.hpp` - Complete vessel/rocket assembly
4. `plugin.cpp` - Real-time physics updates and force application

Key concepts:
- **Degrees of Freedom** - 6DOF rigid body simulation
- **Inertia Tensor** - Rotational dynamics
- **Rigid Motion** - Frame transformations between part and world space
- **Mass Change Tracking** - Fuel consumption and staging

## Namespace Changes
All original references have been replaced:
- `principia::ksp_plugin` → `cosmos::physics`
- `#include "ksp_plugin/...` → `#include "cosmos/...`
- `KSP_PLUGIN` → `COSMOS_LOGIC`

## Getting Started

1. **Physics Engine**: Start with `integrators.cpp/hpp` for the core simulation loop
2. **Rocket Parts**: Study `part.cpp/hpp` for individual component physics
3. **Staging Logic**: Read `pile_up.cpp/hpp` for rocket staging mechanics
4. **Simulation Integration**: Use C# files for engine integration patterns
5. **3D Models**: Extract from asset files using UABEA (see EXTRACTION_MAP.md)
6. **Rendering Pipeline**: Study `BodyComponent.cs` and `PrefabAppearance.cs` for material handling

## Project Status (as of November 15, 2026)

### ✅ Completed
- [x] All C++ physics engine source code extracted (18 files)
- [x] All C# simulation library classes decompiled (15 files)
- [x] Complete SQLite database exported (Cosmos_Logic_Database.zip, 21MB)
- [x] All spacecraft/rocket models identified with asset offsets
- [x] Comprehensive extraction guides created (EXTRACTION_MAP.md, MODEL_EXTRACTION_GUIDE.md)
- [x] Final PDF documentation generated (Cosmos_Logic_Final_Complete.pdf, 511 pages)

### ⏳ Remaining Work
- [ ] Extract actual 3D model files (.fbx/.obj) from asset bundles
  - Use UABEA or AssetRipper following EXTRACTION_MAP.md instructions
  - Primary target: `resources.assets` (759MB)
  - Secondary target: `defaultpersistent_assets_all.bundle` (257MB)
- [ ] Extract texture maps and material files
- [ ] Test extracted models in 3D software (Blender/Unreal/Godot)

## Quick Start for Model Extraction

The fastest way to extract all 15 spacecraft/rocket models:

```bash
# 1. Download UABEA
# https://github.com/AssetRipper/UABEA/releases

# 2. Open resources.assets in UABEA
# 3. Search for: JWST, Hubble, S-IVB-3, Orion, etc.
# 4. Right-click each prefab → Export → .fbx format
# 5. Save to Cosmos_Logic_Source_Code/Models/Extracted_Models/
```

See `EXTRACTION_MAP.md` for exact byte offsets of all models.

## File Sizes
- Total C++ source: ~250 KB
- Total C# source: ~428 KB
- Database: 40 MB
- Main asset bundle: 759 MB
- Spacecraft prefabs bundle: 257 MB

---
*Generated for Cosmos Logic - Physics Engine Recreation Project*
