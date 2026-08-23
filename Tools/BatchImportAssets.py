"""
Batch FBX importer for Alsasua Simulator.
Run from UE5 Python console: exec(open(r'H:\Temp\opencode\AlsasuaUE5Clean\Tools\BatchImportAssets.py').read())
"""
import os
import unreal

BASE_DIR = r'H:\Temp\opencode\AlsasuaUE5Clean\Content\ImportedAssets'
DEST_MAP = {
    'Characters': '/Game/ImportedAssets/Characters',
    'Vegetation': '/Game/ImportedAssets/Vegetation',
    'Buildings': '/Game/ImportedAssets/Buildings',
    'Props': '/Game/ImportedAssets/Props',
    'Animations': '/Game/ImportedAssets/Animations',
}

def find_fbx_files(base_dir):
    results = []
    for root, dirs, files in os.walk(base_dir):
        for f in files:
            if f.lower().endswith('.fbx'):
                results.append(os.path.join(root, f))
    return results

def get_category(fbx_path, base_dir):
    rel = os.path.relpath(fbx_path, base_dir)
    top = rel.split(os.sep)[0]
    return top

def import_fbx(fbx_path, dest_path):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', fbx_path)
    task.set_editor_property('destination_path', dest_path)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    task.set_editor_property('product_sub_directory', '')

    options = unreal.FbxImportUI()
    
    fbx_name = os.path.splitext(os.path.basename(fbx_path))[0].lower()
    is_anim = 'walk' in fbx_name or 'run' in fbx_name or 'sprint' in fbx_name or 'idle' in fbx_name or 'transition' in fbx_name or 'traversal' in fbx_name
    
    if is_anim:
        options.set_editor_property('import_mesh', False)
        options.set_editor_property('import_animations', True)
        anim_options = unreal.FbxAnimSequenceImportData()
        anim_options.set_editor_property('import_content_type', unreal.FBXImportContentType.FBXICT_ANIMATION)
        options.set_editor_property('anim_sequence_import_data', anim_options)
    else:
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_animations', False)
        
        mesh_options = unreal.FbxMeshImportData()
        mesh_options.set_editor_property('import_content_type', unreal.FBXImportContentType.FBXICT_GEOMETRY)
        options.set_editor_property('mesh_import_data', mesh_options)

    options.set_editor_property('import_materials', True)
    options.set_editor_property('import_textures', True)
    options.set_editor_property('import_as_skeletal', True)
    
    task.set_editor_property('options', options)
    
    unreal.AssetImportHelpers.run_asset_import_tasks([task])
    return task.get_editor_property('result')

def main():
    fbx_files = find_fbx_files(BASE_DIR)
    total = len(fbx_files)
    unreal.log(f'Found {total} FBX files to import')
    
    imported = 0
    failed = 0
    
    for i, fbx_path in enumerate(fbx_files):
        category = get_category(fbx_path, BASE_DIR)
        dest = DEST_MAP.get(category, '/Game/ImportedAssets/Misc')
        
        result = import_fbx(fbx_path, dest)
        if result:
            imported += 1
        else:
            failed += 1
            unreal.log_warning(f'Failed: {fbx_path}')
        
        if (i + 1) % 50 == 0:
            unreal.log(f'Progress: {i + 1}/{total} (imported: {imported}, failed: {failed})')
    
    unreal.log(f'Done! Imported: {imported}, Failed: {failed}, Total: {total}')

main()
