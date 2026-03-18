// Assets/Editor/InicializadorAlsasua.cs
// Se ejecuta automáticamente cada vez que Unity compila o abre el proyecto.
// Crea el AnimatorController y configura los personajes Kenney sin requerir
// ninguna acción manual del usuario.
//
// Para forzar una reinicialización: menú Alsasua → Reinicializar Todo

#if UNITY_EDITOR

using UnityEngine;
using UnityEditor;

[InitializeOnLoad]
public static class InicializadorAlsasua
{
    private const string PREF_CONTROLLER = "Alsasua_ControllerCreado_v2";
    private const string PREF_KENNEY     = "Alsasua_KenneyConfigurado_v2";
    private const string RUTA_CONTROLLER = "Assets/Personajes/AnimatorMixamo.controller";
    private const string RUTA_MODELO     = "Assets/Personajes/Modelos/PersonajeBase.fbx";

    // Constructor estático → se llama al cargar el dominio de scripts
    static InicializadorAlsasua()
    {
        // Diferimos la ejecución al siguiente frame para que AssetDatabase esté listo
        EditorApplication.delayCall += AutoConfigurar;
    }

    private static void AutoConfigurar()
    {
        EditorApplication.delayCall -= AutoConfigurar;

        bool hizoPrefabs   = false;
        bool hizoController = false;

        // ── 1. Configurar modelo + animaciones Kenney (rig Humanoid + materiales + prefabs)
        if (!EditorPrefs.GetBool(PREF_KENNEY, false) && System.IO.File.Exists(RUTA_MODELO))
        {
            try
            {
                ConfiguradorPersonajeKenney.Configurar(silencioso: true);
                EditorPrefs.SetBool(PREF_KENNEY, true);
                hizoPrefabs = true;
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[Alsasua] Auto-config Kenney falló (se reintentará): {e.Message}");
            }
        }

        // ── 2. Crear AnimatorController con los 9 clips
        if (!EditorPrefs.GetBool(PREF_CONTROLLER, false) &&
            !System.IO.File.Exists(RUTA_CONTROLLER))
        {
            try
            {
                CreadorAnimatorMixamo.CrearController(silencioso: true);
                EditorPrefs.SetBool(PREF_CONTROLLER, true);
                hizoController = true;
            }
            catch (System.Exception e)
            {
                Debug.LogWarning($"[Alsasua] Auto-creación controller falló (se reintentará): {e.Message}");
            }
        }

        if (hizoPrefabs || hizoController)
        {
            Debug.Log(
                "════════════════════════════════════════════════════\n" +
                "  [Alsasua] ✓ Inicialización automática completada\n" +
                (hizoPrefabs   ? "  · Prefabs Kenney configurados\n"        : "") +
                (hizoController ? "  · AnimatorMixamo.controller creado\n"  : "") +
                "  \n" +
                "  CÓMO USAR:\n" +
                "  1. Selecciona el GameObject del jugador en la escena\n" +
                "  2. Inspector → ControladorJugador:\n" +
                "       · Prefab Personaje       → Assets/Personajes/Prefabs/<skin>.prefab\n" +
                "       · Controlador Animaciones → Assets/Personajes/AnimatorMixamo.controller\n" +
                "════════════════════════════════════════════════════"
            );
        }
    }

    // ── Forzar reinicialización manual ──────────────────────────────────────
    [MenuItem("Alsasua/Reinicializar Todo")]
    public static void Reinicializar()
    {
        EditorPrefs.DeleteKey(PREF_KENNEY);
        EditorPrefs.DeleteKey(PREF_CONTROLLER);

        // Borrar controller existente para que se recree limpio
        if (System.IO.File.Exists(RUTA_CONTROLLER))
        {
            AssetDatabase.DeleteAsset(RUTA_CONTROLLER);
            AssetDatabase.Refresh();
        }

        Debug.Log("[Alsasua] Reinicializando todo…");
        ConfiguradorPersonajeKenney.Configurar();
        CreadorAnimatorMixamo.CrearController();

        EditorPrefs.SetBool(PREF_KENNEY,     true);
        EditorPrefs.SetBool(PREF_CONTROLLER, true);

        Debug.Log("[Alsasua] ✓ Reinicialización completa.");
    }

    // ── Resetear flags (útil en desarrollo) ─────────────────────────────────
    [MenuItem("Alsasua/Resetear flags de inicialización")]
    public static void ResetearFlags()
    {
        EditorPrefs.DeleteKey(PREF_KENNEY);
        EditorPrefs.DeleteKey(PREF_CONTROLLER);
        Debug.Log("[Alsasua] Flags reseteados — la próxima compilación reinicializará todo.");
    }
}

#endif
