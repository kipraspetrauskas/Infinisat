# Unreal Editor Python: materials for the Cosmos explorer (all textures are generated at runtime in C++).
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_cosmos.py -unattended
import unreal

ROOT = "/Game/Infinisat"
at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty


def log(*a):
    unreal.log("[IS] " + " ".join(str(x) for x in a))


def new_material(name):
    path = ROOT + "/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def sparam(m, name, default, x, y):
    e = expr(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def vparam(m, name, default, x, y):
    e = expr(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def tparam(m, name, x, y):
    e = expr(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    e.set_editor_property("parameter_name", name)
    t = eal.load_asset(ROOT + "/Textures/T_Sun")
    if t:
        e.set_editor_property("texture", t)
    return e


def mul(m, a, b, x, y, a_out="", b_out=""):
    e = expr(m, unreal.MaterialExpressionMultiply, x, y)
    mel.connect_material_expressions(a, a_out, e, "A")
    mel.connect_material_expressions(b, b_out, e, "B")
    return e


def finish(m):
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("material", m.get_name())


# Additive glow: sun surface + corona, star glows, nebulae, galaxies, accretion disks. Emissive = Tex * Color * Intensity.
m = new_material("M_IS_Glow")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
t = tparam(m, "Tex", -900, 0)
c = vparam(m, "Color", unreal.LinearColor(1, 1, 1, 1), -900, 300)
k = sparam(m, "Intensity", 1.0, -900, 450)
mel.connect_material_property(mul(m, mul(m, t, c, -600, 100, "RGB", ""), k, -350, 200), "", MP.MP_EMISSIVE_COLOR)
finish(m)

# Translucent rings: Saturn/Uranus rings, debris disks. Colour from RGB, density from alpha.
m = new_material("M_IS_Ring")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("two_sided", True)
t = tparam(m, "Tex", -900, 0)
b = sparam(m, "Brightness", 1.0, -900, 300)
o = sparam(m, "Opacity", 1.0, -900, 450)
mel.connect_material_property(mul(m, t, b, -500, 100, "RGB", ""), "", MP.MP_EMISSIVE_COLOR)
mel.connect_material_property(mul(m, t, o, -500, 300, "A", ""), "", MP.MP_OPACITY)
finish(m)
