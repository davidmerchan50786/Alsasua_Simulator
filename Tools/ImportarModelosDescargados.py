r"""
ImportarModelosDescargados.py — Importa los glTF de Content/ModelosDescargados
a /Game/ModelosDescargados.

Son props CC0 de Poly Haven que rellenan los huecos que no cubría lo ya
descargado en AssetsImportados (comprobado contra Datos/asset_manifest.json):
boca de incendio, tapa de alcantarilla, papelera metálica y banco corrido.

Necesita el plugin InterchangeEditor para el glTF (ya está en el .uproject).

Ejecutar desde el editor: Tools > Execute Python Script, o
    exec(open(unreal.Paths.project_dir() + 'Tools/ImportarModelosDescargados.py').read())
"""
import os

import unreal

ORIGEN = os.path.join(unreal.Paths.project_content_dir(), "ModelosDescargados")
DESTINO = "/Game/ModelosDescargados"


def importar_uno(ruta_gltf, nombre):
    destino = f"{DESTINO}/{nombre}"

    if unreal.EditorAssetLibrary.does_directory_exist(destino):
        unreal.log(f"[Modelos] {nombre} ya importado")
        return True

    tarea = unreal.AssetImportTask()
    tarea.set_editor_property("filename", ruta_gltf)
    tarea.set_editor_property("destination_path", destino)
    tarea.set_editor_property("automated", True)
    tarea.set_editor_property("save", True)
    tarea.set_editor_property("replace_existing", False)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([tarea])

    creados = tarea.get_editor_property("imported_object_paths") or []
    if creados:
        unreal.log(f"[Modelos] {nombre}: {len(creados)} assets")
        return True

    unreal.log_warning(f"[Modelos] {nombre} no importó nada (¿InterchangeEditor activo?)")
    return False


def run():
    if not os.path.isdir(ORIGEN):
        unreal.log_error(f"[Modelos] no existe {ORIGEN}")
        return False

    ok = 0
    total = 0
    for nombre in sorted(os.listdir(ORIGEN)):
        carpeta = os.path.join(ORIGEN, nombre)
        if not os.path.isdir(carpeta):
            continue
        gltf = [f for f in os.listdir(carpeta) if f.lower().endswith(".gltf")]
        if not gltf:
            continue
        total += 1
        if importar_uno(os.path.join(carpeta, gltf[0]), nombre):
            ok += 1

    unreal.log(f"[Modelos] {ok}/{total} importados en {DESTINO}")
    unreal.log("[Modelos] AlsasuaMallaFab ya los busca ahí: se prefieren a las mallas procedurales.")
    return ok == total


if __name__ == "__main__":
    run()
