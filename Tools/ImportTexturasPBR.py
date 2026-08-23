"""
ImportTexturasPBR.py — Importa las texturas PBR de Content/Textures/*.png a uassets.

Los 37 PNG de T_Asphalt / T_Brick / T_Cobblestone / T_Concrete / T_Grass /
T_Ground / T_MetalPlate / T_RoofTiles / T_StoneWall / T_Wood se versionan, pero
sus uassets no (.gitignore: "uassets derivados de textura"). Sin ellos,
M_Terreno_Calles y M_Terreno_Acera compilan con dos TextureSampleParameter2D en
NULL y caen a Default Material: el asfalto y las aceras del pueblo salen en gris
plano.

Ejecutar desde el editor:
    exec(open("Tools/ImportTexturasPBR.py").read())
o headless:
    UnrealEditor-Cmd.exe <proyecto> -run=pythonscript -script=Tools/ImportTexturasPBR.py

Idempotente: salta lo que ya está importado.
"""
import os
import unreal

import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

ORIGEN = unreal.Paths.project_content_dir() + "Textures/"
DESTINO = "/Game/Textures"

# Normal y roughness/AO son datos, no color: en sRGB el shader los lee mal
# (la normal sale aplanada y el rough con la curva torcida).
SUFIJOS_LINEALES = ("_Normal", "_Roughness", "_AO")


def es_normal(nombre):
    return nombre.endswith("_Normal")


def importar(src, nombre):
    destino = f"{DESTINO}/{nombre}"
    if compat.assets().does_asset_exist(destino):
        unreal.log(f"[TexPBR] ya existe, salto: {destino}")
        return True

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", DESTINO)
    task.set_editor_property("destination_name", nombre)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex = compat.assets().load_asset(destino)
    if not tex:
        unreal.log_error(f"[TexPBR] no se pudo importar: {destino}")
        return False

    lineal = nombre.endswith(SUFIJOS_LINEALES)
    tex.set_editor_property("srgb", not lineal)
    if es_normal(nombre):
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_NORMALMAP)
    elif lineal:
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_MASKS)
    else:
        tex.set_editor_property("compression_settings",
                                unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    compat.assets().save_asset(destino, False)
    unreal.log(f"[TexPBR] importada: {destino} (srgb={not lineal})")
    return True


def run():
    if not os.path.isdir(ORIGEN):
        unreal.log_error(f"[TexPBR] no existe {ORIGEN}")
        return False

    pngs = sorted(f for f in os.listdir(ORIGEN)
                  if f.lower().endswith(".png") and f.startswith("T_"))
    if not pngs:
        unreal.log_warning(f"[TexPBR] ningun T_*.png en {ORIGEN}")
        return False

    ok = 0
    for f in pngs:
        # Un puntero LFS son ~130 bytes; importarlo da una textura corrupta.
        ruta = ORIGEN + f
        if os.path.getsize(ruta) < 10000:
            unreal.log_warning(f"[TexPBR] {f} pesa {os.path.getsize(ruta)} B: "
                               "es un puntero LFS sin descargar, lo salto.")
            continue
        if importar(ruta, os.path.splitext(f)[0]):
            ok += 1

    unreal.log(f"[TexPBR] {ok} de {len(pngs)} texturas listas en {DESTINO}")
    return ok > 0


if __name__ == "__main__":
    run()
