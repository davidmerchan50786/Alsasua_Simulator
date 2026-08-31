"""
OptimizarVRAM.py — Fija el LOD group WORLD (mipmaps + streaming) en las
texturas de las carpetas pesadas, para que VRAM solo suba el detalle visible
en vez del tile entero de golpe.

Cubre /Game/Terreno y /Game/Textures (las 8192 de la ortofoto y las 4096 PBR).
Los assets de /Game/AssetsImportados no se listan desde el commandlet headless
(no son packages registrados del directorio), asi que aqui no se tocan.

TEXTUREGROUP_WORLD es inocuo en texturas pequenas (ni generan mip ni
streamean), asi que aplicarlo a todas no rompe nada.

Ejecutar headless (necesita ruta SIN espacios: el commandlet parte la ruta por
el espacio de "Epic Games"):
    UnrealEditor-Cmd.exe <proyecto> -run=pythonscript -Script=H:\Temp\opencode\OptimizarVRAM.py

Idempotente: volver a correrlo no cambia nada.
"""
import sys as _sys, os as _os, unreal
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

CARPETAS = ("/Game/Terreno", "/Game/Textures")


def correr():
    total = 0
    contadores = {}
    for carpeta in CARPETAS:
        if not compat.assets().does_directory_exist(carpeta):
            unreal.log_warning(f"[VRAM] no existe {carpeta}, salto")
            continue
        rutas = compat.assets().list_assets(carpeta, recursive=True)
        n_tex = 0
        for ruta in rutas:
            asset = compat.assets().load_asset(ruta)
            if not asset or not isinstance(asset, unreal.Texture2D):
                continue
            n_tex += 1
            grupo = asset.get_editor_property("lod_group")
            contadores[str(grupo)] = contadores.get(str(grupo), 0) + 1
            if grupo == unreal.TextureGroup.TEXTUREGROUP_WORLD:
                continue
            asset.set_editor_property("lod_group",
                                      unreal.TextureGroup.TEXTUREGROUP_WORLD)
            compat.assets().save_asset(ruta, False)
            unreal.log(f"[VRAM] {ruta}: {grupo} -> WORLD")
            total += 1
        unreal.log(f"[VRAM] {carpeta}: {n_tex} Texture2D")
    unreal.log(f"[VRAM] grupos encontrados: {contadores}")
    unreal.log(f"[VRAM] TOTAL cambiadas a WORLD: {total}")
    return True


# Sin guard __main__: el commandlet -run=pythonscript ejecuta el archivo como
# modulo, no como main, y el bloque no llegaria a correr.
correr()
