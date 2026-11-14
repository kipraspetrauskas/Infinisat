# Blender: Fusion OBJ exports (KiCad parts) -> joined GLB in metres for Unreal.
# Usage: blender -b --factory-startup -P convert_cad.py -- <in_dir> <out_dir>
import bpy, sys, os
argv = sys.argv[sys.argv.index("--") + 1:]
src, dst = argv[0], argv[1]
os.makedirs(dst, exist_ok=True)
for fn in sorted(os.listdir(src)):
    if not fn.endswith(".obj"): continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.wm.obj_import(filepath=os.path.join(src, fn), forward_axis='Y', up_axis='Z')
    ms = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for o in ms: o.select_set(True)
    bpy.context.view_layer.objects.active = ms[0]
    if len(ms) > 1: bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    d = max(ob.dimensions)
    # Fusion writes OBJ in cm: convert to metres if the part looks 100x too big
    s = 0.01 if d > 0.5 else 1.0
    ob.scale = (s, s, s); bpy.ops.object.transform_apply(scale=True)
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    ob.location = (0, 0, ob.dimensions.z / 2)
    bpy.ops.object.transform_apply(location=True)
    name = fn[:-4]; ob.name = name
    bpy.ops.export_scene.gltf(filepath=os.path.join(dst, name + ".glb"), export_format='GLB')
    print("OK", name, [round(x * 100, 2) for x in ob.dimensions], "cm")
