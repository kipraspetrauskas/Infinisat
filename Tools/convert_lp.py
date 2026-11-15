# Blender headless: low-poly CC0 GLBs (Poly Pizza / Kenney / Quaternius) -> one joined mesh per file, scaled so the
# largest dimension is the real size in metres, pivot at the bottom centre, exported as GLB for Unreal.
# Also prints the size (cm) and the heights of large up-facing surfaces (shelf boards / table tops) for placement.
# Usage: blender -b --factory-startup -P convert_lp.py -- <raw_dir> <out_dir> <list.txt: name id size_m>
import bpy, sys, os
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
src, dst, lst = argv
os.makedirs(dst, exist_ok=True)
for line in open(lst):
    if not line.strip():
        continue
    name, _id, size = line.split()
    size = float(size)
    f = os.path.join(src, name + ".glb")
    if not os.path.exists(f):
        print("MISSING", name); continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=f)
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if not meshes:
        print("NO MESH", name); continue
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
    ob.name = name; ob.data.name = name
    vs = [v.co for v in ob.data.vertices]
    mn = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
    mx = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
    k = size / max(mx - mn)
    off = Vector(((mn.x + mx.x) / 2, (mn.y + mx.y) / 2, mn.z))
    for v in ob.data.vertices:
        v.co = (v.co - off) * k
    dims = (mx - mn) * k * 100
    area = {}
    for p in ob.data.polygons:
        if p.normal.z > 0.9:
            z = round(p.center.z * 100 / 2) * 2
            area[z] = area.get(z, 0) + p.area
    tot = sum(area.values()) or 1
    lv = sorted(z for z, a in area.items() if a / tot > 0.05 and z > 3)
    print("LP", name, "size_cm", [round(d) for d in dims], "surfaces_cm", lv)
    bpy.ops.export_scene.gltf(filepath=os.path.join(dst, name + ".glb"), export_format='GLB', use_selection=True)
