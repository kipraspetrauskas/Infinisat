# Unreal Editor Python: import the user's handmade low-poly packs into /Game/Infinisat/<KIT>/<name>/SM_<name>.
#   KB = Kenney Building Kit (house shell), KK = KayKit Furniture Bits, KD = KayKit Dungeon (household-looking props only),
#   KF = KayKit Forest Nature, QN = Quaternius Ultimate Nature, VEH = vehicles.
# Nanite off (tiny low-poly meshes; imported materials lack the Nanite usage flag).
# Run (editor closed): UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_packs.py -unattended
import os, glob
import unreal

U = os.environ.get("IS_USER_PACKS", "C:/Dev/_downloads/user")
at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary

DUNGEON = "barrel_large barrel_small barrel_small_stack box_large box_small box_small_decorated box_stacked crates_stacked keg shelf_large shelf_small shelves " \
          "bottle_A_brown bottle_A_green bottle_B_green bottle_C_brown plate plate_food_A plate_food_B plate_stack trunk_medium_A trunk_small_A stool table_small candle".split()
QUAT = ["CommonTree_%d" % i for i in range(1, 6)] + ["BirchTree_%d" % i for i in range(1, 6)] + ["PineTree_%d" % i for i in range(1, 6)] + \
       ["Bush_1", "Bush_2", "BushBerries_1", "BushBerries_2", "Flowers", "Grass", "Grass_2", "Grass_Short"] + ["Plant_%d" % i for i in range(1, 6)] + ["Rock_%d" % i for i in range(1, 8)]

SETS = [
    ("KB", U + "/kenney_building/Models/GLB format", "glb", None),
    ("KK", U + "/KayKit_Furniture_Bits_1.0_FREE/KayKit_Furniture_Bits_1.0_FREE/Assets/gltf", "gltf", None),
    ("KD", U + "/KayKit_Dungeon_Pack_1.1_FREE/KayKit_Dungeon_Pack_1.1_FREE/Assets/gltf", "gltf", DUNGEON),
    ("KF", U + "/KayKit_Forest_Nature_Pack_1.0_FREE/KayKit_Forest_Nature_Pack_1.0_FREE/Assets/gltf", "gltf", None),
    ("QN", U + "/Ultimate Nature Pack by Quaternius/FBX", "fbx", QUAT),
    ("VEH", U, "glb", ["hilux"]),
    ("VEH", U + "/Car by Poly by Google - 75h3mi6uHuC", "obj", ["Car"]),
]
ONLY = os.environ.get("IS_ONLY", "")


def log(*a):
    unreal.log("[IS-PACK] " + " ".join(str(x) for x in a))


def import_one(kit, path, name):
    clean = name.replace("-", "_").replace(" ", "_")
    folder = "/Game/Infinisat/%s/%s" % (kit, clean)
    target = folder + "/SM_" + clean
    if not eal.does_asset_exist(target):
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = folder
        t.automated = True
        t.replace_existing = True
        t.save = True
        at.import_asset_tasks([t])
        sm = next((p.split(".")[0] for p in eal.list_assets(folder, recursive=True, include_folder=False)
                   if isinstance(eal.load_asset(p), unreal.StaticMesh)), None)
        if not sm:
            log("NO MESH", kit, name)
            return False
        if sm != target:
            eal.rename_asset(sm, target)
    mesh = eal.load_asset(target)
    if not mesh:
        return False
    ns = mesh.get_editor_property("nanite_settings")
    if ns.enabled:
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
        eal.save_loaded_asset(mesh)
    return True


for kit, src, ext, names in SETS:
    if ONLY and kit not in ONLY.split(","):
        continue
    files = sorted(glob.glob(os.path.join(src, "*." + ext)))
    n = 0
    for f in files:
        name = os.path.splitext(os.path.basename(f))[0]
        if names is not None and name not in names:
            continue
        if kit == "KF" and not any(name.startswith(p) for p in ("Tree_", "Bush_", "Rock_1_", "Grass_1_A", "Grass_1_B", "Grass_2_A", "Grass_2_B")):
            continue
        if import_one(kit, f, name):
            n += 1
    log(kit, "imported", n, "from", src)
log("PACKS DONE")
