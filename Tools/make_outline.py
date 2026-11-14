# Unreal Editor Python: post-process material that draws a thin yellow outline around anything written to Custom Depth
# (the object the player is aiming at). Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/make_outline.py -unattended
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
PATH = "/Game/Infinisat/Materials/M_IS_Outline"
if eal.does_asset_exist(PATH):
    eal.delete_asset(PATH)
m = at.create_asset("M_IS_Outline", "/Game/Infinisat/Materials", unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

# scene textures must exist in the graph for SceneTextureLookup() inside the custom node to compile
st_cd = mel.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -700, 0)
st_cd.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_CUSTOM_DEPTH)
st_pp = mel.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -700, 200)
st_pp.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -700, 400)

c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -350, 0)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
c.set_editor_property("code", """
float2 Px = View.ViewSizeAndInvSize.zw * 2.0;
float Mid = SceneTextureLookup(Uv, 13, false).r;
bool In = Mid < 1e6;
float Edge = 0;
const float2 D[8] = { float2(1,0), float2(-1,0), float2(0,1), float2(0,-1), float2(1,1), float2(-1,1), float2(1,-1), float2(-1,-1) };
for (int i = 0; i < 8; i++)
{
    bool N = SceneTextureLookup(Uv + D[i] * Px, 13, false).r < 1e6;
    if (N != In) Edge = 1;
}
float3 Col = SceneTextureLookup(Uv, 14, false).rgb;
return lerp(Col, float3(1.0, 0.86, 0.12), Edge * (In ? 0.0 : 1.0));
""")
inputs = unreal.CustomInput()
inputs.set_editor_property("input_name", "Uv")
dummy = unreal.CustomInput()
dummy.set_editor_property("input_name", "CdUnused")
dummy2 = unreal.CustomInput()
dummy2.set_editor_property("input_name", "PpUnused")
c.set_editor_property("inputs", [inputs, dummy, dummy2])
mel.connect_material_expressions(uv, "", c, "Uv")
mel.connect_material_expressions(st_cd, "Color", c, "CdUnused")
mel.connect_material_expressions(st_pp, "Color", c, "PpUnused")
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m)
eal.save_loaded_asset(m)
unreal.log("[IS] outline material done")
