# Unreal Editor Python: import the normalised low-poly props (Tools/convert_lp.py output) to /Game/Infinisat/LP/<name>/SM_<name>.
# Low-poly meshes don't need Nanite (and imported materials lack the Nanite usage flag), so Nanite is switched off.
# Run (editor closed): IS_LP_DIR=C:/Dev/_downloads/lp/clean UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/setup_lp.py -unattended
import os
import unreal

ROOT = "/Game/Infinisat/LP"
SRC = os.environ.get("IS_LP_DIR", "C:/Dev/_downloads/lp/clean")
at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def log(*a):
    unreal.log("[IS-LP] " + " ".join(str(x) for x in a))


for fn in sorted(os.listdir(SRC)):
    if not fn.endswith(".glb"):
        continue
    name = fn[:-4]
    folder = ROOT + "/" + name
    target = folder + "/SM_" + name
    if not eal.does_asset_exist(target):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, fn)
        t.destination_path = folder
        t.automated = True
        t.replace_existing = True
        t.save = True
        at.import_asset_tasks([t])
        sm = next((p.split(".")[0] for p in eal.list_assets(folder, recursive=True, include_folder=False)
                   if isinstance(eal.load_asset(p), unreal.StaticMesh)), None)
        if not sm:
            log("NO MESH", name)
            continue
        if sm != target:
            eal.rename_asset(sm, target)
    mesh = eal.load_asset(target)
    if not mesh:
        continue
    ns = mesh.get_editor_property("nanite_settings")
    if ns.enabled:
        ns.enabled = False
        mesh.set_editor_property("nanite_settings", ns)
    eal.save_loaded_asset(mesh)
    log("model", name)
log("LP DONE")
