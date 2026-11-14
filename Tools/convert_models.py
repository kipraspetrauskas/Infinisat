# Blender headless: import Draco-compressed NASA GLBs, join, center, normalize to 1 unit max size,
# export uncompressed GLB for Unreal's glTF importer.
# Usage: blender -b --factory-startup -P convert_models.py -- <in_dir> <out_dir>
import bpy, sys, os, mathutils
argv = sys.argv[sys.argv.index("--")+1:]
src, dst = argv[0], argv[1]
os.makedirs(dst, exist_ok=True)
for fn in sorted(os.listdir(src)):
    if not fn.lower().endswith(".glb"): continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    try:
        bpy.ops.import_scene.gltf(filepath=os.path.join(src, fn))
    except Exception as e:
        print("IMPORT FAIL", fn, e); continue
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not meshes: print("NO MESH", fn); continue
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
    bb = [mathutils.Vector(c) for c in ob.bound_box]
    lo = mathutils.Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
    hi = mathutils.Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
    size = max((hi-lo).x, (hi-lo).y, (hi-lo).z) or 1.0
    ctr = (lo+hi)/2
    ob.data.transform(mathutils.Matrix.Scale(1.0/size, 4) @ mathutils.Matrix.Translation(-ctr))
    ob.name = os.path.splitext(fn)[0]; ob.data.name = ob.name
    bpy.ops.export_scene.gltf(filepath=os.path.join(dst, fn), export_format='GLB',
                              export_draco_mesh_compression_enable=False, export_yup=True, export_apply=True)
    print("OK", fn, "faces", len(ob.data.polygons), "mats", len(ob.data.materials), "size", round(size,3))
