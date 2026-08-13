"""
SetupFacadeDetails.py — Añade UAlsasuaFacadeDetailSystem a edificios.
Genera balcones, persianas, macetas, toldos, y aire acondicionado.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_facade_details():
    """Añade UAlsasuaFacadeDetailSystem a edificios."""
    facade_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaFacadeDetailSystem")
    if not facade_class:
        unreal.log_error("[FacadeDetails] No se pudo cargar UAlsasuaFacadeDetailSystem")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "casa", "bloque",
                                    "pisos", "vivienda"]):
            existing = actor.get_component_by_class(facade_class)
            if not existing:
                comp = compat.anadir_componente(actor, facade_class)
                if comp:
                    count += 1

    unreal.log(f"[FacadeDetails] {count} edificios con detalles de fachada")


if __name__ == "__main__":
    unreal.log("=== Facade Details Setup ===")
    add_facade_details()
    unreal.log("=== Facade Details Complete ===")
