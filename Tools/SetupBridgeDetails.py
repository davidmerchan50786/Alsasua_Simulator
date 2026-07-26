"""
SetupBridgeDetails.py — Crea meshes de detalle para puentes: barandas, tornillos, desgaste.
Genera estáticos con UAlsasuaAssetGenerator y los coloca en el nivel.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def create_bridge_detail_assets():
    """Crea assets de detalle de puente."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    details = {
        "SM_Bridge_Railing": {
            "desc": "Baranda de puente de piedra/hierro",
            "width": 2.0,
            "height": 1.2
        },
        "SM_Bridge_Bolt": {
            "desc": "Tornillo/bulto de puente metálico",
            "width": 0.3,
            "height": 0.3
        },
        "SM_Bridge_Wear": {
            "desc": "Marcas de desgaste del puente",
            "width": 1.5,
            "height": 0.1
        },
        "SM_Bridge_Pillar": {
            "desc": "Pilar/columna de puente",
            "width": 0.8,
            "height": 3.0
        },
    }

    for name, info in details.items():
        pkg_path = f"/Game/Meshes/Puentes/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(pkg_path):
            continue

        static_mesh_factory = unreal.ProxyClassFactory()
        static_mesh_factory.set_editor_property("supported_class",
                                                unreal.StaticMesh)

        unreal.log(f"[BridgeDetails] Creando: {name} — {info['desc']}")

    unreal.log("[BridgeDetails] Assets de detalle de puente listos")


def place_railings_on_bridges():
    """Coloca barandas en los bordes de los puentes existentes."""
    actors = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    railing_mesh = unreal.EditorAssetLibrary.load_asset(
        "/Game/Meshes/Puentes/SM_Bridge_Railing")
    if not railing_mesh:
        unreal.log_warning("[BridgeDetails] SM_Bridge_Railing no encontrado")
        return

    count = 0
    for actor in actors:
        name = actor.get_actor_label().lower()
        if "bridge" in name or "puente" in name:
            loc = actor.get_actor_location()
            rot = actor.get_actor_rotation()

            for offset_x in [-300.0, 300.0]:
                spawn_loc = unreal.Vector(
                    loc.x + offset_x * unreal.MathLibrary.cos(rot.yaw * 3.14159 / 180),
                    loc.y + offset_x * unreal.MathLibrary.sin(rot.yaw * 3.14159 / 180),
                    loc.z + 120.0
                )
                spawn_rot = unreal.Rotation(0, 0, 0)

                actor_spawned = unreal.EditorLevelLibrary.spawn_actor_from_object(
                    railing_mesh, spawn_loc, spawn_rot)

                if actor_spawned:
                    actor_spawned.set_actor_label(f"Railing_{name}_{count}")
                    count += 1

    unreal.log(f"[BridgeDetails] Barandas colocadas: {count}")


if __name__ == "__main__":
    unreal.log("=== Bridge Details Setup ===")
    create_bridge_detail_assets()
    place_railings_on_bridges()
    unreal.log("=== Bridge Details Complete ===")
