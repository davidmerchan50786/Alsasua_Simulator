"""
ImportSatellite.py — Importa la imagen satelital real de Alsasua como textura.
Ejecutar desde el editor (consola Python): exec(open("Tools/ImportSatellite.py").read())
"""
import unreal

SRC = "F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Content/Terreno/alsasua_satelite.jpg"
DEST_FOLDER = "/Game/Terreno"
DEST_NAME = "T_Satelite_Alsasua"


def run():
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SRC)
    task.set_editor_property("destination_path", DEST_FOLDER)
    task.set_editor_property("destination_name", DEST_NAME)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex_path = f"{DEST_FOLDER}/{DEST_NAME}"
    tex = unreal.EditorAssetLibrary.load_asset(tex_path)
    if tex:
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        unreal.EditorAssetLibrary.save_asset(tex_path, False)
        unreal.log(f"Textura satelital importada: {tex_path}")
    else:
        unreal.log_error("No se pudo importar la textura satelital")

    # Forzar regeneración del material del terreno con el nuevo satélite
    mat_path = "/Game/Materiales/M_TerrenoAlsasua"
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        unreal.EditorAssetLibrary.delete_asset(mat_path)
        unreal.log("M_TerrenoAlsasua eliminado — se regenerará con satélite en el próximo Play")


if __name__ == "__main__":
    run()
