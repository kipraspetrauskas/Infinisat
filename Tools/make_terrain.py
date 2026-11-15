# Unreal Editor Python: stylised material for the real Cesium terrain in the walking world, so the real Mahia hills
# and coast match the flat-coloured, low-poly look of the site (user choice "b"). The shape stays real; the colour
# comes from the shape: grass on gentle ground, rock on steep slopes, sand at the shore, sea below sea level; faces are
# flat-shaded (faceted). Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/make_terrain.py -unattended
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
PATH = "/Game/Infinisat/Materials/M_IS_Terrain"
if eal.does_asset_exist(PATH):
    eal.delete_asset(PATH)
m = at.create_asset("M_IS_Terrain", "/Game/Infinisat/Materials", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("tangent_space_normal", False)

wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 0)

def custom(code, out_type, x, y):
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("output_type", out_type)
    c.set_editor_property("code", code)
    i = unreal.CustomInput()
    i.set_editor_property("input_name", "P")
    c.set_editor_property("inputs", [i])
    mel.connect_material_expressions(wp, "", c, "P")
    return c

# the walking world's origin is the site floor (141 m above the WGS84 ellipsoid); the Earth's centre lies straight down
COMMON = """
float3 Fn = normalize(cross(ddy(P), ddx(P)));
if (Fn.z < 0) Fn = -Fn;
float3 C = float3(0.0, 0.0, -636974100.0);
float H = length(P - C) - 636960000.0;           // ~height above the ellipsoid, cm (sea level here is ~ +2100 cm)
float Up = dot(Fn, normalize(P - C));
"""

color = custom(COMMON + """
float Slope = 1.0 - saturate(Up);
float2 Cell = floor(P.xy / 5000.0);
float R = frac(sin(dot(Cell, float2(12.9898, 78.233))) * 43758.5453);
float3 Grass = lerp(float3(0.33, 0.53, 0.205), float3(0.37, 0.57, 0.225), R);
float3 Dry   = float3(0.55, 0.58, 0.30);
float3 Rock  = float3(0.56, 0.50, 0.42);
float3 Sand  = float3(0.86, 0.80, 0.60);
float3 Sea   = float3(0.12, 0.42, 0.62);
float3 Col = lerp(Grass, Dry, saturate((H - 25000.0) / 30000.0));   // higher ground drier
Col = lerp(Col, Rock, smoothstep(0.22, 0.42, Slope));                 // steep: bare rock and cliffs
Col = lerp(Sand, Col, smoothstep(3500.0, 6000.0, H));                 // the shore
if (H < 2600.0) Col = lerp(Sea, Sea * 0.7, saturate((2600.0 - H) / 3000.0));
return Col;
""", unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 0)

normal = custom(COMMON + "return Fn;", unreal.CustomMaterialOutputType.CMOT_FLOAT3, -500, 300)
rough = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 500)
rough.set_editor_property("r", 0.95)

mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(normal, "", unreal.MaterialProperty.MP_NORMAL)
mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m)
eal.save_asset(PATH)
unreal.log("M_IS_Terrain written")
