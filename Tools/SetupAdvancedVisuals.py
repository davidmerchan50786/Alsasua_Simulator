"""
SetupAdvancedVisuals.py — Añade sistemas visuales avanzados al nivel.
Incluye: flow map de agua, decals de carretera, fachadas, probes de luz,
post-process stack, densidad de foliage, blend de terreno, reverb zones.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def add_water_flow_map():
    """Añade UAlsasuaWaterFlowMap al agua."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaWaterFlowMap")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaWaterFlowMap")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["agua", "water", "rio", "river", "arakil"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} actores de agua con flow map")


def add_road_decals():
    """Añade UAlsasuaRoadDecalSystem a carreteras."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaRoadDecalSystem")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaRoadDecalSystem")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["calle", "road", "street", "acera", "carrera"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} carreteras con road decals")


def add_building_facades():
    """Añade UAlsasuaBuildingFacadeSystem a edificios."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaBuildingFacadeSystem")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaBuildingFacadeSystem")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "casa", "bloque", "pisos"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} edificios con fachadas procedurales")


def add_light_probes():
    """Añade UAlsasuaLightProbeSystem a edificios interiores."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaLightProbeSystem")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaLightProbeSystem")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "interior", "iglesia"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} edificios con light probes")


def add_post_process_stack():
    """Añade UAlsasuaPostProcessStack al jugador."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaPostProcessStack")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaPostProcessStack")
        return

    pawns = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.Pawn)
    count = 0
    for pawn in pawns:
        if not pawn.get_component_by_class(cls):
            comp = unreal.add_component_to_actor(pawn, cls)
            if comp:
                count += 1
    unreal.log(f"[AdvancedVisuals] {count} post-process stacks")


def add_foliage_density():
    """Añade UAlsasuaFoliageDensitySystem al nivel."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaFoliageDensitySystem")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaFoliageDensitySystem")
        return

    world_settings = unreal.EditorLevelLibrary.get_world_settings()
    if world_settings:
        if not world_settings.get_component_by_class(cls):
            comp = unreal.add_component_to_actor(world_settings, cls)
            if comp:
                unreal.log("[AdvancedVisuals] FoliageDensitySystem añadido")


def add_terrain_blender():
    """Añade UAlsasuaTerrainMaterialBlender al terreno."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaTerrainMaterialBlender")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaTerrainMaterialBlender")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["terreno", "terrain", "landscape"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} terrenos con terrain blender")


def add_reverb_zones():
    """Añade UAlsasuaReverbZoneSystem a edificios y lugares especiales."""
    cls = unreal.load_class("/Script/AlsasuaManifa.AlsasuaReverbZoneSystem")
    if not cls:
        unreal.log_error("[AdvancedVisuals] No se pudo cargar UAlsasuaReverbZoneSystem")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "iglesia", "church",
                                    "cave", "cueva", "callejon", "alley", "interior"]):
            if not actor.get_component_by_class(cls):
                comp = unreal.add_component_to_actor(actor, cls)
                if comp:
                    count += 1
    unreal.log(f"[AdvancedVisuals] {count} reverb zones")


if __name__ == "__main__":
    unreal.log("=== Advanced Visuals Setup ===")
    add_water_flow_map()
    add_road_decals()
    add_building_facades()
    add_light_probes()
    add_post_process_stack()
    add_foliage_density()
    add_terrain_blender()
    add_reverb_zones()
    unreal.log("=== Advanced Visuals Complete ===")
