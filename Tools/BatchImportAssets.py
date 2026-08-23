"""
Batch FBX importer for Alsasua Simulator (UE 5.8 — correct API).
Run in UE5 Output Log: exec(open(r'H:\Temp\opencode\AlsasuaUE5Clean\Tools\BatchImportAssets.py').read())
"""
import os
import unreal

BASE_DIR = r'H:\Temp\opencode\AlsasuaUE5Clean\Content\ImportedAssets'

# Destination mapping — where each category lives in-game
DEST_MAP = {
    'Characters':  '/Game/ImportedAssets/Characters',
    'Vegetation':  '/Game/ImportedAssets/Vegetation',
    'Buildings':   '/Game/ImportedAssets/Buildings',
    'Props':       '/Game/ImportedAssets/Props',
    'Animations':  '/Game/ImportedAssets/Animations',
}

def find_fbx(base_dir):
    results = []
    for root, dirs, files in os.walk(base_dir):
        for f in files:
            if f.lower().endswith('.fbx'):
                results.append(os.path.join(root, f))
    return sorted(results)

def get_category(fbx_path, base_dir):
    rel = os.path.relpath(fbx_path, base_dir)
    return rel.split(os.sep)[0]

def is_animation_file(name):
    name = name.lower()
    return any(kw in name for kw in ['walk', 'run', 'sprint', 'idle', 'transition', 'traversal', 'anim'])

def is_character_file(name):
    name = name.lower()
    return any(kw in name for kw in ['character', 'meshy', 'woman', 'girl', 'guard', 'ninja', 'biker',
                                       'punk', 'elegance', 'rebel', 'sentinel', 'wanderer', 'doll',
                                       'cat', 'pug', 'ratbeast', 'portrait', 'power_pose', 'ladder',
                                       'hoodie', 'denim_portrait', 'crossed_arms', 'open_chest',
                                       'monastic', 'overgrown', 'cage', 'holy', 'corporate',
                                       'cozy', 'rooftop', 'midnight', 'crimson', 'neon', 'rosy',
                                       'striped', 'executive', 'goofy', 'beige', 'azure', 'casual',
                                       'boho', 'leather', 'painter', 'patchwork', 'street_muse'])

def is_static_asset(category, name):
    if category in ('Buildings', 'Props', 'Vegetation'):
        return True
    if category == 'Characters' and not is_animation_file(name) and not is_character_file(name):
        return True  # animals, etc.
    return False

def import_fbx(fbx_path, dest_path):
    task = unreal.AssetImportTask()
    task.Filename = fbx_path
    task.DestinationPath = dest_path
    task.bReplaceExisting = True
    task.bAutomated = True
    task.bSave = False

    options = unreal.FbxImportUI()
    fname = os.path.basename(fbx_path)

    if is_animation_file(fname):
        options.MeshTypeToImport = unreal.EFBXImportType.FBXIT_ANIMATION
        options.bImportMesh = False
        options.bImportAnimations = True
        options.bImportMaterials = False
        options.bImportTextures = False
    elif is_static_asset(get_category(fbx_path, BASE_DIR), fname):
        options.MeshTypeToImport = unreal.EFBXImportType.FBXIT_STATIC_MESH
        options.bImportMesh = True
        options.bImportAnimations = False
        options.bImportMaterials = True
        options.bImportTextures = True
        options.bImportAsSkeletal = False
    else:
        options.MeshTypeToImport = unreal.EFBXImportType.FBXIT_SKELETAL_MESH
        options.bImportMesh = True
        options.bImportAnimations = False
        options.bImportMaterials = True
        options.bImportTextures = True
        options.bImportAsSkeletal = True

    task.Options = options

    unreal.AssetImportHelpers.run_asset_import_tasks([task])
    return task.Results

def main():
    fbx_files = find_fbx(BASE_DIR)
    total = len(fbx_files)
    unreal.log(f'=== FBX Batch Import: {total} files ===')

    ok = 0
    fail = 0

    for i, fbx_path in enumerate(fbx_files):
        cat = get_category(fbx_path, BASE_DIR)
        dest = DEST_MAP.get(cat, '/Game/ImportedAssets/Misc')

        try:
            results = import_fbx(fbx_path, dest)
            if results:
                ok += 1
            else:
                fail += 1
                unreal.log_warning(f'EMPTY: {os.path.basename(fbx_path)}')
        except Exception as e:
            fail += 1
            unreal.log_warning(f'ERROR: {os.path.basename(fbx_path)}: {e}')

        if (i + 1) % 25 == 0:
            unreal.log(f'[{i+1}/{total}] OK={ok} FAIL={fail}')

    unreal.log(f'=== Done: {ok}/{total} imported, {fail} failed ===')

main()
