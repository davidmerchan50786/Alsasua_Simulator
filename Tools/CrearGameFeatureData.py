"""Crea el asset GameFeatureData que exige cada plugin GF_* con contenido.

El motor busca /<Plugin>/GameFeatureData.GameFeatureData al registrar un
game feature plugin; sin ese uasset la maquina de estados cae en
ErrorRegistering (Plugin_Missing_GameFeatureData).

Uso (una sola vez, binario del EDITOR):
  UnrealEditor-Cmd.exe <proyecto> -run=python -script="Tools/CrearGameFeatureData.py"
"""
import os

import unreal

PLUGINS = [
    "GF_Abilities", "GF_AI", "GF_Audio", "GF_Carreteras", "GF_Clima",
    "GF_Core", "GF_Debug", "GF_Dialogos", "GF_Edificios", "GF_Ferrocarril",
    "GF_NPCs", "GF_Optimization", "GF_Politica", "GF_Social",
    "GF_Systems", "GF_Trafico", "GF_UI", "GF_Vehiculos", "GF_Vegetacion",
    "GF_World",
]


def main():
    herramientas = unreal.AssetToolsHelpers.get_asset_tools()
    biblioteca = unreal.EditorAssetLibrary
    creados = 0
    for nombre in PLUGINS:
        ruta = "/%s/GameFeatureData" % nombre
        if biblioteca.does_asset_exist(ruta):
            unreal.log("[GFD] ya existe: %s" % ruta)
            continue
        asset = herramientas.create_asset(
            "GameFeatureData", "/%s" % nombre, unreal.GameFeatureData, None)
        if not asset:
            unreal.log_error("[GFD] fallo creando %s" % ruta)
            continue
        biblioteca.save_loaded_asset(asset)
        creados += 1
        unreal.log("[GFD] creado: %s" % ruta)
    unreal.log("[GFD] total creados: %d de %d" % (creados, len(PLUGINS)))


main()
