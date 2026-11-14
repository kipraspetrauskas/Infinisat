# Blender headless: Poly Haven glTF (real-world scale) -> single joined GLB with embedded textures.
# Usage: blender -b --factory-startup -P convert_ph.py -- <ph_models_dir> <out_dir>
import bpy, sys, os
argv = sys.argv[sys.argv.index("--") + 1:]
src, dst = argv[0], argv[1]
os.makedirs(dst, exist_ok=True)
for aid in sorted(os.listdir(src)):
    folder = os.path.join(src, aid)
    gl = [f for f in os.listdir(folder) if f.endswith(".gltf")]
    if not gl: continue
    out = os.path.join(dst, aid + ".glb")
    if os.path.exists(out): print("SKIP", aid); continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    try:
        bpy.ops.import_scene.gltf(filepath=os.path.join(folder, gl[0]))
    except Exception as e:
        print("IMPORT FAIL", aid, e); continue
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not meshes: print("NO MESH", aid); continue
    for o in meshes:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    for o in list(bpy.context.scene.objects):
        if o.type != 'MESH': bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    for o in meshes: o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if len(meshes) > 1: bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = aid; ob.data.name = aid
    faces = len(ob.data.polygons)
    if faces > 150000:
        mod = ob.modifiers.new("dec", 'DECIMATE')
        mod.ratio = max(0.02, 120000.0 / faces)
        bpy.ops.object.modifier_apply(modifier="dec")
        print("DECIMATED", aid, faces, "->", len(ob.data.polygons))
    bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', export_apply=True, export_yup=True)
    print("OK", aid, len(ob.data.polygons), "faces", len(ob.data.materials), "mats", [round(x, 2) for x in ob.dimensions])
