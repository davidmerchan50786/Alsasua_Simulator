"""
UE5 Editor Script — Import ALL extracted assets into Content/AssetsImportados/
Run in UE5 via: File > Execute Python Script

Auto-discovers all .fbx, .obj, .gltf files and imports them.
Characters (FBX with 'Bot@' or 'Walking' in name) -> SkeletalMesh/Animation
Everything else -> StaticMesh
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

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

    # FBX por el importador clásico: pasarle un FbxImportUI es lo que lo
    # selecciona, y en 5.8 sigue estando.
    if ext == ".fbx":
        s = unreal.FbxImportUI()
        s.set_editor_property("import_mesh", True)
        s.set_editor_property("import_textures", True)
        s.set_editor_property("import_materials", True)
        s.set_editor_property("import_as_skeletal", False)
        # convert_scene se movio de sitio entre versiones de UE (5.8 no lo
        # tiene donde estaba en 5.4): sin este try, la excepcion abortaba la
        # importacion entera y las 106 mallas de Naturaleza se quedaban sin
        # importar. No es esencial - controla el ajuste de ejes en origen,
        # y Fbx ya suele traerlo bien - asi que si no existe, se sigue sin el.
        try:
            s.set_editor_property("convert_scene", True)
        except Exception:
            pass
        task.set_editor_property("options", s)
    # glTF/GLB van por Interchange y no llevan options: unreal.GltfImportUI no
    # existe (era un AttributeError en cada .gltf, y el except de RunAll lo
    # tapaba). Sin options se usa la pipeline por defecto, que importa malla,
    # materiales y texturas.
    #
    # La llamada va por compat.importar_tareas, que además de usar la API buena
    # devuelve las rutas creadas: sirve para saber si la importación hizo algo,
    # que es más fiable que mirar la propiedad "result" de la tarea.
    creados = compat.importar_tareas([task])
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
