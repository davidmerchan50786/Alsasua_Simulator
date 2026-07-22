"""
SetupActors.py — Spawna actores placeholder en L_Alsasua para visualización inmediata.
Ejecutar desde el editor tras RunAll.py.

Crea:
  - BP_Jugador placeholder (cápsula + camera)
  - 5 NPC placeholder (cápsulas)
  - 1 Guardia placeholder
  - 1 Vehicle placeholder (cubo)
  - 2 barricadas (cubos)
  - Herriko Plaza marker (esfera)
  - 2 megafonos (cilindros)
"""
import unreal


def spawn_placeholder(label, class_name, location, scale=(1,1,1), color=(0.5,0.5,0.5,1)):
    """Spawnear un actor placeholder con StaticMeshComponent."""
    try:
        actor_class = unreal.load_class(None, class_name)
        if not actor_class:
            actor_class = unreal.StaticMeshActor
    except:
        actor_class = unreal.StaticMeshActor

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, unreal.Vector(*location)
    )
    if actor:
        actor.set_actor_label(label)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        unreal.log(f"  Spawneado: {label}")
    return actor


def run():
    """Spawna todos los actores placeholder."""
    unreal.log("=== SetupActors: Iniciando ===")

    # Herriko Plaza marker (centro del mapa, ~531m altitud → WorldZ ~2061 cm)
    spawn_placeholder(
        "Herriko_Plaza_Marker",
        "StaticMeshActor",
        (303250, 3749500, 2061),
        scale=(20, 20, 0.5),
        color=(0.8, 0.6, 0.0, 1)
    )

    # Jugador placeholder
    spawn_placeholder(
        "BP_Jugador_Placeholder",
        "StaticMeshActor",
        (303250, 3749500, 300),
        scale=(0.5, 0.5, 1.8)
    )

    # NPCs (distribuidos por la plaza)
    npc_positions = [
        (303300, 3749550, 100),
        (303200, 3749450, 100),
        (303350, 3749600, 100),
        (303150, 3749400, 100),
        (303400, 3749500, 100),
    ]
    for i, pos in enumerate(npc_positions):
        spawn_placeholder(
            f"NPC_{i+1}_Placeholder",
            "StaticMeshActor",
            pos,
            scale=(0.4, 0.4, 1.7)
        )

    # Guardia
    spawn_placeholder(
        "GuardiaCivil_Placeholder",
        "StaticMeshActor",
        (303500, 3749500, 100),
        scale=(0.5, 0.5, 1.8)
    )

    # Vehículos
    spawn_placeholder(
        "Vehicle_Placeholder",
        "StaticMeshActor",
        (303000, 3749500, 50),
        scale=(4.5, 2.0, 1.5)
    )

    # Barricadas
    spawn_placeholder(
        "Barricada_1",
        "StaticMeshActor",
        (303150, 3749600, 50),
        scale=(3, 0.5, 1.2)
    )
    spawn_placeholder(
        "Barricada_2",
        "StaticMeshActor",
        (303350, 3749400, 50),
        scale=(3, 0.5, 1.2)
    )

    # Megáfonos
    spawn_placeholder(
        "Megafono_1",
        "StaticMeshActor",
        (303280, 3749530, 200),
        scale=(0.2, 0.2, 0.8)
    )
    spawn_placeholder(
        "Megafono_2",
        "StaticMeshActor",
        (303220, 3749470, 200),
        scale=(0.2, 0.2, 0.8)
    )

    # Guardar
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("=== SetupActors: Completo (12 actores placeholder) ===")


if __name__ == "__main__":
    run()
