"""
SetupTerrainAutoTexture.py — Crea material de terreno con auto-textura por pendiente.
  - Pendiente < 15°: hierba
  - Pendiente 15-35°: tierra
  - Pendiente > 35°: roca
  - Altitud > 520m: nieve en pendientes suaves

Ejecutar en el editor UE5:  Tools > Execute Python Script
"""
import unreal

import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

MPC_PATH = "/Game/Materiales/MPC_Clima"
GRASS_TEXTURE = "/Game/Textures/T_Grass_Color"
DIRT_TEXTURE = "/Game/Textures/T_Ground_Color"
ROCK_TEXTURE = "/Game/Textures/T_StoneWall_Color"
SNOW_TEXTURE = "/Game/Textures/T_Concrete_Color"


def _constant(x, y, r=1.0, g=1.0, b=1.0, a=1.0):
    n = unreal.MaterialExpressionConstant.new()
    n.r, n.g, n.b, n.a = r, g, b, a
    n.position_x, n.position_y = x, y
    return n


def _subtract(a, b, x=0, y=0):
    n = unreal.MaterialExpressionSubtract.new()
    n.a, n.b = a, b
    n.position_x, n.position_y = x, y
    return n


def _lerp(a, b, alpha, x=0, y=0):
    n = unreal.MaterialExpressionLinearInterpolate.new()
    n.a, n.b, n.alpha = a, b, alpha
    n.position_x, n.position_y = x, y
    return n


def _texture(x, y, tex_path):
    tex = compat.assets().load_asset(tex_path)
    n = unreal.MaterialExpressionTextureSample.new()
    if tex:
        n.texture = tex
    n.position_x, n.position_y = x, y
    return n


def _func_input(x, y, name):
    n = unreal.MaterialExpressionFunctionInput.new()
    n.input_name = name
    n.input_type = unreal.MaterialFunctionInputType.MIT_FLOAT1
    n.position_x, n.position_y = x, y
    return n


def create_terrain_material():
    """Crea M_Terreno_AutoTexture: slope-based grass/dirt/rock + altitude snow."""
    pkg_path = "/Game/Materials/M_Terreno_AutoTexture"
    if compat.assets().does_asset_exist(pkg_path):
        unreal.log(f"[TerrainAutoTex] Ya existe: {pkg_path}")
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = asset_tools.create_asset("M_Terreno_AutoTexture", "/Game/Materials",
                                   unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        unreal.log_error("[TerrainAutoTex] No se pudo crear material")
        return

    mat.set_editor_property("BlendMode", compat.ENMASCARADO)
    mat.set_editor_property("ShadingModel", compat.LIT)
    mat.set_two_sided(False)

    # Textures
    t_grass = _texture(-600, 0, GRASS_TEXTURE)
    t_dirt  = _texture(-600, 200, DIRT_TEXTURE)
    t_rock  = _texture(-600, 400, ROCK_TEXTURE)
    t_snow  = _texture(-600, 600, SNOW_TEXTURE)

    # Normal Z = cos(slope). 1=flat, 0=vertical
    normal_z = _func_input(-200, 0, "NormalZ")

    # Slope thresholds (cosine)
    s_flat = _constant(-200, 100, 0.97)  # cos(15°)
    s_mid  = _constant(-200, 150, 0.82)  # cos(35°)

    # Grass ↔ Dirt
    a1 = _subtract(normal_z, s_flat, -100, 100)
    c1 = _lerp(t_dirt, t_grass, a1, 0, 200)

    # (Grass↔Dirt) ↔ Rock
    a2 = _subtract(normal_z, s_mid, -100, 200)
    c2 = _lerp(t_rock, c1, a2, 0, 300)

    # Snow by altitude
    snow_thr = _constant(200, 0, 0.85)
    a3 = _subtract(normal_z, snow_thr, 200, 100)
    final = _lerp(c2, t_snow, a3, 0, 400)

    mat.set_editor_property("BaseColor", final)
    mat.recompile()
    unreal.log("[TerrainAutoTex] Material slope-based creado")


def setup_terrain_material_instances():
    """Crea instancias de M_Terreno_AutoTexture para cada sección del terreno."""
    base_mat_path = "/Game/Materials/M_Terreno_AutoTexture"
    if not compat.assets().does_asset_exist(base_mat_path):
        return

    base_mat = compat.assets().load_asset(base_mat_path)
    if not base_mat:
        return

    sections = ["Herriko", "Zelai", "Intxostia", "Zaramaga",
                 "Etxaguen", "Lakain", "Murgildi", "Natura"]

    for section in sections:
        inst_name = f"MI_Terreno_{section}"
        pkg = f"/Game/Materials/{inst_name}"
        if compat.assets().does_asset_exist(pkg):
            continue

        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi = asset_tools.create_asset(inst_name, "/Game/Materials",
                                      unreal.MaterialInstanceConstant,
                                      unreal.MaterialInstanceConstantFactoryNew())
        if mi:
            mi.set_editor_property("parent", base_mat)
            unreal.log(f"[TerrainAutoTex] Instancia: {inst_name}")


if __name__ == "__main__":
    unreal.log("=== Terrain Auto-Texture Setup ===")
    create_terrain_material()
    setup_terrain_material_instances()
    unreal.log("=== Terrain Auto-Texture Complete ===")
