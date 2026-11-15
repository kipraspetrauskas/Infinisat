# COSMOS LOGIC - 3D MODEL EXTRACTION MAP
## Complete Asset Locations & Offsets for All Rocket/Spacecraft Models

---

## PRIMARY ASSET FILE: resources.assets (759 MB)
**Path**: `Cosmos_Logic_Source_Code/Assets/resources.assets`

### Spacecraft Model Offsets (byte positions)
| Model Name | Offset (Decimal) | Offset (Hex) | Notes |
|------------|-----------------|--------------|-------|
| Hubble | 1,348,916 | 0x14725C | Multiple material references |
| JWST | 1,403,892 | 0x1565EC | Primary telescope model |
| S-IVB-3 | 1,453,924 | 0x162E7C | Saturn V third stage (ROCKET) |
| arrival_spacecraft | 1,481,220 | 0x1691EC | Arrival movie spacecraft |
| juno | 1,486,772 | 0x16A5DC | Jupiter orbiter |
| Sputnik_1 | 1,687,764 | 0x19E3CC | First satellite |
| parker | 1,770,404 | 0x1B2FEC | Parker Solar Probe |
| Orion | 3,171,652 | 0x303D8C | Orion capsule |

### Additional References (later in file)
- JWST: 45,353,220 / 69,030,756 / 102,664,452 / 108,286,132 / 119,500,580
- juno: 3,179,604 / 34,292,659 / 34,841,582 / 77,451,124 / 121,344,615
- Hubble: 125,839,044
- Orion: 8,798,738 / 125,847,188 / 137,216,356 / 137,218,724

### Prefab Container Offsets
The file contains 30+ Prefab containers starting at:
- 247,083,876 (0x0EB4A1AC)
- 306,886,928 (0x124F5E50)
- 306,887,030 (0x124F5ED6)
- ... and many more through the file

---

## SECONDARY ASSET FILE: defaultpersistent_assets_all.bundle (257 MB)
**Path**: `StreamingAssets/aa/StandaloneWindows64/defaultpersistent_assets_all.bundle`

### Spacecraft Model Offsets
| Model Name | Offset (Decimal) | Offset (Hex) |
|------------|-----------------|--------------|
| parker | 79,369,191 | 0x4B8A5EF |
| Hubble | 112,181,127 | 0x6B5E5FF |
| JWST | 147,862,545 | 0x8C6F5D1 |

---

## EXTRACTION INSTRUCTIONS

### Method 1: UABEA (Recommended for Beginners)
Step 1: Download UABEA from https://github.com/AssetRipper/UABEA/releases
Step 2: Run UABEA.exe
Step 3: File -> Open -> Select resources.assets
Step 4: In tree view, expand Container
Step 5: Search for: JWST, Hubble, S-IVB-3, etc.
Step 6: Right-click prefab -> Export -> Choose .fbx format
Step 7: Save to extraction folder

### Method 2: AssetRipper (Advanced - Full Scene Export)
Step 1: Download AssetRipper from https://github.com/AssetRipper/AssetRipper/releases
Step 2: File -> Import -> Data Folder
Step 3: Select the data folder containing asset files
Step 4: Wait for import (5-10 minutes for 759MB file)
Step 5: Navigate to Resources/Prefabs/SpaceCraft/
Step 6: Right-click -> Export All -> .fbx with textures

### Method 3: Python Script (Automated Batch Extraction)
Requires: pip install pyuac or unitypack

def extract_prefab_at_offset(filepath, offset, output_dir):
    # Read asset object header at specific offset
    with open(filepath, 'rb') as f:
        f.seek(offset)
        header = f.read(128)
    # Parse type, size, and data pointers
    # Export mesh, material, and animation data
    pass

# Usage for all spacecraft:
spacecraft_offsets = {
    'Hubble': 1348916,
    'JWST': 1403892,
    'S-IVB-3': 1453924,
}

for name, offset in spacecraft_offsets.items():
    extract_prefab_at_offset('resources.assets', offset, f'output/{name}/')

### Method 4: Command Line (Quick Preview)
# Extract raw bytes around each model for inspection
dd if=resources.assets bs=1 skip=1403892 count=4096 of=JWST_header.bin

# Search for all prefab references
grep -aob "Prefab" resources.assets > prefab_offsets.txt

---

## MODEL HIERARCHY (from Prefabs.cs)

All spacecraft follow this hierarchy pattern:
[Spacecraft_Name] (GameObject)
  MeshFilter
    Mesh (vertices, UVs, normals)
  MeshRenderer
    Material[] (shaders, textures)
  Collider (for physics)
  [Child Parts] (optional sub-assemblies)

### Expected File Outputs Per Model
- [Name].fbx - Main 3D mesh with materials
- [Name]_Material.mat - Shader/material definitions
- [Name]_Texture.png - Diffuse/albedo texture map
- [Name]_Normal.png - Normal map (if available)

---

## DATABASE INTEGRATION

Each spacecraft has corresponding data in HumanScaleObject table:

SELECT ID, Name, PrefabString, Radius, Mass, DiffuseTexture 
FROM HumanScaleObject 
WHERE PrefabString IN ('JWST', 'Hubble', 'S-IVB-3', ...);

Use this data to:
- Set correct scale (Radius column)
- Configure mass for physics simulation
- Apply proper texture paths

---

## VERIFICATION CHECKLIST

After extraction, verify each model has:
[ ] Valid mesh geometry (no missing vertices)
[ ] UV coordinates mapped correctly
[ ] Materials/textures applied
[ ] Correct scale (compare to database Radius value)
[ ] Origin point at center of mass

---

## TROUBLESHOOTING

| Issue | Solution |
|-------|----------|
| Models appear black | Export with materials enabled or apply textures manually |
| Wrong scale | Use HumanScaleObject.Radius to set correct dimensions |
| Missing animations | Most spacecraft are static; no animation data expected |
| UABEA crashes | Try AssetRipper instead, or export in smaller batches |
| Corrupt mesh data | Check offset alignment - asset files use 4-byte alignment |

---

## QUICK REFERENCE: All 15 Spacecraft/Rocket Models

| # | Name | Type | Primary Offset | Bundle Offset |
|---|------|------|----------------|---------------|
| 1 | Rocket_01 | Generic Rocket | In resources.assets | - |
| 2 | S-IVB-3 | Saturn V Stage III (ROCKET) | 1,453,924 | - |
| 3 | soyuz_complete | Soyuz Spacecraft | In resources.assets | - |
| 4 | Orion | Orion Capsule | 3,171,652 | - |
| 5 | Sputnik_1 | Satellite | 1,687,764 | - |
| 6 | JWST | Space Telescope | 1,403,892 | 147,862,545 |
| 7 | Hubble | Space Telescope | 1,348,916 | 112,181,127 |
| 8 | parker | Solar Probe | 1,770,404 | 79,369,191 |
| 9 | voyager_spacecraft | Deep Space Probe | In resources.assets | - |
| 10 | CassiniHuygens | Saturn Probe | In resources.assets | - |
| 11 | NewHorizonSpacecraft | Pluto Probe | In resources.assets | - |
| 12 | juno | Jupiter Orbiter | 1,486,772 | - |
| 13 | tesla_roadster | Tesla in Space | In resources.assets | - |
| 14 | arrival_spacecraft | Sci-fi Ship | 1,481,220 | - |
| 15 | GenericSat_1 | Satellite Component | In resources.assets | - |

---
*Generated for Cosmos Logic - Model Extraction Reference*
