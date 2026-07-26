"""
SetupRainParticles.py — Crea y configura sistema de lluvia/nieve con Niagara.
Añade UAlsasuaRainParticleComponent al jugador principal.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal


def create_rain_niagara_system():
    """Crea NS_Rain como sistema Niagara básico de partículas de lluvia."""
    pkg = "/Game/Effects/NS_Rain"
    if unreal.EditorAssetLibrary.does_asset_exist(pkg):
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    niagara_factory = unreal.NiagaraSystemFactoryNew()
    ns = asset_tools.create_asset("NS_Rain", "/Game/Effects",
                                  unreal.NiagaraSystem, niagara_factory)
    if ns:
        unreal.log("[RainParticles] NS_Rain creado")


def create_snow_niagara_system():
    """Crea NS_Snow como sistema Niagara básico de partículas de nieve."""
    pkg = "/Game/Effects/NS_Snow"
    if unreal.EditorAssetLibrary.does_asset_exist(pkg):
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    niagara_factory = unreal.NiagaraSystemFactoryNew()
    ns = asset_tools.create_asset("NS_Snow", "/Game/Effects",
                                  unreal.NiagaraSystem, niagara_factory)
    if ns:
        unreal.log("[RainParticles] NS_Snow creado")


def create_thunder_flash_system():
    """Crea NS_ThunderFlash como efecto de relámpago."""
    pkg = "/Game/Effects/NS_ThunderFlash"
    if unreal.EditorAssetLibrary.does_asset_exist(pkg):
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    niagara_factory = unreal.NiagaraSystemFactoryNew()
    ns = asset_tools.create_asset("NS_ThunderFlash", "/Game/Effects",
                                  unreal.NiagaraSystem, niagara_factory)
    if ns:
        unreal.log("[RainParticles] NS_ThunderFlash creado")


def add_rain_to_player():
    """Añade UAlsasuaRainParticleComponent al jugador."""
    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaRainParticleComponent")
    if not wind_class:
        unreal.log_error("[RainParticles] No se pudo cargar UAlsasuaRainParticleComponent")
        return

    pawns = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.Pawn)
    for pawn in pawns:
        existing = pawn.get_component_by_class(wind_class)
        if not existing:
            comp = unreal.add_component_to_actor(pawn, wind_class)
            if comp:
                unreal.log(f"[RainParticles] Componente añadido a {pawn.get_actor_label()}")


def add_rain_to_player_controller():
    """También intenta añadir al PlayerController por si el pawn no existe."""
    wind_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaRainParticleComponent")
    if not wind_class:
        return

    controllers = unreal.EditorLevelLibrary.get_all_level_actors_of_class(
        unreal.PlayerController)
    for pc in controllers:
        existing = pc.get_component_by_class(wind_class)
        if not existing:
            comp = unreal.add_component_to_actor(pc, wind_class)
            if comp:
                unreal.log(f"[RainParticles] Componente añadido a {pc.get_actor_label()}")


if __name__ == "__main__":
    unreal.log("=== Rain Particles Setup ===")
    create_rain_niagara_system()
    create_snow_niagara_system()
    create_thunder_flash_system()
    add_rain_to_player()
    add_rain_to_player_controller()
    unreal.log("=== Rain Particles Complete ===")
