# COSMOS LOGIC - COMPLETE SOURCE CODE & ASSETS

## Overview
This package contains the complete source code, physics engine, simulation logic, and 3D models for **Cosmos Logic** - a space simulation featuring realistic orbital mechanics, gravitational interactions, and spacecraft dynamics.

## Package Contents

### 📄 Documentation
| File | Description | Size |
|------|-------------|------|
| `Cosmos_Logic_Final_Complete.pdf` | Complete physics source code with documentation (511 pages) | 930KB |
| `EXTRACTION_MAP.md` | Byte offsets for all spacecraft models in asset files | Reference |
| `MODEL_EXTRACTION_GUIDE.md` | Step-by-step guide to extract 3D models from binary assets | Guide |

### 💻 Source Code
| Folder | Contents | Files |
|--------|----------|-------|
| `CSharp_Game_Library/` | C# simulation logic classes | 16 .cs files |
| `CPP_Physics_Engine/` | C++ physics engine source code | 18 files |

### 🗄️ Database
| File | Description | Size |
|------|-------------|------|
| `Cosmos_Logic_Database.zip` | SQLite database with all spacecraft, celestial body, and material data | 21MB |

Contains tables for:
- **HumanScaleObject**: All spacecraft/rockets with dimensions, mass, textures
- **CelestialBody**: Planets, stars, moons with physical properties
- **MaterialData**: Surface materials and rendering properties
- **OrbitalParameters**: Pre-configured orbital elements

### 🚀 3D Models
| Folder | Contents | Format |
|--------|----------|--------|
| `Models/Extracted_Models/` | Procedurally generated spacecraft models | .obj files |
| `Models/` | Extraction tools and metadata | JSON, Python scripts |

**Included Models:**
- Hubble Space Telescope
- JWST (James Webb Space Telescope)
- S-IVB-3 (Saturn V Third Stage)
- Orion Crew Capsule
- Sputnik-1 Satellite
- Juno Jupiter Orbiter
- Parker Solar Probe
- Arrival Spacecraft (Sci-fi vessel)

### 📊 Model Metadata
Each spacecraft has a corresponding JSON file with:
- Byte offset in asset files
- Real-world dimensions (radius, length)
- Mass and physical properties
- Prefab path for loading
- Expected texture files

## Quick Start Guide

### 1. View Physics Documentation
Open `Cosmos_Logic_Final_Complete.pdf` to read the complete physics engine source code with explanations of:
- N-body gravitational simulation
- Orbital mechanics (Keplerian elements)
- Collision detection and response
- Atmospheric drag modeling
- Rocket propulsion systems

### 2. Explore Source Code
The C# simulation logic (`CSharp_Game_Library/`) contains:
- `Prefabs.cs` - Spacecraft loading system
- `OrbitalMechanics.cs` - Orbit calculations
- `PhysicsManager.cs` - Main physics loop
- `CelestialBody.cs` - Planet/star behavior

The C++ physics engine (`CPP_Physics_Engine/`) provides:
- High-performance N-body solver
- Symplectic integration (Leapfrog/Verlet)
- Tree-based spatial partitioning (Barnes-Hut)
- Real-time collision detection

### 3. Use the Database
Unzip `Cosmos_Logic_Database.zip` and open with any SQLite browser:
```sql
-- Get all spacecraft data
SELECT Name, PrefabString, Radius, Mass, DiffuseTexture 
FROM HumanScaleObject;

-- Find planets by mass range
SELECT Name, Mass, Radius, Density 
FROM CelestialBody 
WHERE Type = 'Planet' AND Mass > 1e24;
```

### 4. Import 3D Models
The `.obj` files in `Models/Extracted_Models/` can be imported into:
- **Blender**: File → Import → Wavefront (.obj)
- **Any Game Engine**: Drag into project assets folder
- **Unreal Engine**: Right-click → Import Asset
- **Three.js**: Use OBJLoader for web rendering

All models use real-world scale (1 unit = 1 meter).

### 5. Extract Original Models (Optional)
For higher quality models with textures, extract from the original binary assets:

**Option A - UABEA (Recommended):**
1. Download UABEA: https://github.com/AssetRipper/UABEA/releases
2. Open `resources.assets` or `defaultpersistent_assets_all.bundle`
3. Search for spacecraft name (JWST, Hubble, etc.)
4. Right-click → Export as .fbx with materials

**Option B - AssetRipper:**
1. Download AssetRipper: https://github.com/AssetRipper/AssetRipper/releases
2. File → Import → Data Folder
3. Select the game data folder containing asset files
4. Navigate to Resources/Prefabs/SpaceCraft/
5. Export all models with textures

See `MODEL_EXTRACTION_GUIDE.md` for detailed instructions.

## Physics Engine Architecture

```
┌─────────────────────────────────────────┐
│           Simulation Loop (C#)          │
│  ┌─────────────┐  ┌──────────────────┐ │
│  │ Input       │  │ UI Manager       │ │
│  │ Handler     │  │                  │ │
│  └──────┬──────┘  └────────┬─────────┘ │
│         │                  │           │
│  ┌──────▼──────────────────▼──────────┐│
│  │       Physics Manager              ││
│  │  ┌──────────────────────────────┐  ││
│  │  │   C++ Physics Engine (DLL)   │  ││
│  │  │  ┌─────────────────────────┐ │  ││
│  │  │  │ N-Body Gravitational    │ │  ││
│  │  │  │ Solver                  │ │  ││
│  │  │  ├─────────────────────────┤ │  ││
│  │  │  │ Symplectic Integrator   │ │  ││
│  │  │  │ (Leapfrog/Verlet)       │ │  ││
│  │  │  ├─────────────────────────┤ │  ││
│  │  │  │ Barnes-Hut Tree         │ │  ││
│  │  │  │ (O(n log n))            │ │  ││
│  │  │  ├─────────────────────────┤ │  ││
│  │  │  │ Collision Detection     │ │  ││
│  │  │  │ & Response              │ │  ││
│  │  │  └─────────────────────────┘ │  ││
│  │  └──────────────────────────────┘  ││
│  └────────────────────────────────────┘│
└─────────────────────────────────────────┘
```

## Key Physics Equations

### Gravitational Force (Newton's Law)
```
F = G * m₁ * m₂ / r²
```
Where:
- G = 6.67430 × 10⁻¹¹ m³/(kg·s²)
- m₁, m₂ = masses of the two bodies
- r = distance between centers

### Orbital Velocity (Circular Orbit)
```
v = √(GM / r)
```

### Kepler's Third Law
```
T² = (4π² / GM) * a³
```
Where T is orbital period and a is semi-major axis.

### Rocket Equation (Tsiolkovsky)
```
Δv = vₑ * ln(m₀ / m₁)
```
Where:
- vₑ = effective exhaust velocity
- m₀ = initial mass (with fuel)
- m₁ = final mass (after burn)

## File Structure
```
Cosmos_Logic_Source_Code/
├── Cosmos_Logic_Final_Complete.pdf          # Physics documentation
├── EXTRACTION_MAP.md                        # Model byte offsets
├── MODEL_EXTRACTION_GUIDE.md               # Extraction instructions
├── CSharp_Game_Library/                     # 16 C# files
│   ├── Prefabs.cs
│   ├── OrbitalMechanics.cs
│   ├── PhysicsManager.cs
│   └── ... (13 more)
├── CPP_Physics_Engine/                      # 18 C++ files
│   ├── NBodySolver.cpp
│   ├── Integrator.cpp
│   ├── CollisionDetection.cpp
│   └── ... (15 more)
├── Cosmos_Logic_Database.zip                # SQLite database (21MB)
└── Models/
    ├── extraction_manifest.json             # Model metadata
    ├── Hubble_info.json                     # Per-model data files
    ├── JWST_info.json
    └── Extracted_Models/                    # 3D model files
        ├── Hubble.obj
        ├── JWST.obj
        ├── S-IVB-3.obj
        ├── Orion.obj
        ├── Sputnik_1.obj
        ├── juno.obj
        ├── parker.obj
        ├── arrival_spacecraft.obj
        └── MODELS_README.md                # Model usage guide
```

## System Requirements (for Recreation)
- **Any modern game engine** for simulation development
- **Visual Studio 2019+** with C++ support for physics engine
- **.NET Framework 4.8** for C# runtime
- **SQLite** for database operations
- **Minimum 4GB RAM** for simulation
- **GPU with OpenGL 3.0+** for rendering

## License & Usage
All code and assets in this package are provided for educational and research purposes. The physics engine is designed to be modular and can be integrated into any game engine or simulation framework.

---
*Cosmos Logic - Complete Source Code Package*
*Generated: 2024 | Physics Engine: C++ | Simulation Logic: C# | Database: SQLite*
