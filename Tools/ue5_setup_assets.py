r"""
UE5 Materials & Assets Setup Script
Ejecutar DENTRO de UE5 con Python Script Plugin habilitado:
  Window > Developer Tools > Output Log > 
    exec(open(r"F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools\ue5_setup_assets.py").read())

Crea materiales PBR, DataAssets y configura el sistema de assets reales.
"""
import unreal
import os
import json

# Paths
CONTENT = "/Game/Content"
ASSETS_PATH = "/Game/Content/AssetsImportados"
DATOS_PATH = "/Game/Content/Datos"
MATERIALS_PATH = "/Game/Materials"

def create_folder(path):
    """Create a folder in the Content Browser"""
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)
        unreal.log(f"Carpeta creada: {path}")

def create_material(name, parent_material_path, texture_paths, output_path):
    """
    Create a UMaterial with texture assignments
    texture_paths: dict with keys like 'base_color', 'normal', 'roughness', 'ao'
    """
    full_path = f"{output_path}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.log(f"Material ya existe: {full_path}")
        return full_path
    
    # Get parent material
    parent = unreal.EditorAssetLibrary.load_asset(parent_material_path)
    if not parent:
        unreal.log_warning(f"Parent material no encontrado: {parent_material_path}")
        return None
    
    # Create material
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, output_path, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not mat:
        unreal.log_error(f"Error creando material: {name}")
        return None
    
    # Set material domain to Surface
    mat.set_editor_property("material_domain", unreal.MaterialDomain.SURFACE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.OPAQUE)
    mat.set_editor_property("shading_model", unreal.ShadingModel.DEFAULT_LIT)
    
    # Apply textures
    for tex_key, tex_path in texture_paths.items():
        tex = unreal.EditorAssetLibrary.load_asset(tex_path)
        if not tex:
            unreal.log_warning(f"Textura no encontrada: {tex_path}")
            continue
        
        # Create texture sample and connect
        expr = unreal.MaterialExpressionTextureSample()
        expr.texture = tex
        mat.add_expressioned_property("expressions", expr)
        
        if tex_key == 'base_color':
            mat.connect_material_property("Base Color", "RGB", expr, "RGB")
        elif tex_key == 'normal':
            mat.connect_material_property("Normal", "RGB", expr, "RGB")
        elif tex_key == 'roughness':
            mat.connect_material_property("Roughness", "R", expr, "R")
        elif tex_key == 'ao':
            mat.connect_material_property("Ambient Occlusion", "R", expr, "R")
        elif tex_key == 'metallic':
            mat.connect_material_property("Metallic", "R", expr, "R")
        elif tex_key == 'height':
            mat.connect_material_property("World Position Offset", "R", expr, "R")
    
    # Save
    unreal.EditorAssetLibrary.save_asset(full_path)
    unreal.log(f"Material creado: {full_path}")
    return full_path


def create_all_materials():
    """Create all PBR materials for Alsasua"""
    create_folder(MATERIALS_PATH)
    
    # ==========================================
    # PAVIMENTOS
    # ==========================================
    asphalt_path = f"{ASSETS_PATH}/Pavimentos/pebbled-asphalt1/pebbled-asphalt1-unity"
    create_material(
        "M_Asphalt_Alsasua",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{asphalt_path}/pebbled_asphalt_albedo",
            'normal': f"{asphalt_path}/pebbled_asphalt_Normal-ogl",
            'ao': f"{asphalt_path}/pebbled_asphalt_ao",
            'height': f"{asphalt_path}/pebbled_asphalt_Height",
        },
        MATERIALS_PATH
    )
    
    # Asphalt 02 4K
    asphalt02_path = f"{ASSETS_PATH}/Pavimentos/asphalt_02_4k/textures"
    create_material(
        "M_Asphalt_02_Alsasua",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{asphalt02_path}/asphalt_02_diff_4k",
            'roughness': f"{asphalt02_path}/asphalt_02_rough_4k",
            'height': f"{asphalt02_path}/asphalt_02_disp_4k",
        },
        MATERIALS_PATH
    )
    
    # Cobblestone pavement
    create_material(
        "M_Cobblestone_Alsasua",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{ASSETS_PATH}/Pavimentos/t_ground_cobblestone_3",
        },
        MATERIALS_PATH
    )
    
    # ==========================================
    # MUROS
    # ==========================================
    cobble_wall_path = f"{ASSETS_PATH}/Muros/cobblestone_wall"
    create_material(
        "M_Wall_Cobblestone",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{cobble_wall_path}/Wall_cobblestone_1024",
            'normal': f"{cobble_wall_path}/Wall_cobblestone_cra_NormalGL",
            'ao': f"{cobble_wall_path}/Wall_cobblestone_cra_OcclusionRoughness",
        },
        MATERIALS_PATH
    )
    
    # Brick old wall
    create_material(
        "M_Wall_BrickOld",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{ASSETS_PATH}/Muros/wall_bricks_old_1024",
        },
        MATERIALS_PATH
    )
    
    # Brick7
    create_material(
        "M_Wall_Brick7",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{ASSETS_PATH}/Muros/brick7",
        },
        MATERIALS_PATH
    )
    
    # ==========================================
    # VEGETACION
    # ==========================================
    for i in [4, 5, 6]:
        grass_path = f"{ASSETS_PATH}/Naturaleza/grass_02/grass_0{i}"
        create_material(
            f"M_Grass_0{i}",
            f"/Engine/EngineMaterials/DefaultMaterial",
            {
                'base_color': f"{grass_path}/diffus",
                'normal': f"{grass_path}/normal",
                'roughness': f"{grass_path}/specular",
            },
            MATERIALS_PATH
        )
    
    # Grass 07
    grass07_path = f"{ASSETS_PATH}/Naturaleza/grass_07"
    create_material(
        "M_Grass_07",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{grass07_path}/diffus",
            'normal': f"{grass07_path}/normal",
            'roughness': f"{grass07_path}/specular",
        },
        MATERIALS_PATH
    )
    
    # Rocks
    for i in range(1, 6):
        rock_path = f"{ASSETS_PATH}/Naturaleza/rocks/0{i}"
        create_material(
            f"M_Rock_0{i}",
            f"/Engine/EngineMaterials/DefaultMaterial",
            {
                'base_color': f"{rock_path}/diffuse",
                'normal': f"{rock_path}/normal",
                'roughness': f"{rock_path}/specular",
            },
            MATERIALS_PATH
        )
    
    # ==========================================
    # ARBOLES
    # ==========================================
    # Oak bark
    create_material(
        "M_Bark_Oak",
        f"/Engine/EngineMaterials/DefaultMaterial",
        {
            'base_color': f"{ASSETS_PATH}/Arboles/Roble/models/oakbark",
        },
        MATERIALS_PATH
    )


def create_asset_registry():
    """Create a DataAsset that maps all real assets"""
    create_folder(DATOS_PATH)
    
    # Create a JSON config file for the asset mapping
    asset_map = {
        "pavements": {
            "asphalt": f"{ASSETS_PATH}/Pavimentos/pebbled-asphalt1/pebbled-asphalt1-unity/pebbled_asphalt_albedo",
            "asphalt_02": f"{ASSETS_PATH}/Pavimentos/asphalt_02_4k/textures/asphalt_02_diff_4k",
            "cobblestone": f"{ASSETS_PATH}/Pavimentos/t_ground_cobblestone_3",
        },
        "walls": {
            "cobblestone": f"{ASSETS_PATH}/Muros/cobblestone_wall/Wall_cobblestone_1024",
            "brick_old": f"{ASSETS_PATH}/Muros/wall_bricks_old_1024",
            "brick7": f"{ASSETS_PATH}/Muros/brick7",
        },
        "trees": {
            "haltza": f"{ASSETS_PATH}/Arboles/Aliso_Negro/Tree_Black_Alder_01_A",
            "haya": f"{ASSETS_PATH}/Arboles/Haya_Europea/Tree_European_Beech_01_A",
            "abedul": f"{ASSETS_PATH}/Arboles/Abedul/Tree_Silver_Birch_01_A",
            "sauce": f"{ASSETS_PATH}/Arboles/Sauce/Tree_Goat_Willow_01_A",
            "pino_carrasco": f"{ASSETS_PATH}/Arboles/Pino_Carrasco/Tree_Aleppo_Pine_01_A",
            "pino_baltico": f"{ASSETS_PATH}/Arboles/Pino_Baltico/Tree_Baltic_Pine_01_A",
            "alamo": f"{ASSETS_PATH}/Arboles/Alamo_Temblon/Tree_European_Aspen_01_A",
            "roble": f"{ASSETS_PATH}/Arboles/Roble/models/oak",
        },
        "foliage": {
            "grass_variants": [
                f"{ASSETS_PATH}/Naturaleza/grass_02/grass_04/diffus",
                f"{ASSETS_PATH}/Naturaleza/grass_02/grass_05/diffus",
                f"{ASSETS_PATH}/Naturaleza/grass_02/grass_06/diffus",
                f"{ASSETS_PATH}/Naturaleza/grass_07/diffus",
            ],
            "rocks": [
                f"{ASSETS_PATH}/Naturaleza/rocks/01/rock_01",
                f"{ASSETS_PATH}/Naturaleza/rocks/02/rock_02",
                f"{ASSETS_PATH}/Naturaleza/rocks/03/rock_03",
                f"{ASSETS_PATH}/Naturaleza/rocks/04/rock_04",
                f"{ASSETS_PATH}/Naturaleza/rocks/05/rock_05",
            ],
            "hedges": [
                f"{ASSETS_PATH}/Naturaleza/Hedges/Long/HedgeLong",
                f"{ASSETS_PATH}/Naturaleza/Hedges/Small/HedgeSmall",
            ],
        },
        "materials": {
            "asphalt": f"{MATERIALS_PATH}/M_Asphalt_Alsasua",
            "asphalt_02": f"{MATERIALS_PATH}/M_Asphalt_02_Alsasua",
            "cobblestone": f"{MATERIALS_PATH}/M_Cobblestone_Alsasua",
            "wall_cobblestone": f"{MATERIALS_PATH}/M_Wall_Cobblestone",
            "wall_brick": f"{MATERIALS_PATH}/M_Wall_BrickOld",
            "bark_oak": f"{MATERIALS_PATH}/M_Bark_Oak",
        }
    }
    
    # Save as JSON for the C++ systems to load
    json_path = unreal.Paths.project_content_dir() + "Datos/asset_registry.json"
    with open(json_path, 'w') as f:
        json.dump(asset_map, f, indent=2)
    
    unreal.log(f"Asset registry guardado en: {json_path}")
    return asset_map


def main():
    unreal.log("=" * 60)
    unreal.log("ALSASUA COPIA 1:1 - SETUP DE ASSETS REALES")
    unreal.log("=" * 60)
    
    # Create folders
    create_folder(MATERIALS_PATH)
    
    # Create all materials
    create_all_materials()
    
    # Create asset registry
    create_asset_registry()
    
    unreal.log("=" * 60)
    unreal.log("SETUP COMPLETADO - Materiales y Asset Registry creados")
    unreal.log("=" * 60)


if __name__ == "__main__":
    main()
