r"""
AdaptarBiblioteca58.py — Deja la biblioteca importada en condiciones de 5.8.

Qué arregla
-----------
El proyecto declara Nanite activo y lo dice en el log, pero
UAlsasuaLODConfigComponent::ApplyGlobalNaniteSettings sólo toca CVars
(r.Nanite.MaxPixelsPerEdge, r.Nanite.ProxyTriangleThreshold). Nanite se activa
por malla, en sus build settings, y ninguna de las mallas importadas lo trae:
Village, ForestPack, MeshyAI y lo bajado de Fab vienen de packs anteriores.
Resultado: Nanite "ON" en el log y ni un triángulo pasando por Nanite.

Esto recorre las mismas carpetas que mira AlsasuaMallaFab y activa Nanite en
las mallas estáticas que lo aprovechan.

Qué NO toca, a propósito
------------------------
- Mallas por debajo de MIN_TRIANGULOS: un banco de 40 triángulos no gana nada
  con Nanite y sí añade datos al paquete.
- Colisión, LOD group, materiales y texturas: eso ya lo fija el importador o
  los sistemas del proyecto, y cambiarlo aquí a ciegas rompe lo que funciona.
- /Game/Meshes (las mallas procedurales del propio proyecto): se generan con
  el detalle justo y no son biblioteca.

Ejecutar en el editor: Tools > Execute Python Script, o desde RunAll.py.
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

# Las mismas raíces que RaicesFab en AlsasuaMallaFab.cpp, para que lo que el
# juego puede llegar a usar sea exactamente lo que se adapta aquí.
RAICES = [
    "/Game/AssetsImportados",
    "/Game/ModelosDescargados",
    "/Game/Megascans",
    "/Game/MSPresets",
    "/Game/Fab",
]

# Por debajo de esto Nanite no compensa.
MIN_TRIANGULOS = 1000


def _triangulos(malla):
    """Triángulos del LOD 0, o -1 si la API no lo da."""
    try:
        return malla.get_num_triangles(0)
    except Exception:
        return -1


def _activar_nanite(malla):
    """True si quedó activado (o ya lo estaba)."""
    sub = None
    clase = getattr(unreal, "StaticMeshEditorSubsystem", None)
    if clase is not None:
        try:
            sub = unreal.get_editor_subsystem(clase)
        except Exception:
            sub = None

    try:
        ajustes = malla.get_editor_property("nanite_settings")
        if ajustes.get_editor_property("enabled"):
            return True
        ajustes.set_editor_property("enabled", True)

        # El subsistema es el camino de 5.x y además reconstruye la malla;
        # set_editor_property a secas deja los datos de Nanite sin generar.
        if sub is not None:
            sub.set_nanite_settings(malla, ajustes, apply_changes=True)
        else:
            malla.set_editor_property("nanite_settings", ajustes)
        return True
    except Exception as e:
        unreal.log_warning(f"[Biblioteca] {malla.get_name()}: {e}")
        return False


def run():
    activadas = 0
    ya_estaban = 0
    pequenas = 0
    total = 0

    for raiz in RAICES:
        if not compat.assets().does_directory_exist(raiz):
            continue

        for ruta in compat.assets().list_assets(raiz, recursive=True):
            activo = compat.assets().load_asset(ruta)
            if not isinstance(activo, unreal.StaticMesh):
                continue
            total += 1

            tris = _triangulos(activo)
            if 0 <= tris < MIN_TRIANGULOS:
                pequenas += 1
                continue

            antes = False
            try:
                antes = activo.get_editor_property("nanite_settings").get_editor_property("enabled")
            except Exception:
                pass

            if antes:
                ya_estaban += 1
                continue

            if _activar_nanite(activo):
                compat.assets().save_asset(ruta)
                activadas += 1

    unreal.log(f"[Biblioteca] {total} mallas estáticas en la biblioteca")
    unreal.log(f"[Biblioteca] Nanite activado en {activadas}, ya lo tenían {ya_estaban}, "
               f"{pequenas} por debajo de {MIN_TRIANGULOS} triángulos")
    if total == 0:
        unreal.log_warning("[Biblioteca] no hay nada importado todavía: "
                           "pasa antes Tools/ue5_import_all_assets.py")
    return total > 0


if __name__ == "__main__":
    run()
