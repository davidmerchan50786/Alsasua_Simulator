r"""
ue5_compat.py — La API de editor de UE 5.8, en un sitio.

Por qué existe
--------------
Los scripts de Tools/ estaban escritos contra tres cosas distintas:

  1. Nombres que no existen en ninguna versión de la API de Python de Unreal
     (unreal.AssetHelpers, unreal.AssetImportHelpers, unreal.add_component_to_actor,
     unreal.MaterialBlendMode, unreal.ShadingModel, unreal.Rotation). Eso es un
     AttributeError en la primera línea que se ejecuta: el script no llegaba a
     hacer nada, y como cada paso de RunAll.py se traga sus errores, el pipeline
     decía "hecho" con la mitad sin hacer.
  2. unreal.EditorLevelLibrary y unreal.EditorAssetLibrary, que están obsoletas
     desde 5.0 en favor de los subsistemas de editor. Siguen respondiendo, pero
     son el camino que Epic va retirando.
  3. Lo correcto (unreal.AssetToolsHelpers, unreal.BlendMode...), en algún
     fichero suelto.

Aquí se centraliza el camino de 5.8 —los subsistemas— con vuelta atrás a la
librería obsoleta si el subsistema no estuviera. Así el día que Epic retire
EditorLevelLibrary se toca este fichero y no veinte.

Uso desde cualquier script de Tools/:

    import sys, os, unreal
    sys.path.append(os.path.join(unreal.Paths.project_dir(), "Tools"))
    import ue5_compat as compat

    compat.assets().save_asset(ruta)
    actor = compat.actores().spawn_actor_from_class(clase, pos)

Tools/ComprobarAPI58.py comprueba que no queda ningún nombre de los del punto 1
ni ninguna llamada directa a las librerías obsoletas, y se ejecuta sin motor.
"""
import unreal


def _subsistema(nombre, respaldo=None):
    """Subsistema de editor por nombre, o el respaldo si no está disponible."""
    clase = getattr(unreal, nombre, None)
    if clase is not None:
        try:
            sub = unreal.get_editor_subsystem(clase)
            if sub:
                return sub
        except Exception as e:      # pragma: no cover - depende del editor
            unreal.log_warning(f"[compat] {nombre} no disponible: {e}")
    if respaldo is not None:
        return getattr(unreal, respaldo, None)
    return None


# ── Assets ──────────────────────────────────────────────────────────────────

def assets():
    """EditorAssetSubsystem (5.8) o EditorAssetLibrary (obsoleta).

    Mismos nombres de método en ambos: does_asset_exist, load_asset,
    save_asset, delete_asset, make_directory, list_assets,
    does_directory_exist.
    """
    return _subsistema("EditorAssetSubsystem", "EditorAssetLibrary")


def crear_asset(nombre, ruta_paquete, clase, factory):
    """Crea un asset. unreal.AssetHelpers.create_asset no existe."""
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name=nombre, package_path=ruta_paquete,
        asset_class=clase, factory=factory)


def importar_tareas(tareas):
    """Ejecuta AssetImportTask. unreal.AssetImportHelpers no existe."""
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tareas)
    creados = []
    for t in tareas:
        creados.extend(t.get_editor_property("imported_object_paths") or [])
    return creados


def cargar_clase(ruta):
    """
    Clase (UClass) desde su ruta. EditorAssetLibrary.load_class no existe.

    Una clase nativa se pide tal cual (/Script/Modulo.Clase); la de un
    Blueprint lleva el sufijo _C, que se añade aquí para no tener que
    recordarlo en cada llamada.
    """
    if not ruta.startswith("/Script/") and not ruta.endswith("_C"):
        ruta = ruta + "_C"
    try:
        return unreal.load_class(None, ruta)
    except Exception as e:
        unreal.log_warning(f"[compat] no pude cargar la clase {ruta}: {e}")
        return None


# ── Actores y nivel ─────────────────────────────────────────────────────────

def actores():
    """EditorActorSubsystem (5.8) o EditorLevelLibrary (obsoleta).

    Mismos nombres: spawn_actor_from_class, spawn_actor_from_object,
    get_all_level_actors, get_all_level_actors_of_class, destroy_actor.
    """
    return _subsistema("EditorActorSubsystem", "EditorLevelLibrary")


def nivel():
    """LevelEditorSubsystem (5.8) o EditorLevelLibrary (obsoleta).

    Mismos nombres: new_level, load_level, save_current_level.
    """
    return _subsistema("LevelEditorSubsystem", "EditorLevelLibrary")


def mundo():
    """El mundo del editor."""
    sub = _subsistema("UnrealEditorSubsystem")
    if sub is not None:
        return sub.get_editor_world()
    return unreal.EditorLevelLibrary.get_editor_world()


def ajustes_mundo():
    """
    AWorldSettings del nivel abierto.

    EditorLevelLibrary.get_world_settings no existe: se saca del mundo, que es
    de donde sale de verdad.
    """
    w = mundo()
    return w.get_world_settings() if w else None


def anadir_componente(actor, clase, transform=None):
    """
    Añade un componente a un actor. unreal.add_component_to_actor no existe;
    lo que hay es AActor::AddComponentByClass, expuesto en el actor.
    """
    if not actor or not clase:
        return None
    return actor.add_component_by_class(
        clase, False, transform if transform else unreal.Transform(), False)


# ── Enumeraciones que se escribían con el nombre equivocado ─────────────────
# EBlendMode se expone como unreal.BlendMode (no MaterialBlendMode) y sus
# valores son BLEND_*, no BM_*. EMaterialShadingModel es
# unreal.MaterialShadingModel (no ShadingModel).

OPACO       = unreal.BlendMode.BLEND_OPAQUE
ENMASCARADO = unreal.BlendMode.BLEND_MASKED
TRASLUCIDO  = unreal.BlendMode.BLEND_TRANSLUCENT
ADITIVO     = unreal.BlendMode.BLEND_ADDITIVE

LIT         = unreal.MaterialShadingModel.MSM_DEFAULT_LIT
UNLIT       = unreal.MaterialShadingModel.MSM_UNLIT
SUBSURFACE  = unreal.MaterialShadingModel.MSM_SUBSURFACE
