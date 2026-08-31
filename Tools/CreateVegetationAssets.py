"""
Script para crear Data Assets de vegetación nativa de Navarra en Unreal Editor.

EJECUTAR EN UNREAL EDITOR:
1. Abrir Alsasua Simulator en UE 5.8
2. Tools > Execute Python Script
3. Seleccionar este archivo

O desde la consola de Python en Unreal:
exec(open(r'F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Tools/CreateVegetationAssets.py').read())
"""

import unreal

def resolve_vegetation_asset_class():
    """Resuelve la clase UVegetationType del módulo del proyecto."""
    for class_path in [
        "/Script/GF_Vegetacion.VegetationType",
        "/Script/GF_Vegetacion.UVegetationType",
        "/Script/Engine.DataAsset",
    ]:
        asset_class = unreal.load_class(class_path)
        if asset_class:
            return asset_class
    return None


def create_vegetation_data_asset(asset_name, asset_path, config):
    """
    Crea un Data Asset de tipo UVegetationType con la configuración especificada.
    
    Args:
        asset_name: Nombre del asset (ej: "VT_Aliso_Ribera")
        asset_path: Ruta en Content (ej: "/Game/Vegetation/DataAssets/")
        config: Diccionario con configuración de vegetación
    """
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_class = resolve_vegetation_asset_class()
    if not asset_class:
        unreal.log_error("No se pudo resolver UVegetationType")
        return None
    
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", asset_class)
    
    full_path = asset_path + asset_name
    
    data_asset = asset_tools.create_asset(
        asset_name=asset_name,
        package_path=asset_path.rstrip('/'),
        asset_class=asset_class,
        factory=factory
    )
    
    if not data_asset:
        unreal.log_error(f"No se pudo crear {asset_name}")
        return None
    
    data_asset.set_editor_property("bEnabled", True)
    data_asset.set_editor_property("TypeName", config.get("type_name", asset_name))
    data_asset.set_editor_property("Seed", config.get("seed", 0))
    
    data_asset.set_editor_property("GlobalProbability", config.get("global_probability", 25.0))
    data_asset.set_editor_property("DensityPerM2", config.get("density_per_m2", 0.5))
    data_asset.set_editor_property("MinDistance", config.get("min_distance", 5.0))
    
    height_range = unreal.Vector2D(
        config.get("height_min", 0.0),
        config.get("height_max", 1000.0)
    )
    data_asset.set_editor_property("HeightRange", height_range)
    
    slope_range = unreal.Vector2D(
        config.get("slope_min", 0.0),
        config.get("slope_max", 60.0)
    )
    data_asset.set_editor_property("SlopeRange", slope_range)
    
    scale_range = unreal.Vector2D(
        config.get("scale_min", 0.8),
        config.get("scale_max", 1.2)
    )
    data_asset.set_editor_property("ScaleRange", scale_range)
    
    data_asset.set_editor_property("bRandomRotation", config.get("random_rotation", True))
    data_asset.set_editor_property("SinkAmount", config.get("sink_amount", 0.0))
    data_asset.set_editor_property("bCollisionCheck", config.get("collision_check", False))
    data_asset.set_editor_property("bRejectUnderwater", config.get("reject_underwater", False))
    
    data_asset.set_editor_property("bEnableNanite", config.get("enable_nanite", True))
    data_asset.set_editor_property("bUseNaturalClustering", config.get("use_clustering", False))
    data_asset.set_editor_property("ClusterSize", config.get("cluster_size", 50.0))
    data_asset.set_editor_property("ClusterStrength", config.get("cluster_strength", 0.5))
    
    data_asset.set_editor_property("bRiverAffinity", config.get("river_affinity", False))
    water_dist_range = unreal.Vector2D(
        config.get("water_dist_min", 0.0),
        config.get("water_dist_max", 30.0)
    )
    data_asset.set_editor_property("WaterDistanceRange", water_dist_range)
    
    data_asset.set_editor_property("bNorthFacing", config.get("north_facing", False))
    data_asset.set_editor_property("bSouthFacing", config.get("south_facing", False))
    
    if "meshes" in config:
        prefabs = []
        for mesh_info in config["meshes"]:
            mesh_path = mesh_info["path"]
            mesh = unreal.load_asset(mesh_path)
            
            if mesh:
                prefab = unreal.VegetationPrefab()
                prefab.set_editor_property("Mesh", mesh)
                prefab.set_editor_property("Probability", mesh_info.get("probability", 100.0))
                prefabs.append(prefab)
            else:
                unreal.log_warning(f"No se pudo cargar mesh: {mesh_path}")
        
        if prefabs:
            data_asset.set_editor_property("Prefabs", prefabs)
    
    unreal.EditorAssetLibrary.save_loaded_asset(data_asset)
    
    unreal.log(f"✅ Creado: {full_path}")
    return data_asset

def create_all_vegetation_assets():
    """
    Crea todos los Data Assets de vegetación nativa de Navarra.
    """
    
    base_path = "/Game/Vegetation/DataAssets/"
    
    unreal.EditorAssetLibrary.make_directory(base_path)
    
    vegetation_configs = {
        "VT_Aliso_Ribera": {
            "type_name": "Aliso (Alnus glutinosa) - Ribera",
            "seed": 1001,
            "global_probability": 80.0,
            "density_per_m2": 1.5,
            "min_distance": 3.0,
            "height_min": 500.0,
            "height_max": 550.0,
            "slope_max": 15.0,
            "scale_min": 0.9,
            "scale_max": 1.3,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 30.0,
            "cluster_strength": 0.7,
            "river_affinity": True,
            "water_dist_min": 0.0,
            "water_dist_max": 15.0,
            "meshes": [
                {"path": "/Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree", "probability": 100.0}
            ]
        },
        
        "VT_Sauce_Ribera": {
            "type_name": "Sauce (Salix alba) - Ribera",
            "seed": 1002,
            "global_probability": 70.0,
            "density_per_m2": 1.0,
            "min_distance": 4.0,
            "height_min": 500.0,
            "height_max": 545.0,
            "slope_max": 10.0,
            "scale_min": 0.8,
            "scale_max": 1.2,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 25.0,
            "cluster_strength": 0.6,
            "river_affinity": True,
            "water_dist_min": 0.0,
            "water_dist_max": 20.0,
            "meshes": [
                {"path": "/Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree", "probability": 100.0}
            ]
        },
        
        "VT_Haya_Norte": {
            "type_name": "Haya (Fagus sylvatica) - Ladera Norte",
            "seed": 2001,
            "global_probability": 60.0,
            "density_per_m2": 0.8,
            "min_distance": 6.0,
            "height_min": 600.0,
            "height_max": 1200.0,
            "slope_min": 10.0,
            "slope_max": 50.0,
            "scale_min": 1.0,
            "scale_max": 1.4,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 50.0,
            "cluster_strength": 0.5,
            "north_facing": True,
            "meshes": [
                {"path": "/Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree", "probability": 100.0}
            ]
        },
        
        "VT_Roble_Sur": {
            "type_name": "Roble (Quercus robur) - Ladera Sur",
            "seed": 2002,
            "global_probability": 50.0,
            "density_per_m2": 0.5,
            "min_distance": 7.0,
            "height_min": 580.0,
            "height_max": 1000.0,
            "slope_min": 5.0,
            "slope_max": 45.0,
            "scale_min": 0.9,
            "scale_max": 1.3,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 60.0,
            "cluster_strength": 0.4,
            "south_facing": True,
            "meshes": [
                {"path": "/Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree", "probability": 100.0}
            ]
        },
        
        "VT_Abedul_Mixto": {
            "type_name": "Abedul (Betula pendula) - Mixto",
            "seed": 2003,
            "global_probability": 40.0,
            "density_per_m2": 0.3,
            "min_distance": 5.0,
            "height_min": 600.0,
            "height_max": 1100.0,
            "slope_max": 55.0,
            "scale_min": 0.8,
            "scale_max": 1.1,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 40.0,
            "cluster_strength": 0.6,
            "meshes": [
                {"path": "/Game/Nanite_Plants_Sample_Collection/Geometries/SM_3DGardenPlants_Acer_buergerianum_01_003_Free", "probability": 50.0},
                {"path": "/Game/Nanite_Plants_Sample_Collection/Geometries/SM_3DGardenPlants_Acer_buergerianum_02_002_Free", "probability": 50.0}
            ]
        },
        
        "VT_Arbusto_Sotobosque": {
            "type_name": "Arbustos - Sotobosque",
            "seed": 3001,
            "global_probability": 50.0,
            "density_per_m2": 2.0,
            "min_distance": 2.0,
            "height_min": 520.0,
            "height_max": 1000.0,
            "slope_max": 60.0,
            "scale_min": 0.6,
            "scale_max": 1.0,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 20.0,
            "cluster_strength": 0.8,
            "meshes": [
                {"path": "/Game/Nanite_Plants_Sample_Collection/Geometries/SM_Abelia_x_grandiflora_Nanite_Free_Sample", "probability": 100.0}
            ]
        },
        
        "VT_Flores_Silvestres": {
            "type_name": "Flores Silvestres - Prados",
            "seed": 4001,
            "global_probability": 40.0,
            "density_per_m2": 3.0,
            "min_distance": 1.0,
            "height_min": 520.0,
            "height_max": 800.0,
            "slope_max": 30.0,
            "scale_min": 0.5,
            "scale_max": 0.9,
            "enable_nanite": False,
            "use_clustering": True,
            "cluster_size": 15.0,
            "cluster_strength": 0.9,
            "meshes": [
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_01_1", "probability": 15.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_01_2", "probability": 15.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_02_1", "probability": 15.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_03_1", "probability": 15.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_04_1", "probability": 10.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_05_1", "probability": 10.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_06_1", "probability": 10.0},
                {"path": "/Game/OWD_Flowers_Pack/Meshes/SM_Flower_07_1", "probability": 10.0}
            ]
        },
        
        "VT_Hierba_Alta_Ribera": {
            "type_name": "Hierba Alta - Ribera",
            "seed": 4002,
            "global_probability": 60.0,
            "density_per_m2": 4.0,
            "min_distance": 0.8,
            "height_min": 500.0,
            "height_max": 560.0,
            "slope_max": 15.0,
            "scale_min": 0.7,
            "scale_max": 1.1,
            "enable_nanite": True,
            "use_clustering": True,
            "cluster_size": 10.0,
            "cluster_strength": 0.7,
            "river_affinity": True,
            "water_dist_min": 5.0,
            "water_dist_max": 30.0,
            "meshes": [
                {"path": "/Game/Nanite_Plants_Sample_Collection/Geometries/SM_Free_Lolium_perenne_3DGardenPlants", "probability": 60.0},
                {"path": "/Game/Nanite_Plants_Sample_Collection/Geometries/SM_Free_Ophiopogon_japonicus_3DGardenPlants", "probability": 40.0}
            ]
        }
    }
    
    unreal.log("=" * 60)
    unreal.log("CREANDO DATA ASSETS DE VEGETACIÓN NATIVA")
    unreal.log("Alsasua Simulator - Especies de Navarra")
    unreal.log("=" * 60)
    
    created_assets = []
    
    for asset_name, config in vegetation_configs.items():
        try:
            asset = create_vegetation_data_asset(asset_name, base_path, config)
            if asset:
                created_assets.append(asset_name)
        except Exception as e:
            unreal.log_error(f"Error creando {asset_name}: {str(e)}")

    # Fallback simple: si el tipo del proyecto no se resolvió, se crea un asset base
    # con la clase DataAsset para no dejar el script roto.
    if not created_assets:
        fallback_name = "VT_Fallback"
        fallback_config = {
            "type_name": "Fallback",
            "seed": 0,
            "global_probability": 25.0,
            "density_per_m2": 0.5,
            "min_distance": 5.0,
        }
        asset = create_vegetation_data_asset(fallback_name, base_path, fallback_config)
        if asset:
            created_assets.append(fallback_name)
    
    unreal.log("\n" + "=" * 60)
    unreal.log(f"✅ COMPLETADO: {len(created_assets)}/{len(vegetation_configs)} assets creados")
    unreal.log("=" * 60)
    unreal.log("\nAssets creados en: /Game/Vegetation/DataAssets/")
    unreal.log("\nPRÓXIMOS PASOS:")
    unreal.log("1. Revisar cada Data Asset en el Content Browser")
    unreal.log("2. Ajustar meshes si es necesario (algunos son placeholders)")
    unreal.log("3. Añadir los assets a VegetationSpawnerSubsystem en el nivel")
    unreal.log("4. Ejecutar 'Spawn All Vegetation' para probar")
    
    return created_assets

if __name__ == "__main__":
    create_all_vegetation_assets()
