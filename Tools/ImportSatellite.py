"""
ImportSatellite.py — Importa los ortomosaicos PNOA reales de Alsasua como texturas.
Ejecutar desde el editor (consola Python): exec(open("Tools/ImportSatellite.py").read())
"""
import unreal

FULL_SRC = "F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Content/Terreno/alsasua_satelite_pnoa_8192.png"
FULL_FOLDER = "/Game/Terreno"
FULL_NAME = "T_Satelite_Alsasua"

TOWN_SRC = "F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject/Content/Textures/ortofoto_pnoa_plaza_8192.png"
TOWN_FOLDER = "/Game/Textures"
TOWN_NAME = "T_Ortofoto"


def import_tex(src, folder, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", folder)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex_path = f"{folder}/{name}"
    tex = unreal.EditorAssetLibrary.load_asset(tex_path)
    if tex:
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        unreal.EditorAssetLibrary.save_asset(tex_path, False)
        unreal.log(f"Textura importada: {tex_path}")
        return True
    unreal.log_error(f"No se pudo importar la textura: {tex_path}")
    return False


def run():
    ok_full = import_tex(FULL_SRC, FULL_FOLDER, FULL_NAME)
    ok_town = import_tex(TOWN_SRC, TOWN_FOLDER, TOWN_NAME)

    # Regenerar los materiales que drapean las ortofotos (bounds UTM corregidos)
    ok_mat = True
    for cls, fn in [
        ("CreadorMaterialTerrenoOrto", "crear_material_terreno_orto"),
        ("CreadorMaterialTejadoOrto", "crear_material_tejado_orto"),
    ]:
        try:
            ok = getattr(getattr(unreal, cls), fn)()
            unreal.log(f"{cls}.{fn} -> {ok}")
        except Exception as e:
            unreal.log_error(f"{cls}.{fn} fallo: {e}")
            ok_mat = False

    # Forzar regeneración del material del terreno con el nuevo satélite
    mat_path = "/Game/Materiales/M_TerrenoAlsasua"
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        unreal.EditorAssetLibrary.delete_asset(mat_path)
        unreal.log("M_TerrenoAlsasua eliminado — se regenerará con satélite en el próximo Play")

    unreal.log(f"IMPORT_DONE full={ok_full} town={ok_town} mats={ok_mat}")
    unreal.SystemLibrary.execute_console_command(None, "quit")


if __name__ == "__main__":
    run()
