"""
RunAll.py — Script maestro: ejecuta TODO el setup de AlsasuaSimulator en orden.

Ejecutar desde el editor: Window > Output Log > consola de Python:
    exec(open(unreal.Paths.project_dir() + 'Tools/RunAll.py').read())

o desde Tools > Execute Python Script eligiendo este fichero.

El orden respeta dependencias reales, no es una lista cualquiera:

  1. Ortofoto + materiales (C++: UAlsasuaAssetGenerator)
       MPC_Clima se crea aquí y TODO lo demás lee de él su Wetness.
       Las texturas tienen que ser assets antes de que un material las
       muestree, o el material se queda con la textura por defecto (gris).
  2. Nivel: L_Alsasua + cielo + niebla + post-process + DirectorArranque.
  3. Input, landscape, actores, audio.
  4. Capas visuales: dependen de que existan los materiales del paso 1
       (instancias de terreno, fachadas, ventanas nocturnas, decals...).
  5. Partículas y VFX.
  6. Foliage de Fab: lo descargado sustituye a lo procedural, así que va
       después de haber generado lo procedural. Y al final se adapta la
       biblioteca a 5.8 (Nanite por malla), que necesita las mallas ya
       importadas.

Cada paso está aislado: si uno falla, se registra y el resto continúa. Al
final imprime un resumen con lo que salió y lo que no.
"""
import os
import sys
import runpy

import unreal


# La ruta se deriva del proyecto. Antes estaba fija a
# "F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Tools": sólo funcionaba en
# esa máquina, y al pasar el proyecto a 5.8 dejó de existir hasta ahí, así que
# sys.path apuntaba a una carpeta inexistente y todos los __import__ fallaban.
TOOLS_DIR = os.path.join(unreal.Paths.project_dir(), "Tools")


# La importación va la PRIMERA de todo y antes faltaba entera: RunAll saltaba
# directo a materiales y capas visuales, así que en un clon limpio los .gltf y
# .fbx seguían siendo ficheros sueltos en disco, nunca uassets. AlsasuaMallaFab
# escaneaba /Game/ModelosDescargados, no encontraba nada, y todo el pueblo caía a
# formas básicas con los modelos descargados al lado sin usar.
PASOS_IMPORT = [
    ("ImportarModelosDescargados", "Props CC0 de Poly Haven (glTF)"),
    ("ue5_import_all_assets",      "Mallas de AssetsImportados (FBX/OBJ/glTF)"),
    ("ImportSatellite",            "Ortofotos PNOA + materiales que las drapean"),
]

# (módulo, descripción). Los que exponen run() se importan y se llama a run();
# el resto se ejecutan con runpy, que activa su guard __main__.
PASOS_NIVEL = [
    ("SetupLevel",              "Nivel + cielo + niebla + DirectorArranque"),
    ("SetupInput",              "Input (IMC + InputActions)"),
    ("SetupLandscape",          "Landscape (guía de importación)"),
    ("SetupActors",             "Actores placeholder"),
    ("SetupAudio",              "Audio ambiente"),
]

PASOS_VISUALES = [
    ("SetupMaterials",          "Instancias de material del nivel"),
    ("SetupTerrainAutoTexture", "Terreno con auto-textura por pendiente"),
    ("SetupEnhancedMaterials",  "Charcos y ventanas emisivas"),
    ("SetupBarrioStyles",       "Estilos de fachada por barrio"),
    ("SetupFacadeDetails",      "Detalles de fachada"),
    ("SetupNightBuildings",     "Ventanas encendidas de noche"),
    ("SetupStreetLights",       "Farolas con control automático"),
    ("SetupInteriorLights",     "Luces de interior"),
    ("SetupDecals",             "Decals de suciedad y marcas"),
    ("SetupGroundCover",        "Cobertura de suelo"),
    ("SetupFoliageWind",        "Viento en la vegetación"),
    ("SetupWaterReflections",   "Reflejos de agua"),
    ("SetupAdvancedVisuals",    "Stack de post-proceso avanzado"),
]

PASOS_VFX = [
    ("SetupAmbientParticles",   "Partículas de ambiente"),
    ("SetupRainParticles",      "Partículas de lluvia"),
    ("create_niagara_vfx",      "Niagara VFX"),
]

PASOS_FAB = [
    ("SetupMegascansFoliage",   "Foliage de Fab / Megascans"),
    ("AdaptarBiblioteca58",     "Nanite en la biblioteca importada"),
]


def _ejecutar_modulo(nombre):
    """Llama a run() si existe; si no, ejecuta el script como __main__."""
    ruta = os.path.join(TOOLS_DIR, nombre + ".py")
    if not os.path.exists(ruta):
        raise FileNotFoundError(ruta)

    mod = __import__(nombre)
    if hasattr(mod, "run"):
        return mod.run()
    # La mayoría de los Setup* no tienen run(), sólo un guard __main__.
    runpy.run_path(ruta, run_name="__main__")
    return None


def _fase(titulo, pasos, resultados):
    unreal.log("")
    unreal.log("--- %s ---" % titulo)
    for nombre, desc in pasos:
        try:
            r = _ejecutar_modulo(nombre)
            # Sólo un False explícito cuenta como fallo. Un entero NO: el
            # importador masivo devuelve 0 cuando no hay nada que importar
            # (AssetsImportados no se versiona), y eso es normal, no un error.
            # Los pasos sin run() devuelven None y siguen contando como OK.
            ok = (r is not False)
            resultados.append((ok, desc))
            unreal.log("   %s %s" % ("OK   " if ok else "FALLO", desc))
        except Exception as e:
            resultados.append((False, desc))
            unreal.log_warning("   FALLO %s: %s" % (desc, e))


def run():
    unreal.log("=" * 62)
    unreal.log("  ALSASUA SIMULATOR — SETUP COMPLETO")
    unreal.log("=" * 62)

    if TOOLS_DIR not in sys.path:
        sys.path.insert(0, TOOLS_DIR)

    resultados = []

    # 0. Importar: los ficheros de disco tienen que ser uassets antes de que
    #    nadie los busque. Los materiales muestrean texturas y MallaFab escanea
    #    el registro de assets; si esto no ha corrido, ambos encuentran vacío.
    _fase("0. Importación de assets", PASOS_IMPORT, resultados)

    # 1. Ortofoto + MPC_Clima + materiales, en el orden que impone el C++.
    unreal.log("")
    unreal.log("--- 1. Assets y materiales (UAlsasuaAssetGenerator) ---")
    try:
        ok = unreal.AlsasuaAssetGenerator.generar_todos_los_assets()
        resultados.append((bool(ok), "Ortofoto + MPC_Clima + materiales"))
        unreal.log("   %s Assets y materiales" % ("OK   " if ok else "FALLO"))
    except Exception as e:
        resultados.append((False, "Ortofoto + MPC_Clima + materiales"))
        unreal.log_error("   FALLO Assets y materiales: %s" % e)
        unreal.log_error("   Sin MPC_Clima el resto sale sin respuesta a la lluvia.")

    _fase("2. Nivel y sistemas", PASOS_NIVEL, resultados)
    _fase("3. Capas visuales", PASOS_VISUALES, resultados)
    _fase("4. Partículas y VFX", PASOS_VFX, resultados)
    _fase("5. Foliage de Fab", PASOS_FAB, resultados)

    ok = sum(1 for r, _ in resultados if r)
    unreal.log("")
    unreal.log("=" * 62)
    unreal.log("  RESUMEN: %d/%d pasos completados" % (ok, len(resultados)))
    for correcto, desc in resultados:
        if not correcto:
            unreal.log_warning("    pendiente: %s" % desc)
    unreal.log("=" * 62)
    unreal.log("  A MANO (no automatizable desde aquí):")
    unreal.log("   1. Landscape Mode (Shift+3) -> Import from File")
    unreal.log("      Content/Terreno/alsasua_landscape_4033.r16")
    unreal.log("      Scale XY: 178.5714 | Scale Z: 200 | Loc Z: 53194")
    unreal.log("   2. Ventana > Fab: iniciar sesion y descargar los arboles.")
    unreal.log("      Luego re-ejecutar el paso 5 para que los detecte.")
    unreal.log("   3. Play (PIE): DirectorArranque genera los sistemas.")
    unreal.log("   4. Guardar el nivel al terminar.")
    unreal.log("=" * 62)
    return ok == len(resultados)


if __name__ == "__main__":
    run()
