"""
RunAll.py — Script maestro que ejecuta TODO el setup de AlsasuaSimulator.
Ejecutar desde el editor: Window > Output Log > consola:
    exec(open(r'F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Tools/RunAll.py').read())

Orden de ejecución:
  1. SetupMaterials  — MPC_Clima + 6 materiales
  2. SetupLevel      — L_Alsasua + cielo + niebla + post-process
  3. SetupInput      — IMC_Jugador + 10 InputActions
  4. SetupLandscape  — Guía de importación del heightmap
  5. SetupActors     — 12 actores placeholder (jugador, NPCs, etc.)
  6. SetupAudio      — 4 AmbientSounds

Después de ejecutar:
  1. Landscape Mode → Import from File → alsasua_landscape_4033.r16
  2. Abrir materiales en Material Editor para refinar
  3. Play!
"""
import unreal
import sys
import os

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))


def run():
    """Ejecuta todos los scripts de setup en orden."""
    unreal.log("=" * 60)
    unreal.log("  ALSASUA SIMULATOR — SETUP COMPLETO")
    unreal.log("=" * 60)

    if TOOLS_DIR not in sys.path:
        sys.path.insert(0, TOOLS_DIR)

    scripts = [
        ("SetupMaterials", "Materiales"),
        ("SetupLevel", "Nivel"),
        ("SetupInput", "Input"),
        ("SetupLandscape", "Landscape (guía)"),
        ("SetupActors", "Actores placeholder"),
        ("SetupAudio", "Audio ambiente"),
    ]

    for i, (module_name, desc) in enumerate(scripts, 1):
        unreal.log(f"\n--- Paso {i}/{len(scripts)}: {desc} ---")
        try:
            mod = __import__(module_name)
            mod.run()
        except Exception as e:
            unreal.log_error(f"Error en {module_name}: {e}")

    unreal.log("\n" + "=" * 60)
    unreal.log("  SETUP COMPLETO — SIGUIENTES PASOS:")
    unreal.log("=" * 60)
    unreal.log("  1. Landscape Mode (Shift+3) → Import from File")
    unreal.log("     Archivo: Content/Terreno/alsasua_landscape_4033.r16")
    unreal.log("     Scale XY: 178.5714 | Scale Z: 200 | Loc Z: 49567")
    unreal.log("  2. Abrir materiales en Material Editor para refinar")
    unreal.log("  3. Play!")
    unreal.log("=" * 60)


if __name__ == "__main__":
    run()
