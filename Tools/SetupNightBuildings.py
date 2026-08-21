"""
SetupNightBuildings.py — Configura edificios para ventanas emissivas nocturnas.
Crea material M_Edificio_Night y lo asigna a las ventanas de cada edificio.

Ejecutar en editor:  Tools > Execute Python Script
"""
import unreal

# La API de editor de 5.8 pasa por aquí: subsistemas en vez de las
# librerías obsoletas, y los nombres que no existían. Ver ue5_compat.py.
import sys as _sys, os as _os
_sys.path.append(_os.path.join(unreal.Paths.project_dir(), "Tools"))
import ue5_compat as compat

def create_night_building_material():
    """Material base para ventanas nocturnas con MPC NightEmissiveIntensity."""
    pkg = "/Game/Materials/M_Edificio_Night"
    if compat.assets().does_asset_exist(pkg):
        return

    # Ver la nota de SetupEnhancedMaterials.py: MPC_AlsasuaGlobal no existe.
    mpc_path = "/Game/Materiales/MPC_Clima"
    mpc = compat.assets().load_asset(mpc_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset("M_Edificio_Night", "/Game/Materials",
                                   unreal.Material, mat_factory)
    if not mat:
        return

    mat.set_editor_property("BlendMode", compat.OPACO)
    mat.set_editor_property("ShadingModel", compat.LIT)

    mat.recompile()
    unreal.log("[NightBuildings] M_Edificio_Night creado")


def assign_materials_to_buildings():
    """Selecciona todos los StaticMeshActors de edificio y asigna materiales nocturnos."""
    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label()
        if any(x in name.lower() for x in ["edificio", "building", "casa", "bloque"]):
            comps = actor.get_components_by_class(unreal.StaticMeshComponent)
            for comp in comps:
                num_mats = comp.get_num_materials()
                for i in range(num_mats):
                    mat = comp.get_material(i)
                    if mat and "fachada" in str(mat.get_name()).lower():
                        night_mat = compat.assets().load_asset(
                            "/Game/Materials/M_Edificio_Night")
                        if night_mat:
                            comp.set_material(i, night_mat)
                            count += 1

    unreal.log(f"[NightBuildings] Materiales asignados: {count}")


def add_emissive_components():
    """Añade UAlsasuaBuildingEmissiveComponent a cada edificio."""
    actors = compat.actores().get_all_level_actors_of_class(
        unreal.StaticMeshActor)
    count = 0
    for actor in actors:
        name = actor.get_actor_label()
        if any(x in name.lower() for x in ["edificio", "building", "casa", "bloque"]):
            emissive_class = unreal.load_class(
                "/Script/AlsasuaManifa.AlsasuaBuildingEmissiveComponent")
            if emissive_class:
                existing = actor.get_component_by_class(emissive_class)
                if not existing:
                    comp = compat.anadir_componente(actor, emissive_class)
                    if comp:
                        count += 1

    unreal.log(f"[NightBuildings] Componentes añadidos: {count}")


if __name__ == "__main__":
    unreal.log("=== Night Buildings Setup ===")
    create_night_building_material()
    assign_materials_to_buildings()
    add_emissive_components()
    unreal.log("=== Night Buildings Complete ===")
