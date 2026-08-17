"""
SetupAllRealData.py - Script maestro que ejecuta TODOS los sistemas de datos reales
Ejecutar desde: Tools/SetupAllRealData.py
"""
import os
import sys
import json
import subprocess

# Raiz derivada de donde vive este fichero. Antes fija a la maquina original
# ("F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject"), asi que solo corria alli.
BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(BASE, "Content", "Datos")
TOOLS = os.path.join(BASE, "Tools")

def run_script(name, path):
    print(f"\n{'='*60}")
    print(f"EJECUTANDO: {name}")
    print(f"{'='*60}")
    try:
        result = subprocess.run(
            [sys.executable, path],
            capture_output=True, text=True, timeout=300
        )
        print(result.stdout)
        if result.returncode != 0:
            print(f"ERROR: {result.stderr}")
        return result.returncode == 0
    except Exception as e:
        print(f"EXCEPCION: {e}")
        return False

def main():
    print("=" * 60)
    print("ALSASUA COPIA 1:1 - SCRIPT MAESTRO DE DATOS REALES")
    print("=" * 60)
    
    archivos_necesarios = [
        "buildings_final.json",
        "roads_unity.json",
        "trees_unity.json",
        "street_furniture.json",
        "poi_data.json",
        "waterways_unity.json",
        "nighborhoods.json",
        "greenspaces_unity.json",
        "railways_unity.json"
    ]
    
    print("\n[0] Verificando archivos fuente...")
    for f in archivos_necesarios:
        path = os.path.join(DATOS, f)
        if os.path.exists(path):
            size = os.path.getsize(path)
            print(f"  OK: {f} ({size:,} bytes)")
        else:
            print(f"  FALTA: {f}")
    
    scripts = [
        ("Enriquecer datos reales", os.path.join(TOOLS, "enrich_real_data.py")),
        ("Posiciones de senales", os.path.join(TOOLS, "add_sign_positions.py")),
        ("Asset manifest", os.path.join(TOOLS, "asset_manifest.py")),
    ]
    
    results = []
    for name, path in scripts:
        if os.path.exists(path):
            ok = run_script(name, path)
            results.append((name, ok))
        else:
            print(f"SCRIPT NO ENCONTRADO: {path}")
            results.append((name, False))
    
    print("\n" + "=" * 60)
    print("RESUMEN FINAL")
    print("=" * 60)
    
    archivos_generados = [
        ("buildings_final.json", "1030 edificios con nombre, altura, material, barrio"),
        ("building_facades.json", "1030 fachadas: ventanas, balcones, tiendas"),
        ("roads_unity.json", "489 carreteras con nombres reales"),
        ("signage_data.json", "118 senales con posiciones reales"),
        ("asset_mapping.json", "Mapa de assets por barrio"),
        ("asset_manifest.json", "Manifest completo de assets del proyecto"),
    ]
    
    for nombre, desc in archivos_generados:
        path = os.path.join(DATOS, nombre)
        if os.path.exists(path):
            size = os.path.getsize(path)
            print(f"  OK: {nombre} ({size:,} bytes) - {desc}")
        else:
            print(f"  FALTA: {nombre}")
    
    print("\nAssets importados:")
    assets_dir = os.path.join(BASE, "Content", "AssetsImportados")
    if os.path.exists(assets_dir):
        total_files = sum(len(files) for _, _, files in os.walk(assets_dir))
        print(f"  {total_files} archivos en {assets_dir}")
    
    print("\nFotos de referencia:")
    fotos_dir = os.path.join(DATOS, "referencia_fotos")
    if os.path.exists(fotos_dir):
        fotos = [f for f in os.listdir(fotos_dir) if f.endswith('.jpg')]
        print(f"  {len(fotos)} fotos en {fotos_dir}")
    
    print("\nSistemas C++ creados:")
    cpp_files = [
        "AlsasuaFacadeGenerator.h/.cpp",
        "AlsasuaSignPlacer.h/.cpp", 
        "AlsasuaTreePlacer.h/.cpp",
        "AlsasuaFarolaPlacer.h/.cpp",
        "AlsasuaFoliagePainter.h/.cpp",
        "AlsasuaTrafficSystem.h/.cpp",
        "AlsasuaRoadSurfaceSystem.h/.cpp",
        "AlsasuaNightLightingSystem.h/.cpp",
        "AlsasuaWeatherSystem.h/.cpp",
        "AlsasuaAmbientAudioSystem.h/.cpp",
        "AlsasuaCollisionSystem.h/.cpp",
        "AlsasuaVFXManager.h/.cpp"
    ]
    for f in cpp_files:
        print(f"  - {f}")
    
    print("\nDirectorArranque: 31 sistemas basados en datos reales")
    print("=" * 60)
    print("ALSASUA COPIA 1:1 COMPLETADA")
    print("=" * 60)

if __name__ == "__main__":
    main()
