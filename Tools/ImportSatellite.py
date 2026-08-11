"""
ImportSatellite.py — Importa los ortomosaicos PNOA reales de Alsasua como texturas.
Ejecutar desde el editor (consola Python): exec(open("Tools/ImportSatellite.py").read())
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

FULL_SRC = unreal.Paths.project_content_dir() + "Terreno/alsasua_satelite_pnoa_8192.png"
FULL_FOLDER = "/Game/Terreno"
FULL_NAME = "T_Satelite_Alsasua"

TOWN_SRC = unreal.Paths.project_content_dir() + "Textures/ortofoto_pnoa_plaza_8192.png"
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
    tex = compat.assets().load_asset(tex_path)
    if tex:
        tex.set_editor_property("srgb", True)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
        compat.assets().save_asset(tex_path, False)
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
    if compat.assets().does_asset_exist(mat_path):
        compat.assets().delete_asset(mat_path)
        unreal.log("M_TerrenoAlsasua eliminado — se regenerará con satélite en el próximo Play")

    unreal.log(f"IMPORT_DONE full={ok_full} town={ok_town} mats={ok_mat}")
    unreal.SystemLibrary.execute_console_command(None, "quit")


if __name__ == "__main__":
    run()
