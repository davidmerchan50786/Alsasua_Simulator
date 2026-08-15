"""
SetupBarrioStyles.py — Aplica estilos reales de barrios a edificios.
Lee nighborhoods.json y asigna materiales correctos a cada edificio.

Ejecutar en editor:  Tools > Execute Python Script
"""
import json
import os
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def load_neighborhoods():
    """Carga nighborhoods.json."""
    project_dir = unreal.Paths.project_dir()
    json_path = os.path.join(project_dir, "Content", "Datos", "nighborhoods.json")

    if not os.path.exists(json_path):
        unreal.log_warning("[BarrioStyles] No se encontró nighborhoods.json")
        return {}

    with open(json_path, 'r') as f:
        data = json.load(f)

    barrios = {}
    for barrio in data.get("barrios", []):
        barrios[barrio["id"]] = barrio
    return barrios


def load_buildings():
    """Carga buildings_final.json."""
    project_dir = unreal.Paths.project_dir()
    json_path = os.path.join(project_dir, "Content", "Datos", "buildings_final.json")

    if not os.path.exists(json_path):
        unreal.log_warning("[BarrioStyles] No se encontró buildings_final.json")
        return []

    with open(json_path, 'r') as f:
        data = json.load(f)

    return data if isinstance(data, list) else data.get("buildings", [])


def get_barrio_for_building(building, barrios):
    """Determina el barrio de un edificio por su posición."""
    x = building.get("x", 0)
    z = building.get("z", 0)

    closest_barrio = None
    min_dist = float('inf')

    for barrio_id, barrio in barrios.items():
        center = barrio.get("centro", {})
        cx = center.get("x", 0)
        cz = center.get("z", 0)
        radius = barrio.get("radio_m", 300)

        dist = ((x - cx) ** 2 + (z - cz) ** 2) ** 0.5
        if dist < radius and dist < min_dist:
            min_dist = dist
            closest_barrio = barrio

    return closest_barrio


def apply_material_by_barrio():
    """Aplica materiales basados en el barrio real."""
    barrios = load_neighborhoods()
    buildings = load_buildings()

    if not barrios or not buildings:
        return

    # Materiales reales por tipo de fachada
    material_map = {
        "Piedra caliza": "/Game/Materials/M_Fachada_Piedra",
        "Hormigón pintado": "/Game/Materials/M_Fachada_Hormigon",
        "Hormigón": "/Game/Materials/M_Fachada_Hormigon",
        "Ladrillo": "/Game/Materials/M_Fachada_Ladrillo",
        "Piedra y hormigón": "/Game/Materials/M_Fachada_Piedra",
        "Piedra": "/Game/Materials/M_Fachada_Piedra",
        "Ladrillo industrial": "/Game/Materials/M_Fachada_Ladrillo",
        "Piedra rústica": "/Game/Materials/M_Fachada_Piedra",
    }

    # Colores de tejado reales
    roof_colors = {
        "Terracota": (0.7, 0.35, 0.15),
        "Gris": (0.5, 0.5, 0.5),
        "Plano gris": (0.55, 0.55, 0.55),
        "Mixto": (0.6, 0.55, 0.5),
        "Oxidado": (0.5, 0.3, 0.15),
        "Pizarra": (0.3, 0.3, 0.32),
    }

    count = 0
    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)

    for actor in actors:
        name = actor.get_actor_label()
        if "edificio" not in name.lower() and "building" not in name.lower():
            continue

        # Intentar extraer índice del edificio
        try:
            idx = int(name.split("_")[-1])
        except ValueError:
            continue

        if idx >= len(buildings):
            continue

        building = buildings[idx]
        barrio = get_barrio_for_building(building, barrios)

        if not barrio:
            continue

        # Aplicar material de fachada
        facade_type = barrio.get("material_fachada", "Piedra")
        mat_path = material_map.get(facade_type, "/Game/Materials/M_Fachada_Piedra")

        mat = compat.assets().load_asset(mat_path)
        if not mat:
            continue

        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for comp in comps:
            num_mats = comp.get_num_materials()
            for i in range(num_mats):
                if i == 0:  # Fachada
                    comp.set_material(i, mat)
                    count += 1

    unreal.log(f"[BarrioStyles] {count} materiales de fachada aplicados por barrio")


if __name__ == "__main__":
    unreal.log("=== Barrio Styles Setup ===")
    apply_material_by_barrio()
    unreal.log("=== Barrio Styles Complete ===")
