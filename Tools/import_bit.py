# Unreal Editor Python: import Docs/BIT_portrait.png (a render of the 3D BIT model) as /Game/Infinisat/Textures/T_UI_Bit.
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/import_bit.py -unattended
import os
import unreal
PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
t = unreal.AssetImportTask()
t.filename = os.path.join(PROJ, "Docs", "BIT_portrait.png")
t.destination_path = "/Game/Infinisat/Textures"
t.destination_name = "T_UI_Bit"
t.automated = True
t.replace_existing = True
t.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
a = unreal.EditorAssetLibrary.load_asset("/Game/Infinisat/Textures/T_UI_Bit")
a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
a.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
unreal.EditorAssetLibrary.save_loaded_asset(a)
unreal.log("[IS] BIT portrait imported %dx%d" % (a.blueprint_get_size_x(), a.blueprint_get_size_y()))
