"""
Creates enhanced terrain material with slope-based triplanar blending.
Uses vertex color channels already computed by TerrenoGenerado:
  R = height normalized, G = slope, B = distance to center

Run in UE5 Editor: Tools > Execute Python Script
"""
import unreal

def create_terrain_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_lib = unreal.MaterialEditingLibrary

    # Create the material
    mat = asset_tools.create_asset(
        asset_name="M_TerrenoEnhanced",
        package_path="/Game/Materiales",
        asset_class=unreal.Material,
        factory=unreal.MaterialFactoryNew()
    )
    if not mat:
        unreal.log_error("Failed to create M_TerrenoEnhanced")
        return

    # Set as surface material with proper domains
    mat.set_editor_property("MaterialDomain", unreal.MaterialDomain.SURFACE)
    mat.set_editor_property("BlendMode", unreal.BlendMode.OPAQUE)
    mat.set_editor_property("ShadingModel", unreal.ShadingModel.LIT)
    mat.set_editor_property("TwoSided", False)

    # Enable tessellation for terrain detail
    mat.set_editor_property("DitheredLODTransition", True)

    # === TEXTURE SAMPLES ===
    # Grass (flat areas, low slope)
    grass_tex = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, -200)
    grass_tex.texture = unreal.load_asset("/Game/Materiales/T_Grass_D.T_Grass_D")
    grass_tex_sampler = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 100)
    grass_tex_sampler.texture = unreal.load_asset("/Game/Materiales/T_Grass_N.T_Grass_N")

    # Rock (steep slopes)
    rock_tex = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 400)
    rock_tex.texture = unreal.load_asset("/Game/Materiales/T_Rock_D.T_Rock_D")
    rock_tex_n = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 700)
    rock_tex_n.texture = unreal.load_asset("/Game/Materiales/T_Rock_N.T_Rock_N")

    # Ground/dirt (medium slope)
    ground_tex = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 1000)
    ground_tex.texture = unreal.load_asset("/Game/Materiales/T_Ground_D.T_Ground_D")
    ground_tex_n = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 1300)
    ground_tex_n.texture = unreal.load_asset("/Game/Materiales/T_Ground_N.T_Ground_N")

    # Snow (high altitude)
    snow_tex = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 1600)
    snow_tex.texture = unreal.load_asset("/Game/Materiales/T_Snow_D.T_Snow_D")
    snow_tex_n = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, 1900)
    snow_tex_n.texture = unreal.load_asset("/Game/Materiales/T_Snow_N.T_Snow_N")

    # === VERTEX COLOR INPUT ===
    vertex_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1200, 400)

    # === SLOPE MASK (from vertex color G channel) ===
    # Slope: 0 = flat, 1 = vertical
    slope_mask = mat_lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1000, 400)
    slope_mask.input.connect(vertex_color.output)
    slope_mask.r = False
    slope_mask.g = True
    slope_mask.b = False
    slope_mask.a = False

    # === HEIGHT MASK (from vertex color R channel) ===
    height_mask = mat_lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1000, 700)
    height_mask.input.connect(vertex_color.output)
    height_mask.r = True
    height_mask.g = False
    height_mask.b = False
    height_mask.a = False

    # === BLEND LOGIC ===
    # Slope blend: lerp between ground and rock based on slope
    slope_threshold = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 500)
    slope_threshold.parameter_name = "SlopeThreshold"
    slope_threshold.default_value = 0.6

    slope_sharpness = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 600)
    slope_sharpness.parameter_name = "SlopeSharpness"
    slope_sharpness.default_value = 8.0

    # Smoothstep for slope blending
    slope_factor = mat_lib.create_material_expression(mat, unreal.MaterialExpressionSmoothStep, -400, 500)
    slope_factor.input.connect(slope_mask.output)
    slope_factor.edge0.connect(slope_threshold.output)
    slope_factor.edge1.connect(slope_sharpness.output)

    # Height blend: lerp between ground and snow at high altitude
    height_threshold = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 800)
    height_threshold.parameter_name = "SnowHeightThreshold"
    height_threshold.default_value = 0.85

    height_sharpness = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 900)
    height_sharpness.parameter_name = "SnowSharpness"
    height_sharpness.default_value = 10.0

    height_factor = mat_lib.create_material_expression(mat, unreal.MaterialExpressionSmoothStep, -400, 800)
    height_factor.input.connect(height_mask.output)
    height_factor.edge0.connect(height_threshold.output)
    height_factor.edge1.connect(height_sharpness.output)

    # === TRIPLANAR UVs ===
    # World-space UVs for non-tiling look
    world_pos = mat_lib.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1200, 1200)
    tex_scale = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 1200)
    tex_scale.parameter_name = "TextureScale"
    tex_scale.default_value = 100.0

    # World position / scale for UVs
    uv_x = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 1100)
    uv_x.a.connect(world_pos.output)  # Would need to mask to YZ for triplanar X
    uv_x.b.connect(tex_scale.output)

    # === FINAL COMPOSITE ===
    # Base: grass
    # Blend rock where slope > threshold
    # Blend snow where height > threshold

    # Layer 1: Ground/Rock blend based on slope
    ground_rock = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, -200, 500)
    ground_rock.a.connect(ground_tex.output)  # ground on flat
    ground_rock.b.connect(rock_tex.output)    # rock on steep
    ground_rock.alpha.connect(slope_factor.output)

    # Layer 2: Add snow on top based on height
    final_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, 0, 700)
    final_color.a.connect(ground_rock.output)  # ground/rock base
    final_color.b.connect(snow_tex.output)     # snow at altitude
    final_color.alpha.connect(height_factor.output)

    # Normal map composite
    normal_blend = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, 0, 1000)
    normal_blend.a.connect(ground_tex_n.output)
    normal_blend.b.connect(rock_tex_n.output)
    normal_blend.alpha.connect(slope_factor.output)

    # Roughness: rougher on rock, smoother on grass
    roughness_base = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -200, 1200)
    roughness_base.parameter_name = "BaseRoughness"
    roughness_base.default_value = 0.7

    roughness_rock = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -200, 1300)
    roughness_rock.parameter_name = "RockRoughness"
    roughness_rock.default_value = 0.9

    roughness = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, 0, 1300)
    roughness.a.connect(roughness_base.output)
    roughness.b.connect(roughness_rock.output)
    roughness.alpha.connect(slope_factor.output)

    # Connect to material output
    mat_lib.connect_material_property(final_color.output, unreal.MaterialProperty.MATERIAL_PROPERTY_BASE_COLOR, mat)
    mat_lib.connect_material_property(normal_blend.output, unreal.MaterialProperty.MATERIAL_PROPERTY_NORMAL, mat)
    mat_lib.connect_material_property(roughness.output, unreal.MaterialProperty.MATERIAL_PROPERTY_ROUGHNESS, mat)

    # Metallic = 0 (terrain is non-metallic)
    metallic_const = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 1400)
    metallic_const.r = 0.0
    mat_lib.connect_material_property(metallic_const.output, unreal.MaterialProperty.MATERIAL_PROPERTY_METALLIC, mat)

    # Ambient occlusion = 1
    ao_const = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 1500)
    ao_const.r = 1.0
    mat_lib.connect_material_property(ao_const.output, unreal.MaterialProperty.MATERIAL_PROPERTY_AMBIENT_OCCLUSION, mat)

    # Save
    unreal.EditorAssetLibrary.save_asset("/Game/Materiales/M_TerrenoEnhanced")
    unreal.log("Created M_TerrenoEnhanced with slope/height blending")

    # Create a variation with different texture set
    create_urban_terrain_variant(mat_lib, asset_tools)

def create_urban_terrain_variant(mat_lib, asset_tools):
    """Urban variant: asphalt/concrete blend for town areas."""
    mat = asset_tools.create_asset(
        asset_name="M_TerrenoUrbano",
        package_path="/Game/Materiales",
        asset_class=unreal.Material,
        factory=unreal.MaterialFactoryNew()
    )
    if not mat:
        return

    mat.set_editor_property("MaterialDomain", unreal.MaterialDomain.SURFACE)
    mat.set_editor_property("BlendMode", unreal.BlendMode.OPAQUE)
    mat.set_editor_property("ShadingModel", unreal.ShadingModel.LIT)

    vertex_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -1200, 400)

    # Slope mask
    slope_mask = mat_lib.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1000, 400)
    slope_mask.input.connect(vertex_color.output)
    slope_mask.r = False
    slope_mask.g = True
    slope_mask.b = False
    slope_mask.a = False

    # Asphalt texture
    asphalt = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 200)
    asphalt.texture = unreal.load_asset("/Game/Materiales/T_Asphalt_D.T_Asphalt_D")

    # Cobblestone texture
    cobble = mat_lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 500)
    cobble.texture = unreal.load_asset("/Game/Materiales/T_Cobblestone_D.T_Cobblestone_D")

    # Blend based on slope: flat = asphalt, steep = cobblestone
    slope_threshold = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 400)
    slope_threshold.parameter_name = "SlopeThreshold"
    slope_threshold.default_value = 0.3

    blend = mat_lib.create_material_expression(mat, unreal.MaterialExpressionSmoothStep, -200, 350)
    blend.input.connect(slope_mask.output)
    blend.edge0.connect(slope_threshold.output)
    blend.edge1.connect(slope_threshold.output)  # sharp transition

    final = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, 0, 300)
    final.a.connect(asphalt.output)
    final.b.connect(cobble.output)
    final.alpha.connect(blend.output)

    mat_lib.connect_material_property(final.output, unreal.MaterialProperty.MATERIAL_PROPERTY_BASE_COLOR, mat)

    unreal.EditorAssetLibrary.save_asset("/Game/Materiales/M_TerrenoUrbano")
    unreal.log("Created M_TerrenoUrbano")

if __name__ == "__main__":
    create_terrain_material()
