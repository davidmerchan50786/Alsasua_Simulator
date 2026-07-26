"""
AlsasuaManifa - Script de integración Megascans Foliage.
Ejecutar después de importar packs de Megascans vía Fab.

Crea FoliageType assets para grass, bushes y árboles Europeos decíduos.
Requiere que los meshes estén en /Game/Meshes/Foliage/
"""

import unreal
import os

FOLIAGE_DIR = "/Game/Meshes/Foliage"
FOLIAGE_TYPE_DIR = "/Game/FoliageTypes"

# Megascans European Deciduous species mapping
SPECIES_CONFIG = {
    "EuropeanBeech": {
        "mesh_pattern": "SM_EuropeanBeech",
        "scale_min": 0.8,
        "scale_max": 1.5,
        "density": 0.003,
        "align_to_normal": True,
        "random_yaw": True,
    },
    "EuropeanOak": {
        "mesh_pattern": "SM_EuropeanOak",
        "scale_min": 0.9,
        "scale_max": 1.6,
        "density": 0.002,
        "align_to_normal": True,
        "random_yaw": True,
    },
    "EuropeanBirch": {
        "mesh_pattern": "SM_EuropeanBirch",
        "scale_min": 0.7,
        "scale_max": 1.3,
        "density": 0.004,
        "align_to_normal": True,
        "random_yaw": True,
    },
    "EuropeanElm": {
        "mesh_pattern": "SM_EuropeanElm",
        "scale_min": 0.8,
        "scale_max": 1.4,
        "density": 0.002,
        "align_to_normal": True,
        "random_yaw": True,
    },
    "EuropeanLime": {
        "mesh_pattern": "SM_EuropeanLime",
        "scale_min": 0.7,
        "scale_max": 1.2,
        "density": 0.003,
        "align_to_normal": True,
        "random_yaw": True,
    },
}

GRASS_MESHES = [
    "SM_GrassCluster01",
    "SM_GrassCluster02",
    "SM_GrassCluster03",
]

BUSH_MESHES = [
    "SM_Bush01",
    "SM_Bush02",
    "SM_Bush03",
]


def find_foliage_meshes(pattern):
    all_meshes = unreal.EditorAssetLibrary.list_assets(FOLIAGE_DIR, True, True)
    matching = []
    for path in all_meshes:
        name = path.split("/")[-1].split(".")[0]
        if pattern.lower() in name.lower() and path.endswith(".StaticMesh"):
            matching.append(path)
    return matching


def create_foliage_type_asset(name, mesh_path, config):
    if not unreal.EditorAssetLibrary.does_asset_exist(FOLIAGE_TYPE_DIR):
        unreal.EditorAssetLibrary.make_directory(FOLIAGE_TYPE_DIR)

    asset_path = f"{FOLIAGE_TYPE_DIR}/FT_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"[MegascansFoliage] FoliageType ya existe: {asset_path}")
        return asset_path

    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        unreal.log_warning(f"[MegascansFoliage] Mesh no encontrado: {mesh_path}")
        return None

    foliage_type = unreal.FoliageType()
    foliage_type.set_editor_property("mesh", mesh)

    foliage_type.set_editor_property("scaling", unreal.FoliageScaling.UNIFORM)
    foliage_type.set_editor_property("uniform_scale", unreal.FloatInterval(
        config.get("scale_min", 0.8), config.get("scale_max", 1.5)))

    foliage_type.set_editor_property("density", config.get("density", 0.003))
    foliage_type.set_editor_property("radius", 15000.0)
    foliage_type.set_editor_property("align_to_normal", config.get("align_to_normal", True))
    foliage_type.set_editor_property("random_yaw", config.get("random_yaw", True))

    foliage_type.set_editor_property("collision_with_world", True)
    foliage_type.set_editor_property("collision_with_others", False)

    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log(f"[MegascansFoliage] Creado: {asset_path}")
    return asset_path


def create_grass_foliage_types():
    unreal.log("[MegascansFoliage] === Creando Grass FoliageTypes ===")
    grass_config = {
        "scale_min": 0.5,
        "scale_max": 1.2,
        "density": 0.015,
        "align_to_normal": False,
        "random_yaw": True,
    }

    count = 0
    for mesh_name in GRASS_MESHES:
        meshes = find_foliage_meshes(mesh_name)
        for mesh_path in meshes:
            name = mesh_path.split("/")[-1].split(".")[0]
            result = create_foliage_type_asset(f"Grass_{name}", mesh_path, grass_config)
            if result:
                count += 1

    unreal.log(f"[MegascansFoliage] {count} grass FoliageTypes creados")
    return count


def create_bush_foliage_types():
    unreal.log("[MegascansFoliage] === Creando Bush FoliageTypes ===")
    bush_config = {
        "scale_min": 0.6,
        "scale_max": 1.4,
        "density": 0.005,
        "align_to_normal": True,
        "random_yaw": True,
    }

    count = 0
    for mesh_name in BUSH_MESHES:
        meshes = find_foliage_meshes(mesh_name)
        for mesh_path in meshes:
            name = mesh_path.split("/")[-1].split(".")[0]
            result = create_foliage_type_asset(f"Bush_{name}", mesh_path, bush_config)
            if result:
                count += 1

    unreal.log(f"[MegascansFoliage] {count} bush FoliageTypes creados")
    return count


def create_tree_foliage_types():
    unreal.log("[MegascansFoliage] === Creando Tree FoliageTypes ===")
    count = 0

    for species_name, config in SPECIES_CONFIG.items():
        meshes = find_foliage_meshes(config["mesh_pattern"])
        for mesh_path in meshes:
            name = mesh_path.split("/")[-1].split(".")[0]
            result = create_foliage_type_asset(f"Tree_{name}", mesh_path, config)
            if result:
                count += 1

    unreal.log(f"[MegascansFoliage] {count} tree FoliageTypes creados")
    return count


def setup_foliage_instances():
    unreal.log("[MegascansFoliage] === Configurando Foliage instances ===")

    foliage_actor = None
    world = unreal.EditorLevelLibrary.get_editor_world()
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.FoliageType)
    for actor in all_actors:
        foliage_actor = actor
        break

    if not foliage_actor:
        foliage_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.Actor, unreal.Vector(0, 0, 0))
        if foliage_actor:
            foliage_actor.set_actor_label("Alsasua_Foliage")
            foliage_actor.set_actor_mobility(unreal.ComponentMobility.STATIC)

    ft_paths = unreal.EditorAssetLibrary.list_assets(FOLIAGE_TYPE_DIR, True, True)
    ft_count = 0
    for path in ft_paths:
        if path.endswith(".FoliageType"):
            ft_count += 1

    unreal.log(f"[MegascansFoliage] {ft_count} FoliageTypes disponibles para paint en editor")
    return ft_count


def main():
    import time
    start = time.time()
    unreal.log("=" * 60)
    unreal.log("[MegascansFoliage] Iniciando integración Megascans Foliage...")
    unreal.log("=" * 60)

    total = 0
    total += create_grass_foliage_types()
    total += create_bush_foliage_types()
    total += create_tree_foliage_types()
    total += setup_foliage_instances()

    elapsed = time.time() - start
    unreal.log("=" * 60)
    unreal.log(f"[MegascansFoliage] COMPLETADO en {elapsed:.1f}s - {total} assets procesados")
    unreal.log("=" * 60)


if __name__ == "__main__":
    main()
