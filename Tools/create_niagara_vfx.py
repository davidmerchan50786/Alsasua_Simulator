"""
Create all missing Niagara VFX systems in UE5 editor.
Run via: Editor > Window > Output Log > execute console command: py Tools/create_niagara_vfx.py
Or: Edit > Editor Preferences > Plugins > Python > enable, then run from Python console.
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def ensure_folder(path):
    if not compat.assets().does_asset_exist(path):
        compat.assets().make_directory(path)


def crear_limpio(name, folder):
    """
    Crea el sistema Niagara borrando antes el que hubiera.

    Los ocho generadores hacían "crear; si falla, cargar el existente y salir",
    y salir era salir ANTES de añadirle emisor y módulos: un asset que ya
    estuviera ahí se quedaba como estuviera, para siempre. Eso dejó NS_Rain,
    NS_Snow y NS_ThunderFlash en sistemas sin una sola partícula, porque
    SetupRainParticles.py los creaba vacíos y RunAll.py lo ejecuta justo antes
    que a este.

    Reconfigurar encima tampoco vale: add_emitter() apila, así que cada pasada
    de RunAll.py añadiría un emisor más y el efecto cambiaría en cada ejecución.
    Borrar y rehacer es lo único idempotente.
    """
    asset_path = f"{folder}/{name}"
    ensure_folder(folder)
    if compat.assets().does_asset_exist(asset_path):
        compat.assets().delete_asset(asset_path)
        unreal.log(f"[VFX] {name} ya existía: se rehace desde cero")
    system = compat.crear_asset(
        name, folder, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        unreal.log_warning(f"[VFX] Could not create {name}")
    return system

def create_rain_system(name="NS_Rain", folder="/Game/Effects"):
    """Lluvia de verdad: chorros cayendo. Parametrizado porque hacen falta dos
    copias — ver la llamada de NS_Lluvia al final del fichero."""
    asset_path = f"{folder}/{name}"

    system = crear_limpio(name, folder)
    if not system:
        return None

    # Add emitter
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "RainStreaks")
    
    # Spawn rate
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 5000.0)
    
    # Initialize particle
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 0.3)
    init.set_editor_property("lifetime_max", 0.6)
    init.set_editor_property("mass_min", 0.001)
    init.set_editor_property("mass_max", 0.001)
    
    # Shape location - box around camera
    shape = emitter.find_or_add_module("Shape Location")
    shape.set_editor_property("shape_type", unreal.NiagaraShapeType.BOX)
    
    # Add velocity - downward
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(0, 0, -3000))
    vel.set_editor_property("velocity_max", unreal.Vector(0, 0, -4000))
    
    # Sprite renderer
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log(f"[VFX] {name} created")
    return system

def create_snow_system():
    """NS_Snow: Gentle falling snowflakes"""
    asset_path = "/Game/Effects/NS_Snow"
    system = crear_limpio("NS_Snow", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Snowflakes")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 2000.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 2.0)
    init.set_editor_property("lifetime_max", 4.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-20, 0, -100))
    vel.set_editor_property("velocity_max", unreal.Vector(20, 0, -200))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_Snow created")
    return system

def create_dust_system():
    """NS_Dust: Ambient dust particles"""
    asset_path = "/Game/Effects/NS_Dust"
    system = crear_limpio("NS_Dust", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "DustMotes")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 200.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 3.0)
    init.set_editor_property("lifetime_max", 6.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-5, -5, 2))
    vel.set_editor_property("velocity_max", unreal.Vector(5, 5, 8))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_Dust created")
    return system

def create_leaves_system():
    """NS_Leaves: Falling autumn leaves"""
    asset_path = "/Game/Effects/NS_Leaves"
    system = crear_limpio("NS_Leaves", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Leaves")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 100.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 3.0)
    init.set_editor_property("lifetime_max", 6.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-30, -30, -60))
    vel.set_editor_property("velocity_max", unreal.Vector(30, 30, -120))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_Leaves created")
    return system

def create_thunder_system():
    """NS_ThunderFlash: Lightning flash"""
    asset_path = "/Game/Effects/NS_ThunderFlash"
    system = crear_limpio("NS_ThunderFlash", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Flash")
    
    spawn = emitter.find_or_add_module("SpawnBurstInstantaneous")
    spawn.set_editor_property("spawn_count", 1)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 0.05)
    init.set_editor_property("lifetime_max", 0.15)
    
    size = emitter.find_or_add_module("Scale Sprite Size")
    size.set_editor_property("scale_factor", unreal.Vector2D(50000, 50000))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_ThunderFlash created")
    return system

def create_pollen_system():
    """NS_Pollen: Spring pollen particles"""
    asset_path = "/Game/Effects/NS_Pollen"
    system = crear_limpio("NS_Pollen", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Pollen")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 50.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 4.0)
    init.set_editor_property("lifetime_max", 8.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-10, -10, 5))
    vel.set_editor_property("velocity_max", unreal.Vector(10, 10, 15))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_Pollen created")
    return system

def create_firefly_system():
    """NS_Fireflies: Night-time fireflies"""
    asset_path = "/Game/Effects/NS_Fireflies"
    system = crear_limpio("NS_Fireflies", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Fireflies")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 30.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 3.0)
    init.set_editor_property("lifetime_max", 6.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-15, -15, -5))
    vel.set_editor_property("velocity_max", unreal.Vector(15, 15, 10))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_Fireflies created")
    return system

def create_mist_system():
    """NS_RainMist: Ground mist during rain"""
    asset_path = "/Game/Effects/NS_RainMist"
    system = crear_limpio("NS_RainMist", "/Game/Effects")
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Mist")
    
    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 50.0)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 2.0)
    init.set_editor_property("lifetime_max", 4.0)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-20, -20, 5))
    vel.set_editor_property("velocity_max", unreal.Vector(20, 20, 15))
    
    size = emitter.find_or_add_module("Scale Sprite Size")
    size.set_editor_property("scale_factor", unreal.Vector2D(10, 10))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(asset_path)
    unreal.log("[VFX] NS_RainMist created")
    return system

def create_fire_system():
    """NS_Fuego: Fuego sostenido de vehículo dañado."""
    path = "/Game/VFX/NS_Fuego"
    system = crear_limpio("NS_Fuego", "/Game/VFX")
    if not system:
        return None

    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Fire")

    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 300.0)

    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 0.4)
    init.set_editor_property("lifetime_max", 0.8)

    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-30, -30, 100))
    vel.set_editor_property("velocity_max", unreal.Vector(30, 30, 260))

    size = emitter.find_or_add_module("Scale Sprite Size")
    size.set_editor_property("scale_factor", unreal.Vector2D(60, 60))

    renderer = emitter.find_or_add_module("Sprite Renderer")

    system = compat.assets().save_asset(path)
    unreal.log("[VFX] NS_Fuego created")
    return system

def create_spark_car_system():
    """NS_SparkCar: Chispas de debajo del vehículo dañado."""
    path = "/Game/VFX/NS_SparkCar"
    system = crear_limpio("NS_SparkCar", "/Game/VFX")
    if not system:
        return None

    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Sparks")

    spawn = emitter.find_or_add_module("SpawnRate")
    spawn.set_editor_property("spawn_rate", 200.0)

    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 0.2)
    init.set_editor_property("lifetime_max", 0.6)

    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-120, -120, -80))
    vel.set_editor_property("velocity_max", unreal.Vector(120, 120, 60))

    size = emitter.find_or_add_module("Scale Sprite Size")
    size.set_editor_property("scale_factor", unreal.Vector2D(12, 12))

    renderer = emitter.find_or_add_module("Sprite Renderer")

    system = compat.assets().save_asset(path)
    unreal.log("[VFX] NS_SparkCar created")
    return system


# --- Weapon VFX (simpler placeholders) ---

def create_weapon_vfx(name, folder="/Game/VFX", color=unreal.LinearColor(1, 0.5, 0.2)):
    """Generic weapon VFX placeholder"""
    path = f"{folder}/{name}"

    system = crear_limpio(name, folder)
    if not system:
        return None
    
    emitter = system.add_emitter()
    emitter.set_editor_property("name", "Burst")
    
    spawn = emitter.find_or_add_module("SpawnBurstInstantaneous")
    spawn.set_editor_property("spawn_count", 50)
    
    init = emitter.find_or_add_module("Initialize Particle")
    init.set_editor_property("lifetime_min", 0.1)
    init.set_editor_property("lifetime_max", 0.4)
    
    vel = emitter.find_or_add_module("Add Velocity")
    vel.set_editor_property("velocity_min", unreal.Vector(-100, -100, -100))
    vel.set_editor_property("velocity_max", unreal.Vector(100, 100, 100))
    
    renderer = emitter.find_or_add_renderer("Sprite Renderer")
    
    system = compat.assets().save_asset(path)
    unreal.log(f"[VFX] {name} created")
    return system


if __name__ == "__main__":
    unreal.log("[VFX] Creating all Niagara systems...")
    
    # Weather/Ambient
    create_rain_system()
    create_snow_system()
    create_dust_system()
    create_leaves_system()
    create_thunder_system()
    create_pollen_system()
    create_firefly_system()
    create_mist_system()
    create_fire_system()
    create_spark_car_system()
    
    # Weapon VFX
    weapon_vfx = [
        ("NS_Fogonazo", unreal.LinearColor(1, 0.6, 0.1)),
        ("NS_Sangre", unreal.LinearColor(0.6, 0, 0)),
        ("NS_Impacto", unreal.LinearColor(0.8, 0.7, 0.5)),
        ("NS_Spray", unreal.LinearColor(0.9, 0.9, 0.9)),
        ("NS_Molotov", unreal.LinearColor(1, 0.3, 0)),
        ("NS_Explosion", unreal.LinearColor(1, 0.5, 0.1)),
        ("NS_ExplosionCoche", unreal.LinearColor(1, 0.4, 0)),
        ("NS_Humo", unreal.LinearColor(0.5, 0.5, 0.5)),
        ("NS_SlingshotImpact", unreal.LinearColor(0.8, 0.7, 0.5)),
        ("NS_SmokeBomb", unreal.LinearColor(0.4, 0.4, 0.4)),
        ("NS_RallyWave", unreal.LinearColor(0.2, 0.8, 1.0)),
    ]
    for name, color in weapon_vfx:
        create_weapon_vfx(name, "/Game/VFX", color)
    
    # La lluvia que el juego enseña de verdad. UClimaSubsystem carga
    # /Game/VFX/NS_Lluvia, no /Game/Effects/NS_Rain, así que hace falta la copia
    # ahí — pero con la fábrica de LLUVIA, no con create_weapon_vfx, que es la de
    # los fogonazos: 50 partículas de golpe, ±100 de velocidad en cualquier
    # dirección y 0,1-0,4 s de vida. Teñida de azul y llamada NS_Lluvia, pero un
    # chispazo. La de arriba son 5000 partículas/s cayendo a 30-40 m/s en una
    # caja alrededor de la cámara.
    create_rain_system("NS_Lluvia", "/Game/VFX")
    
    unreal.log("[VFX] All Niagara systems created! (22 total)")
    unreal.log("[VFX] NOTE: These are basic placeholders. Open each in Niagara editor to tune.")
