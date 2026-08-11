import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

NAME = "/Game/Materiales/M_Marca_Blanca"

if compat.assets().does_asset_exist(NAME):
    compat.assets().delete_asset(NAME)

factory = unreal.MaterialFactoryNew()
mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "M_Marca_Blanca", "/Game/Materiales", unreal.Material, factory)
if not mat:
    unreal.log_error("No se pudo crear M_Marca_Blanca")
    raise SystemExit

mat.set_editor_property("blend_mode", compat.OPACO)
mat.set_editor_property("shading_model", compat.LIT)

# White constant color
const3 = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, 0, 0)
const3.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
unreal.MaterialEditingLibrary.connect_material_property(const3, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

# Roughness
const1 = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, 0, 120)
const1.set_editor_property("r", 0.8)
unreal.MaterialEditingLibrary.connect_material_property(const1, "", unreal.MaterialProperty.MP_ROUGHNESS)

unreal.MaterialEditingLibrary.recompile_material(mat)
compat.assets().save_asset(NAME)
unreal.log("Creado M_Marca_Blanca")
