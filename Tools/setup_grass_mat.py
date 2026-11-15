# Unreal Editor Python: M_IS_Grass - stylised grass blades (two-sided, matte), dark root -> bright tip by local height,
# a little per-instance hue variation. Parameters: Height (local mesh units), Root, Tip.
# Run (editor closed): UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_grass_mat.py -unattended
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
at = unreal.AssetToolsHelpers.get_asset_tools()
path = "/Game/Infinisat/Materials/M_IS_Grass"
if eal.does_asset_exist(path):
    eal.delete_asset(path)
m = at.create_asset("M_IS_Grass", "/Game/Infinisat/Materials", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("two_sided", True)
m.set_editor_property("tangent_space_normal", False)   # world-space normal: every blade lit like the ground (no dark backlit faces)
m.set_editor_property("used_with_instanced_static_meshes", True)


def e(cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def vparam(name, c, x, y):
    p = e(unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", unreal.LinearColor(*c))
    return p


def sparam(name, v, x, y):
    p = e(unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", v)
    return p


root = vparam("Root", (0.16, 0.34, 0.05, 1), -900, -200)
tip = vparam("Tip", (0.46, 0.74, 0.14, 1), -900, 0)
lp = e(unreal.MaterialExpressionLocalPosition, -1100, 250)
mask = e(unreal.MaterialExpressionComponentMask, -900, 250)
mask.set_editor_property("b", True)
mel.connect_material_expressions(lp, "", mask, "")
h = sparam("Height", 100.0, -900, 400)
div = e(unreal.MaterialExpressionDivide, -700, 300)
mel.connect_material_expressions(mask, "", div, "A")
mel.connect_material_expressions(h, "", div, "B")
sat = e(unreal.MaterialExpressionSaturate, -550, 300)
mel.connect_material_expressions(div, "", sat, "")
lerp = e(unreal.MaterialExpressionLinearInterpolate, -400, 0)
mel.connect_material_expressions(root, "", lerp, "A")
mel.connect_material_expressions(tip, "", lerp, "B")
mel.connect_material_expressions(sat, "", lerp, "Alpha")
# per-instance variation: 0.85 .. 1.15 brightness
pir = e(unreal.MaterialExpressionPerInstanceRandom, -700, 550)
var = e(unreal.MaterialExpressionLinearInterpolate, -500, 550)
lo = e(unreal.MaterialExpressionConstant, -700, 650); lo.set_editor_property("r", 0.92)
hi = e(unreal.MaterialExpressionConstant, -700, 720); hi.set_editor_property("r", 1.08)
mel.connect_material_expressions(lo, "", var, "A")
mel.connect_material_expressions(hi, "", var, "B")
mel.connect_material_expressions(pir, "", var, "Alpha")
mul = e(unreal.MaterialExpressionMultiply, -200, 100)
mel.connect_material_expressions(lerp, "", mul, "A")
mel.connect_material_expressions(var, "", mul, "B")
mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)
r = e(unreal.MaterialExpressionConstant, -200, 300); r.set_editor_property("r", 1.0)
mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
sp = e(unreal.MaterialExpressionConstant, -200, 380); sp.set_editor_property("r", 0.15)
mel.connect_material_property(sp, "", MP.MP_SPECULAR)
up = e(unreal.MaterialExpressionConstant3Vector, -200, 460)
up.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
mel.connect_material_property(up, "", MP.MP_NORMAL)
mel.recompile_material(m)
eal.save_loaded_asset(m)
unreal.log("[IS-GRASS] M_IS_Grass built")
