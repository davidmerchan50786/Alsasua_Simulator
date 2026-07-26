"""
SetupStreetLights.py — Coloca farolas con UAlsasuaStreetLightController en todo el nivel.
Detecta actores de farola existentes y les añade el componente,
o crea farolas nuevas basándose en las posiciones de street_furniture.json.

Ejecutar en editor:  Tools > Execute Python Script
"""
import json
import os
import unreal


FURNITURE_PATH = "/Game/Datos/street_furniture.json"
STREET_LIGHT_MESH = "/Game/Meshes/Mobiliario/SM_Farola"
FAROLA_HEIGHT = 400.0


def load_furniture_data():
    """Carga street_furniture.json y filtra farolas."""
    project_dir = unreal.Paths.project_dir()
    json_path = os.path.join(project_dir, "Content", "Datos", "street_furniture.json")

    if not os.path.exists(json_path):
        unreal.log_warning("[StreetLights] No se encontró street_furniture.json")
        return []

    with open(json_path, 'r') as f:
        data = json.load(f)

    farolas = []
    items = data if isinstance(data, list) else data.get("items", data.get("furniture", []))
    for item in items:
        item_type = item.get("type", "").lower()
        if "farola" in item_type or "street_light" in item_type or "lamp" in item_type:
            farolas.append(item)

    return farolas


def spawn_farolas_from_json():
    """Crea farolas desde street_furniture.json."""
    farolas = load_furniture_data()
    if not farolas:
        unreal.log_warning("[StreetLights] No se encontraron farolas en JSON")
        return

    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaStreetLightController")
    if not wind_class:
        unreal.log_error("[StreetLights] No se pudo cargar UAlsasuaStreetLightController")
        return

    farola_mesh = unreal.EditorAssetLibrary.load_asset(STREET_LIGHT_MESH)
    count = 0

    for item in farolas:
        coords = item.get("coords", item.get("position", {}))
        if isinstance(coords, dict):
            x = coords.get("x", 0) * 100.0
            y = coords.get("y", coords.get("z", 0)) * 100.0
        elif isinstance(coords, list) and len(coords) >= 2:
            x = coords[0] * 100.0
            y = coords[1] * 100.0
        else:
            continue

        loc = unreal.Vector(x, y, FAROLA_HEIGHT)
        rot = unreal.Rotation(0, 0, 0)

        actor = None
        if farola_mesh:
            actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
                farola_mesh, loc, rot)

        if actor:
            actor.set_actor_label(f"Farola_{count:03d}")
            comp = unreal.add_component_to_actor(actor, wind_class)
            if comp:
                count += 1

    unreal.log(f"[StreetLights] {count} farolas colocadas desde JSON")


def add_light_to_existing_farolas():
    """Añade UAlsasuaStreetLightController a farolas existentes en el nivel."""
    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaStreetLightController")
    if not wind_class:
        return

    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["farola", "lamp", "street_light", "poste"]):
            existing = actor.get_component_by_class(wind_class)
            if not existing:
                comp = unreal.add_component_to_actor(actor, wind_class)
                if comp:
                    count += 1

    unreal.log(f"[StreetLights] {count} farolas existentes con componente")


if __name__ == "__main__":
    unreal.log("=== Street Lights Setup ===")
    spawn_farolas_from_json()
    add_light_to_existing_farolas()
    unreal.log("=== Street Lights Complete ===")
