# Unreal Editor Python: give every Poly Haven foliage mesh a foliage material instance with its own colour texture
# (found by name next to the mesh: "<slot>_diff..."). Fixes invisible leaves / black grass on meshes imported earlier.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/fix_foliage.py -unattended
import unreal

eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "/Game/Infinisat"
FOLIAGE = ["fir_tree_01", "pine_tree_01", "tree_small_02", "shrub_01", "shrub_02", "shrub_03", "shrub_04",
           "island_tree_01", "jacaranda_tree", "grass_medium_01", "grass_medium_02", "dandelion_01",
           "potted_plant_01", "potted_plant_02", "potted_plant_04"]

fol = eal.load_asset(ROOT + "/Materials/M_IS_Foliage")
for aid in FOLIAGE:
    folder = ROOT + "/PH/" + aid
    mesh = eal.load_asset(folder + "/SM_" + aid)
    if not mesh:
        unreal.log("[IS] missing " + aid)
        continue
    texs = {}
    for p in eal.list_assets(folder, recursive=True, include_folder=False):
        a = eal.load_asset(p)
        if isinstance(a, unreal.Texture2D):
            texs[a.get_name().lower()] = a
    diffs = sorted([n for n in texs if "_diff" in n], key=len)
    mats = mesh.get_editor_property("static_materials")
    for i, sm_mat in enumerate(mats):
        slot = str(sm_mat.material_slot_name).lower()
        name = next((n for n in diffs if n.startswith(slot + "_diff")), None)
        if name is None and len(mats) == 1 and diffs:
            name = diffs[0]
        if name is None:
            # wood slots without their own texture (dead branches, extra trunk pieces): use a bark/trunk texture
            name = next((n for n in diffs if "bark" in n or "trunk" in n or "branch" in n), None)
        tex = texs.get(name) if name else None
        mi_name = "MI_%s_%d" % (aid, i)
        mi_path = folder + "/" + mi_name
        if eal.does_asset_exist(mi_path):
            eal.delete_asset(mi_path)
        mi = at.create_asset(mi_name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", fol)
        if tex:
            unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi, "Tex", tex)
        eal.save_loaded_asset(mi)
        mesh.set_material(i, mi)
        unreal.log("[IS] %s slot %d %s -> %s" % (aid, i, slot, name or "NO TEXTURE"))
    # Nanite's fallback mesh (used when Nanite is off, e.g. the Low preset) decimates thin leaves away completely:
    # foliage uses classic LODs instead so the leaves are always there
    ns = mesh.get_editor_property("nanite_settings")
    if ns.enabled:
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
    opts = unreal.EditorScriptingMeshReductionOptions()
    opts.auto_compute_lod_screen_size = False
    opts.reduction_settings = [
        unreal.EditorScriptingMeshReductionSettings(1.0, 1.0),
        unreal.EditorScriptingMeshReductionSettings(0.35, 0.45),
        unreal.EditorScriptingMeshReductionSettings(0.12, 0.18),
        unreal.EditorScriptingMeshReductionSettings(0.04, 0.06),
    ]
    try:
        unreal.EditorStaticMeshLibrary.set_lods(mesh, opts)
    except Exception as e:
        unreal.log("[IS] lod fail %s %s" % (aid, e))
    unreal.log("[IS] %s nanite off, LODs %d" % (aid, mesh.get_num_lods()))
    eal.save_loaded_asset(mesh)
unreal.log("[IS] FOLIAGE FIXED")
