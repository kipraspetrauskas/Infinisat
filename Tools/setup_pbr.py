# Unreal Editor Python: full PBR for the world surfaces (demo look pass).
#  - imports Poly Haven normal (_nor) and roughness (_rough) maps next to the existing _diff maps in PHTex
#  - rebuilds M_IS_World: world-aligned colour + normal + roughness (Tex, Nor, Rough, Tile, Tint, RoughMul, NormalStrength)
#  - M_IS_Color: subtle world-space roughness variation so flat blocks stop looking like plastic
# Downloads: py -I Tools/polyhaven_dl.py C:/Dev/_downloads/ph textures:<ids>
# Run (editor closed): UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_pbr.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat"
TEX = os.environ.get("IS_PH_TEX", "C:/Dev/_downloads/ph/textures")

at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
WAT = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture"
WAN = "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedNormal"


def log(*a):
    unreal.log("[IS-PBR] " + " ".join(str(x) for x in a))


def import_tex(src, name, normal):
    t = unreal.AssetImportTask()
    t.filename = src
    t.destination_path = ROOT + "/PHTex"
    t.destination_name = name
    t.automated = True
    t.replace_existing = True
    t.save = False
    at.import_asset_tasks([t])
    tex = eal.load_asset(ROOT + "/PHTex/" + name)
    if not tex:
        log("FAILED import", name)
        return
    tex.set_editor_property("srgb", False)
    if normal:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("flip_green_channel", True)   # Poly Haven nor_gl is OpenGL (+Y); UE is DirectX
    else:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    eal.save_loaded_asset(tex)
    log("tex", name)


def import_maps():
    if not os.path.isdir(TEX):
        log("no downloads at", TEX)
        return
    for aid in sorted(os.listdir(TEX)):
        for key, normal in (("nor", True), ("rough", False)):
            f = os.path.join(TEX, aid, f"{aid}_{key}.jpg")
            if os.path.exists(f):
                import_tex(f, f"{aid}_{key}", normal)


def expr(m, cls, x, y):
    return mel.create_material_expression(m, cls, x, y)


def sparam(m, name, v, x, y):
    p = expr(m, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", v)
    return p


def tobj(m, name, default, x, y, sampler=None):
    p = expr(m, unreal.MaterialExpressionTextureObjectParameter, x, y)
    p.set_editor_property("parameter_name", name)
    t = unreal.load_asset(default)
    if t:
        p.set_editor_property("texture", t)
    if sampler is not None:
        p.set_editor_property("sampler_type", sampler)
    return p


def func(m, path, x, y):
    f = expr(m, unreal.MaterialExpressionMaterialFunctionCall, x, y)
    f.set_editor_property("material_function", unreal.load_asset(path))
    return f


def connect_any(src, outs, dst, dst_in):
    for o in outs:
        if mel.connect_material_expressions(src, o, dst, dst_in):
            return o
    log("could not connect", outs, "->", dst_in)
    return None


def wa(m, path, tex, size, x, y):
    f = func(m, path, x, y)
    names = mel.get_material_expression_input_names(f)
    tex_in = next((n for n in names if "Texture" in n and "Size" not in n), "TextureObject")
    size_in = next((n for n in names if "Size" in n), "TextureSize")
    mel.connect_material_expressions(tex, "", f, tex_in)
    mel.connect_material_expressions(size, "", f, size_in)
    return f


def rebuild_world():
    path = ROOT + "/Materials/M_IS_World"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    m = at.create_asset("M_IS_World", ROOT + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("tangent_space_normal", False)   # WorldAlignedNormal outputs a world-space normal
    tile = sparam(m, "Tile", 200.0, -1300, 300)
    a1 = expr(m, unreal.MaterialExpressionAppendVector, -1100, 300)
    mel.connect_material_expressions(tile, "", a1, "A")
    mel.connect_material_expressions(tile, "", a1, "B")
    size = expr(m, unreal.MaterialExpressionAppendVector, -950, 300)
    mel.connect_material_expressions(a1, "", size, "A")
    mel.connect_material_expressions(tile, "", size, "B")

    # colour
    tex = tobj(m, "Tex", ROOT + "/PHTex/leafy_grass_diff", -1100, -200)
    col = wa(m, WAT, tex, size, -700, -200)
    tint = expr(m, unreal.MaterialExpressionVectorParameter, -700, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mul = expr(m, unreal.MaterialExpressionMultiply, -350, -150)
    connect_any(col, ["XYZ Texture"], mul, "A")
    mel.connect_material_expressions(tint, "", mul, "B")
    mel.connect_material_property(mul, "", MP.MP_BASE_COLOR)

    # roughness: map (white mask default: a Masks sampler can't default to a colour texture) * RoughMul
    rtex = tobj(m, "Rough", ROOT + "/PHTex/T_IS_WhiteMask", -1100, 500, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    rs = wa(m, WAT, rtex, size, -700, 500)
    rmul = sparam(m, "RoughMul", 0.85, -700, 700)
    rm = expr(m, unreal.MaterialExpressionMultiply, -350, 550)
    connect_any(rs, ["XYZ Texture"], rm, "A")
    mel.connect_material_expressions(rmul, "", rm, "B")
    mask = expr(m, unreal.MaterialExpressionComponentMask, -200, 550)
    mask.set_editor_property("r", True)
    mel.connect_material_expressions(rm, "", mask, "")
    mel.connect_material_property(mask, "", MP.MP_ROUGHNESS)

    # normal: world aligned, blended toward the flat vertex normal by NormalStrength
    ntex = tobj(m, "Nor", "/Engine/EngineMaterials/FlatNormal", -1100, 900, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    nf = wa(m, WAN, ntex, size, -700, 900)
    vn = expr(m, unreal.MaterialExpressionVertexNormalWS, -700, 1100)
    ns = sparam(m, "NormalStrength", 1.0, -700, 1200)
    lerp = expr(m, unreal.MaterialExpressionLinearInterpolate, -350, 950)
    mel.connect_material_expressions(vn, "", lerp, "A")
    connect_any(nf, ["XYZ Texture", "Normal", "XYZ"], lerp, "B")
    mel.connect_material_expressions(ns, "", lerp, "Alpha")
    mel.connect_material_property(lerp, "", MP.MP_NORMAL)

    m.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("M_IS_World rebuilt")


def vary_color_roughness():
    m = eal.load_asset(ROOT + "/Materials/M_IS_Color")
    if not m:
        log("no M_IS_Color")
        return
    rough = mel.get_material_property_input_node(m, MP.MP_ROUGHNESS)
    if not rough or rough.get_class().get_name() != "MaterialExpressionScalarParameter":
        log("M_IS_Color roughness already varied or unexpected:", rough.get_class().get_name() if rough else None)
        return
    noise = expr(m, unreal.MaterialExpressionNoise, -600, 600)
    noise.set_editor_property("scale", 0.02)            # ~50 cm blotches in world space
    noise.set_editor_property("levels", 3)
    noise.set_editor_property("output_min", 0.85)
    noise.set_editor_property("output_max", 1.15)
    vm = expr(m, unreal.MaterialExpressionMultiply, -300, 450)
    mel.connect_material_expressions(rough, "", vm, "A")
    mel.connect_material_expressions(noise, "", vm, "B")
    sat = expr(m, unreal.MaterialExpressionSaturate, -150, 450)
    mel.connect_material_expressions(vm, "", sat, "")
    mel.connect_material_property(sat, "", MP.MP_ROUGHNESS)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    log("M_IS_Color roughness varied")


def white_mask():
    """4x4 white PNG imported as a linear Masks texture: the default for the Rough slot."""
    import struct, zlib
    path = ROOT + "/PHTex/T_IS_WhiteMask"
    if eal.does_asset_exist(path):
        return
    raw = b"".join(b"\x00" + b"\xff\xff\xff" * 4 for _ in range(4))
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
    f = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "T_IS_WhiteMask.png")
    open(f, "wb").write(png)
    import_tex(f, "T_IS_WhiteMask", False)


def fix_nanite_usage():
    """Imported glTF/CAD materials lack 'Used with Nanite', so cooked builds would draw them with the default material."""
    n = 0
    for p in eal.list_assets(ROOT, recursive=True, include_folder=False):
        if not p.split(".")[0].rsplit("/", 1)[-1]:
            continue
        a = eal.find_asset_data(p)
        if str(a.asset_class_path.asset_name) != "Material":
            continue
        m = eal.load_asset(p)
        if m and not m.get_editor_property("used_with_nanite"):
            m.set_editor_property("used_with_nanite", True)
            eal.save_loaded_asset(m)
            n += 1
    log("nanite usage set on", n, "materials")


ONLY = os.environ.get("IS_ONLY", "")
if not ONLY or "maps" in ONLY:
    import_maps()
white_mask()
rebuild_world()
vary_color_roughness()
if not ONLY or "nanite" in ONLY:
    fix_nanite_usage()
log("done")
