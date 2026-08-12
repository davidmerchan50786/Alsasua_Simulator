"""
SetupDecals.py — Añade UAlsasuaDecalSystem a carreteras y aceras.
Genera marcas de carretera, charcos, grietas, y desgaste procedurales.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_decals_to_roads():
    """Añade UAlsasuaDecalSystem a carreteras."""
    decal_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaDecalSystem")
    if not decal_class:
        unreal.log_error("[Decals] No se pudo cargar UAlsasuaDecalSystem")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["calle", "road", "street", "acera",
                                    "sidewalk", "carrera", "pasarela"]):
            existing = actor.get_component_by_class(decal_class)
            if not existing:
                comp = compat.anadir_componente(actor, decal_class)
                if comp:
                    count += 1

    unreal.log(f"[Decals] {count} carreteras con decals")


if __name__ == "__main__":
    unreal.log("=== Decals Setup ===")
    add_decals_to_roads()
    unreal.log("=== Decals Complete ===")
