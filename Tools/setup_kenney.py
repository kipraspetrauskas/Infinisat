# Unreal Editor Python: import Kenney CC0 kits (stylised open-world look) and a stylised ground material.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_kenney.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat"
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
K = os.path.join(PROJ, "_downloads", "kenney_vc")
KITS = {k: (k, None) for k in ("furn", "sub", "road", "car", "nat")}
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


def new_mat(name):
    path = ROOT + "/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def finish(m):
    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("material", m.get_name())


def make_kenney_material():
    """Kenney models colour themselves from a small palette texture: sample it, soft matte finish."""
    m = new_mat("M_IS_Kenney")
    m.set_editor_property("two_sided", True)
    t = expr(m, unreal.MaterialExpressionVertexColor, -600, 0)
    c = expr(m, unreal.MaterialExpressionVectorParameter, -600, 250)
    c.set_editor_property("parameter_name", "Tint")
    c.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = expr(m, unreal.MaterialExpressionMultiply, -300, 50)
    mel.connect_material_expressions(t, "", mul, "A")
    mel.connect_material_expressions(c, "", mul, "B")
    mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)
    r = expr(m, unreal.MaterialExpressionConstant, -300, 250)
    r.set_editor_property("r", 0.75)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    finish(m)


def make_ground_material():
    """Astroneer-style ground: two soft colours blended by large-scale noise, no photo texture."""
    m = new_mat("M_IS_Ground")
    wp = expr(m, unreal.MaterialExpressionWorldPosition, -1000, 0)
    n1 = expr(m, unreal.MaterialExpressionNoise, -800, 0)
    n1.set_editor_property("scale", 0.0004)
    n1.set_editor_property("levels", 3)
    n1.set_editor_property("output_min", 0.0)
    n1.set_editor_property("output_max", 1.0)
    mel.connect_material_expressions(wp, "", n1, "Position")
    a = expr(m, unreal.MaterialExpressionVectorParameter, -800, 250)
    a.set_editor_property("parameter_name", "ColorA")
    a.set_editor_property("default_value", unreal.LinearColor(0.32, 0.62, 0.22, 1))
    b = expr(m, unreal.MaterialExpressionVectorParameter, -800, 400)
    b.set_editor_property("parameter_name", "ColorB")
    b.set_editor_property("default_value", unreal.LinearColor(0.55, 0.75, 0.25, 1))
    lp = expr(m, unreal.MaterialExpressionLinearInterpolate, -450, 200)
    mel.connect_material_expressions(a, "", lp, "A")
    mel.connect_material_expressions(b, "", lp, "B")
    mel.connect_material_expressions(n1, "", lp, "Alpha")
    mel.connect_material_property(lp, "", MP.MP_BASE_COLOR)
    r = expr(m, unreal.MaterialExpressionConstant, -450, 400)
    r.set_editor_property("r", 0.9)
    mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
    finish(m)


def base_texture(mi):
    try:
        for p in mi.get_editor_property("texture_parameter_values"):
            if p.parameter_value:
                return p.parameter_value
    except Exception:
        pass
    return None


def import_kits():
    km = eal.load_asset(ROOT + "/Materials/M_IS_Kenney")
    for key, (sub, filt) in KITS.items():
        src = os.path.join(K, sub)
        if not os.path.isdir(src):
            log("missing kit", sub)
            continue
        for fn in sorted(os.listdir(src)):
            if not fn.endswith(".glb"):
                continue
            name = fn[:-4]
            safe = name.replace("-", "_")
            folder = "%s/K/%s/%s" % (ROOT, key, safe)
            target = folder + "/SM_" + safe
            if not eal.does_asset_exist(target):
                import_file(os.path.join(src, fn), folder)
                sm = None
                for p in eal.list_assets(folder, recursive=True, include_folder=False):
                    if isinstance(eal.load_asset(p), unreal.StaticMesh):
                        sm = p.split(".")[0]
                        break
                if not sm:
                    log("NO MESH", name)
                    continue
                if sm != target:
                    eal.rename_asset(sm, target)
            mesh = eal.load_asset(target)
            if not mesh or not km:
                continue
            for i, slot in enumerate(mesh.get_editor_property("static_materials")):
                mesh.set_material(i, km)
            eal.save_loaded_asset(mesh)
        log("kit done", key)


make_kenney_material()
make_ground_material()
if os.environ.get("IS_ONLY", "") != "mat":
    import_kits()
log("KENNEY DONE")
