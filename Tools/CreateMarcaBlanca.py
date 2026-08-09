import unreal

NAME = "/Game/Materiales/M_Marca_Blanca"

if unreal.EditorAssetLibrary.does_asset_exist(NAME):
    unreal.EditorAssetLibrary.delete_asset(NAME)

factory = unreal.MaterialFactoryNew()
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "M_Marca_Blanca", "/Game/Materiales", unreal.Material, factory)
if not mat:
    unreal.log_error("No se pudo crear M_Marca_Blanca")
    raise SystemExit

mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

# White constant color
const3 = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, 0, 0)
const3.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
unreal.MaterialEditingLibrary.connect_material_property(const3, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

# Roughness
const1 = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, 0, 120)
const1.set_editor_property("r", 0.8)
unreal.MaterialEditingLibrary.connect_material_property(const1, "", unreal.MaterialProperty.MP_ROUGHNESS)

unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(NAME)
unreal.log("Creado M_Marca_Blanca")
