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

MATERIALS_DIR = "/Game/Materiales"
ORTO_TEXTURE = "/Game/Textures/T_Ortofoto"


def ensure_folder():
    """Crea la carpeta de materiales si no existe."""
    if not unreal.EditorAssetLibrary.does_asset_exist(MATERIALS_DIR):
        unreal.EditorAssetLibrary.make_directory(MATERIALS_DIR)
        unreal.log("Creada carpeta /Game/Materiales")


def create_mpc_clima():
    """Material Parameter Collection: Wetness (scalar) + Night (scalar)."""
    name = f"{MATERIALS_DIR}/MPC_Clima"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log(f"MPC_Clima ya existe, saltando.")
        return unreal.EditorAssetLibrary.load_asset(name)

    factory = unreal.MaterialParameterCollectionFactoryNew()
    mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MPC_Clima", MATERIALS_DIR, unreal.MaterialParameterCollection, factory
    )
    if not mpc:
        unreal.log_error("No se pudo crear MPC_Clima")
        return None

    # Añadir escalares
    mpc.add_scalar_parameter("Wetness", 0.0)
    mpc.add_scalar_parameter("Night", 0.0)
    mpc.add_scalar_parameter("GlobalRain", 0.0)
    mpc.add_scalar_parameter("GlobalSnow", 0.0)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado MPC_Clima")
    return mpc


def create_material_edificio():
    """M_Edificio: opaco, vertex-color, mojado via MPC_Clima."""
    name = f"{MATERIALS_DIR}/M_Edificio"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_Edificio ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Edificio", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Edificio")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_OPAQUE)
    mat.set_editor_property("two_sided", False)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado M_Edificio (placeholder — ajustar en Material Editor)")


def create_material_fachada():
    """M_Fachada: vertex-color + mojado + ventanas procedurales."""
    name = f"{MATERIALS_DIR}/M_Fachada"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_Fachada ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Fachada", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Fachada")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_OPAQUE)
    mat.set_editor_property("two_sided", False)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado M_Fachada (placeholder — ventanas procedurales en Material Editor)")


def create_material_agua():
    """M_AguaRio: traslúcido, reflectante, oleaje."""
    name = f"{MATERIALS_DIR}/M_AguaRio"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_AguaRio ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_AguaRio", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_AguaRio")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado M_AguaRio (placeholder — ajustar reflejos en Material Editor)")


def create_material_arbol():
    """M_Arbol: vertex-color con parámetro vectorial Color."""
    name = f"{MATERIALS_DIR}/M_Arbol"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_Arbol ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Arbol", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Arbol")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_OPAQUE)
    mat.set_editor_property("two_sided", True)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado M_Arbol (placeholder — parámetro Color en Material Editor)")


def create_material_terreno_orto():
    """M_Terreno_Orto: ortofoto en planta + verde neutro."""
    name = f"{MATERIALS_DIR}/M_Terreno_Orto"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_Terreno_Orto ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Terreno_Orto", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Terreno_Orto")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_OPAQUE)
    mat.set_editor_property("two_sided", False)

    unreal.EditorAssetLibrary.save_asset(name)
    unreal.log("Creado M_Terreno_Orto (placeholder — WorldPosition mapping en Material Editor)")


def create_material_tejado_orto():
    """M_Tejado_Orto: ortofoto proyectada en tejados."""
    name = f"{MATERIALS_DIR}/M_Tejado_Orto"
    if unreal.EditorAssetLibrary.does_asset_exist(name):
        unreal.log("M_Tejado_Orto ya existe, saltando.")
        return

    factory = unreal.MaterialFactoryNew()
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Tejado_orto", MATERIALS_DIR, unreal.Material, factory
    )
    if not mat:
        unreal.log_error("No se pudo crear M_Tejado_Orto")
        return

    mat.set_editor_property("blend_mode", unreal.BlendMode.BM_OPAQUE)

    unreal.EditorAssetLibrary.save_asset(name)
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
