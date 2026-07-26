"""
SetupAmbientParticles.py — Añade UAlsasuaAmbientParticles al jugador.
Crea partículas ambientales: polvo, polen, hojas, luciérnagas, niebla de lluvia.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def add_ambient_particles():
    """Añade UAlsasuaAmbientParticles al jugador."""
    ambient_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaAmbientParticles")
    if not ambient_class:
        unreal.log_error("[AmbientParticles] No se pudo cargar UAlsasuaAmbientParticles")
        return

    pawns = unreal.EditorLevelLibrary.get_all_level_actors_of_class(unreal.Pawn)
    count = 0
    for pawn in pawns:
        existing = pawn.get_component_by_class(ambient_class)
        if not existing:
            comp = unreal.add_component_to_actor(pawn, ambient_class)
            if comp:
                count += 1

    controllers = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.PlayerController)
    for pc in controllers:
        existing = pc.get_component_by_class(ambient_class)
        if not existing:
            comp = unreal.add_component_to_actor(pc, ambient_class)
            if comp:
                count += 1

    unreal.log(f"[AmbientParticles] Componentes añadidos: {count}")


if __name__ == "__main__":
    unreal.log("=== Ambient Particles Setup ===")
    add_ambient_particles()
    unreal.log("=== Ambient Particles Complete ===")
