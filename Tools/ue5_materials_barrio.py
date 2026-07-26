"""
UE5 Material Setup - Creates all materials by barrio with real PBR textures
Run inside UE5: exec(open(r"F:\...\Tools\ue5_materials_barrio.py").read())
"""
import unreal
import os
import json

ASSETS = "/Game/Content/AssetsImportados"
MAT_BASE = "/Game/Materiales"

def ensure_folder(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)

def create_material_with_texture(name, texture_path, output_path, mat_domain="surface", 
                                  blend="opaque", two_sided=False, is_water=False):
    full = f"{output_path}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        return full
    
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, output_path, unreal.Material, unreal.MaterialFactoryNew()
    )
    if not mat:
        return None
    
    mat.set_editor_property("material_domain", unreal.MaterialDomain.SURFACE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.TRANSLUCENT if is_water else unreal.BlendMode.OPAQUE)
    mat.set_editor_property("shading_model", unreal.ShadingModel.DEFAULT_LIT)
    mat.set_editor_property("two_sided", two_sided)
    
    tex = unreal.EditorAssetLibrary.load_asset(texture_path)
    if tex:
        expr = unreal.MaterialExpressionTextureSample()
        expr.texture = tex
        mat.add_editor_only_property("expressions", expr)
        mat.connect_material_property("Base Color", "RGB", expr, "RGB")
    
    unreal.EditorAssetLibrary.save_asset(full)
    return full

def main():
    unreal.log("=" * 60)
    unreal.log("ALSASUA - MATERIALES POR BARRIO")
    unreal.log("=" * 60)
    
    ensure_folder(MAT_BASE)
    
    # --- Wall Materials ---
    wall_textures = {
        "M_Wall_Cobblestone": f"{ASSETS}/Muros/cobblestone_wall/Wall_cobblestone_1024",
        "M_Wall_BrickOld": f"{ASSETS}/Muros/wall_bricks_old_1024",
        "M_Wall_Brick7": f"{ASSETS}/Muros/brick7",
        "M_Wall_Cobblestone_1024": f"{ASSETS}/Muros/wall_cobblestone_1024px",
    }
    for name, tex in wall_textures.items():
        create_material_with_texture(name, tex, MAT_BASE)
    
    # --- Pavement Materials ---
    pavement_textures = {
        "M_Asphalt_Pealed": f"{ASSETS}/Pavimentos/pebbled-asphalt1/pebbled-asphalt1-unity/pebbled_asphalt_albedo",
        "M_Asphalt_02": f"{ASSETS}/Pavimentos/asphalt_02_4k/textures/asphalt_02_diff_4k",
        "M_Cobblestone_Pavement": f"{ASSETS}/Pavimentos/t_ground_cobblestone_3",
    }
    for name, tex in pavement_textures.items():
        create_material_with_texture(name, tex, MAT_BASE)
    
    # --- Rock Materials ---
    for i in range(1, 6):
        name = f"M_Rock_0{i}"
        tex = f"{ASSETS}/Naturaleza/rocks/0{i}/diffuse"
        create_material_with_texture(name, tex, MAT_BASE)
    
    # --- Grass Materials ---
    grass_textures = {
        "M_Grass_Larga": f"{ASSETS}/Naturaleza/grass_02/grass_04/diffus",
        "M_Grass_Corta": f"{ASSETS}/Naturaleza/grass_07/diffus",
    }
    for name, tex in grass_textures.items():
        create_material_with_texture(name, tex, MAT_BASE, two_sided=True)
    
    # --- Bark Material ---
    create_material_with_texture("M_Bark_Oak", f"{ASSETS}/Arboles/Roble/models/oakbark", MAT_BASE)
    
    # --- Barrio-specific materials ---
    barrio_map = {
        "Herriko": {"wall": "M_Wall_Cobblestone", "roof": "M_Wall_BrickOld"},
        "Zelai": {"wall": "M_Wall_BrickOld", "roof": "M_Wall_Brick7"},
        "Intxostia": {"wall": "M_Wall_Cobblestone_1024", "roof": "M_Wall_Brick7"},
        "SanPedro": {"wall": "M_Wall_Brick7", "roof": "M_Wall_BrickOld"},
        "Errota": {"wall": "M_Wall_Cobblestone", "roof": "M_Wall_BrickOld"},
        "Harrobieta": {"wall": "M_Wall_Cobblestone", "roof": "M_Wall_Cobblestone"},
        "Ferroviario": {"wall": "M_Wall_Brick7", "roof": "M_Wall_BrickOld"},
        "Monte": {"wall": "M_Wall_Cobblestone", "roof": "M_Wall_BrickOld"},
    }
    
    for barrio, mats in barrio_map.items():
        wall_mat = unreal.EditorAssetLibrary.load_asset(f"{MAT_BASE}/{mats['wall']}")
        if wall_mat:
            instance_name = f"MI_{barrio}_Wall"
            instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                instance_name, MAT_BASE, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()
            )
            if instance:
                instance.set_editor_property("parent", wall_mat)
                unreal.EditorAssetLibrary.save_asset(f"{MAT_BASE}/{instance_name}")
                unreal.log(f"  Material instance: {instance_name}")
    
    unreal.log("=" * 60)
    unreal.log("MATERIALES COMPLETADOS")
    unreal.log("=" * 60)

if __name__ == "__main__":
    main()
