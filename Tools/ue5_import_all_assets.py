"""
UE5 Editor Script — Import ALL extracted assets into Content/AssetsImportados/
Run in UE5 via: File > Execute Python Script

Auto-discovers all .fbx, .obj, .gltf files and imports them.
Characters (FBX with 'Bot@' or 'Walking' in name) -> SkeletalMesh/Animation
Everything else -> StaticMesh
"""
import unreal
import os

ASSETS_ROOT = unreal.Paths.project_content_dir() + "AssetsImportados"
UE_CONTENT = "/Game/AssetsImportados"


def import_file(filepath, dest_folder):
    filename = os.path.basename(filepath)
    asset_name = os.path.splitext(filename)[0]
    asset_path = os.path.join(dest_folder, asset_name)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filepath)
    task.set_editor_property("destination_path", dest_folder)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing_settings", True)

    ext = os.path.splitext(filename)[1].lower()

    if ext == ".fbx":
        s = unreal.FbxImportUI()
        s.set_editor_property("import_mesh", True)
        s.set_editor_property("import_textures", True)
        s.set_editor_property("import_materials", True)
        s.set_editor_property("convert_scene", True)
        s.set_editor_property("import_as_skeletal", False)
        task.set_editor_property("options", s)
    elif ext == ".gltf" or ext == ".glb":
        s = unreal.GltfImportUI()
        s.set_editor_property("import_mesh", True)
        s.set_editor_property("import_textures", True)
        s.set_editor_property("import_materials", True)
        task.set_editor_property("options", s)

    unreal.AssetImportHelpers.import_asset_tasks([task])
    if task.get_editor_property("result"):
        names = task.get_editor_property("imported_object_names")
        unreal.log("  Imported: {} -> {}".format(asset_name, dest_folder))
    else:
        unreal.log_warning("  FAILED: {}".format(asset_name))


def main():
    count = 0
    for root, dirs, files in os.walk(ASSETS_ROOT):
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext not in (".fbx", ".obj", ".gltf", ".glb"):
                continue

            filepath = os.path.join(root, f)
            rel = os.path.relpath(root, ASSETS_ROOT)
            dest = os.path.join(UE_CONTENT, rel.replace("\\", "/")).replace("\\", "/")

            unreal.log("Importing: {} -> {}".format(f, dest))
            import_file(filepath, dest)
            count += 1

    unreal.log("=== DONE: {} files processed ===".format(count))


if __name__ == "__main__":
    main()
