r"""
ImportAssets.bat - Batch script to import assets into UE5
Run from: Tools/ImportAssets.bat
Requires: UE 5.8 with Python Script Plugin enabled

Open UE5 Editor, then in Output Log run:
  exec(open(unreal.Paths.project_dir() + 'Tools/ue5_setup_assets.py').read())

Then run this script to import FBX/OBJ/USD files.
"""

import unreal
import os
import glob

ASSETS_ROOT = unreal.Paths.project_content_dir() + "AssetsImportados"

def import_fbx(fbx_path, dest_path, name):
    """Import a single FBX file"""
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing_settings", True)
    
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    task.set_editor_property("options", options)
    
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    
    if task.get_editor_property("result"):
        unreal.log(f"  Importado: {name}")
    else:
        unreal.log_warning(f"  Error importando: {name}")

def import_obj(obj_path, dest_path, name):
    """Import a single OBJ file"""
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", obj_path)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing_settings", True)
    
    options = unreal.AssetImportTask()
    task.set_editor_property("options", options)
    
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    
    if task.get_editor_property("result"):
        unreal.log(f"  Importado OBJ: {name}")
    else:
        unreal.log_warning(f"  Error importando OBJ: {name}")

def import_textures(texture_dir, dest_path, pattern="*"):
    """Import all textures from a directory"""
    textures = glob.glob(os.path.join(texture_dir, pattern))
    textures = [t for t in textures if t.lower().endswith(('.png', '.jpg', '.tga', '.exr', '.hdr'))]
    
    tasks = []
    for tex_path in textures:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", tex_path)
        task.set_editor_property("destination_path", dest_path)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        tasks.append(task)
    
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
        imported = sum(1 for t in tasks if t.get_editor_property("result"))
        unreal.log(f"  {imported}/{len(tasks)} texturas importadas de {dest_path}")

def main():
    unreal.log("=" * 60)
    unreal.log("ALSASUA - IMPORTACION DE ASSETS REALES")
    unreal.log("=" * 60)
    
    # Import tree meshes (FBX files where available)
    tree_fbx_dir = os.path.join(ASSETS_ROOT, "Arboles")
    for species_dir in os.listdir(tree_fbx_dir):
        species_path = os.path.join(tree_fbx_dir, species_dir)
        if not os.path.isdir(species_path):
            continue
        fbx_files = glob.glob(os.path.join(species_path, "**", "*.fbx"), recursive=True)
        for fbx in fbx_files:
            name = os.path.splitext(os.path.basename(fbx))[0]
            dest = f"/Game/AssetsImportados/Arboles/{species_dir}"
            import_fbx(fbx, dest, name)
    
    # Import rock meshes
    rock_dir = os.path.join(ASSETS_ROOT, "Naturaleza", "rocks")
    for rock_sub in os.listdir(rock_dir):
        rock_sub_path = os.path.join(rock_dir, rock_sub)
        if not os.path.isdir(rock_sub_path):
            continue
        fbx_files = glob.glob(os.path.join(rock_sub_path, "*.fbx"))
        for fbx in fbx_files:
            name = os.path.splitext(os.path.basename(fbx))[0]
            dest = f"/Game/AssetsImportados/Naturaleza/rocks/{rock_sub}"
            import_fbx(fbx, dest, name)
    
    # Import hedge meshes
    hedge_dir = os.path.join(ASSETS_ROOT, "Naturaleza", "Hedges")
    for hedge_sub in os.listdir(hedge_dir):
        hedge_sub_path = os.path.join(hedge_dir, hedge_sub)
        if not os.path.isdir(hedge_sub_path):
            continue
        fbx_files = glob.glob(os.path.join(hedge_sub_path, "*.fbx"))
        for fbx in fbx_files:
            name = os.path.splitext(os.path.basename(fbx))[0]
            dest = f"/Game/AssetsImportados/Naturaleza/Hedges/{hedge_sub}"
            import_fbx(fbx, dest, name)
    
    # Import grass meshes
    grass_dir = os.path.join(ASSETS_ROOT, "Naturaleza")
    for grass_sub in ["grass_02", "grass_07", "multi_stylized_grass"]:
        grass_path = os.path.join(grass_dir, grass_sub)
        if not os.path.isdir(grass_path):
            continue
        fbx_files = glob.glob(os.path.join(grass_path, "*.fbx"))
        for fbx in fbx_files:
            name = os.path.splitext(os.path.basename(fbx))[0]
            dest = f"/Game/AssetsImportados/Naturaleza/{grass_sub}"
            import_fbx(fbx, dest, name)
    
    # Import tiny weeds meshes
    for weeds_sub in ["tiny_weeds_2", "tiny_weeds_3"]:
        weeds_path = os.path.join(grass_dir, weeds_sub)
        if not os.path.isdir(weeds_path):
            continue
        fbx_files = glob.glob(os.path.join(weeds_path, "**", "*.fbx"), recursive=True)
        for fbx in fbx_files:
            name = os.path.splitext(os.path.basename(fbx))[0]
            dest = f"/Game/AssetsImportados/Naturaleza/{weeds_sub}"
            import_fbx(fbx, dest, name)
    
    # Import wall textures
    wall_dir = os.path.join(ASSETS_ROOT, "Muros")
    wall_textures = []
    for root, dirs, files in os.walk(wall_dir):
        for f in files:
            if f.lower().endswith(('.png', '.jpg')):
                wall_textures.append(os.path.join(root, f))
    for tex in wall_textures:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", tex)
        task.set_editor_property("destination_path", "/Game/AssetsImportados/Muros")
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    
    # Import pavement textures
    pavement_dir = os.path.join(ASSETS_ROOT, "Pavimentos")
    for root, dirs, files in os.walk(pavement_dir):
        for f in files:
            if f.lower().endswith(('.png', '.jpg')) and 'preview' not in f.lower():
                tex = os.path.join(root, f)
                task = unreal.AssetImportTask()
                task.set_editor_property("filename", tex)
                task.set_editor_property("destination_path", "/Game/AssetsImportados/Pavimentos")
                task.set_editor_property("save", True)
                task.set_editor_property("replace_existing", True)
                task.set_editor_property("automated", True)
                unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    
    unreal.log("=" * 60)
    unreal.log("IMPORTACION COMPLETADA")
    unreal.log("=" * 60)

if __name__ == "__main__":
    main()
