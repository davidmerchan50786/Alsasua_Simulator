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


MAPS_DIR = "/Game/Maps"
LEVEL_NAME = "L_Alsasua"
LEVEL_PATH = f"{MAPS_DIR}/{LEVEL_NAME}"


def ensure_folder():
    if not unreal.EditorAssetLibrary.does_asset_exist(MAPS_DIR):
        unreal.EditorAssetLibrary.make_directory(MAPS_DIR)
        unreal.log("Creada carpeta /Game/Maps")


def create_level():
    """Crea un nivel vacío y lo guarda."""
    unreal.log("=== SetupLevel: Iniciando ===")

    # Verificar si ya existe
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        unreal.log(f"Nivel {LEVEL_NAME} ya existe, cargando...")
        unreal.EditorLevelLibrary.load_level(LEVEL_PATH)
        return True

    # Crear nivel vacío
    unreal.log("Creando nivel vacío L_Alsasua...")
    unreal.EditorLevelLibrary.new_level(LEVEL_PATH)

    if not unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        # Intento alternativo: crear directamente
        world = unreal.EditorLevelLibrary.get_editor_world()
        if world:
            unreal.log("Nivel creado (guardando)...")
            unreal.EditorLevelLibrary.save_current_level()
        else:
            unreal.log_error("No se pudo crear el nivel")
            return False

    unreal.log(f"Nivel {LEVEL_NAME} creado.")
    return True


def spawn_sky():
    """Crea iluminación base: Sun + SkyAtmosphere + SkyLight."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        return

    # Verificar si ya hay un DirectionalLight
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    has_sun = False
    for a in actors:
        if isinstance(a, unreal.DirectionalLight):
            has_sun = True
            break

    if has_sun:
        unreal.log("DirectionalLight ya existe, saltando.")
        return

    # Spawn DirectionalLight (Sol)
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500)
    )
    if sun:
        sun.set_actor_label("Sun")
        # Rotación: ~45° pitch, orientada al sur para Alsasua (lat 42.9°N)
        sun.set_actor_rotation(unreal.Rotator(-45.0, 0.0, 0.0))
        # Intensidad
        light_comp = sun.get_editor_property("light_component")
        if light_comp:
            light_comp.set_editor_property("intensity", 10.0)
        unreal.log("Creado DirectionalLight (Sun)")

    # Spawn SkyAtmosphere
    sky_atmo = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)
    )
    if sky_atmo:
        sky_atmo.set_actor_label("SkyAtmosphere")
        unreal.log("Creado SkyAtmosphere")

    # Spawn SkyLight
    sky_light = unreal.EditorLevelLibrary.spawn_actor_from_class(
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
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.ExponentialHeightFog):
            unreal.log("ExponentialHeightFog ya existe, saltando.")
            return

    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
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
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.PostProcessVolume):
            unreal.log("PostProcessVolume ya existe, saltando.")
            return

    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PostProcessVolume, unreal.Vector(0, 0, 0)
    )
    if pp:
        pp.set_actor_label("PostProcess_Alsasua")
        pp.set_editor_property("infinite_extent", True)  # Afecta a todo el nivel
        # Auto-exposure
        pp.set_editor_property("priority", 1.0)
        unreal.log("Creado PostProcessVolume (infinite extent, auto-exposure)")


def set_gamemode():
    """Configura el GameMode del nivel."""
    # El GameMode se configura por ini (DefaultEngine.ini ya lo tiene)
    # Pero podemos setearlo en el nivel también
    world_settings = unreal.EditorLevelLibrary.get_world_settings()
    if world_settings:
        gm_class = unreal.EditorAssetLibrary.load_class(
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
    set_gamemode()
    # Guardar
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log("=== SetupLevel: Nivel guardado ===")


if __name__ == "__main__":
    run()
