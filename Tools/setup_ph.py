# Unreal Editor Python: import Poly Haven (CC0) props/foliage + textures, build world materials.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_ph.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat"
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
MODELS = os.environ.get("IS_PH_MODELS", os.path.join(PROJ, "_downloads", "ph_clean"))   # downloads now live outside the project: C:/Dev/_downloads/ph_clean
TEX = os.environ.get("IS_PH_TEX", os.path.join(PROJ, "_downloads", "ph", "textures"))
FOLIAGE = {"fir_tree_01", "pine_tree_01", "tree_small_02", "shrub_01", "shrub_02", "shrub_03", "shrub_04", "island_tree_01", "jacaranda_tree", "potted_plant_01", "potted_plant_02", "potted_plant_04",
           "grass_medium_01", "grass_medium_02", "dandelion_01", "boulder_01", "rock_07"}

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty


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


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def new_mat(name):
    path = ROOT + "/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())


def finish(m):
    for flag in ("used_with_instanced_static_meshes",):
        try:
            m.set_editor_property(flag, True)
        except Exception as e:
            log("flag", e)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("material", m.get_name())


def make_world_material():
    m = new_mat("M_IS_World")
    tex = expr(m, unreal.MaterialExpressionTextureObjectParameter, -900, 0)
    tex.set_editor_property("parameter_name", "Tex")
    t0 = eal.load_asset(ROOT + "/PHTex/leafy_grass_diff")
    if t0:
        tex.set_editor_property("texture", t0)
    tile = expr(m, unreal.MaterialExpressionScalarParameter, -900, 200)
    tile.set_editor_property("parameter_name", "Tile")
    tile.set_editor_property("default_value", 200.0)
    a1 = expr(m, unreal.MaterialExpressionAppendVector, -700, 200)
    mel.connect_material_expressions(tile, "", a1, "A")
    mel.connect_material_expressions(tile, "", a1, "B")
    a2 = expr(m, unreal.MaterialExpressionAppendVector, -550, 200)
    mel.connect_material_expressions(a1, "", a2, "A")
    mel.connect_material_expressions(tile, "", a2, "B")
    fn = expr(m, unreal.MaterialExpressionMaterialFunctionCall, -400, 0)
    fn.set_editor_property("material_function", eal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture"))
    try:
        names = mel.get_material_expression_input_names(fn)
    except Exception:
        names = []
    log("WAT inputs", names)
    tex_in = next((n for n in names if "Texture" in n and "Size" not in n), "TextureObject")
    size_in = next((n for n in names if "Size" in n), "TextureSize")
    mel.connect_material_expressions(tex, "", fn, tex_in)
    mel.connect_material_expressions(a2, "", fn, size_in)
    tint = expr(m, unreal.MaterialExpressionVectorParameter, -400, 250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = expr(m, unreal.MaterialExpressionMultiply, -150, 50)
    mel.connect_material_expressions(fn, "XYZ Texture", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)
    r = expr(m, unreal.MaterialExpressionConstant, -150, 250)
    r.set_editor_property("r", 0.85)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    finish(m)


def make_glass_material():
    m = new_mat("M_IS_Glass")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    c = expr(m, unreal.MaterialExpressionVectorParameter, -500, 0)
    c.set_editor_property("parameter_name", "Color")
    c.set_editor_property("default_value", unreal.LinearColor(0.8, 0.85, 0.9, 1))
    o = expr(m, unreal.MaterialExpressionScalarParameter, -500, 200)
    o.set_editor_property("parameter_name", "Opacity")
    o.set_editor_property("default_value", 0.2)
    mel.connect_material_property(c, "", MP.MP_BASE_COLOR)
    mel.connect_material_property(o, "", MP.MP_OPACITY)
    r = expr(m, unreal.MaterialExpressionConstant, -500, 300)
    r.set_editor_property("r", 0.1)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    finish(m)


def make_foliage_material():
    m = new_mat("M_IS_Foliage")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    m.set_editor_property("two_sided", True)
    t = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    t.set_editor_property("parameter_name", "Tex")
    tint = expr(m, unreal.MaterialExpressionVectorParameter, -600, 300)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = expr(m, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(t, "RGB", mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)
    mel.connect_material_property(t, "A", MP.MP_OPACITY_MASK)
    r = expr(m, unreal.MaterialExpressionConstant, -300, 200)
    r.set_editor_property("r", 0.8)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    finish(m)


def import_textures():
    if not os.path.isdir(TEX): log("no texture downloads, skipping"); return
    for aid in sorted(os.listdir(TEX)):
        f = os.path.join(TEX, aid, aid + "_diff.jpg")
        if os.path.exists(f):
            import_file(f, ROOT + "/PHTex", aid + "_diff")
            log("tex", aid)


def base_texture(mi):
    """Find the colour texture used by an imported glTF material instance."""
    try:
        for p in mi.get_editor_property("texture_parameter_values"):
            n = str(p.parameter_info.name).lower()
            if p.parameter_value and ("base" in n or "color" in n or "diffuse" in n or "albedo" in n):
                return p.parameter_value
        for p in mi.get_editor_property("texture_parameter_values"):
            if p.parameter_value:
                return p.parameter_value
    except Exception as e:
        log("tex lookup", e)
    return None


def import_models():
    fol = eal.load_asset(ROOT + "/Materials/M_IS_Foliage")
    for fn in sorted(os.listdir(MODELS)):
        if not fn.endswith(".glb"):
            continue
        aid = fn[:-4]
        folder = ROOT + "/PH/" + aid
        target = folder + "/SM_" + aid
        if not eal.does_asset_exist(target):
            import_file(os.path.join(MODELS, fn), folder)
            sm = None
            for p in eal.list_assets(folder, recursive=True, include_folder=False):
                if isinstance(eal.load_asset(p), unreal.StaticMesh):
                    sm = p.split(".")[0]
                    break
            if not sm:
                log("NO MESH", aid)
                continue
            if sm != target:
                eal.rename_asset(sm, target)
        mesh = eal.load_asset(target)
        if not mesh:
            continue
        if aid in FOLIAGE and fol:
            # replace materials with an instancing-safe foliage material using the same colour texture
            mats = mesh.get_editor_property("static_materials")
            # the imported textures sit next to the mesh; match each slot by name: "<slot>_diff..." (the glTF
            # material may be a plain Material without texture parameters, so base_texture() can come back empty)
            texs = {}
            for p in eal.list_assets(folder, recursive=True, include_folder=False):
                a = eal.load_asset(p)
                if isinstance(a, unreal.Texture2D):
                    texs[a.get_name().lower()] = a
            for i, sm_mat in enumerate(mats):
                src = sm_mat.material_interface
                slot = str(sm_mat.material_slot_name).lower()
                tex = next((t for n, t in sorted(texs.items(), key=lambda kv: len(kv[0])) if n.startswith(slot + "_diff")), None)
                if tex is None:
                    tex = base_texture(src) if src else None
                log("foliage", aid, i, slot, tex.get_name() if tex else "NO TEXTURE")
                mi_path = folder + "/MI_%s_%d" % (aid, i)
                if eal.does_asset_exist(mi_path):
                    eal.delete_asset(mi_path)
                mi = at.create_asset("MI_%s_%d" % (aid, i), folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
                mi.set_editor_property("parent", fol)
                if tex:
                    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi, "Tex", tex)
                eal.save_loaded_asset(mi)
                mesh.set_material(i, mi)
            # automatic LODs so hundreds of trees stay cheap at distance
            if aid in ("fir_tree_01", "pine_tree_01", "tree_small_02", "shrub_01", "shrub_02", "shrub_04", "boulder_01"):
                opts = unreal.EditorScriptingMeshReductionOptions()
                opts.auto_compute_lod_screen_size = False
                opts.reduction_settings = [
                    unreal.EditorScriptingMeshReductionSettings(1.0, 1.0),
                    unreal.EditorScriptingMeshReductionSettings(0.25, 0.5),
                    unreal.EditorScriptingMeshReductionSettings(0.06, 0.2),
                    unreal.EditorScriptingMeshReductionSettings(0.015, 0.06),
                ]
                try:
                    n = unreal.EditorStaticMeshLibrary.set_lods(mesh, opts)
                    log("lods", aid, n)
                except Exception as e:
                    log("lod fail", aid, e)
            eal.save_loaded_asset(mesh)
        log("model", aid)


ONLY = os.environ.get("IS_ONLY", "")
if not ONLY or "tex" in ONLY:
    import_textures()
if not ONLY or "mat" in ONLY:
    make_world_material()   # NOTE: Tools/setup_pbr.py rebuilds M_IS_World with normal/roughness maps: run it after this step
    make_glass_material()
    make_foliage_material()
if not ONLY or "models" in ONLY:
    import_models()
log("PH DONE")
