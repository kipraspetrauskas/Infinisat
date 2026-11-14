# Unreal Editor Python: imports NASA models/textures, creates materials and the main map.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_assets.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat"
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
CLEAN = os.path.join(PROJ, "_downloads", "clean")
RAW = os.path.join(PROJ, "_downloads", "raw")
DOCS = os.path.join(PROJ, "Docs")

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    unreal.log("[IS] " + " ".join(str(x) for x in a))


def import_file(src, dest_path, dest_name=None):
    t = unreal.AssetImportTask()
    t.filename = src
    t.destination_path = dest_path
    if dest_name:
        t.destination_name = dest_name
    t.automated = True
    t.replace_existing = True
    t.save = True
    at.import_asset_tasks([t])
    return list(t.imported_object_paths or [])


# ---------------------------------------------------------------- textures
def import_textures():
    tex = [
        (os.path.join(RAW, "T_Earth.jpg"), "T_Earth", False),
        (os.path.join(RAW, "T_Moon.jpg"), "T_Moon", False),
        (os.path.join(RAW, "T_Jupiter.jpg"), "T_Jupiter", False),
        (os.path.join(RAW, "T_Stars.jpg"), "T_Stars", False),
        (os.path.join(DOCS, "Logo.jpg"), "T_UI_Logo", True),
        (os.path.join(DOCS, "Character more detail.jpg"), "T_UI_Bit", True),
    ]
    for src, name, ui in tex:
        if not os.path.exists(src):
            log("missing", src)
            continue
        import_file(src, ROOT + "/Textures", name)
        a = eal.load_asset(ROOT + "/Textures/" + name)
        if a and ui:
            a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            a.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            eal.save_loaded_asset(a)
        log("texture", name, bool(a))


# ---------------------------------------------------------------- models
def import_models():
    for fn in sorted(os.listdir(CLEAN)):
        if not fn.lower().endswith(".glb"):
            continue
        name = os.path.splitext(fn)[0]
        folder = ROOT + "/Models/" + name
        target = folder + "/SM_" + name
        if eal.does_asset_exist(target):
            log("model exists", name)
            continue
        paths = import_file(os.path.join(CLEAN, fn), folder)
        # find the static mesh produced by the importer and give it a stable name
        sm = None
        for p in eal.list_assets(folder, recursive=True, include_folder=False):
            a = eal.load_asset(p)
            if isinstance(a, unreal.StaticMesh):
                sm = p
                break
        if sm is None:
            log("NO STATIC MESH for", name, paths)
            continue
        if sm.split(".")[0] != target:
            eal.rename_asset(sm.split(".")[0], target)
        log("model", name, "->", target)


# ---------------------------------------------------------------- materials
def new_material(name):
    path = ROOT + "/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def vparam(m, name, default, x, y):
    e = expr(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def sparam(m, name, default, x, y):
    e = expr(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def tparam(m, name, x, y, tex_path):
    e = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    t = eal.load_asset(tex_path)
    if t:
        e.set_editor_property("texture", t)
    return e


def mul(m, a, b, x, y, a_out="", b_out=""):
    e = expr(m, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, a_out, e, "A")
    mel.connect_material_expressions(b, b_out, e, "B")
    return e


def finish(m):
    for flag in ("used_with_instanced_static_meshes", "used_with_static_lighting"):
        try:
            m.set_editor_property(flag, True)
        except Exception as e:
            log("flag", flag, e)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("material", m.get_name())


def make_materials():
    MP = unreal.MaterialProperty
    # Lit colour material for satellite parts
    m = new_material("M_IS_Color")
    c = vparam(m, "Color", unreal.LinearColor(0.7, 0.7, 0.7, 1), -600, 0)
    met = sparam(m, "Metallic", 0.3, -600, 200)
    rough = sparam(m, "Roughness", 0.5, -600, 300)
    emi = sparam(m, "Emissive", 0.0, -600, 400)
    mel.connect_material_property(c, "", MP.MP_BASE_COLOR)
    mel.connect_material_property(met, "", MP.MP_METALLIC)
    mel.connect_material_property(rough, "", MP.MP_ROUGHNESS)
    mel.connect_material_property(mul(m, c, emi, -300, 400), "", MP.MP_EMISSIVE_COLOR)
    finish(m)

    # Unlit glowing colour (orbit lines, markers, UI-ish 3D elements)
    m = new_material("M_IS_Unlit")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    c = vparam(m, "Color", unreal.LinearColor(0.2, 0.8, 1.0, 1), -600, 0)
    k = sparam(m, "Intensity", 2.0, -600, 200)
    mel.connect_material_property(mul(m, c, k, -300, 100), "", MP.MP_EMISSIVE_COLOR)
    finish(m)

    # Planet: textured, lit, faint emissive so the night side is not pitch black
    m = new_material("M_IS_Planet")
    t = tparam(m, "Tex", -700, 0, ROOT + "/Textures/T_Earth")
    amb = sparam(m, "Ambient", 0.04, -700, 300)
    mel.connect_material_property(t, "RGB", MP.MP_BASE_COLOR)
    r = expr(m, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", 0.85)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    mel.connect_material_property(mul(m, t, amb, -300, 300, "RGB", ""), "", MP.MP_EMISSIVE_COLOR)
    finish(m)

    # Sky sphere: unlit two-sided star map
    m = new_material("M_IS_Sky")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("two_sided", True)
    t = tparam(m, "Tex", -700, 0, ROOT + "/Textures/T_Stars")
    b = sparam(m, "Brightness", 0.35, -700, 300)
    mel.connect_material_property(mul(m, t, b, -300, 100, "RGB", ""), "", MP.MP_EMISSIVE_COLOR)
    finish(m)

    # Atmosphere rim glow: additive, fresnel
    m = new_material("M_IS_Atmo")
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    c = vparam(m, "Color", unreal.LinearColor(0.25, 0.55, 1.0, 1), -700, 0)
    f = expr(m, unreal.MaterialExpressionFresnel, -700, 200)
    f.set_editor_property("exponent", 3.0)
    k = sparam(m, "Intensity", 1.5, -700, 400)
    mel.connect_material_property(mul(m, mul(m, c, f, -450, 100), k, -250, 200), "", MP.MP_EMISSIVE_COLOR)
    finish(m)


def make_text_material():
    # Hard reference to the engine's unlit text material so packaging cooks it.
    path = ROOT + "/Materials/MI_IS_Text"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mi = at.create_asset("MI_IS_Text", ROOT + "/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", eal.load_asset("/Engine/EngineMaterials/UnlitText"))
    eal.save_loaded_asset(mi)
    log("text material ok")


# ---------------------------------------------------------------- map
def make_map():
    path = ROOT + "/Maps/L_Space"
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not eal.does_asset_exist(path):
        les.new_level(path)
    les.load_level(path)
    les.save_current_level()
    log("map ok")


import sys
ONLY = os.environ.get("IS_ONLY", "")
if not ONLY or "tex" in ONLY: import_textures()
if not ONLY or "models" in ONLY: import_models()
if not ONLY or "mat" in ONLY: make_materials()
if not ONLY or "text" in ONLY: make_text_material()
if not ONLY or "map" in ONLY: make_map()
log("ALL DONE")
