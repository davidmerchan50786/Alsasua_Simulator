import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

import os
import struct

HEIGHTMAP_R16 = "Content/Terreno/alsasua_landscape_4033.r16"
LANDSCAPE_PATH = "/Game/Terreno/Alsasua_Landscape"

def run():
    unreal.log("=== SetupLandscape: Iniciando ===")
    r16_path = os.path.join(unreal.Paths.project_dir(), HEIGHTMAP_R16)
    if not os.path.exists(r16_path):
        unreal.log_error(f"No existe: {r16_path}")
        return

    world = compat.mundo()
    if not world:
        unreal.log_error("Sin mundo editor")
        return

    size = os.path.getsize(r16_path)
    res = int(size ** 0.5 / 2)
    unreal.log(f"Heightmap: {r16_path} ({size}B, {res}x{res})")

    subsys = unreal.LandscapeSubsystem.get()
    if not subsys:
        unreal.log_warning("Sin LandscapeSubsystem, intento manual...")
        ImportLandscapeManual(r16_path, res)
        return

    # Leer raw R16
    with open(r16_path, "rb") as f:
        raw = f.read()

    heights = struct.unpack(f"{res*res}H", raw)
    min_h, max_h = min(heights), max(heights)
    unreal.log(f"Height range: {min_h} - {max_h} (raw)")

    # Crear Landscape via EditorLevelLibrary
    # Usamos CreateLandscape de la API
    try:
        scale = 178.5714  # cm/quad (7200m / 4032 quads)
        loc_z = 53194.0
        loc_x = 606500.0 - 360000.0  # HerrikoPlaza - half world
        loc_y = 4749500.0 - 360000.0

        # El heightmap no se puede importar desde Python: no hay factory
        # expuesta (unreal.HeightmapImportFactory no existe, y ese create_asset
        # saltaba al except de abajo sin dejar rastro). Se crea el Landscape
        # vacío y se importa el .r16 desde Landscape Mode > Import from File.
        compat.actores().spawn_actor_from_class(
            unreal.Landscape, unreal.Vector(loc_x, loc_y, loc_z)
        )
        unreal.log("Landscape creado. Asigna heightmap manualmente si no se ve.")
        unreal.log(f"  X={loc_x:.0f} Y={loc_y:.0f} Z={loc_z:.0f} Scale={scale}")

    except Exception as e:
        unreal.log_warning(f"Auto-import falló: {e}")
        ImportLandscapeManual(r16_path, res)

def ImportLandscapeManual(r16_path, res):
    scale = 178.5714
    loc_z = 53194.0
    loc_x = 606500.0 - 360000.0
    loc_y = 4749500.0 - 360000.0
    unreal.log("=" * 60)
    unreal.log("IMPORTACIÓN MANUAL:")
    unreal.log("Modo Paisaje: botón 'Modos' (arriba izq viewport) → Paisaje")
    unreal.log(f"  Importar desde archivo → {r16_path}")
    unreal.log(f"  Scale X/Y: {scale} | Scale Z: 200 | Loc Z: {loc_z}")
    unreal.log(f"  Section Size: 63 quads | Sections: 1x1")
    unreal.log("=" * 60)

if __name__ == "__main__":
    run()
