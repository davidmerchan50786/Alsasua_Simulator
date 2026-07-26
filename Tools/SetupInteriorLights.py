"""
SetupInteriorLights.py — Añade UAlsasuaInteriorLightComponent a todos los edificios.
Crea luces puntuales dentro de las ventanas para simular habitaciones iluminadas.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def add_interior_lights_to_buildings():
    """Añade UAlsasuaInteriorLightComponent a edificios."""
    light_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaInteriorLightComponent")
    if not light_class:
        unreal.log_error("[InteriorLights] No se pudo cargar UAlsasuaInteriorLightComponent")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "casa", "bloque",
                                    "pisos", "vivienda"]):
            existing = actor.get_component_by_class(light_class)
            if not existing:
                comp = unreal.add_component_to_actor(actor, light_class)
                if comp:
                    count += 1

    unreal.log(f"[InteriorLights] {count} edifices con luces interiores")


if __name__ == "__main__":
    unreal.log("=== Interior Lights Setup ===")
    add_interior_lights_to_buildings()
    unreal.log("=== Interior Lights Complete ===")
