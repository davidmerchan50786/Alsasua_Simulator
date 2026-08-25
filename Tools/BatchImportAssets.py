"""
Batch FBX importer for Alsasua Simulator (UE 5.8).
Run in UE5 Output Log: exec(open(r'H:\Temp\opencode\AlsasuaUE5Clean\Tools\BatchImportAssets.py').read())
"""
import os
import unreal

# Source directories (real FBX files)
SOURCE_DIRS = [
    r'H:\UnrealProjects\AlsasuaSimulator\Content\AssetsImportados',
    r'J:\assets',
]

# Destination mapping — where each category lives in-game
DEST_MAP = {
    'Characters':    '/Game/ImportedAssets/Characters',
    'MeshyAI':       '/Game/ImportedAssets/Buildings',  # Navarra-specific buildings/props
    'Casas':         '/Game/ImportedAssets/Buildings',
    'ExtractedUnity': '/Game/ImportedAssets/Buildings',
    'Naturaleza':    '/Game/ImportedAssets/Vegetation',
    'Animals':       '/Game/ImportedAssets/Vegetation',
    'Bushes':        '/Game/ImportedAssets/Vegetation',
    'ForestPack':    '/Game/ImportedAssets/Vegetation',
    'Hedges':        '/Game/ImportedAssets/Vegetation',
    'Mundo':         '/Game/ImportedAssets/Vegetation',
    'SmallArms':     '/Game/ImportedAssets/Props',
    'Farolas':       '/Game/ImportedAssets/Props',
    'Fleurs':        '/Game/ImportedAssets/Props',
    'Pavimentos':    '/Game/ImportedAssets/Props',
    'Containers':    '/Game/ImportedAssets/Props',
    'Locomotive':    '/Game/ImportedAssets/Props',
    'Shrooms':       '/Game/ImportedAssets/Props',
    'DestroyedWalls':'/Game/ImportedAssets/Props',
    'Cypress':       '/Game/ImportedAssets/Vegetation',
    'Bamboo':        '/Game/ImportedAssets/Vegetation',
    'FPSWeapons':    '/Game/ImportedAssets/Props',
    'VillagePack':   '/Game/ImportedAssets/Buildings',
    'VillageHouses': '/Game/ImportedAssets/Buildings',
    'HousePack':     '/Game/ImportedAssets/Buildings',
    'Standard Assets': '/Game/ImportedAssets/Props',
    'SpaceZeta_StreetLamps2': '/Game/ImportedAssets/Props',
    'YughuesFreeBushes2018': '/Game/ImportedAssets/Vegetation',
    'Pure Poly':     '/Game/ImportedAssets/Vegetation',
    'Animals':       '/Game/ImportedAssets/Vegetation',
    'Arboles':       '/Game/ImportedAssets/Vegetation',
    'ArbolesExtracted': '/Game/ImportedAssets/Vegetation',
    'CharactersExtracted': '/Game/ImportedAssets/Characters',
    'CasasExtracted': '/Game/ImportedAssets/Buildings',
    'PropsExtracted': '/Game/ImportedAssets/Props',
    'TerrainExtracted': '/Game/ImportedAssets/Props',
    'TownsExtracted': '/Game/ImportedAssets/Buildings',
    'VehiclesExtracted': '/Game/ImportedAssets/Props',
    'WeaponsExtracted': '/Game/ImportedAssets/Props',
}

DEFAULT_DEST = '/Game/ImportedAssets/Misc'

def find_fbx(directories):
    results = []
    for base_dir in directories:
        if not os.path.isdir(base_dir):
            continue
        for root, dirs, files in os.walk(base_dir):
            # Skip subdirs we don't want
            skip = False
            for part in root.split(os.sep):
                if part.lower() in ('nueva carpeta', 'disco pipero', 'ventoy'):
                    skip = True
                    break
            if skip:
                continue
            for f in files:
                if f.lower().endswith('.fbx'):
                    results.append(os.path.join(root, f))
    return sorted(results)

def get_category(fbx_path):
    """Get the most specific category from the path."""
    parts = fbx_path.replace('\\', '/').split('/')
    # Find the first meaningful dir after known roots
    for i, p in enumerate(parts):
        if p in ('AssetsImportados', 'assets', 'Content'):
            if i + 1 < len(parts):
                return parts[i + 1]
    return parts[-2] if len(parts) > 1 else 'Misc'

def is_animation_file(name):
    name_lower = name.lower()
    return any(kw in name_lower for kw in ['walk', 'run', 'sprint', 'idle', 'transition', 'traversal', 'anim'])

def is_character_file(name):
    name_lower = name.lower()
    return any(kw in name_lower for kw in ['character', 'meshy', 'woman', 'girl', 'guard', 'ninja', 'biker',
                                        'punk', 'elegance', 'rebel', 'sentinel', 'wanderer', 'doll',
                                        'cat', 'pug', 'ratbeast', 'portrait', 'power_pose', 'ladder',
                                        'hoodie', 'denim_portrait', 'crossed_arms', 'open_chest',
                                        'monastic', 'overgrown', 'cage', 'holy', 'corporate',
                                        'cozy', 'rooftop', 'midnight', 'crimson', 'neon', 'rosy',
                                        'striped', 'executive', 'goofy', 'beige', 'azure', 'casual',
                                        'boho', 'leather', 'painter', 'patchwork', 'street_muse',
                                        'x bot'])

def is_static_asset(category, name):
    if category in ('MeshyAI', 'Casas', 'VillagePack', 'VillageHouses', 'HousePack',
                    'ExtractedUnity', 'Props', 'Buildings', 'Farolas', 'SmallArms',
                    'Containers', 'Locomotive', 'Pavimentos', 'DestroyedWalls', 'FPSWeapons',
                    'SpaceZeta_StreetLamps2', 'Standard Assets', 'Shrooms'):
        return True
    if category in ('Naturaleza', 'Animals', 'Bushes', 'ForestPack', 'Hedges', 'Mundo',
                    'Cypress', 'Bamboo', 'YughuesFreeBushes2018', 'Pure Poly', 'Vegetation',
                    'Arboles', 'ArbolesExtracted'):
        return True
    if category == 'Characters' and not is_animation_file(name) and not is_character_file(name):
        return True
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
        options.bImportMaterials = True
        options.bImportTextures = True
    elif is_static_asset(get_category(fbx_path), fname):
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
    fbx_files = find_fbx(SOURCE_DIRS)
    total = len(fbx_files)
    unreal.log(f'=== FBX Batch Import: {total} files from {len(SOURCE_DIRS)} sources ===')

    ok = 0
    fail = 0

    for i, fbx_path in enumerate(fbx_files):
        cat = get_category(fbx_path)
        dest = DEST_MAP.get(cat, DEFAULT_DEST)

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
