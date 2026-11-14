# Blender: render the Infinisat logo artwork from public-domain NASA assets.
# Usage: blender -b --factory-startup -P logo_render.py -- <project_dir> <out_png> [earth_rot_deg]
import bpy, sys, os, math
from mathutils import Vector, Euler

argv = sys.argv[sys.argv.index("--") + 1:]
PROJ, OUT = argv[0], argv[1]
EARTH_ROT = float(argv[2]) if len(argv) > 2 else 200.0
RAW = os.path.join(PROJ, "_downloads", "raw")
CLEAN = os.path.join(PROJ, "_downloads", "clean")

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
try:
    prefs = bpy.context.preferences.addons['cycles'].preferences
    prefs.compute_device_type = 'OPTIX'
    prefs.get_devices()
    for d in prefs.devices: d.use = True
    sc.cycles.device = 'GPU'
except Exception as e:
    print("GPU setup failed", e)
sc.cycles.samples = int(argv[3]) if len(argv) > 3 else 160
sc.cycles.use_denoising = True
sc.render.resolution_x = sc.render.resolution_y = 2048
sc.view_settings.view_transform = 'AgX'
sc.view_settings.look = 'AgX - Punchy'

def emission_mat(name, color, strength):
    m = bpy.data.materials.new(name); m.use_nodes = True
    nt = m.node_tree; nt.nodes.clear()
    o = nt.nodes.new('ShaderNodeOutputMaterial'); e = nt.nodes.new('ShaderNodeEmission')
    e.inputs['Color'].default_value = (*color, 1); e.inputs['Strength'].default_value = strength
    nt.links.new(e.outputs[0], o.inputs[0]); return m

# --- world: NASA Tycho star map, dimmed
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
nt = w.node_tree; nt.nodes.clear()
env = nt.nodes.new('ShaderNodeTexEnvironment')
env.image = bpy.data.images.load(os.path.join(PROJ, "_downloads", "planets", "stars_milky_way.jpg"))
bg = nt.nodes.new('ShaderNodeBackground'); bg.inputs['Strength'].default_value = 0.35
out = nt.nodes.new('ShaderNodeOutputWorld')
nt.links.new(env.outputs[0], bg.inputs[0]); nt.links.new(bg.outputs[0], out.inputs[0])

# --- camera looks along +Y; screen X = world X, screen up = world Z
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
sc.collection.objects.link(cam); sc.camera = cam
cam.location = (0, -14, 0); cam.rotation_euler = (math.radians(90), 0, 0)
cam.data.lens = 58
cam.location = (0.25, -14, 0.35)

EARTH_C = Vector((0.75, 0, 0.35)); EARTH_R = 2.35
bpy.ops.mesh.primitive_uv_sphere_add(segments=192, ring_count=96, radius=EARTH_R, location=EARTH_C)
earth = bpy.context.active_object; bpy.ops.object.shade_smooth()
earth.rotation_euler = (math.radians(-20), 0, math.radians(EARTH_ROT))
PL = os.path.join(PROJ, "_downloads", "planets")
m = bpy.data.materials.new("Earth"); m.use_nodes = True
nt2 = m.node_tree
bsdf = nt2.nodes["Principled BSDF"]
tex = nt2.nodes.new('ShaderNodeTexImage'); tex.image = bpy.data.images.load(os.path.join(PL, "earth_day.jpg"))
night = nt2.nodes.new('ShaderNodeTexImage'); night.image = bpy.data.images.load(os.path.join(PL, "earth_night.jpg"))
nt2.links.new(tex.outputs[0], bsdf.inputs['Base Color'])
bsdf.inputs['Roughness'].default_value = 0.6
# city lights only on the night side: emission * smoothstep(-dot(N, L))
geo = nt2.nodes.new('ShaderNodeNewGeometry')
dot = nt2.nodes.new('ShaderNodeVectorMath'); dot.operation = 'DOT_PRODUCT'
dot.inputs[1].default_value = (0.75, 0.85, -0.45)
mr = nt2.nodes.new('ShaderNodeMapRange'); mr.inputs['From Min'].default_value = 0.0; mr.inputs['From Max'].default_value = 0.35
nt2.links.new(geo.outputs['Normal'], dot.inputs[0]); nt2.links.new(dot.outputs['Value'], mr.inputs['Value'])
mul = nt2.nodes.new('ShaderNodeMath'); mul.operation = 'MULTIPLY'; mul.inputs[1].default_value = 6.0
nt2.links.new(mr.outputs[0], mul.inputs[0])
nt2.links.new(night.outputs[0], bsdf.inputs['Emission Color'])
nt2.links.new(mul.outputs[0], bsdf.inputs['Emission Strength'])
earth.data.materials.append(m)

# cloud layer
bpy.ops.mesh.primitive_uv_sphere_add(segments=192, ring_count=96, radius=EARTH_R * 1.008, location=EARTH_C)
cl = bpy.context.active_object; bpy.ops.object.shade_smooth(); cl.rotation_euler = earth.rotation_euler
cm = bpy.data.materials.new("Clouds"); cm.use_nodes = True
cn = cm.node_tree; cb = cn.nodes["Principled BSDF"]
ct = cn.nodes.new('ShaderNodeTexImage'); ct.image = bpy.data.images.load(os.path.join(PL, "earth_clouds.jpg"))
cn.links.new(ct.outputs[0], cb.inputs['Alpha']); cb.inputs['Base Color'].default_value = (1, 1, 1, 1)
cl.data.materials.append(cm)

# atmosphere: fresnel rim glow
bpy.ops.mesh.primitive_uv_sphere_add(segments=128, ring_count=64, radius=EARTH_R * 1.035, location=EARTH_C)
atmo = bpy.context.active_object; bpy.ops.object.shade_smooth()
am = bpy.data.materials.new("Atmo"); am.use_nodes = True
an = am.node_tree; an.nodes.clear()
lw = an.nodes.new('ShaderNodeLayerWeight'); lw.inputs['Blend'].default_value = 0.35
pw = an.nodes.new('ShaderNodeMath'); pw.operation = 'POWER'; pw.inputs[1].default_value = 4.0
em = an.nodes.new('ShaderNodeEmission'); em.inputs['Color'].default_value = (0.35, 0.65, 1.0, 1); em.inputs['Strength'].default_value = 2.2
tr = an.nodes.new('ShaderNodeBsdfTransparent')
mx = an.nodes.new('ShaderNodeMixShader')
o = an.nodes.new('ShaderNodeOutputMaterial')
an.links.new(lw.outputs['Facing'], pw.inputs[0]); an.links.new(pw.outputs[0], mx.inputs[0])
an.links.new(tr.outputs[0], mx.inputs[1]); an.links.new(em.outputs[0], mx.inputs[2]); an.links.new(mx.outputs[0], o.inputs[0])
atmo.data.materials.append(am)
try: am.surface_render_method = 'BLENDED'
except Exception: pass

# sun from upper-left-front
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN'))
sun.data.energy = 4.5; sun.data.angle = math.radians(1.0)
light_dir = Vector((0.75, 0.85, -0.45)).normalized()   # direction the light travels
sun.rotation_euler = (-light_dir).to_track_quat('Z', 'Y').to_euler()
sc.collection.objects.link(sun)

def curve(name, pts, bevel, mat, closed=False, order_smooth=True):
    cd = bpy.data.curves.new(name, 'CURVE'); cd.dimensions = '3D'
    cd.bevel_depth = bevel; cd.bevel_resolution = 6
    sp = cd.splines.new('NURBS'); sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts): sp.points[i].co = (p[0], p[1], p[2], 1)
    sp.use_endpoint_u = not closed; sp.use_cyclic_u = closed; sp.order_u = 4
    ob = bpy.data.objects.new(name, cd); sc.collection.objects.link(ob)
    ob.data.materials.append(mat); ob.visible_shadow = False; return ob

# orbit ring low around Earth (as in the sketch)
orbit_pts = []
tilt = Euler((math.radians(14), math.radians(-12), 0))
for i in range(72):
    a = 2 * math.pi * i / 72
    v = Vector((math.cos(a) * 3.0, math.sin(a) * 3.0, 0)); v.rotate(tilt)
    orbit_pts.append(EARTH_C + v + Vector((0, 0, -0.75)))
curve("Orbit", orbit_pts, 0.016, emission_mat("OrbitM", (0.2, 0.75, 1.0), 3.5), closed=True)

# long launch streak: from the sail (bottom-left) up across Earth to the rocket (top-right)
SAIL = Vector((-2.05, -2.6, -1.9))
TIP = Vector((2.75, -2.2, 2.55))
mid = (SAIL + TIP) / 2 + Vector((0.15, -1.3, -0.25))
traj = [SAIL + (TIP - SAIL) * 0.08, SAIL + (TIP - SAIL) * 0.3 + Vector((0, -0.9, 0)), mid, TIP - (TIP - SAIL) * 0.2 + Vector((0, -0.6, 0)), TIP]
curve("TrajGlow", traj, 0.05, emission_mat("TrajGlowM", (1.0, 0.42, 0.06), 7))
curve("TrajCore", traj, 0.016, emission_mat("TrajCoreM", (1.0, 0.9, 0.7), 16))

# NASA Ares I rocket at the tip, nose along the streak
bpy.ops.import_scene.gltf(filepath=os.path.join(CLEAN, "AresI.glb"))
rk = [o for o in bpy.context.selected_objects if o.parent is None][0]
rk.rotation_mode = 'QUATERNION'
d = (TIP - traj[-2]).normalized()
d = (TIP - (TIP - (TIP - SAIL) * 0.2 + Vector((0, -0.6, 0)))).normalized()
rk.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(d)
rk.scale = (1.9,) * 3
rk.location = TIP + d * 0.92
bpy.ops.mesh.primitive_cone_add(radius1=0.05, radius2=0.0, depth=0.35, location=TIP - d * 0.1)
fl = bpy.context.active_object
fl.rotation_mode = 'QUATERNION'; fl.rotation_quaternion = Vector((0, 0, -1)).rotation_difference(d)
fl.data.materials.append(emission_mat("FlameM", (1.0, 0.6, 0.15), 25))

# solar-sail spacecraft at the end of the streak: NASA CubeSat body + segmented membrane
ORI = Euler((math.radians(62), math.radians(-28), math.radians(-24)))
root = bpy.data.objects.new("SailRoot", None); sc.collection.objects.link(root)
root.location = SAIL; root.rotation_euler = ORI
bpy.ops.import_scene.gltf(filepath=os.path.join(CLEAN, "CubeSat2U.glb"))
bus = [o for o in bpy.context.selected_objects if o.parent is None][0]
bus.rotation_mode = "XYZ"; bus.parent = root; bus.location = (0, 0, 0.08); bus.scale = (0.42,) * 3
bpy.ops.mesh.primitive_plane_add(size=1.0)
mem = bpy.context.active_object; mem.parent = root; mem.scale = (1.7, 1.7, 1)
mm = bpy.data.materials.new("Film"); mm.use_nodes = True
mb = mm.node_tree.nodes["Principled BSDF"]
mb.inputs['Base Color'].default_value = (0.95, 0.78, 0.45, 1); mb.inputs['Metallic'].default_value = 0.45
mb.inputs['Roughness'].default_value = 0.3
mb.inputs['Emission Color'].default_value = (1.0, 0.78, 0.42, 1); mb.inputs['Emission Strength'].default_value = 0.5
mem.data.materials.append(mm)
seam = bpy.data.materials.new("Seam"); seam.use_nodes = True
seam.node_tree.nodes["Principled BSDF"].inputs['Base Color'].default_value = (0.3, 0.25, 0.18, 1)
for k in range(5):
    t = -0.85 + k * 0.425
    for rot in (0, 90):
        loc = (t, 0, 0.006) if rot == 90 else (0, t, 0.006)
        bpy.ops.mesh.primitive_cylinder_add(radius=0.006 if k not in (0, 4) else 0.012, depth=1.7, location=loc, rotation=(0, math.radians(90), math.radians(rot)))
        c = bpy.context.active_object; c.parent = root; c.data.materials.append(seam)
for i in range(2):
    bpy.ops.mesh.primitive_cylinder_add(radius=0.013, depth=2.4, location=(0, 0, 0.012), rotation=(0, math.radians(90), math.radians(45 + 90 * i)))
    c = bpy.context.active_object; c.parent = root; c.data.materials.append(seam)
fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", 'POINT'))
fill.data.energy = 700; fill.location = SAIL + Vector((1.0, -2.5, 1.5)); sc.collection.objects.link(fill)

# glow (compositor) if available
try:
    sc.use_nodes = True
    ct = sc.node_tree
    rl = ct.nodes.get("Render Layers") or ct.nodes.new('CompositorNodeRLayers')
    comp = ct.nodes.get("Composite") or ct.nodes.new('CompositorNodeComposite')
    gl = ct.nodes.new('CompositorNodeGlare'); gl.glare_type = 'FOG_GLOW'
    try: gl.quality = 'HIGH'; gl.threshold = 1.0; gl.size = 8
    except Exception: pass
    ct.links.new(rl.outputs['Image'], gl.inputs[0]); ct.links.new(gl.outputs[0], comp.inputs[0])
except Exception as e:
    print("compositor skipped", e)

sc.render.filepath = OUT
sc.render.image_settings.file_format = 'PNG'
bpy.ops.render.render(write_still=True)
print("RENDERED", OUT)
