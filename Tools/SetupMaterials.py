"""
SetupMaterials.py — Crea materiales AAA para AlsasuaSimulator.
Ejecutar desde el editor: Window > Output Log > consola o desde Python Editor Utility.

Materiales creados:
  /Game/Materiales/MPC_Clima          — Material Parameter Collection (Wetness, Night)
  /Game/Materiales/M_Edificio         — Opaco vertex-color + mojado
  /Game/Materiales/M_Fachada          — Vertex-color + mojado + ventanas procedurales
  /Game/Materiales/M_AguaRio          — Traslúcido, reflectante, normal en paneo
  /Game/Materiales/M_Arbol            — Vertex-color con parámetro Color
  /Game/Materiales/M_Terreno_Orto     — Ortofoto en planta + verde neutro
  /Game/Materiales/M_Tejado_Orto      — Ortofoto en tejados

Requisitos: Content/Datos/ortofoto_unity.png importado como T_Ortofoto.
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

MATERIALS_DIR = "/Game/Materiales"
ORTO_TEXTURE = "/Game/Textures/T_Ortofoto"


def ensure_folder():
    """Crea la carpeta de materiales si no existe."""
    if not compat.assets().does_asset_exist(MATERIALS_DIR):
        compat.assets().make_directory(MATERIALS_DIR)
        unreal.log("Creada carpeta /Game/Materiales")


# La ÚNICA colección de parámetros del proyecto, con todo lo que alguien le
# escribe. Antes tenía cuatro escalares y el resto se escribía contra
# /Game/Materials/MPC_AlsasuaGlobal, que no lo crea nadie: cuatro sistemas de C++
# y tres scripts de editor alimentaban un null, y poner un parámetro que no
# existe sale por un warning y ya está. Charcos, ventanas de noche, auto-textura
# del terreno, viento y grano de película escribían al vacío.
#
# Debe cuadrar con AsegurarMPCClima() de CreadorMaterialEdificio.cpp, que hace lo
# mismo desde C++.
ESCALARES_MPC = [
    ("Wetness", 0.0), ("Night", 0.0), ("GlobalRain", 0.0), ("GlobalSnow", 0.0),
    ("GlobalWetness", 0.0), ("PuddleOpacity", 0.0), ("RainIntensity", 0.0),
    ("SnowAmount", 0.0), ("NormalizedTimeOfDay", 0.0), ("DayNightBlend", 0.0),
    ("NightEmissiveIntensity", 0.0), ("WindIntensity", 0.0), ("WindDirection", 0.0),
    ("FogDensityMult", 1.0), ("RoadWearAmount", 0.0),
    ("ChromaticAberration", 0.0), ("FilmGrain", 0.0),
    ("WPaintRadius", 0.0), ("WPaintIntensity", 0.0),
]
VECTORES_MPC = ["WindVector", "WPaintLocation"]


def create_mpc_clima():
    """Material Parameter Collection del clima. Idempotente: si ya existe, le
    completa los parámetros que le falten en vez de salirse."""
    name = f"{MATERIALS_DIR}/MPC_Clima"

    mpc = None
    if compat.assets().does_asset_exist(name):
        # Antes se salía aquí, así que un MPC creado en una versión anterior del
        # script se quedaba para siempre con los parámetros de aquel día. Es el
        # mismo fallo que dejó la lluvia en un Niagara vacío.
        mpc = compat.assets().load_asset(name)
        unreal.log("MPC_Clima ya existe: se le completan los parámetros que falten.")
    else:
        factory = unreal.MaterialParameterCollectionFactoryNew()
        mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MPC_Clima", MATERIALS_DIR, unreal.MaterialParameterCollection, factory
        )

    if not mpc:
        unreal.log_error("No se pudo crear MPC_Clima")
        return None

    ya = {p.get_editor_property("parameter_name")
          for p in (mpc.get_editor_property("scalar_parameters") or [])}
    for nombre, valor in ESCALARES_MPC:
        if str(nombre) not in {str(x) for x in ya}:
            mpc.add_scalar_parameter(nombre, valor)

    ya_v = {p.get_editor_property("parameter_name")
            for p in (mpc.get_editor_property("vector_parameters") or [])}
    for nombre in VECTORES_MPC:
        if str(nombre) not in {str(x) for x in ya_v}:
            mpc.add_vector_parameter(nombre, unreal.LinearColor(0.0, 0.0, 0.0, 0.0))

    compat.assets().save_asset(name)
    unreal.log("MPC_Clima al día (%d escalares, %d vectores)"
               % (len(ESCALARES_MPC), len(VECTORES_MPC)))
    return mpc


def create_material_edificio():
    """M_Edificio: opaco, vertex-color, mojado via MPC_Clima."""
    name = f"{MATERIALS_DIR}/M_Edificio"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_Edificio ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Edificio", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Edificio")
        return

    mat.set_editor_property("blend_mode", compat.OPACO)
    mat.set_editor_property("two_sided", False)

    compat.assets().save_asset(name)
    unreal.log("Creado M_Edificio (placeholder — ajustar en Material Editor)")


def create_material_fachada():
    """M_Fachada: vertex-color + mojado + ventanas procedurales."""
    name = f"{MATERIALS_DIR}/M_Fachada"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_Fachada ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Fachada", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Fachada")
        return

    mat.set_editor_property("blend_mode", compat.OPACO)
    mat.set_editor_property("two_sided", False)

    compat.assets().save_asset(name)
    unreal.log("Creado M_Fachada (placeholder — ventanas procedurales en Material Editor)")


def create_material_agua():
    """M_AguaRio: traslúcido, reflectante, oleaje."""
    name = f"{MATERIALS_DIR}/M_AguaRio"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_AguaRio ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_AguaRio", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_AguaRio")
        return

    mat.set_editor_property("blend_mode", compat.TRASLUCIDO)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("shading_model", compat.LIT)

    compat.assets().save_asset(name)
    unreal.log("Creado M_AguaRio (placeholder — ajustar reflejos en Material Editor)")


def create_material_arbol():
    """M_Arbol: vertex-color con parámetro vectorial Color."""
    name = f"{MATERIALS_DIR}/M_Arbol"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_Arbol ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Arbol", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Arbol")
        return

    mat.set_editor_property("blend_mode", compat.OPACO)
    mat.set_editor_property("two_sided", True)

    compat.assets().save_asset(name)
    unreal.log("Creado M_Arbol (placeholder — parámetro Color en Material Editor)")


def create_material_terreno_orto():
    """M_Terreno_Orto: ortofoto en planta + verde neutro."""
    name = f"{MATERIALS_DIR}/M_Terreno_Orto"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_Terreno_Orto ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Terreno_Orto", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Terreno_Orto")
        return

    mat.set_editor_property("blend_mode", compat.OPACO)
    mat.set_editor_property("two_sided", False)

    compat.assets().save_asset(name)
    unreal.log("Creado M_Terreno_Orto (placeholder — WorldPosition mapping en Material Editor)")


def create_material_tejado_orto():
    """M_Tejado_Orto: ortofoto proyectada en tejados."""
    name = f"{MATERIALS_DIR}/M_Tejado_Orto"
    if compat.assets().does_asset_exist(name):
        unreal.log("M_Tejado_Orto ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Tejado_orto", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Tejado_Orto")
        return

    mat.set_editor_property("blend_mode", compat.OPACO)

    compat.assets().save_asset(name)
    unreal.log("Creado M_Tejado_Orto (placeholder — WorldPosition mapping en Material Editor)")


def run():
    """Ejecuta toda la creación de materiales."""
    unreal.log("=== SetupMaterials: Iniciando ===")
    ensure_folder()
    create_mpc_clima()
    create_material_edificio()
    create_material_fachada()
    create_material_agua()
    create_material_arbol()
    create_material_terreno_orto()
    create_material_tejado_orto()
    unreal.log("=== SetupMaterials: Completo ===")


if __name__ == "__main__":
    run()
