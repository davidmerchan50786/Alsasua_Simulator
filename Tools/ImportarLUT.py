"""Importa los LUT de gradacion de color a /Game/LUTs.

AlsasuaBarrioStyleSystem pide /Game/LUTs/LUT_Neutral una vez por edificio: 1030
LoadObject fallidos por arranque mientras la carpeta no exista, y el pueblo sin
gradacion de color por barrio.

El PNG lo genera Tools/GenerateMatchedLUT.py. Este script hace el paso que
faltaba: meterlo en el proyecto como Texture2D con los ajustes que exige un
LUT de color grading —sin sRGB, sin mips, sin compresion— porque un LUT no es
una textura decorativa: cada texel es una coordenada de color y comprimirlo o
aplicarle gamma lo destroza.

Uso, en la consola Python del editor:
    exec(open(r'<raiz>\\Tools\\ImportarLUT.py').read())
"""
import os
import unreal

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ORIGEN = os.path.join(RAIZ, "Content", "LUTs")
DESTINO = "/Game/LUTs"


def main():
    if not os.path.isdir(ORIGEN):
        unreal.log_error("[LUT] no existe %s (corre antes GenerateMatchedLUT.py)" % ORIGEN)
        return

    pngs = sorted(f for f in os.listdir(ORIGEN) if f.lower().endswith(".png"))
    if not pngs:
        unreal.log_warning("[LUT] no hay PNG en %s" % ORIGEN)
        return

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    hechos = 0
    for f in pngs:
        datos = unreal.AutomatedAssetImportData()
        datos.set_editor_property("destination_path", DESTINO)
        datos.set_editor_property("filenames", [os.path.join(ORIGEN, f)])
        datos.set_editor_property("replace_existing", True)

        creados = tools.import_assets_automated(datos)
        if not creados:
            unreal.log_error("[LUT] %s: no se importo" % f)
            continue

        tex = creados[0]
        # Ajustes de LUT. Sin esto el color grading sale mal aunque la textura
        # exista: sRGB aplica gamma a lo que son coordenadas, los mips mezclan
        # celdas vecinas del cubo de color y la compresion inventa valores.
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP)
        tex.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
        # El grupo NO es decorativo: ColorLookupTable exime a la textura del
        # sesgo de mip y del streaming. En cualquier otro grupo el LUT se
        # reduce como una textura normal, las celdas del cubo de color se
        # mezclan entre si y el resultado es un tinte raro en toda la pantalla
        # —que es exactamente lo que pasaba al importarlo sin esto—.
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_COLOR_LOOKUP_TABLE)
        tex.set_editor_property("never_stream", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex, False)

        hechos += 1
        unreal.log("[LUT]   %s -> %s/%s" % (f, DESTINO, os.path.splitext(f)[0]))

    unreal.log("[LUT] importados %d de %d" % (hechos, len(pngs)))


main()
