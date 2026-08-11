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
    # El glTF va sin options a propósito: lo importa Interchange (el plugin
    # InterchangeEditor del .uproject) con sus pipelines por defecto. La clase
    # GltfImportUI que había aquí no existe en 5.8, así que sólo servía para
    # reventar la importación de los glTF antes de empezarla.

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    creados = task.get_editor_property("imported_object_paths") or []
    if creados:
        unreal.log("  Importado: {} -> {} ({} assets)".format(asset_name, dest_folder, len(creados)))
        return True
    unreal.log_warning("  FALLO: {} ({})".format(asset_name, filepath))
    return False


def main():
    if not os.path.isdir(ASSETS_ROOT):
        # AssetsImportados no se versiona: en un clon limpio no está y no pasa
        # nada, pero conviene decirlo en vez de reportar "0 ficheros" sin más.
        unreal.log_warning(
            "No existe {}: no hay nada que importar. Baja los packs primero.".format(ASSETS_ROOT))
        return 0

    count = 0
    ok = 0
    for root, dirs, files in os.walk(ASSETS_ROOT):
        for f in files:
            ext = os.path.splitext(f)[1].lower()
            if ext not in (".fbx", ".obj", ".gltf", ".glb"):
                continue

            filepath = os.path.join(root, f)
            rel = os.path.relpath(root, ASSETS_ROOT).replace("\\", "/")
            # relpath devuelve "." para los ficheros que están en la raíz, y
            # entonces salía "/Game/AssetsImportados/." — una ruta de contenido
            # inválida, así que esos assets no se importaban. Las rutas /Game se
            # componen con "/" siempre, nunca con os.path.join (en Windows mete
            # barras invertidas).
            dest = UE_CONTENT if rel in (".", "") else UE_CONTENT + "/" + rel

            if import_file(filepath, dest):
                ok += 1
            count += 1

    unreal.log("=== Importacion: {}/{} ficheros ===".format(ok, count))
    return ok


def run():
    """Punto de entrada para RunAll.py."""
    return main()


if __name__ == "__main__":
    main()
