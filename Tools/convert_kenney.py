# Blender: Kenney glTF -> single mesh with colours baked into vertex colours (one shared material in Unreal).
# Usage: blender -b --factory-startup -P convert_kenney.py -- <kenney_dir> <out_dir>
import bpy, sys, os

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, DST = argv[0], argv[1]
KITS = {
    "furn": ("furniture-kit/Models/GLTF format", None),
    "sub": ("city-kit-suburban/Models/GLB format", None),
    "road": ("city-kit-roads/Models/GLB format", None),
    "car": ("car-kit/Models/GLB format", ("sedan", "suv", "hatchback-sports", "van", "truck", "delivery", "taxi", "police")),
    "nat": ("nature-kit/Models/GLTF format", ("tree_", "flower_", "grass", "plant_", "rock_", "fence_", "mushroom_", "log", "stump_", "crop_", "pot_")),
}
_img_cache = {}


def image_pixels(img):
    key = img.name
    if key not in _img_cache:
        w, h = img.size
        _img_cache[key] = (w, h, list(img.pixels[:]))
    return _img_cache[key]


def material_color_source(mat):
    """Return ('tex', image) if base colour comes from an image, else ('rgb', (r,g,b))."""
    if not mat or not mat.use_nodes:
        return ("rgb", (0.8, 0.8, 0.8))
    for n in mat.node_tree.nodes:
        if n.type == 'BSDF_PRINCIPLED':
            inp = n.inputs['Base Color']
            if inp.is_linked:
                src = inp.links[0].from_node
                if src.type == 'TEX_IMAGE' and src.image:
                    return ("tex", src.image)
            c = inp.default_value
            return ("rgb", (c[0], c[1], c[2]))
    return ("rgb", (0.8, 0.8, 0.8))


def bake(ob):
    me = ob.data
    attr = me.color_attributes.new("Col", 'FLOAT_COLOR', 'CORNER')
    uv = me.uv_layers.active.data if me.uv_layers.active else None
    srcs = [material_color_source(s.material) for s in ob.material_slots] or [("rgb", (0.8, 0.8, 0.8))]
    for poly in me.polygons:
        kind, val = srcs[min(poly.material_index, len(srcs) - 1)]
        col = val
        if kind == "tex" and uv is not None:
            w, h, px = image_pixels(val)
            # sample at the face's average UV (Kenney palettes are flat colour swatches)
            u = sum(uv[i].uv[0] for i in poly.loop_indices) / len(poly.loop_indices)
            v = sum(uv[i].uv[1] for i in poly.loop_indices) / len(poly.loop_indices)
            x = min(w - 1, max(0, int((u % 1.0) * w)))
            y = min(h - 1, max(0, int((v % 1.0) * h)))
            k = (y * w + x) * 4
            col = (px[k], px[k + 1], px[k + 2])
        for li in poly.loop_indices:
            attr.data[li].color = (col[0], col[1], col[2], 1.0)
    me.color_attributes.active_color = attr


for key, (sub, filt) in KITS.items():
    src = os.path.join(SRC, sub)
    out = os.path.join(DST, key)
    os.makedirs(out, exist_ok=True)
    n = 0
    for fn in sorted(os.listdir(src)):
        if not fn.endswith(".glb"):
            continue
        name = fn[:-4]
        if filt and not any(name.startswith(f) for f in filt):
            continue
        dst = os.path.join(out, name.replace("-", "_") + ".glb")
        if os.path.exists(dst):
            continue
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=os.path.join(src, fn))
        meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        if not meshes:
            continue
        for o in meshes:
            mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
        for o in list(bpy.context.scene.objects):
            if o.type != 'MESH':
                bpy.data.objects.remove(o, do_unlink=True)
        bpy.ops.object.select_all(action='DESELECT')
        for o in meshes: o.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.make_single_user(type='SELECTED_OBJECTS', object=True, obdata=True)
        for o in meshes:
            bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
            bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            bake(o)
        bpy.ops.object.select_all(action='DESELECT')
        for o in meshes: o.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        if len(meshes) > 1:
            bpy.ops.object.join()
        ob = bpy.context.view_layer.objects.active
        ob.data.materials.clear()
        m = bpy.data.materials.new("K"); ob.data.materials.append(m)
        ob.name = name
        try:
            bpy.ops.export_scene.gltf(filepath=dst, export_format='GLB', export_vertex_color='ACTIVE', export_all_vertex_colors=False)
        except TypeError:
            bpy.ops.export_scene.gltf(filepath=dst, export_format='GLB', export_colors=True)
        n += 1
    print("KIT", key, n)
