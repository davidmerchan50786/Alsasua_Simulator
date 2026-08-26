"""
SetupWaterReflections.py — Añade UAlsasuaWaterReflectionManager al río.
Configura reflejos planos y parámetros de agua dinámicos.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_water_reflections():
    """Añade UAlsasuaWaterReflectionManager al agua del río."""
    reflection_class = unreal.load_class(
        "/Script/GF_World.AlsasuaWaterReflectionManager")
    if not reflection_class:
        unreal.log_error("[WaterReflections] No se pudo cargar UAlsasuaWaterReflectionManager")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["agua", "water", "rio", "river", "arakil"]):
            existing = actor.get_component_by_class(reflection_class)
            if not existing:
                comp = compat.anadir_componente(actor, reflection_class)
                if comp:
                    count += 1

    unreal.log(f"[WaterReflections] {count} actores de agua con reflejos")


def add_fog_to_valley():
    """Añade UAlsasuaAtmosphereFogComponent al nivel para niebla de valle."""
    fog_class = unreal.load_class(
        "/Script/GF_Clima.AlsasuaAtmosphereFogComponent")
    if not fog_class:
        unreal.log_error("[ValleyFog] No se pudo cargar UAlsasuaAtmosphereFogComponent")
        return

    world_settings = compat.ajustes_mundo()
    if world_settings:
        existing = world_settings.get_component_by_class(fog_class)
        if not existing:
            comp = compat.anadir_componente(world_settings, fog_class)
            if comp:
                unreal.log("[ValleyFog] AtmosphereFogComponent añadido al WorldSettings")


if __name__ == "__main__":
    unreal.log("=== Water Reflections Setup ===")
    add_water_reflections()
    add_fog_to_valley()
    unreal.log("=== Water Reflections Complete ===")
