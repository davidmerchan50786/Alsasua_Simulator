"""
SetupFoliageWind.py — Añade UAlsasuaFoliageWindComponent a todo el foliage del nivel.
Detecta actores de foliage y les asigna el componente de viento.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def add_wind_to_foliage():
    """Añade UAlsasuaFoliageWindComponent a actores FoliageInstancedStaticMeshComponent."""
    foliage_class = unreal.load_class(
        "/Script/Engine.FoliageInstancedStaticMeshComponent")

    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaFoliageWindComponent")

    if not wind_class:
        unreal.log_error("[FoliageWind] No se pudo cargar UAlsasuaFoliageWindComponent")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    count = 0

    for actor in actors:
        comps = actor.get_components_by_class(unreal.ActorComponent)
        has_foliage = False
        for comp in comps:
            if comp.get_class().get_name() == "FoliageInstancedStaticMeshComponent":
                has_foliage = True
                break

        if has_foliage:
            existing = actor.get_component_by_class(wind_class)
            if not existing:
                comp = unreal.add_component_to_actor(actor, wind_class)
                if comp:
                    count += 1

    unreal.log(f"[FoliageWind] Componentes añadidos: {count}")


def add_wind_to_tree_actors():
    """Añade viento a actores de árboles individuales."""
    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaFoliageWindComponent")
    if not wind_class:
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["tree", "arbol", "árbol", "beech", "oak", "birch"]):
            existing = actor.get_component_by_class(wind_class)
            if not existing:
                comp = unreal.add_component_to_actor(actor, wind_class)
                if comp:
                    count += 1

    unreal.log(f"[FoliageWind] Árboles con viento: {count}")


if __name__ == "__main__":
    unreal.log("=== Foliage Wind Setup ===")
    add_wind_to_foliage()
    add_wind_to_tree_actors()
    unreal.log("=== Foliage Wind Complete ===")
