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

def create_rain_system(name="NS_Rain", folder="/Game/Effects"):
    """Lluvia de verdad: chorros cayendo. Parametrizado porque hacen falta dos
    copias — ver la llamada de NS_Lluvia al final del fichero."""
    asset_path = f"{folder}/{name}"
    ensure_folder(folder)

    system = compat.crear_asset(
        name, folder, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        # Ya existía. Antes se devolvía tal cual y se salía ANTES de añadirle
        # emisor y módulos, así que si otro script había reservado el nombre con
        # un asset vacío —SetupRainParticles.py lo hacía, y RunAll.py lo ejecuta
        # justo antes que a este— la lluvia se quedaba en un Niagara sin una sola
        # partícula, para siempre y sin un aviso. Ahora se configura igual: crear
        # y configurar son cosas distintas.
        system = compat.assets().load_asset(asset_path)
        if not system:
            unreal.log_warning(f"[VFX] Could not create {name}")
            return None
        unreal.log(f"[VFX] {name} ya existía: se reconfigura")

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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_Snow", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_Snow")
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_Dust", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_Dust")
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_Leaves", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_Leaves")
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_ThunderFlash", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_ThunderFlash")
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_Pollen", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_Pollen")
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_Fireflies", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_Fireflies")
        return None
    
    emitter = system.add_or_add_emitter()
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
    ensure_folder("/Game/Effects")
    
    system = compat.crear_asset(
        "NS_RainMist", "/Game/Effects", unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(asset_path)
        if system: return system
        unreal.log_warning("[VFX] Could not create NS_RainMist")
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

# --- Weapon VFX (simpler placeholders) ---

def create_weapon_vfx(name, folder="/Game/VFX", color=unreal.LinearColor(1, 0.5, 0.2)):
    """Generic weapon VFX placeholder"""
    path = f"{folder}/{name}"
    ensure_folder(folder)
    
    system = compat.crear_asset(
        name, folder, unreal.NiagaraSystem, unreal.NiagaraSystemFactoryNew())
    if not system:
        system = compat.assets().load_asset(path)
        if system: return system
        unreal.log_warning(f"[VFX] Could not create {name}")
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
