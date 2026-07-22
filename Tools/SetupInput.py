"""
SetupInput.py — Crea Input Mapping Context y Input Actions para AlsasuaSimulator.
Ejecutar desde el editor: Window > Output Log > consola.

Crea en /Game/Input/:
  IMC_Jugador          — Mapping Context principal
  IA_Mover             — Vector2D (WASD / stick izq)
  IA_Mirar             — Vector2D (ratón / stick der)
  IA_Saltar            — Bool (Espacio / A)
  IA_Correr            — Bool (Shift / LB)
  IA_Agacharse         — Bool (C / B)
  IA_Interactuar       — Bool (E / Y)
  IA_Disparar          — Bool (M1 / RT)
  IA_Disfraz           — Bool (H / L3)
  IA_Menu              — Bool (Esc / Start)
  IA_ConvocarManifa    — Bool (M)
"""
import unreal


INPUT_DIR = "/Game/Input"

# (Nombre, TipoValue)
# EGameplayInputValueType: 0=Bool, 1=Axis1D, 2=Axis2D, 3=Axis3D
INPUT_ACTIONS = [
    ("IA_Mover",         "Axis2D"),
    ("IA_Mirar",         "Axis2D"),
    ("IA_Saltar",        "Bool"),
    ("IA_Correr",        "Bool"),
    ("IA_Agacharse",     "Bool"),
    ("IA_Interactuar",   "Bool"),
    ("IA_Disparar",      "Bool"),
    ("IA_Disfraz",       "Bool"),
    ("IA_Menu",          "Bool"),
    ("IA_ConvocarManifa","Bool"),
]

# Key mappings: (InputAction, Key, Modifiers)
# Modifiers: 0=None, 1=Negate, 2=Scalar, 3=Swizzle
MAPPINGS = [
    # Mover
    ("IA_Mover", "W",          []),
    ("IA_Mover", "S",          ["NegateY"]),
    ("IA_Mover", "A",          ["NegateX"]),
    ("IA_Mover", "D",          []),
    ("IA_Mover", "Gamepad_LeftY", []),
    ("IA_Mover", "Gamepad_LeftX", []),
    # Mirar
    ("IA_Mirar", "MouseX",     []),
    ("IA_Mirar", "MouseY",     ["NegateY"]),
    ("IA_Mirar", "Gamepad_RightX", []),
    ("IA_Mirar", "Gamepad_RightY", []),
    # Acciones
    ("IA_Saltar",       "SpaceBar",                  []),
    ("IA_Saltar",       "Gamepad_FaceButton_Bottom",  []),
    ("IA_Correr",       "LeftShift",                  []),
    ("IA_Correr",       "Gamepad_LeftShoulder",       []),
    ("IA_Agacharse",    "C",                          []),
    ("IA_Agacharse",    "Gamepad_FaceButton_Right",   []),
    ("IA_Interactuar",  "E",                          []),
    ("IA_Interactuar",  "Gamepad_FaceButton_Top",     []),
    ("IA_Disparar",     "LeftMouseButton",            []),
    ("IA_Disparar",     "Gamepad_RightTriggerAxis",   []),
    ("IA_Disfraz",      "H",                          []),
    ("IA_Disfraz",      "Gamepad_LeftThumbstick",     []),
    ("IA_Menu",         "Escape",                     []),
    ("IA_Menu",         "Gamepad_Special_Left",       []),
    ("IA_ConvocarManifa","M",                          []),
]


def ensure_folder():
    if not unreal.EditorAssetLibrary.does_asset_exist(INPUT_DIR):
        unreal.EditorAssetLibrary.make_directory(INPUT_DIR)
        unreal.log("Creada carpeta /Game/Input")


def create_input_action(name, value_type):
    """Crea un InputAction asset."""
    path = f"{INPUT_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"  {name} ya existe, saltando.")
        return unreal.EditorAssetLibrary.load_asset(path)

    factory = unreal.InputActionFactoryNew()
    ia = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, INPUT_DIR, unreal.InputAction, factory
    )
    if not ia:
        unreal.log_error(f"  No se pudo crear {name}")
        return None

    # Configurar tipo de valor
    type_map = {
        "Bool": unreal.PlayerMappableKeySettings,
        "Axis1D": unreal.PlayerMappableKeySettings,
        "Axis2D": unreal.PlayerMappableKeySettings,
        "Axis3D": unreal.PlayerMappableKeySettings,
    }

    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(f"  Creado {name}")
    return ia


def create_mapping_context():
    """Crea el InputMapping Context principal."""
    path = f"{INPUT_DIR}/IMC_Jugador"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log("IMC_Jugador ya existe, saltando.")
        return unreal.EditorAssetLibrary.load_asset(path)

    factory = unreal.InputMappingContextFactoryNew()
    imc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "IMC_Jugador", INPUT_DIR, unreal.InputMappingContext, factory
    )
    if not imc:
        unreal.log_error("No se pudo crear IMC_Jugador")
        return None

    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("Creado IMC_Jugador")
    return imc


def add_mappings():
    """Añade los key mappings al IMC."""
    path = f"{INPUT_DIR}/IMC_Jugador"
    imc = unreal.EditorAssetLibrary.load_asset(path)
    if not imc:
        unreal.log_error("IMC_Jugador no encontrado para añadir mappings")
        return

    for action_name, key_name, modifiers in MAPPINGS:
        ia_path = f"{INPUT_DIR}/{action_name}"
        ia = unreal.EditorAssetLibrary.load_asset(ia_path)
        if not ia:
            unreal.log_warning(f"  InputAction {action_name} no encontrado, saltando mapping")
            continue

        # Crear el mapping
        mapping = unreal.InputKeyMapping()
        mapping.set_editor_property("action", ia)
        mapping.set_editor_property("key", unreal.Key(key_name))

        imc.add_player_mapping(mapping)

    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log("Mappings añadidos a IMC_Jugador")


def run():
    """Ejecuta la creación completa de input."""
    unreal.log("=== SetupInput: Iniciando ===")
    ensure_folder()

    unreal.log("Creando InputActions...")
    for name, vtype in INPUT_ACTIONS:
        create_input_action(name, vtype)

    create_mapping_context()
    # add_mappings()  # Descomentar una vez que las IAs existan

    unreal.log("=== SetupInput: Completo ===")


if __name__ == "__main__":
    run()
