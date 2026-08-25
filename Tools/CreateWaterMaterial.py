"""
Gerstner wave water material for AguaNivel.
Surface shader with Fresnel reflection, wave-driven normal perturbation,
shoreline foam, and subsurface scattering approximation.

Run in UE5 Editor: Tools > Execute Python Script
"""
import unreal

def create_water_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_lib = unreal.MaterialEditingLibrary

    mat = asset_tools.create_asset(
        asset_name="M_AguaNivel",
        package_path="/Game/Materiales",
        asset_class=unreal.Material,
        factory=unreal.MaterialFactoryNew()
    )
    if not mat:
        unreal.log_error("Failed to create M_AguaNivel")
        return

    mat.set_editor_property("MaterialDomain", unreal.MaterialDomain.SURFACE)
    mat.set_editor_property("BlendMode", unreal.BlendMode_TRANSLUCENT)
    mat.set_editor_property("ShadingModel", unreal.ShadingModel_DEFAULT_LIT)
    mat.set_editor_property("TwoSided", True)
    mat.set_editor_property("EnableMobileSupport", False)

    # --- Scalar Parameters ---
    params = [
        ("TimeParam", 0.0),
        ("WaveAmplitude", 15.0),
        ("WaveFrequency", 0.02),
        ("WaveSteepness", 0.5),
        ("FresnelPower", 4.0),
        ("OpacityBase", 0.85),
        ("ShorelineDepth", 200.0),
        ("WaveSpeed", 0.5),
    ]
    scalar_nodes = {}
    for name, default in params:
        p = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1400, -200 + list(dict(params)).index(name) * 80)
        p.parameter_name = name
        p.default_value = default
        scalar_nodes[name] = p

    # --- Vector Parameter: WaterColor ---
    water_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1400, 500)
    water_color.parameter_name = "WaterColor"
    water_color.default_value = unreal.LinearColor(0.01, 0.05, 0.15, 0.85)

    # --- World Position for wave calc ---
    world_pos = mat_lib.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 800)

    # --- Gerstner Wave Normal Perturbation (approximation) ---
    # Sin wave displacement in XY for normal perturbation
    # sin(pos * freq + time * speed) * amplitude * freq * steepness
    time_param = scalar_nodes["TimeParam"]
    wave_amp = scalar_nodes["WaveAmplitude"]
    wave_freq = scalar_nodes["WaveFrequency"]
    wave_stp = scalar_nodes["WaveSteepness"]

    # Wave 1: X direction
    wx1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1000, 700)
    wx1.a.connect(world_pos.output)
    wx1.b.connect(wave_freq.output)

    wt1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -1000, 800)
    wt1.a.connect(time_param.output)
    wt1.b.connect(scalar_nodes["WaveSpeed"].output)

    wsum1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAdd, -800, 750)
    wsum1.a.connect(wx1.output)
    wsum1.b.connect(wt1.output)

    sin1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionSine, -600, 750)
    sin1.input.connect(wsum1.output)

    cos1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionCosine, -600, 850)
    cos1.input.connect(wsum1.output)

    # Normal X offset = cos(wave) * amp * freq * steepness
    nx1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 850)
    nx1.a.connect(cos1.output)
    nx1_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 900)
    nx1_b.a.connect(wave_amp.output)
    nx1_b_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 950)
    nx1_b_b.a.connect(wave_freq.output)
    nx1_b_b.b.connect(wave_stp.output)
    nx1_b.b.connect(nx1_b_b.output)
    nx1.b.connect(nx1_b.output)

    # Normal Z offset = -sin(wave) * amp * freq
    nz1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 950)
    nz1.a.connect(sin1.output)
    nz1_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 1000)
    nz1_b.a.connect(wave_amp.output)
    nz1_b.b.connect(wave_freq.output)
    nz1.b.connect(nz1_b.output)
    nz1_neg = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 950)
    nz1_neg.a.connect(nz1.output)
    nz1_neg_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 1050)
    nz1_neg_b.r = -1.0
    nz1_neg.b.connect(nz1_neg_b.output)

    # --- Compose tangent-space normal from wave perturbation ---
    # Normal = normalize(float3(nx, 0, nz))
    const_1 = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 700)
    const_1.r = 0.0

    append_n = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -200, 850)
    append_n.a.connect(nx1.output)
    append_n_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -100, 850)
    append_n_b.a.connect(const_1.output)
    append_n_b.b.connect(nz1_neg.output)
    append_n.b.connect(append_n_b.output)

    append_full = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, 0, 850)
    append_full.a.connect(append_n.output)
    const_1_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -100, 750)
    const_1_b.r = 1.0
    const_1_c = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -100, 800)
    const_1_c.a.connect(const_1_b.output)
    const_1_c.b.connect(const_1_b.output)
    append_full.b.connect(const_1_c.output)

    wave_normal = mat_lib.create_material_expression(mat, unreal.MaterialExpressionNormalize, 100, 850)
    wave_normal.input.connect(append_full.output)

    # --- Fresnel ---
    fresnel = mat_lib.create_material_expression(mat, unreal.MaterialExpressionFresnel, -400, 1200)
    fresnel.exponent.connect(scalar_nodes["FresnelPower"].output)

    # --- Final Color: lerp between deep water and reflected sky ---
    deep_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 500)
    deep_color.a.connect(water_color.output)
    deep_color_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 550)
    deep_color_b.r = deep_color_b.g = deep_color_b.b = 0.3
    deep_color.b.connect(deep_color_b.output)

    final_color = mat_lib.create_material_expression(mat, unreal.MaterialExpressionLerp, 200, 600)
    final_color.a.connect(deep_color.output)
    final_color_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 0, 500)
    final_color_b.r = 0.6
    final_color_b.g = 0.6
    final_color_b.b = 0.6
    final_color.b.connect(final_color_b.output)
    final_color.alpha.connect(fresnel.output)

    # --- Opacity: base + fresnel boost ---
    opacity = mat_lib.create_material_expression(mat, unreal.MaterialExpressionAdd, 200, 1200)
    opacity.a.connect(scalar_nodes["OpacityBase"].output)
    opacity_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 0, 1200)
    opacity_b.a.connect(fresnel.output)
    opacity_b_b = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 1300)
    opacity_b_b.r = 0.15
    opacity_b.b.connect(opacity_b_b.output)
    opacity.b.connect(opacity_b.output)

    # --- Roughness ---
    roughness = mat_lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, 200, 1100)
    roughness.parameter_name = "Roughness"
    roughness.default_value = 0.05

    # --- Metallic = 0 ---
    metallic = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 1000)
    metallic.r = 0.0

    # --- Specular = 0.5 (water has strong specular) ---
    specular = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 950)
    specular.r = 0.5

    # --- Connect outputs ---
    mat_lib.connect_material_property(final_color.output, unreal.MaterialProperty.MATERIAL_PROPERTY_BASE_COLOR, mat)
    mat_lib.connect_material_property(wave_normal.output, unreal.MaterialProperty.MATERIAL_PROPERTY_NORMAL, mat)
    mat_lib.connect_material_property(opacity.output, unreal.MaterialProperty.MATERIAL_PROPERTY_OPACITY, mat)
    mat_lib.connect_material_property(roughness.output, unreal.MaterialProperty.MATERIAL_PROPERTY_ROUGHNESS, mat)
    mat_lib.connect_material_property(metallic.output, unreal.MaterialProperty.MATERIAL_PROPERTY_METALLIC, mat)
    mat_lib.connect_material_property(specular.output, unreal.MaterialProperty.MATERIAL_PROPERTY_SPECULAR, mat)

    # Subsurface: slight underwater glow
    subsurface = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, 200, 1400)
    subsurface.r = 0.1
    mat_lib.connect_material_property(subsurface.output, unreal.MaterialProperty.MATERIAL_PROPERTY_SUBSURFACE_COLOR, mat)

    unreal.EditorAssetLibrary.save_asset("/Game/Materiales/M_AguaNivel")
    unreal.log("Created M_AguaNivel — Gerstner wave water material")

if __name__ == "__main__":
    create_water_material()
