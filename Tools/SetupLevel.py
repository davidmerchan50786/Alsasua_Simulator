"""
SetupLevel.py — Crea el nivel L_Alsasua y configura los actores básicos.
Ejecutar desde el editor: Window > Output Log > consola.

Crea:
  1. Nivel vacío /Game/Maps/L_Alsasua
  2. Sky Atmosphere + DirectionalLight + SkyLight (si no existen)
  3. ExponentialHeightFog
  4. PostProcessVolume (auto-exposure, Lumen)
  5. GameMode override a AlsasuaGameplayGameMode
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

MAPS_DIR = "/Game/Maps"
LEVEL_NAME = "L_Alsasua"
LEVEL_PATH = f"{MAPS_DIR}/{LEVEL_NAME}"


def ensure_folder():
    if not compat.assets().does_asset_exist(MAPS_DIR):
        compat.assets().make_directory(MAPS_DIR)
        unreal.log("Creada carpeta /Game/Maps")


def create_level():
    """Crea un nivel vacío y lo guarda."""
    unreal.log("=== SetupLevel: Iniciando ===")

    # Verificar si ya existe
    if compat.assets().does_asset_exist(LEVEL_PATH):
        unreal.log(f"Nivel {LEVEL_NAME} ya existe, cargando...")
        compat.nivel().load_level(LEVEL_PATH)
        return True

    # Crear nivel vacío
    unreal.log("Creando nivel vacío L_Alsasua...")
    compat.nivel().new_level(LEVEL_PATH)

    if not compat.assets().does_asset_exist(LEVEL_PATH):
        # Intento alternativo: crear directamente
        world = compat.mundo()
        if world:
            unreal.log("Nivel creado (guardando)...")
            compat.nivel().save_current_level()
        else:
            unreal.log_error("No se pudo crear el nivel")
            return False

    unreal.log(f"Nivel {LEVEL_NAME} creado.")
    return True


def spawn_sky():
    """Crea iluminación base: Sun + SkyAtmosphere + SkyLight."""
    world = compat.mundo()
    if not world:
        return

    # Eliminar DirectionalLights existentes para evitar duplicados
    actors = compat.actores().get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.DirectionalLight):
            compat.actores().destroy_actor(a)
            unreal.log(f"Eliminado DirectionalLight existente: {a.get_actor_label()}")

    # Spawn DirectionalLight (Sol)
    sun = compat.actores().spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500)
    )
    if sun:
        sun.set_actor_label("Sun")
        sun.set_actor_rotation(unreal.Rotator(-45.0, 0.0, 0.0))
        # Movilidad estática — evita warning spam de "Movilidad Movible"
        mobility = unreal.ActorMobility.STATIC
        sun.set_mobility(mobility)
        light_comp = sun.get_editor_property("light_component")
        if light_comp:
            light_comp.set_editor_property("intensity", 10.0)
        unreal.log("Creado DirectionalLight (Sun) — Static mobility")

    # Spawn SkyAtmosphere
    sky_atmo = compat.actores().spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)
    )
    if sky_atmo:
        sky_atmo.set_actor_label("SkyAtmosphere")
        unreal.log("Creado SkyAtmosphere")

    # Spawn SkyLight
    sky_light = compat.actores().spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 200)
    )
    if sky_light:
        sky_light.set_actor_label("SkyLight")
        sl_comp = sky_light.get_editor_property("light_component")
        if sl_comp:
            sl_comp.set_editor_property("real_time_capture", True)
        unreal.log("Creado SkyLight (captura en tiempo real)")


def spawn_fog():
    """Crea ExponentialHeightFog."""
    actors = compat.actores().get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.ExponentialHeightFog):
            compat.actores().destroy_actor(a)
            unreal.log(f"Eliminado HeightFog existente: {a.get_actor_label()}")

    fog = compat.actores().spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0)
    )
    if fog:
        fog.set_actor_label("HeightFog")
        fog_comp = fog.get_editor_property("fog_component")
        if fog_comp:
            fog_comp.set_editor_property("fog_density", 0.02)
        unreal.log("Creado ExponentialHeightFog")


def spawn_post_process():
    """Crea PostProcessVolume con auto-exposure y Lumen."""
    actors = compat.actores().get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.PostProcessVolume):
            compat.actores().destroy_actor(a)
            unreal.log(f"Eliminado PostProcessVolume existente: {a.get_actor_label()}")

    pp = compat.actores().spawn_actor_from_class(
        unreal.PostProcessVolume, unreal.Vector(0, 0, 0)
    )
    if pp:
        pp.set_actor_label("PostProcess_Alsasua")
        pp.set_editor_property("bInfiniteExtent", True)
        pp.set_editor_property("priority", 1.0)
        unreal.log("Creado PostProcessVolume (infinite extent, auto-exposure)")


def spawn_director_arranque():
    """Coloca ADirectorArranque en el nivel — orquesta los 51 sistemas."""
    actors = compat.actores().get_all_level_actors()
    for a in actors:
        if a.get_actor_label() == "DirectorArranque":
            compat.actores().destroy_actor(a)
            unreal.log("DirectorArranque existente eliminado.")

    # Cargar clase del módulo AlsasuaWorld
    director_class = compat.cargar_clase(
        "/Script/AlsasuaWorld.DirectorArranque"
    )
    if not director_class:
        unreal.log_warning("No se pudo cargar DirectorArranque (¿módulo compilado?)")
        return

    # Colocar en origen — el Director lee las coordenadas del mundo
    actor = compat.actores().spawn_actor_from_class(
        director_class, unreal.Vector(0, 0, 0)
    )
    if actor:
        actor.set_actor_label("DirectorArranque")
        unreal.log("Creado DirectorArranque — orquestador de 51 sistemas")
    else:
        unreal.log_error("No se pudo crear DirectorArranque")


def set_gamemode():
    """Configura el GameMode del nivel."""
    world_settings = compat.ajustes_mundo()
    if world_settings:
        gm_class = compat.cargar_clase(
            "/Script/AlsasuaGameplay.AlsasuaGameplayGameMode"
        )
        if gm_class:
            world_settings.set_gamemode_override(gm_class)
            unreal.log("GameMode override: AlsasuaGameplayGameMode")
        else:
            unreal.log_warning("No se pudo cargar AlsasuaGameplayGameMode (¿módulo compilado?)")


def run():
    """Ejecuta la configuración completa del nivel."""
    ensure_folder()
    create_level()
    spawn_sky()
    spawn_fog()
    spawn_post_process()
    spawn_director_arranque()
    set_gamemode()
    compat.nivel().save_current_level()
    unreal.log("=== SetupLevel: Nivel guardado ===")
    unreal.log("  SIGUIENTE PASO: Play (PIE) → DirectorArranque genera 51 sistemas")


if __name__ == "__main__":
    run()
