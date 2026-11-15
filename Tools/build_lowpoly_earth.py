# Blender headless: stylised low-poly Earth for the orbit view + a low-poly cloud puff.
#   Earth: icosphere (subdiv 5 = 20,480 flat faces, radius 1 m -> scaled in Unreal), every face painted with one palette
#   colour (deep/shallow ocean, forest, grassland, desert, highland, ice) classified from an equirectangular Earth map;
#   land vertices raised ~1.2% so the continents read as chunky low-poly. Colours go out as vertex colours (corner domain).
#   CloudPuff: a flattened icosphere (subdiv 1), white.
# Usage: blender -b --factory-startup -P build_lowpoly_earth.py -- <earth_map.png> <out_dir>
import bpy, bmesh, sys, os, math, random
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:]
src, out = argv[0], argv[1]
os.makedirs(out, exist_ok=True)
random.seed(7)

img = bpy.data.images.load(src)
W, H = img.size
px = np.empty(W * H * 4, dtype=np.float32)
img.pixels.foreach_get(px)
px = px.reshape(H, W, 4)

PAL = {
    "deep": (0.06, 0.20, 0.52), "shallow": (0.11, 0.42, 0.72), "seaice": (0.80, 0.90, 0.97),
    "forest": (0.13, 0.42, 0.17), "grass": (0.40, 0.64, 0.22), "desert": (0.88, 0.74, 0.44),
    "high": (0.56, 0.49, 0.40), "ice": (0.96, 0.98, 1.00),
}


def sample(d):
    lat = math.asin(max(-1.0, min(1.0, d.z)))
    lon = math.atan2(d.y, d.x)
    u = lon / (2 * math.pi) + 0.5
    v = lat / math.pi + 0.5
    x = min(W - 1, max(0, int(u * W)))
    y = min(H - 1, max(0, int(v * H)))
    r, g, b, _ = px[y, x]
    return float(r), float(g), float(b), math.degrees(lat)


def classify(r, g, b, lat):
    # tuned for NASA "Land Ocean Ice" (public domain): ocean ~(0,0,0.2), ice white, deserts pale yellow/tan
    lum = 0.3 * r + 0.59 * g + 0.11 * b
    if b > r + 0.06 and b > g + 0.03:
        return "seaice" if abs(lat) > 76 else "deep"
    if min(r, g, b) > 0.85 and b >= r - 0.05:
        return "ice"
    if r > 0.45 and (r - b) > 0.18:
        return "desert"
    if g >= r:
        return "forest" if lum < 0.3 else "grass"
    return "high"


def is_land(c):
    return c in ("forest", "grass", "desert", "high", "ice")


# ---- Earth
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=6, radius=1.0)   # 20,480 faces
ob = bpy.context.active_object
ob.name = "LowPolyEarth"
me = ob.data
bm = bmesh.new()
bm.from_mesh(me)
# raise land vertices (majority of a small neighbourhood is land)
for v in bm.verts:
    d = v.co.normalized()
    cls = classify(*sample(d))
    if is_land(cls):
        lift = 1.012 + (0.006 if cls in ("high", "ice") else 0.0)
        v.co = d * lift
    else:
        v.co = d
bm.to_mesh(me)
bm.free()
col = me.color_attributes.new(name="Col", type='BYTE_COLOR', domain='CORNER')
counts = {}
for poly in me.polygons:
    c = poly.center.normalized()
    cls = classify(*sample(c))
    if cls == "deep" and any(me.vertices[vi].co.length > 1.005 for vi in poly.vertices):
        cls = "shallow"
    counts[cls] = counts.get(cls, 0) + 1
    base = PAL[cls]
    k = 1.0 + random.uniform(-0.05, 0.05)   # faceted variation
    rgb = tuple(min(1.0, max(0.0, ch * k)) for ch in base)
    for li in poly.loop_indices:
        col.data[li].color = (rgb[0], rgb[1], rgb[2], 1.0)
me.color_attributes.active_color = col
for p in me.polygons:
    p.use_smooth = False
print("EARTH faces", len(me.polygons), counts)


def export(path, sel):
    bpy.ops.object.select_all(action='DESELECT')
    sel.select_set(True)
    bpy.context.view_layer.objects.active = sel
    kw = dict(filepath=path, export_format='GLB', use_selection=True)
    for opt in (dict(export_vertex_color='ACTIVE'), dict(export_colors=True), {}):
        try:
            bpy.ops.export_scene.gltf(**kw, **opt)
            print("EXPORTED", path, opt)
            return
        except TypeError:
            continue


export(os.path.join(out, "LowPolyEarth.glb"), ob)

# ---- cloud puff
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1.0)
p = bpy.context.active_object
p.name = "CloudPuff"
p.scale = (1.0, 0.8, 0.42)
bpy.ops.object.transform_apply(scale=True)
pc = p.data.color_attributes.new(name="Col", type='BYTE_COLOR', domain='CORNER')
for poly in p.data.polygons:
    s = 0.93 + 0.07 * max(0.0, poly.normal.z)
    for li in poly.loop_indices:
        pc.data[li].color = (s, s, s, 1.0)
p.data.color_attributes.active_color = pc
export(os.path.join(out, "CloudPuff.glb"), p)
