"""
SetupGroundCover.py — Añade UAlsasuaGroundCoverSystem al terreno.
Genera hojas caídas, piedras, musgo, y agujas de pino procedurales.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def add_ground_cover():
    """Añade UAlsasuaGroundCoverSystem al terreno."""
    cover_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaGroundCoverSystem")
    if not cover_class:
        unreal.log_error("[GroundCover] No se pudo cargar UAlsasuaGroundCoverSystem")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["terreno", "terrain", "landscape", "suelo"]):
            existing = actor.get_component_by_class(cover_class)
            if not existing:
                comp = compat.anadir_componente(actor, cover_class)
                if comp:
                    count += 1

    unreal.log(f"[GroundCover] {count} terrenos con cobertura de suelo")


def add_seasonal_foliage():
    """Añade UAlsasuaSeasonalFoliage a actores de vegetation."""
    seasonal_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaSeasonalFoliage")
    if not seasonal_class:
        unreal.log_error("[SeasonalFoliage] No se pudo cargar UAlsasuaSeasonalFoliage")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["tree", "arbol", "bush", "arbusto",
                                    "foliage", "hierba", "grass"]):
            existing = actor.get_component_by_class(seasonal_class)
            if not existing:
                comp = compat.anadir_componente(actor, seasonal_class)
                if comp:
                    count += 1

    unreal.log(f"[SeasonalFoliage] {count} actores con colores estacionales")


def add_environmental_decals():
    """Añade UAlsasuaEnvironmentalDecals a edificios."""
    env_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaEnvironmentalDecals")
    if not env_class:
        unreal.log_error("[EnvironmentalDecals] No se pudo cargar UAlsasuaEnvironmentalDecals")
        return

    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0

    for actor in actors:
        name = actor.get_actor_label().lower()
        if any(x in name for x in ["edificio", "building", "muro", "wall",
                                    "casa", "bloque"]):
            existing = actor.get_component_by_class(env_class)
            if not existing:
                comp = compat.anadir_componente(actor, env_class)
                if comp:
                    count += 1

    unreal.log(f"[EnvironmentalDecals] {count} edificios con decals ambientales")


def add_procedural_audio():
    """Añade UAlsasuaProceduralAudio al jugador."""
    audio_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaProceduralAudio")
    if not audio_class:
        unreal.log_error("[ProceduralAudio] No se pudo cargar UAlsasuaProceduralAudio")
        return

    pawns = compat.actores().get_all_level_actors_of_class(unreal.Pawn)
    count = 0
    for pawn in pawns:
        existing = pawn.get_component_by_class(audio_class)
        if not existing:
            comp = compat.anadir_componente(pawn, audio_class)
            if comp:
                count += 1

    unreal.log(f"[ProceduralAudio] {count} componentes de audio procedural")


def add_dynamic_cloud_shadows():
    """Añade UAlsasuaDynamicCloudShadows al WorldSettings."""
    cloud_class = unreal.load_class(
        "/Script/AlsasuaManifa.AlsasuaDynamicCloudShadows")
    if not cloud_class:
        unreal.log_error("[CloudShadows] No se pudo cargar UAlsasuaDynamicCloudShadows")
        return

    world_settings = compat.ajustes_mundo()
    if world_settings:
        existing = world_settings.get_component_by_class(cloud_class)
        if not existing:
            comp = compat.anadir_componente(world_settings, cloud_class)
            if comp:
                unreal.log("[CloudShadows] DynamicCloudShadows añadido al WorldSettings")


if __name__ == "__main__":
    unreal.log("=== Ground Cover & Environmental Setup ===")
    add_ground_cover()
    add_seasonal_foliage()
    add_environmental_decals()
    add_procedural_audio()
    add_dynamic_cloud_shadows()
    unreal.log("=== Ground Cover & Environmental Complete ===")
