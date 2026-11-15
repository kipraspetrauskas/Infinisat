# COSMOS LOGIC - 3D MODEL EXTRACTION GUIDE
## Complete Guide to Extracting Rocket & Spacecraft Models from Asset Files

---

## OVERVIEW

This guide explains how to extract all 15 rocket and spacecraft 3D models from the Cosmos Logic assets. The models are stored in a serialized asset format within `resources.assets` (759MB).

### What You'll Extract
- **2 Rocket Models**: Generic rocket, Saturn V third stage
- **3 Spacecraft**: Soyuz, Orion capsule, Sputnik 1
- **7 Space Probes/Telescopes**: JWST, Hubble, Parker Solar Probe, Voyager, Cassini-Huygens, New Horizons, Juno
- **3 Special Objects**: Tesla Roadster, Arrival spacecraft, Generic satellite

---

## METHOD 1: UABEA (Asset Bundle Extractor) - RECOMMENDED

### Step 1: Download & Install UABEA
1. Download UABEA from: https://github.com/AssetRipper/UABEA/releases
2. Extract to a folder (e.g., `C:\UABEA\`)
3. Run `UABEA.exe` (no installation needed)

### Step 2: Open the Asset File
1. In UABEA, click **File → Open**
2. Navigate to: `Cosmos_Logic_Source_Code/Assets/resources.assets`
3. Select file and open

### Step 3: Export All Models
1. In the tree view, expand **Container**
2. Right-click on the container root → **Export**
3. Choose export folder (e.g., `Cosmos_Logic_Source_Code/Models/Extracted_Models/`)
4. Select format: **.fbx** or **.obj** (recommended for 3D models)
5. Click **OK** to start extraction

### Step 4: Filter Spacecraft Models
After export, navigate to the exported folder and look for these paths:
```
Prefabs/SpaceCraft/Rocket_01
Prefabs/SpaceCraft/S-IVB-3
Prefabs/SpaceCraft/soyuz_complete
Prefabs/Spacecraft/Orion
Prefabs/Spacecraft/Sputnik_1
Prefabs/SpaceCraft/JWST
Prefabs/SpaceCraft/Hubble
Prefabs/SpaceCraft/parker
Prefabs/SpaceCraft/voyager_spacecraft
Prefabs/SpaceCraft/CassiniHuygens
Prefabs/SpaceCraft/NewHorizonSpacecraft
Prefabs/SpaceCraft/juno
Prefabs/SpaceCraft/tesla_roadster
Prefabs/SpaceCraft/arrival_spacecraft
Prefabs/SpacecraftParts/GenericSat_1
```

---

## METHOD 2: ASSETRIPPER (Advanced Users)

### Step 1: Install AssetRipper
1. Download from: https://github.com/AssetRipper/AssetRipper/releases
2. Run installer or extract portable version

### Step 2: Import Assets
1. Open AssetRipper
2. **File → Import → Data Folder**
3. Select the data folder containing asset files
4. Wait for import to complete (may take 5-10 minutes)

### Step 3: Export Models
1. In the tree, navigate to **Resources → Prefabs**
2. Right-click on SpaceCraft folder → **Export All**
3. Choose format: **.fbx** with textures
4. Select output folder and export

---

## METHOD 3: PYTHON SCRIPT (Automated)

Use this Python script with `pyuac` or `unitypack` library:

```python
# pip install pyuac
import pyuac

# Open asset file
asset = pyuac.load('resources.assets')

# Search for spacecraft models
spacecraft_paths = [
    'Prefabs/SpaceCraft/Rocket_01',
    'Prefabs/SpaceCraft/S-IVB-3',
    'Prefabs/SpaceCraft/JWST',
    # ... add all paths from manifest
]

for path in spacecraft_paths:
    obj = asset.find(path)
    if obj:
        obj.export(f'{path.replace("/", "_")}.fbx')
        print(f"Exported: {path}")
```

---

## EXTRACTED MODEL FORMATS

### Supported Output Formats
- **.FBX** (Recommended): Full 3D model with materials, animations
- **.OBJ**: Universal format, widely supported
- **.GLTF/.GLB**: Modern web-friendly format
- **.BLEND**: Direct import to Blender

### What's Included in Each Model
- **Mesh Data**: Vertices, faces, normals, UV coordinates
- **Materials**: Textures, shaders, color maps
- **Animations** (if any): Movement, rotation sequences
- **Hierarchy**: Child objects, transforms

---

## DATABASE INTEGRATION

All spacecraft have corresponding database entries in `HumanScaleObject` table:

```json
{
  "ID": 123,
  "Name": "JWST",
  "PrefabString": "JWST",
  "Radius": 6.0,
  "Mass": 6500.0,
  "DiffuseTexture": "path/to/texture.png"
}
```

Use this data to:
- Set correct scale and mass in your engine
- Apply proper textures and materials
- Configure physics properties

---

## QUICK START CHECKLIST

- [ ] Download UABEA or AssetRipper
- [ ] Open `resources.assets` (759MB)
- [ ] Export all prefabs to .FBX format
- [ ] Filter for 15 spacecraft/rocket models listed above
- [ ] Import extracted models into your 3D software
- [ ] Use database data for scale, mass, and textures
- [ ] Test in your simulation engine

---

## TROUBLESHOOTING

### Models appear black/dark
→ Missing textures. Export with materials enabled or manually apply texture maps from `StreamingAssets/` folder.

### Wrong scale
→ Check `HumanScaleObject.Radius` column in database for correct dimensions.

### Missing animations
→ Some spacecraft have no animations (static models). This is normal.

### UABEA crashes on large files
→ Use AssetRipper instead, or export in smaller batches by filtering specific paths.

---

## FILE LOCATIONS SUMMARY

| File | Path | Size | Contents |
|------|------|------|----------|
| resources.assets | `Cosmos_Logic_Source_Code/Assets/resources.assets` | 759MB | **ALL prefabs including spacecraft** |
| defaultpersistent_assets_all.bundle | `StreamingAssets/aa/StandaloneWindows64/` | 257MB | Persistent assets (secondary) |
| sharedassets0-4.assets | `Cosmos_Logic_Source_Code/Assets/` | 44MB+ | Shared simulation assets |
| ubox.db | `StreamingAssets/ubox.db` | 40MB | SQLite database with all object data |

---

## NEXT STEPS AFTER EXTRACTION

1. **Import to Blender/Godot/Unreal**: Open .FBX files in your preferred 3D software
2. **Optimize Meshes**: Reduce polygon count if needed for real-time rendering
3. **Apply Textures**: Use texture maps from database `DiffuseTexture` field
4. **Set Up Physics**: Use mass and radius data from database for collision bodies
5. **Animate** (optional): Add rotation, thruster effects, etc.

---

## SUPPORT

For issues with extraction:
- UABEA GitHub: https://github.com/AssetRipper/UABEA/issues
- AssetRipper Discord: https://discord.gg/asseteripper

---

*Generated for Cosmos Logic simulation project*
