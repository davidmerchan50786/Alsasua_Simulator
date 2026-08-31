"""Importa a Unreal los FBX que el launcher de Epic deja en la caché de Fab.

El launcher NO descarga los packs de Fab como uassets: los deja en
    F:\\ProgramData\\Epic\\EpicGamesLauncher\\VaultCache\\FabLibrary\\
en formato fuente (fbx, blend), y hasta que no se importan no existen para el
proyecto. Este script hace ese paso para los que ya están copiados en
Content/AssetsImportados/Fab/.

Uso, en la consola Python del editor:
    exec(open(r'<raiz>\\Tools\\ImportarFab.py').read())

o headless:
    UnrealEditor-Cmd.exe <proyecto> -run=pythonscript -script="<esta ruta>"

Los .blend de la caché (residential_building, house) NO se pueden importar:
Unreal no lee ese formato. Hay que exportarlos a FBX con Blender antes, que en
esta máquina no está instalado.
"""
import os
import unreal

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ORIGEN = os.path.join(RAIZ, "Content", "AssetsImportados", "Fab")
DESTINO = "/Game/AssetsImportados/Fab"


def importar(ruta_fbx, destino):
    """Un FBX -> un asset. Devuelve el objeto creado o None.

    UE 5.8 importa por Interchange y ya no acepta que se le pase un FbxFactory
    con su FbxImportUI: ni FbxFactory.import_ui ni
    AutomatedAssetImportData.automated existen ya. Sin fabrica, el motor elige
    el importador que toca segun la extension, que es lo que queremos.
    """
    datos = unreal.AutomatedAssetImportData()
    datos.set_editor_property("destination_path", destino)
    datos.set_editor_property("filenames", [ruta_fbx])
    datos.set_editor_property("replace_existing", True)

    creados = unreal.AssetToolsHelpers.get_asset_tools().import_assets_automated(datos)
    return creados[0] if creados else None


def main():
    if not os.path.isdir(ORIGEN):
        unreal.log_error("[Fab] no existe %s" % ORIGEN)
        return

    fbxs = sorted(f for f in os.listdir(ORIGEN) if f.lower().endswith(".fbx"))
    if not fbxs:
        unreal.log_warning("[Fab] no hay FBX en %s" % ORIGEN)
        return

    unreal.log("[Fab] %d FBX por importar -> %s" % (len(fbxs), DESTINO))
    ok, fallos = [], []
    for f in fbxs:
        ruta = os.path.join(ORIGEN, f)
        try:
            a = importar(ruta, DESTINO)
        except Exception as e:            # el importador puede lanzar por FBX corrupto
            a = None
            unreal.log_error("[Fab] %s: %s" % (f, e))
        (ok if a else fallos).append(f)
        unreal.log("[Fab]   %-28s %s" % (f, "OK" if a else "FALLO"))

    if ok:
        unreal.EditorAssetLibrary.save_directory(DESTINO, False, True)

    unreal.log("[Fab] importados %d de %d" % (len(ok), len(fbxs)))
    if fallos:
        unreal.log_warning("[Fab] fallaron: %s" % ", ".join(fallos))


main()
