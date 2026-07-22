"""
SetupAudio.py — Coloca actores de sonido ambiente en L_Alsasua.
Ejecutar desde el editor tras SetupLevel.py.

Crea:
  - Ambient_ZonaUrbana (Ambient Sound para zona urbana)
  - Ambient_Rio (Ambient Sound para río)
  - Ambient_Multitud (Ambient Sound para multitud)
  - Ambient_Viento (Ambient Sound para viento)
"""
import unreal


def spawn_ambient(label, location, sound_class="AmbientSound", volume=1.0, radius=5000):
    """Spawnear un AmbientSound actor."""
    try:
        actor_class = unreal.AmbientSound
    except:
        actor_class = unreal.Actor

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        actor_class, unreal.Vector(*location)
    )
    if actor:
        actor.set_actor_label(label)
        # Configurar componente de audio si existe
        try:
            audio_comp = actor.get_audio_component()
            if audio_comp:
                audio_comp.set_editor_property("volume_multiplier", volume)
                audio_comp.set_editor_property("attenuation_shape_extents", unreal.Vector(radius, radius, radius))
        except:
            pass
        unreal.log(f"  Ambient Sound: {label}")
    return actor


def run():
    """Coloca todos los actores de audio ambiente."""
    unreal.log("=== SetupAudio: Iniciando ===")

    # Audio ambiente general (urbana)
    spawn_ambient(
        "Ambient_ZonaUrbana",
        (303250, 3749500, 500),
        volume=0.8,
        radius=10000
    )

    # Río (Zuribai)
    spawn_ambient(
        "Ambient_Rio_Zuribai",
        (303500, 3749200, 300),
        volume=0.6,
        radius=2000
    )

    # Multitud (para manifestaciones)
    spawn_ambient(
        "Ambient_Multitud",
        (303250, 3749500, 100),
        volume=0.5,
        radius=500
    )

    # Viento
    spawn_ambient(
        "Ambient_Viento",
        (303250, 3749500, 1000),
        volume=0.3,
        radius=20000
    )

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("=== SetupAudio: Completo ===")
    unreal.log("NOTA: Asigna sonidos reales a cada AmbientSound en el Details Panel")


if __name__ == "__main__":
    run()
