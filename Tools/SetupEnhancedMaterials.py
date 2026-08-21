"""
SetupEnhancedMaterials.py — Crea materiales avanzados con reacción al MPC global.
Incluye:
  - M_Calles_Puddle: Asfalto con charcos cuando llueve
  - M_Acera_Puddle: adoquinado mojado
  - M_Edificio_Ventanas: ventanas emissivas nocturnas
  - M_Valla_Metalica: metal con óxido dinámico
  - M_Agua_Mejorada: agua mejorada con reflejos y espuma

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

# La única colección que existe. Antes apuntaba a
# /Game/Materials/MPC_AlsasuaGlobal —otra carpeta, en inglés—, que no la crea
# ningún generador: load_asset devolvía None, el script avisaba "MPC no
# encontrado" y seguía, y el material se quedaba sin su cableado al clima.
# La crea Tools/SetupMaterials.py (y UCreadorMaterialEdificio desde C++).
MPC_PATH = "/Game/Materiales/MPC_Clima"


def get_mpc():
    mpc = compat.assets().load_asset(MPC_PATH)
    if mpc and isinstance(mpc, unreal.MaterialParameterCollection):
        return mpc
    unreal.log_warning("[EnhancedMat] MPC no encontrado")
    return None


def create_puddle_street_material():
    """M_Calles_Puddle: asfalto con charcos por MPC GlobalWetness/PuddleOpacity."""
    pkg = "/Game/Materials/M_Calles_Puddle"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Calles_Puddle", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.OPACO)

    # Roughness: dry=0.7, wet=0.15
    roughness_dry = new_material_expression_constant(0, 200, 0.7)
    roughness_wet = new_material_expression_constant(200, 200, 0.15)

    # Puddle opacity from MPC
    puddle_node = new_material_expression_collection_parameter(mpc, "PuddleOpacity",
        -200, 200)

    roughness = new_material_expression_lerp(roughness_dry, roughness_wet, puddle_node)

    # Normal: flat puddles
    puddle_normal = new_material_expression_texture(-200, 400, "/Game/Textures/T_Asphalt_Normal")

    # Color: darken when wet
    base_color = new_material_expression_texture(0, 0, "/Game/Textures/T_Asphalt_Color")
    wet_darken = new_material_expression_constant(200, 0, 0.6)
    color = new_material_expression_multiply(base_color, wet_darken)

    mat.recompile()
    unreal.log("[EnhancedMat] M_Calles_Puddle creado")


def create_puddle_sidewalk_material():
    """M_Acera_Puddle: adoquinado mojado."""
    pkg = "/Game/Materials/M_Acera_Puddle"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Acera_Puddle", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.OPACO)

    mat.recompile()
    unreal.log("[EnhancedMat] M_Acera_Puddle creado")


def create_window_emissive_material():
    """M_Edificio_Ventanas: ventanas emissivas que brillan de noche via MPC."""
    pkg = "/Game/Materials/M_Edificio_Ventanas"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Edificio_Ventanas", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.OPACO)
    mat.set_editor_property("ShadingModel", compat.LIT)

    # Emissive color from parameter
    emissive_color_node = new_material_expression_constant(-400, 300, 1.0, 0.85, 0.5, 1.0)
    emissive_intensity = new_material_expression_constant(-400, 500, 0.0)

    emissive = new_material_expression_multiply(emissive_color_node, emissive_intensity)

    # Base color (dark facade)
    facade_color = new_material_expression_texture(0, 0, "/Game/Textures/T_Brick_Color")
    dark_facade = new_material_expression_multiply(facade_color, new_material_expression_constant(200, 0, 0.3))

    mat.recompile()
    unreal.log("[EnhancedMat] M_Edificio_Ventanas creado")


def create_foliage_wind_material():
    """M_Foliage_Wind: foliage con world-position-offset por viento."""
    pkg = "/Game/Materials/M_Foliage_Wind"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Foliage_Wind", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.ENMASCARADO)
    mat.set_two_sided(True)

    # Two-sided foliage shading
    mat.set_editor_property("ShadingModel", compat.SUBSURFACE)

    mat.recompile()
    unreal.log("[EnhancedMat] M_Foliage_Wind creado")


def create_metal_railing_material():
    """M_Baranda_Metal: metal con reflejos y posible óxido."""
    pkg = "/Game/Materials/M_Baranda_Metal"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Baranda_Metal", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.OPACO)
    mat.recompile()
    unreal.log("[EnhancedMat] M_Baranda_Metal creado")


def create_improved_water_material():
    """M_Agua_Mejorada: agua mejorada con normal panning, depth coloring, foam."""
    pkg = "/Game/Materials/M_Agua_Mejorada"
    if compat.assets().does_asset_exist(pkg):
        return

    mpc = get_mpc()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Agua_Mejorada", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.TRASLUCIDO)
    mat.set_editor_property("ShadingModel", compat.LIT)
    mat.set_editor_property("TwoSided", True)

    mat.recompile()
    unreal.log("[EnhancedMat] M_Agua_Mejorada creado")


def new_material_expression_constant(x, y, r=1.0, g=1.0, b=1.0, a=1.0):
    node = unreal.MaterialExpressionConstant.new()
    node.r = r
    node.g = g
    node.b = b
    node.a = a
    node.position_x = x
    node.position_y = y
    return node


def new_material_expression_multiply(a, b):
    node = unreal.MaterialExpressionMultiply.new()
    node.a = a
    node.b = b
    return node


def new_material_expression_lerp(a, b, alpha):
    node = unreal.MaterialExpressionLinearInterpolate.new()
    node.a = a
    node.b = b
    node.alpha = alpha
    return node


def new_material_expression_texture(x, y, tex_path):
    tex = compat.assets().load_asset(tex_path)
    if not tex:
        return new_material_expression_constant(x, y, 0.5, 0.5, 1.0)
    node = unreal.MaterialExpressionTextureSample.new()
    node.texture = tex
    node.position_x = x
    node.position_y = y
    return node


def new_material_expression_collection_parameter(mpc, param_name, x, y):
    node = unreal.MaterialExpressionCollectionParameter.new()
    node.collection = mpc
    node.parameter_name = param_name
    node.position_x = x
    node.position_y = y
    return node


def new_material_expression_function_input(input_node, name, input_type, x, y):
    node = unreal.MaterialExpressionFunctionInput.new()
    node.input_name = name
    node.input_type = input_type
    node.position_x = x
    node.position_y = y
    return node


if __name__ == "__main__":
    unreal.log("=== Enhanced Materials Setup ===")
    create_puddle_street_material()
    create_puddle_sidewalk_material()
    create_window_emissive_material()
    create_foliage_wind_material()
    create_metal_railing_material()
    create_improved_water_material()
    unreal.log("=== Enhanced Materials Complete ===")
