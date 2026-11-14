# Unreal Editor Python: replace SM_SphereHi with a dense, smooth UV sphere (radius 50 uu, Nanite OFF).
# The old Nanite sphere fell back to a coarse mesh and planets looked faceted ("3D hexagon").
# Run: UnrealEditor-Cmd.exe Infinisat.uproject -run=pythonscript -script=Tools/make_sphere.py -unattended
import math, os
import unreal

PROJ = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OBJ = os.path.join(PROJ, "Saved", "sphere_hires.obj")
SEG, RINGS, R = 256, 128, 50.0


def log(*a):
    unreal.log("[IS] " + " ".join(str(x) for x in a))


def write_obj():
    os.makedirs(os.path.dirname(OBJ), exist_ok=True)
    with open(OBJ, "w") as f:
        f.write("o SM_SphereHi\n")
        # OBJ is Y-up; UE converts to Z-up on import
        for j in range(RINGS + 1):
            v = j / RINGS
            lat = math.pi * (0.5 - v)
            for i in range(SEG + 1):
                u = i / SEG
                lon = 2.0 * math.pi * u
                x, y, z = math.cos(lat) * math.cos(lon), math.sin(lat), math.cos(lat) * math.sin(lon)
                f.write("v %.5f %.5f %.5f\n" % (x * R, y * R, z * R))
        for j in range(RINGS + 1):
            for i in range(SEG + 1):
                f.write("vt %.6f %.6f\n" % (i / SEG, 1.0 - j / RINGS))
        for j in range(RINGS + 1):
            lat = math.pi * (0.5 - j / RINGS)
            for i in range(SEG + 1):
                lon = 2.0 * math.pi * i / SEG
                f.write("vn %.5f %.5f %.5f\n" % (math.cos(lat) * math.cos(lon), math.sin(lat), math.cos(lat) * math.sin(lon)))
        row = SEG + 1
        for j in range(RINGS):
            for i in range(SEG):
                a = j * row + i + 1
                b = a + 1
                c = a + row
                d = c + 1
                if j != 0:
                    f.write("f %d/%d/%d %d/%d/%d %d/%d/%d\n" % (a, a, a, b, b, b, c, c, c))
                if j != RINGS - 1:
                    f.write("f %d/%d/%d %d/%d/%d %d/%d/%d\n" % (b, b, b, d, d, d, c, c, c))


write_obj()
t = unreal.AssetImportTask()
t.filename = OBJ
t.destination_path = "/Game/Infinisat/Meshes"
t.destination_name = "SM_SphereHi"
t.automated = True
t.replace_existing = True
t.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
log("imported", list(t.imported_object_paths or []))
sm = unreal.EditorAssetLibrary.load_asset("/Game/Infinisat/Meshes/SM_SphereHi")
if not isinstance(sm, unreal.StaticMesh):
    for p in t.imported_object_paths or []:
        a = unreal.EditorAssetLibrary.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            sm = a
log("mesh", sm)
if sm:
    ns = sm.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", False)
    sm.set_editor_property("nanite_settings", ns)
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
    b = sm.get_bounds()
    log("bounds origin", b.origin, "extent", b.box_extent, "tris", sm.get_num_triangles(0))
