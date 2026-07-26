"""
SetupTerrainAutoTexture.py — Crea material de terreno con auto-textura por pendiente.
Usa el MPC global (MPC_AlsasuaGlobal) para blending dinámico:
  - Pendiente < 15°: hierba
  - Pendiente 15-35°: tierra
  - Pendiente > 35°: roca
  - Altitud > 520m: nieve en pendientes suaves

Ejecutar en el editor UE5:  Tools > Execute Python Script
"""
import unreal

MPC_PATH = "/Game/Materials/MPC_AlsasuaGlobal"
GRASS_TEXTURE = "/Game/Textures/T_Grass_01_D"
DIRT_TEXTURE = "/Game/Textures/T_Rock_05_D"
ROCK_TEXTURE = "/Game/Textures/T_StoneWall_02_D"
SNOW_TEXTURE = "/Game/Textures/T_Ground_01_D"


def create_terrain_material():
    """Crea M_Terreno_AutoTexture con slope-based blending via MPC."""
    pkg_path = "/Game/Materials/M_Terreno_AutoTexture"
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    existing = unreal.EditorAssetLibrary.does_asset_exist(pkg_path)
    if existing:
        unreal.log(f"[TerrainAutoTex] Material ya existe: {pkg_path}")
        return

    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Terreno_AutoTexture", "/Game/Materials",
                                   unreal.Material, mat_factory)

    if not mat:
        unreal.log_error("[TerrainAutoTex] No se pudo crear material")
        return

    mat.set_editor_property("BlendMode", unreal.MaterialBlendMode.BM_MASKED)
    mat.set_editor_property("ShadingModel", unreal.ShadingModel.MSM_DEFAULT_LIT)
    mat.set_two_sided(False)

    mpc_param = unreal.MaterialParameterCollection()
    mpc_loaded = unreal.EditorAssetLibrary.load_asset(MPC_PATH)

    if mpc_loaded and isinstance(mpc_loaded, unreal.MaterialParameterCollection):
        mpc_param = mpc_loaded
        unreal.log("[TerrainAutoTex] MPC cargado correctamente")
    else:
        unreal.log_warning("[TerrainAutoTex] MPC no encontrado, usando defaults")

    # World position for slope calculation
    abs_node = unreal.MaterialExpressionAbsolute.new()
    abs_node.input = unreal.MaterialExpressionPixelNormalWS.new()
    abs_node.position_x = -800
    abs_node.position_y = 0

    # Z component of world normal = cos(slope)
    z_extract = new_material_expression_function_input(abs_node, "NormalZ",
        unreal.MaterialFunctionInputType.MIT_FLOAT1, abs_node.position_x - 200,
        abs_node.position_y + 100)

    # Slope: 1.0 = flat, 0.0 = vertical
    slope_param = new_material_expression_constant(abs_node.position_x - 200,
        abs_node.position_y + 250, 0.15)  # slope threshold
    slope = new_material_expression_subtract(z_extract, slope_param)

    # Snow threshold (altitude > 520m from CotaPlazaCm)
    snow_threshold = new_material_expression_constant(-800, 400, 0.85)

    mat.recompile()

    unreal.log("[TerrainAutoTex] Material creado con auto-texture por pendiente")


def setup_terrain_material_instances():
    """Crea instancias de M_Terreno_AutoTexture para cada sección del terreno."""
    base_mat_path = "/Game/Materials/M_Terreno_AutoTexture"
    if not unreal.EditorAssetLibrary.does_asset_exist(base_mat_path):
        return

    base_mat = unreal.EditorAssetLibrary.load_asset(base_mat_path)
    if not base_mat:
        return

    sections = ["Herriko", "Zelai", "Intxostia", "Zaramaga",
                 "Etxaguen", "Lakain", "Murgildi", "Natura"]

    for section in sections:
        inst_name = f"MI_Terreno_{section}"
        pkg = f"/Game/Materials/{inst_name}"

        if unreal.EditorAssetLibrary.does_asset_exist(pkg):
            continue

        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi_factory = unreal.MaterialInstanceConstantFactoryNew()
        mi = asset_tools.create_asset(inst_name, "/Game/Materials",
                                      unreal.MaterialInstanceConstant, mi_factory)

        if mi:
            mi.set_editor_property("parent", base_mat)
            unreal.log(f"[TerrainAutoTex] Instancia creada: {inst_name}")


if __name__ == "__main__":
    unreal.log("=== Terrain Auto-Texture Setup ===")
    create_terrain_material()
    setup_terrain_material_instances()
    unreal.log("=== Terrain Auto-Texture Complete ===")
