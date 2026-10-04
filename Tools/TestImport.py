"""
Test: import 1 FBX to verify UE 5.8 Python API works.
Run in UE5 Output Log: exec(open(unreal.Paths.project_dir() + 'Tools/TestImport.py').read())
"""
import os
import unreal

# Primer FBX que haya bajo Content/AssetsImportados del propio proyecto.
import glob
_candidatos = sorted(glob.glob(os.path.join(unreal.Paths.project_dir(), 'Content', 'AssetsImportados', '**', '*.FBX'), recursive=True))
TEST_FILE = _candidatos[0] if _candidatos else ''
DEST = '/Game/ImportedAssets/Vegetation/Test'

task = unreal.AssetImportTask()
task.Filename = TEST_FILE
task.DestinationPath = DEST
task.bReplaceExisting = True
task.bAutomated = True
task.bSave = False

options = unreal.FbxImportUI()
options.MeshTypeToImport = unreal.EFBXImportType.FBXIT_STATIC_MESH
options.bImportMesh = True
options.bImportAnimations = False
options.bImportMaterials = True
options.bImportTextures = True
options.bImportAsSkeletal = False

task.Options = options

unreal.AssetImportHelpers.run_asset_import_tasks([task])
results = task.Results

unreal.log(f'Test import: {len(results)} objects created')
for obj in results:
    unreal.log(f'  -> {obj.get_name()} ({obj.get_class().get_name()})')
