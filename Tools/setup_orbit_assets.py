# Unreal Editor Python: assets for the stylised orbit view.
#   /Game/Infinisat/LP/LowPolyEarth, /Game/Infinisat/LP/CloudPuff  (Tools/build_lowpoly_earth.py output, vertex colours)
#   /Game/Infinisat/Textures/T_SpacePano   (the user's 360 space panorama, 8K, no mips -> no seam)
#   M_IS_VertexColor  matte material from the mesh's vertex colours (Tint multiplier)
#   M_IS_SkyPano      unlit sky: equirectangular panorama looked up from the view direction (Intensity)
# Run (editor closed): UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_orbit_assets.py -unattended
import os
import unreal

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
EARTH = os.environ.get("IS_EARTH_DIR", "C:/Dev/_downloads/earth/out")
PANO = os.environ.get("IS_PANO", "C:/Dev/_downloads/sky/T_SpacePano.jpg")


def log(*a):
    unreal.log("[IS-ORBIT] " + " ".join(str(x) for x in a))


def imp(src, dest, name=None):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest; t.automated = True; t.replace_existing = True; t.save = True
    if name: t.destination_name = name
    at.import_asset_tasks([t])


for name in ("LowPolyEarth", "CloudPuff"):
    folder = "/Game/Infinisat/LP/" + name
    target = folder + "/SM_" + name
    if eal.does_asset_exist(target):
        eal.delete_directory(folder)
    imp(os.path.join(EARTH, name + ".glb"), folder)
    sm = next((p.split(".")[0] for p in eal.list_assets(folder, recursive=True, include_folder=False) if isinstance(eal.load_asset(p), unreal.StaticMesh)), None)
    if sm and sm != target:
        eal.rename_asset(sm, target)
    mesh = eal.load_asset(target)
    if mesh:
        ns = mesh.get_editor_property("nanite_settings")
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
        eal.save_loaded_asset(mesh)
    log("mesh", name, bool(mesh))

if os.path.exists(PANO):
    imp(PANO, "/Game/Infinisat/Textures", "T_SpacePano")
    tx = eal.load_asset("/Game/Infinisat/Textures/T_SpacePano")
    tx.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tx.set_editor_property("never_stream", True)
    tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
    eal.save_loaded_asset(tx)
    log("pano", tx.blueprint_get_size_x(), tx.blueprint_get_size_y())


def new_mat(name):
    path = "/Game/Infinisat/Materials/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    return at.create_asset(name, "/Game/Infinisat/Materials", unreal.Material, unreal.MaterialFactoryNew())


def e(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


# matte vertex colour
m = new_mat("M_IS_VertexColor")
m.set_editor_property("used_with_instanced_static_meshes", True)
vc = e(m, unreal.MaterialExpressionVertexColor, -600, 0)
tint = e(m, unreal.MaterialExpressionVectorParameter, -600, 200)
tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
mul = e(m, unreal.MaterialExpressionMultiply, -300, 50)
mel.connect_material_expressions(vc, "", mul, "A"); mel.connect_material_expressions(tint, "", mul, "B")
mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)
r = e(m, unreal.MaterialExpressionConstant, -300, 250); r.set_editor_property("r", 1.0)
mel.connect_material_property(r, "", MP.MP_ROUGHNESS)
sp = e(m, unreal.MaterialExpressionConstant, -300, 320); sp.set_editor_property("r", 0.25)
mel.connect_material_property(sp, "", MP.MP_SPECULAR)
mel.recompile_material(m); eal.save_loaded_asset(m); log("M_IS_VertexColor")

# panorama sky
m = new_mat("M_IS_SkyPano")
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
try:
    m.set_editor_property("is_sky", False)   # a real far sphere instead: the is_sky path drew over the scene
except Exception as ex:
    log("is_sky", ex)
cv = e(m, unreal.MaterialExpressionCameraVectorWS, -900, 0)
cu = e(m, unreal.MaterialExpressionCustom, -650, 0)
ci = unreal.CustomInput(); ci.set_editor_property("input_name", "CV")
cu.set_editor_property("inputs", [ci])
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
cu.set_editor_property("code", "float3 d = normalize(-CV); float u = atan2(d.y, d.x) / 6.2831853 + 0.5; float v = acos(clamp(d.z, -1.0, 1.0)) / 3.1415927; return float2(u, v);")
mel.connect_material_expressions(cv, "", cu, "CV")
tex = e(m, unreal.MaterialExpressionTextureSampleParameter2D, -400, 0)
tex.set_editor_property("parameter_name", "Pano")
t = eal.load_asset("/Game/Infinisat/Textures/T_SpacePano")
if t: tex.set_editor_property("texture", t)
tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
mel.connect_material_expressions(cu, "", tex, "UVs")
inten = e(m, unreal.MaterialExpressionScalarParameter, -400, 250)
inten.set_editor_property("parameter_name", "Intensity"); inten.set_editor_property("default_value", 1.0)
mul = e(m, unreal.MaterialExpressionMultiply, -150, 50)
mel.connect_material_expressions(tex, "RGB", mul, "A"); mel.connect_material_expressions(inten, "", mul, "B")
mel.connect_material_property(mul, "", MP.MP_EMISSIVE_COLOR)
mel.recompile_material(m); eal.save_loaded_asset(m); log("M_IS_SkyPano")
log("done")
