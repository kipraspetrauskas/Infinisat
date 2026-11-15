# COSMOS LOGIC - PROCEDURAL 3D MODELS

## Overview
This folder contains procedurally generated 3D models for all spacecraft in the Cosmos Logic simulation. These models are based on real-world dimensions and can be imported into any 3D software or game engine.

## Generated Models (8 total)
| Model | Type | Vertices | Faces | File Size |
|-------|------|----------|-------|-----------|
| Hubble | Space Telescope | 50 | 60 | 1.9KB |
| JWST | Space Telescope | 50 | 60 | 1.9KB |
| S-IVB-3 | Rocket Stage | 52 | 80 | 2.1KB |
| Orion | Capsule | 52 | 80 | 2.1KB |
| Sputnik_1 | Satellite | 147 | 216 | 6.1KB |
| juno | Jupiter Orbiter | 8 | 6 | 0.3KB |
| parker | Solar Probe | 8 | 6 | 0.3KB |
| arrival_spacecraft | Sci-fi Ship | 8 | 6 | 0.3KB |

## Usage Instructions

### Import to Blender:
1. Open Blender
2. File → Import → Wavefront (.obj)
3. Select the .obj file you want to import
4. The model will be scaled in meters (real-world scale)

### Import to Any Game Engine:
1. Drag .obj files into your project assets folder
2. The engine will auto-import and generate materials
3. Apply appropriate textures from Model_Textures/

### Import to Unreal Engine:
1. Right-click in Content Browser → Import Asset
2. Select the .obj file
3. Check "Import Mesh" and "Generate UVs"

## Model Specifications
All models use real-world dimensions (meters):
- Origin point at center of mass
- Y-axis is up direction
- Scale factor: 1 unit = 1 meter

## Texture Maps
Placeholder texture maps are provided in the Model_Textures/ folder. For production use, replace with high-resolution textures matching your visual style.

---
*Generated for Cosmos Logic - Procedural Model Reference*
