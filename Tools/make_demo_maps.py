# Unreal Editor Python: create the empty demo maps. The game spawns everything from C++; the map name sets
# demo mode (ISDemo: Demo_* -> bDemo) and Demo_Backyard starts in the open world.
# Run (editor closed): UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/make_demo_maps.py -unattended
import unreal

eal = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
for name in ("Demo_Backyard", "Demo_Space"):
    path = "/Game/Infinisat/Maps/" + name
    if eal.does_asset_exist(path):
        unreal.log("[IS-MAPS] exists " + path)
        continue
    if les.new_level(path):
        les.save_current_level()
        unreal.log("[IS-MAPS] created " + path)
    else:
        unreal.log_error("[IS-MAPS] FAILED " + path)
