"""
SetupAmbientParticles.py — Añade UAlsasuaAmbientParticles al jugador.
Crea partículas ambientales: polvo, polen, hojas, luciérnagas, niebla de lluvia.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_ambient_particles():
    """Añade UAlsasuaAmbientParticles al jugador."""
    ambient_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaAmbientParticles")
    if not ambient_class:
        unreal.log_error("[AmbientParticles] No se pudo cargar UAlsasuaAmbientParticles")
        return

    pawns = compat.actores().get_all_level_actors_of_class(unreal.Pawn)
    count = 0
    for pawn in pawns:
        existing = pawn.get_component_by_class(ambient_class)
        if not existing:
            comp = compat.anadir_componente(pawn, ambient_class)
            if comp:
                count += 1

    controllers = compat.actores().get_all_level_actors_of_class(
        unreal.PlayerController)
    for pc in controllers:
        existing = pc.get_component_by_class(ambient_class)
        if not existing:
            comp = compat.anadir_componente(pc, ambient_class)
            if comp:
                count += 1

    unreal.log(f"[AmbientParticles] Componentes añadidos: {count}")


if __name__ == "__main__":
    unreal.log("=== Ambient Particles Setup ===")
    add_ambient_particles()
    unreal.log("=== Ambient Particles Complete ===")
