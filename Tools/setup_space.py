# Unreal Editor Python: realistic space assets - hi-res sphere, Earth day/night/cloud materials, planet maps.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_space.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat"
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
PL = os.path.join(PROJ, "_downloads", "planets")
at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty


def log(*a):
    unreal.log("[IS] " + " ".join(str(x) for x in a))


def imp(src, dest, name):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest
    if name:
        t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    at.import_asset_tasks([t])


def new_mat(name):
    path = ROOT + "/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())


def ex(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def tex_param(m, name, path, x, y):
    e = ex(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    t = eal.load_asset(path)
    if t:
        e.set_editor_property("texture", t)
    return e


def finish(m):
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("material", m.get_name())


# ---- textures
for f, n in [] if os.environ.get("IS_SKIPTEX") else [("earth_day.jpg", "T_EarthDay"), ("earth_night.jpg", "T_EarthNight"), ("earth_clouds.jpg", "T_EarthClouds"),
             ("moon_ssc.jpg", "T_Moon2"), ("jupiter.jpg", "T_Jupiter2"), ("mars.jpg", "T_Mars"), ("venus_atmosphere.jpg", "T_Venus"),
             ("saturn.jpg", "T_Saturn"), ("uranus.jpg", "T_Uranus"), ("neptune.jpg", "T_Neptune"), ("mercury.jpg", "T_Mercury"),
             ("sun.jpg", "T_Sun"), ("stars_milky_way.jpg", "T_MilkyWay")]:
    p = os.path.join(PL, f)
    if os.path.exists(p):
        imp(p, ROOT + "/Textures", n)
        t = eal.load_asset(ROOT + "/Textures/" + n)
        if t and n == "T_EarthClouds":
            t.set_editor_property("srgb", False)
            eal.save_loaded_asset(t)
        log("tex", n)

# ---- hi-res sphere (Blender-made GLB, 100 uu diameter like the engine sphere)
sph = os.path.join(PROJ, "_downloads", "sphere_hires.glb")
if os.path.exists(sph):
    imp(sph, ROOT + "/Meshes", None)
    for p in eal.list_assets(ROOT + "/Meshes", recursive=True, include_folder=False):
        a = eal.load_asset(p)
        if isinstance(a, unreal.StaticMesh) and not p.split(".")[0].endswith("SM_SphereHi"):
            eal.rename_asset(p.split(".")[0], ROOT + "/Meshes/SM_SphereHi")
            break
    sm = eal.load_asset(ROOT + "/Meshes/SM_SphereHi")
    if sm:
        ns = sm.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", True)
        try:
            ns.set_editor_property("fallback_percent_triangles", 1.0)
        except Exception as e:
            log("fallback", e)
        sm.set_editor_property("nanite_settings", ns)
        eal.save_loaded_asset(sm)
    log("sphere", eal.does_asset_exist(ROOT + "/Meshes/SM_SphereHi"))

# ---- Earth: day map lit by the sun, city lights only on the night side (SunDir parameter from C++)
m = new_mat("M_IS_Earth")
day = tex_param(m, "Day", ROOT + "/Textures/T_EarthDay", -900, -100)
night = tex_param(m, "Night", ROOT + "/Textures/T_EarthNight", -900, 250)
sun = ex(m, unreal.MaterialExpressionVectorParameter, -900, 600)
sun.set_editor_property("parameter_name", "SunDir")
sun.set_editor_property("default_value", unreal.LinearColor(1, 0, 0, 0))
nrm = ex(m, unreal.MaterialExpressionVertexNormalWS, -900, 800)
dot = ex(m, unreal.MaterialExpressionDotProduct, -650, 650)
mel.connect_material_expressions(nrm, "", dot, "A")
mel.connect_material_expressions(sun, "", dot, "B")
neg = ex(m, unreal.MaterialExpressionOneMinus, -500, 650)
mel.connect_material_expressions(dot, "", neg, "")
sat = ex(m, unreal.MaterialExpressionSaturate, -380, 650)
mel.connect_material_expressions(neg, "", sat, "")
pw = ex(m, unreal.MaterialExpressionPower, -260, 650)
mel.connect_material_expressions(sat, "", pw, "Base")
e8 = ex(m, unreal.MaterialExpressionConstant, -380, 780); e8.set_editor_property("r", 6.0)
mel.connect_material_expressions(e8, "", pw, "Exp")
nl = ex(m, unreal.MaterialExpressionMultiply, -150, 300)
mel.connect_material_expressions(night, "RGB", nl, "A")
mel.connect_material_expressions(pw, "", nl, "B")
k = ex(m, unreal.MaterialExpressionScalarParameter, -150, 450); k.set_editor_property("parameter_name", "NightGlow"); k.set_editor_property("default_value", 2.5)
nl2 = ex(m, unreal.MaterialExpressionMultiply, 50, 300)
mel.connect_material_expressions(nl, "", nl2, "A")
mel.connect_material_expressions(k, "", nl2, "B")
mel.connect_material_property(day, "RGB", MP.MP_BASE_COLOR)
mel.connect_material_property(nl2, "", MP.MP_EMISSIVE_COLOR)
r = ex(m, unreal.MaterialExpressionConstant, 50, 0); r.set_editor_property("r", 0.7)
mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
sp = ex(m, unreal.MaterialExpressionConstant, 50, 100); sp.set_editor_property("r", 0.3)
mel.connect_material_property(sp, "", MP.MP_SPECULAR)
finish(m)

# ---- clouds: translucent white layer, alpha from the cloud map
m = new_mat("M_IS_Clouds")
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
c = tex_param(m, "Clouds", ROOT + "/Textures/T_EarthClouds", -600, 0)
w = ex(m, unreal.MaterialExpressionConstant3Vector, -600, 250); w.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
mel.connect_material_property(w, "", MP.MP_BASE_COLOR)
op = ex(m, unreal.MaterialExpressionMultiply, -300, 100)
mel.connect_material_expressions(c, "R", op, "A")
o9 = ex(m, unreal.MaterialExpressionConstant, -450, 200); o9.set_editor_property("r", 0.75)
mel.connect_material_expressions(o9, "", op, "B")
mel.connect_material_property(op, "", MP.MP_OPACITY)
finish(m)
log("SPACE DONE")
