"""
SetupInteriorLights.py — Añade UAlsasuaInteriorLightComponent a todos los edificios.
Crea luces puntuales dentro de las ventanas para simular habitaciones iluminadas.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_interior_lights_to_buildings():
    """Añade UAlsasuaInteriorLightComponent a edificios."""
    light_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaInteriorLightComponent")
    if not light_class:
        unreal.log_error("[InteriorLights] No se pudo cargar UAlsasuaInteriorLightComponent")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "casa", "bloque",
                                    "pisos", "vivienda"]):
            existing = actor.get_component_by_class(light_class)
            if not existing:
                comp = compat.anadir_componente(actor, light_class)
                if comp:
                    count += 1

    unreal.log(f"[InteriorLights] {count} edifices con luces interiores")


if __name__ == "__main__":
    unreal.log("=== Interior Lights Setup ===")
    add_interior_lights_to_buildings()
    unreal.log("=== Interior Lights Complete ===")
