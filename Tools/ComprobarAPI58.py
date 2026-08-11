r"""
ComprobarAPI58.py — Comprueba que los scripts de Tools/ hablan la API de 5.8.

Se ejecuta SIN Unreal (python3 Tools/ComprobarAPI58.py), que es justo la gracia:
un AttributeError de la API de Python del editor no se ve hasta que abres el
editor, ejecutas el script y lees el log, y RunAll.py se traga los errores de
cada paso para que un fallo no tumbe el resto. Esto lo caza antes.

Comprueba dos cosas:

  INEXISTENTES  nombres que no están en la API de Python de Unreal en ninguna
                versión. Son error seguro en cuanto se ejecuta la línea.
  OBSOLETAS     unreal.EditorLevelLibrary y unreal.EditorAssetLibrary, retiradas
                en favor de los subsistemas de editor desde 5.0. Funcionan aún,
                pero el proyecto pasa por Tools/ue5_compat.py, que prueba el
                subsistema primero y cae a la librería si hiciera falta.

ue5_compat.py está exento: es el único sitio donde puede nombrarse lo obsoleto.
"""
import os
import re
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
EXENTOS = {"ue5_compat.py", "ComprobarAPI58.py"}

# Nombre usado -> qué poner en su lugar.
INEXISTENTES = {
    "unreal.AssetHelpers":          "compat.crear_asset(...)",
    "unreal.AssetImportHelpers":    "compat.importar_tareas(...)",
    "unreal.add_component_to_actor": "compat.anadir_componente(actor, clase)",
    "unreal.MaterialBlendMode":     "unreal.BlendMode (compat.OPACO, ...)",
    "unreal.ShadingModel":          "unreal.MaterialShadingModel (compat.LIT, ...)",
    "unreal.Rotation":              "unreal.Rotator",
    "unreal.ProxyClassFactory":     "la factory concreta del asset",
    "unreal.GltfImportUI":          "sin options: el importador de 5.8 es Interchange",
    "unreal.HeightmapImportFactory": "importar el heightmap desde Landscape Mode",
    "get_world_settings()":         "compat.ajustes_mundo()",
    "get_all_level_metadata":       "compat.actores().get_all_level_actors_of_class(...)",
    "EditorAssetLibrary.load_class": "compat.cargar_clase(ruta)",
    # Valores de EBlendMode: son BLEND_*, nunca BM_*.
    "BlendMode.BM_":                "unreal.BlendMode.BLEND_* (compat.OPACO, ...)",
}

OBSOLETAS = {
    "unreal.EditorLevelLibrary": "compat.actores() / compat.nivel() / compat.mundo()",
    "unreal.EditorAssetLibrary": "compat.assets()",
}


def revisar(ruta):
    """-> lista de (linea, texto, encontrado, sugerencia, es_error)."""
    fallos = []
    with open(ruta, encoding="utf-8") as f:
        for n, linea in enumerate(f, 1):
            codigo = linea.split("#", 1)[0]
            for mal, bien in INEXISTENTES.items():
                if mal in codigo:
                    fallos.append((n, linea.strip(), mal, bien, True))
            for mal, bien in OBSOLETAS.items():
                if mal in codigo:
                    fallos.append((n, linea.strip(), mal, bien, False))
    return fallos


def main():
    scripts = sorted(f for f in os.listdir(TOOLS)
                     if f.endswith(".py") and f not in EXENTOS)

    errores = 0
    avisos = 0
    for nombre in scripts:
        fallos = revisar(os.path.join(TOOLS, nombre))
        if not fallos:
            continue
        print(f"\n{nombre}")
        for n, texto, mal, bien, es_error in fallos:
            marca = "ERROR " if es_error else "aviso "
            print(f"  {marca}L{n:<4} {mal}  ->  {bien}")
            print(f"         {texto[:100]}")
            if es_error:
                errores += 1
            else:
                avisos += 1

    print(f"\n{len(scripts)} scripts revisados: {errores} errores, {avisos} avisos")
    if errores:
        print("Los errores son AttributeError seguros en cuanto se ejecute la línea.")
    return 1 if errores else 0


if __name__ == "__main__":
    sys.exit(main())
