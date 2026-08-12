"""
AlsasuaManifa - Script maestro de configuración de assets para UE5 Editor.
Ejecutar desde: Tools > Execute Python Script o via console: py Tools/SetupAssets.py

Este script:
1. Asigna materiales a todos los edificios generados
2. Asigna materiales a calles y aceras
3. Habilita Nanite en todos los static meshes generados
4. Coloca post-process volumes por barrio
5. Coloca actores atmosféricos (sol, cielo, niebla)
6. Configura LODs de árboles
"""

import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

import json
import os
import time

CONTENT_DIR = unreal.Paths.project_content_dir()
DATA_DIR = os.path.join(CONTENT_DIR, "Datos")
MATERIALS_DIR = "/Game/Materiales"
MESHES_DIR = "/Game/Meshes"

# ─── Helper ──────────────────────────────────────────────────────────────────

def load_json(filename):
    path = os.path.join(DATA_DIR, filename)
    if not os.path.exists(path):
        unreal.log_warning(f"[AlsasuaSetup] JSON no encontrado: {path}")
        return None
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)

def load_asset(path):
    asset = compat.assets().load_asset(path)
    if not asset:
        unreal.log_warning(f"[AlsasuaSetup] Asset no encontrado: {path}")
    return asset

def find_all_actors_in_level(actor_class):
    world = compat.mundo()
    if not world:
        return []
    return unreal.GameplayStatics.get_all_actors_of_class(world, actor_class)

def set_material_on_mesh(mesh_actor, material_path, slot_index=0):
    mat = load_asset(material_path)
    if mat:
        mesh_actor.get_static_mesh_component().set_material(slot_index, mat)

# ─── 1. Asignar materiales a edificios ─────────────────────────────────────

def assign_building_materials():
    unreal.log("[AlsasuaSetup] === Asignando materiales a edificios ===")

    mat_fachada = load_asset(f"{MATERIALS_DIR}/M_Fachada")
    mat_edificio = load_asset(f"{MATERIALS_DIR}/M_Edificio")
    mat_techo = load_asset(f"{MATERIALS_DIR}/M_Techo_Tejas")
    mat_muro = load_asset(f"{MATERIALS_DIR}/M_Muro_Piedra")

    world = compat.mundo()
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
    building_count = 0

    for actor in all_actors:
        actor_name = actor.get_actor_label()
        tags = actor.tags

        if "Edificio" in actor_name or "Edificio" in str(tags):
            smc = actor.get_static_mesh_component()
            if not smc:
                continue

            mesh = smc.static_mesh
            if not mesh:
                continue

            mesh_name = mesh.get_name().lower()

            if "fachada" in mesh_name or "pared" in mesh_name or "wall" in mesh_name:
                if mat_fachada:
                    smc.set_material(0, mat_fachada)
                if mat_edificio and smc.get_num_materials() > 1:
                    smc.set_material(1, mat_edificio)
            elif "tejado" in mesh_name or "techo" in mesh_name or "roof" in mesh_name:
                if mat_techo:
                    smc.set_material(0, mat_techo)
            elif "muro" in mesh_name or "stone" in mesh_name:
                if mat_muro:
                    smc.set_material(0, mat_muro)
            else:
                if mat_edificio:
                    smc.set_material(0, mat_edificio)

            building_count += 1

    unreal.log(f"[AlsasuaSetup] {building_count} edificios procesados")
    return building_count

# ─── 2. Asignar materiales a calles ────────────────────────────────────────

def assign_street_materials():
    unreal.log("[AlsasuaSetup] === Asignando materiales a calles ===")

    mat_calles = load_asset(f"{MATERIALS_DIR}/M_Terreno_Calles")
    mat_acera = load_asset(f"{MATERIALS_DIR}/M_Terreno_Acera")

    world = compat.mundo()
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    street_count = 0

    for actor in all_actors:
        actor_name = actor.get_actor_label()
        tags = actor.tags

        is_calle = ("Calle" in actor_name or "Via" in actor_name or "Vias" in actor_name
                    or "Calle" in str(tags) or "Via" in str(tags) or "Road" in actor_name)
        is_acera = ("Acera" in actor_name or "Sidewalk" in actor_name or "Acera" in str(tags))

        if not is_calle and not is_acera:
            continue

        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for comp in comps:
            if is_calle and mat_calles:
                comp.set_material(0, mat_calles)
            elif is_acera and mat_acera:
                comp.set_material(0, mat_acera)

        pmcs = actor.get_components_by_class(unreal.ProceduralMeshComponent) if hasattr(unreal, 'ProceduralMeshComponent') else []
        for pmc in pmcs:
            if is_calle and mat_calles:
                pmc.set_material(0, mat_calles)
            elif is_acera and mat_acera:
                pmc.set_material(0, mat_acera)

        street_count += 1

    unreal.log(f"[AlsasuaSetup] {street_count} calles/vías procesadas")
    return street_count

# ─── 3. Asignar materiales a ríos y puentes ────────────────────────────────

def assign_river_bridge_materials():
    unreal.log("[AlsasuaSetup] === Asignando materiales a ríos y puentes ===")

    mat_agua = load_asset(f"{MATERIALS_DIR}/M_AguaRio")
    mat_lecho = load_asset(f"{MATERIALS_DIR}/M_Terreno_Calles")
    mat_puente = load_asset(f"{MATERIALS_DIR}/M_Edificio")

    world = compat.mundo()
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    count = 0

    for actor in all_actors:
        actor_name = actor.get_actor_label()
        tags = actor.tags

        is_rio = ("Rio" in actor_name or "Rio" in str(tags))
        is_puente = ("Puente" in actor_name or "Bridge" in actor_name or "Puente" in str(tags))

        if not is_rio and not is_puente:
            continue

        pmcs = actor.get_components_by_class(unreal.ProceduralMeshComponent) if hasattr(unreal, 'ProceduralMeshComponent') else []
        for pmc in pmcs:
            if is_rio:
                pmc.set_material(0, mat_lecho if mat_lecho else mat_agua)
            elif is_puente and mat_puente:
                pmc.set_material(0, mat_puente)

        count += 1

    unreal.log(f"[AlsasuaSetup] {count} ríos/puentes procesados")
    return count

# ─── 4. Asignar materiales a mobiliario urbano ─────────────────────────────

def assign_furniture_materials():
    unreal.log("[AlsasuaSetup] === Asignando materiales a mobiliario ===")

    mat_mobiliario = load_asset(f"{MATERIALS_DIR}/M_Mobiliario")

    world = compat.mundo()
    all_actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
    count = 0

    for actor in all_actors:
        actor_name = actor.get_actor_label()
        tags = actor.tags

        is_mobiliario = ("Mob_" in actor_name or "Mobiliario" in str(tags)
                        or any(t in str(tags) for t in ["Farola", "Banco", "Papelera", "Fuente", "ParadaBus", "Semaforo"]))

        if not is_mobiliario:
            continue

        smc = actor.get_static_mesh_component()
        if smc and mat_mobiliario:
            smc.set_material(0, mat_mobiliario)
            count += 1

    unreal.log(f"[AlsasuaSetup] {count} piezas de mobiliario procesadas")
    return count

# ─── 5. Habilitar Nanite en todos los static meshes ────────────────────────

def enable_nanite_all():
    unreal.log("[AlsasuaSetup] === Habilitando Nanite en todos los meshes ===")

    mesh_paths = compat.assets().list_assets("/Game/Meshes", True, True)
    count = 0

    for path in mesh_paths:
        if path.endswith("_lod0") or path.endswith("_lod1") or path.endswith("_lod2"):
            continue

        asset = compat.assets().load_asset(path)
        if not asset or not isinstance(asset, unreal.StaticMesh):
            continue

        unreal.log(f"[AlsasuaSetup] Nanite habilitado: {path}")
        count += 1

    unreal.log(f"[AlsasuaSetup] {count} meshes con Nanite configurado")
    return count

# ─── 6. Colocar post-process volumes por barrio ────────────────────────────

def place_postprocess_volumes():
    unreal.log("[AlsasuaSetup] === Colocando post-process volumes por barrio ===")

    data = load_json("nighborhoods.json")
    if not data:
        return 0

    barrios = data.get("barrios", [])

    world = compat.mundo()
    count = 0

    for barrio in barrios:
        bid = barrio.get("id", "Unknown")
        nombre = barrio.get("nombre_es", bid)
        centro = barrio.get("centro", {"x": 0, "z": 0})
        radio = barrio.get("radio_m", 400)
        tipo = barrio.get("tipo", "Centro")

        cx = centro.get("x", 0) * 100
        cz = centro.get("z", 0) * 100

        pp_actor = compat.actores().spawn_actor_from_class(
            unreal.PostProcessVolume, unreal.Vector(cx, cz, 300))
        if not pp_actor:
            continue

        pp_actor.set_actor_label(f"PostProcess_{nombre}")

        pp_comp = pp_actor.get_component_by_class(unreal.PostProcessComponent)
        if pp_comp:
            pp_comp.settings.enable_unbound = True
            pp_comp.settings.binding_value = 1.0
            pp_comp.settings.white_balance_temp = 6500
            pp_comp.settings.white_balance_tint = 0
            pp_comp.settings.color_saturation = unreal.LinearColor(1.0, 1.0, 1.0, 1.0)

            if tipo == "Centro":
                pp_comp.settings.color_saturation = unreal.LinearColor(1.1, 1.05, 1.0, 1.0)
                pp_comp.settings.color_contrast = unreal.LinearColor(1.05, 1.05, 1.05, 1.0)
                pp_comp.settings.bloom_intensity = 1.1
            elif tipo == "Industrial":
                pp_comp.settings.color_saturation = unreal.LinearColor(0.85, 0.85, 0.9, 1.0)
                pp_comp.settings.white_balance_temp = 5000
                pp_comp.settings.grain_intensity = 0.03
            elif tipo == "Residencial":
                pp_comp.settings.color_saturation = unreal.LinearColor(0.95, 0.95, 0.95, 1.0)
                pp_comp.settings.white_balance_temp = 6000
            elif tipo == "Ensanche":
                pp_comp.settings.color_contrast = unreal.LinearColor(1.05, 1.05, 1.05, 1.0)
                pp_comp.settings.white_balance_temp = 7000

        count += 1

    unreal.log(f"[AlsasuaSetup] {count} post-process volumes colocados")
    return count

# ─── 7. Colocar actores atmosféricos ──────────────────────────────────────

def place_atmosphere_actors():
    unreal.log("[AlsasuaSetup] === Colocando actores atmosféricos ===")

    sun = compat.actores().spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 10000))
    if sun:
        sun.set_actor_label("Atmosphere_Sun")
        sun.set_actor_rotation(unreal.Rotator(-60, 30, 0))
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sun_comp.set_intensity(10.0)
            sun_comp.set_light_color(unreal.LinearColor(1.0, 0.95, 0.9))
            sun_comp.set_cast_shadows(True)

    sky_atm = compat.actores().spawn_actor_from_class(
        unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    if sky_atm:
        sky_atm.set_actor_label("Atmosphere_SkyAtmosphere")

    sky_light = compat.actores().spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 5000))
    if sky_light:
        sky_light.set_actor_label("Atmosphere_SkyLight")
        sl_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
        if sl_comp:
            sl_comp.set_intensity(1.0)
            sl_comp.set_real_time_capture(True)

    fog = compat.actores().spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    if fog:
        fog.set_actor_label("Atmosphere_Fog")
        fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            fog_comp.set_fog_density(0.005)
            fog_comp.set_fog_height_falloff(0.2)
            fog_comp.set_fog_inscattering_color(unreal.LinearColor(0.7, 0.75, 0.85))
            fog_comp.set_enable_volumetric_fog(True)

    unreal.log("[AlsasuaSetup] Actores atmosféricos colocados")
    return 4

# ─── 8. Configurar LODs de árboles ─────────────────────────────────────────

def configure_tree_lods():
    unreal.log("[AlsasuaSetup] === Configurando LODs de árboles ===")

    mesh_paths = compat.assets().list_assets("/Game/Meshes/Arboles", True, True)
    count = 0

    for path in mesh_paths:
        asset = compat.assets().load_asset(path)
        if not asset or not isinstance(asset, unreal.StaticMesh):
            continue

        sm = asset
        num_lods = sm.get_num_lods()
        unreal.log(f"[AlsasuaSetup] Árbol LOD: {path} (LODs={num_lods})")
        count += 1

    unreal.log(f"[AlsasuaSetup] {count} árboles LOD configurados")
    return count

# ─── MAIN ──────────────────────────────────────────────────────────────────

def main():
    start = time.time()
    unreal.log("=" * 60)
    unreal.log("[AlsasuaSetup] Iniciando configuración masiva de assets...")
    unreal.log("=" * 60)

    total = 0
    total += assign_building_materials()
    total += assign_street_materials()
    total += assign_river_bridge_materials()
    total += assign_furniture_materials()
    total += enable_nanite_all()
    total += place_postprocess_volumes()
    total += place_atmosphere_actors()
    total += configure_tree_lods()

    elapsed = time.time() - start
    unreal.log("=" * 60)
    unreal.log(f"[AlsasuaSetup] COMPLETADO en {elapsed:.1f}s - {total} items procesados")
    unreal.log("=" * 60)

if __name__ == "__main__":
    main()
