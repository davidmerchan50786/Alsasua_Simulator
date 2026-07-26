"""
SetupFacadeDetails.py — Añade UAlsasuaFacadeDetailSystem a edificios.
Genera balcones, persianas, macetas, toldos, y aire acondicionado.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def add_facade_details():
    """Añade UAlsasuaFacadeDetailSystem a edificios."""
    facade_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaFacadeDetailSystem")
    if not facade_class:
        unreal.log_error("[FacadeDetails] No se pudo cargar UAlsasuaFacadeDetailSystem")
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "casa", "bloque",
                                    "pisos", "vivienda"]):
            existing = actor.get_component_by_class(facade_class)
            if not existing:
                comp = unreal.add_component_to_actor(actor, facade_class)
                if comp:
                    count += 1

    unreal.log(f"[FacadeDetails] {count} edificios con detalles de fachada")


if __name__ == "__main__":
    unreal.log("=== Facade Details Setup ===")
    add_facade_details()
    unreal.log("=== Facade Details Complete ===")
