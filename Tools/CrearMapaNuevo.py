"""
CrearMapaNuevo.py — Crea /Game/Maps/L_AlsasuaNuevo desde la plantilla «Basic».

L_Alsasua venía de la plantilla OpenWorld (World Partition), creado en 5.4, con
cuatro soles, cuatro cielos, cuatro nieblas y cuatro luces de cielo guardados.
Un nivel Basic trae UNA luz de cada, bien configurada; el pueblo no vive en el
mapa (lo genera DirectorArranque al empezar la partida), así que no se pierde
nada por empezar de cero.

Ejecutar en el editor: Tools > Execute Python Script > Tools/CrearMapaNuevo.py
Luego: Play, o lanzar con /Game/Maps/L_AlsasuaNuevo.
"""
import unreal

RUTA = "/Game/Maps/L_AlsasuaNuevo"
PLANTILLA = "/Engine/Maps/Templates/Template_Default"   # la plantilla «Basic»
GAMEMODE = "/Script/AlsasuaGameplay.AlsasuaGameplayGameMode"

niveles = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(RUTA):
    unreal.log("CrearMapaNuevo: %s ya existe, se abre." % RUTA)
    niveles.load_level(RUTA)
elif not niveles.new_level_from_template(RUTA, PLANTILLA):
    # Sin la plantilla, nivel vacío: UGuardianIluminacion pone el sol al jugar.
    unreal.log_warning("CrearMapaNuevo: plantilla Basic no disponible, nivel vacío.")
    niveles.new_level(RUTA)

mundo = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
gm = unreal.load_class(None, GAMEMODE)
if gm:
    mundo.get_world_settings().set_editor_property("default_game_mode", gm)
    unreal.log("CrearMapaNuevo: GameMode = AlsasuaGameplayGameMode")
else:
    unreal.log_error("CrearMapaNuevo: no carga %s (¿está compilado AlsasuaGameplay?)" % GAMEMODE)

niveles.save_current_level()
unreal.log("CrearMapaNuevo: listo -> %s. Dale a Play." % RUTA)
