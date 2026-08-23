"""
Test: import 1 FBX to verify UE 5.8 Python API works.
Run in UE5 Output Log: exec(open(r'H:\Temp\opencode\AlsasuaUE5Clean\Tools\TestImport.py').read())
"""
import os
import unreal

# Pick the smallest FBX we found
TEST_FILE = r'H:\Temp\opencode\AlsasuaUE5Clean\Content\ImportedAssets\Vegetation\multi stylized grass\06_s.FBX'
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
