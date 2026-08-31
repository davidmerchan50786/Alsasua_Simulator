"""
SetupRainParticles.py — Enchufa el componente de lluvia al jugador.

Los sistemas Niagara los crea create_niagara_vfx.py, que es quien sabe
configurarlos; aquí sólo se adjunta UAlsasuaRainParticleComponent al peón y al
PlayerController.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

# Los tres creadores de asset que había aquí —NS_Rain, NS_Snow y
# NS_ThunderFlash— se han quitado, y no por duplicados: creaban el asset VACÍO,
# sin emisor ni módulos, y RunAll.py ejecuta este script JUSTO ANTES que
# create_niagara_vfx.py, que es quien sí sabe configurarlos. Al llegar allí el
# nombre ya estaba cogido, create_asset devolvía None, y la rama de respaldo
# cargaba el asset vacío y se salía. Resultado: la lluvia, la nieve y el
# relámpago eran sistemas Niagara sin una sola partícula, y no lo decía nadie.
#
# Crear los assets es trabajo de create_niagara_vfx.py. El de este script es
# enchufarle el componente de lluvia al jugador, que es lo que queda debajo.

def add_rain_to_player():
    """Añade UAlsasuaRainParticleComponent al jugador."""
    wind_class = unreal.load_class(
        "/Script/GF_World.AlsasuaRainParticleComponent")
    if not wind_class:
        unreal.log_error("[RainParticles] No se pudo cargar UAlsasuaRainParticleComponent")
        return

    pawns = compat.actores().get_all_level_actors_of_class(
        unreal.Pawn)
    for pawn in pawns:
        existing = pawn.get_component_by_class(wind_class)
        if not existing:
            comp = compat.anadir_componente(pawn, wind_class)
            if comp:
                unreal.log(f"[RainParticles] Componente añadido a {pawn.get_actor_label()}")


def add_rain_to_player_controller():
    """También intenta añadir al PlayerController por si el pawn no existe."""
    wind_class = unreal.load_class(
        "/Script/GF_World.AlsasuaRainParticleComponent")
    if not wind_class:
        return

    controllers = compat.actores().get_all_level_actors_of_class(
        unreal.PlayerController)
    for pc in controllers:
        existing = pc.get_component_by_class(wind_class)
        if not existing:
            comp = compat.anadir_componente(pc, wind_class)
            if comp:
                unreal.log(f"[RainParticles] Componente añadido a {pc.get_actor_label()}")


if __name__ == "__main__":
    unreal.log("=== Rain Particles Setup ===")
    add_rain_to_player()
    add_rain_to_player_controller()
    unreal.log("=== Rain Particles Complete ===")
