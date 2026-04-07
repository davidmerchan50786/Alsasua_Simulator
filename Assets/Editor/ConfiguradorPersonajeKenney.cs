// Assets/Editor/ConfiguradorPersonajeKenney.cs
// Configura automáticamente los assets de Kenney (CC0) para usarlos como personajes
// en Alsasua Simulator. Ejecuta "Alsasua → Configurar Personajes Kenney" una sola vez
// tras importar los FBX.
//
// QUÉ HACE:
//   1. Establece el rig del modelo como Humanoid (necesario para retargeting de animaciones)
//   2. Configura cada clip FBX como Humanoid + Loop + In Place (sin root motion)
//   3. Asigna avatares y normaliza la escala (los FBX de Kenney vienen a 1:1)
//   4. Genera materiales URP con cada skin (textura atlas del personaje)
//   5. Crea prefabs listos para arrastrar al campo "Prefab Personaje" del ControladorJugador

#if UNITY_EDITOR

using UnityEngine;
using UnityEditor;
using System.IO;

public static class ConfiguradorPersonajeKenney
{
    // ── Rutas relativas dentro de Assets/ ────────────────────────────────
    private const string RUTA_MODELO      = "Assets/Personajes/Modelos/PersonajeBase.fbx";
    private const string RUTA_ANIM_IDLE   = "Assets/Personajes/Animaciones/Anim_Idle.fbx";
    private const string RUTA_ANIM_CORRER = "Assets/Personajes/Animaciones/Anim_Correr.fbx";
    private const string RUTA_ANIM_SALTAR = "Assets/Personajes/Animaciones/Anim_Saltar.fbx";
    private const string RUTA_TEXTURAS    = "Assets/Personajes/Texturas";
    private const string RUTA_PREFABS     = "Assets/Personajes/Prefabs";
    private const string RUTA_MATERIALES  = "Assets/Personajes/Materiales";

    // Skins disponibles → nombre que aparecerá en el prefab
    private static readonly (string archivo, string nombre, string descripcion)[] SKINS =
    {
        ("skin_manifestante.png", "Manifestante",  "Ropa oscura, aspecto de activista"),
        ("skin_civil.png",        "CivilMasculino","Ropa de calle, civil neutro"),
        ("skin_mujer.png",        "CivilFemenino", "Ropa de calle, mujer civil"),
        ("skin_guardia.png",      "GuardiaCivil",  "Uniforme oscuro, aspecto de agente"),
        ("skin_jarrai.png",       "Jarrai",         "Ropa casual, aspecto de joven"),
    };

    [MenuItem("Alsasua/Configurar Personajes Kenney (CC0)")]
    public static void Configurar() => Configurar(silencioso: false);

    public static void Configurar(bool silencioso)
    {
        bool todoOK = true;

        // ── 1. Verificar que los FBX existen ─────────────────────────────────
        foreach (var ruta in new[] { RUTA_MODELO, RUTA_ANIM_IDLE, RUTA_ANIM_CORRER, RUTA_ANIM_SALTAR })
        {
            if (!File.Exists(ruta))
            {
                Debug.LogError($"[Personajes] No se encontró: {ruta}\n" +
                               "Asegúrate de haber ejecutado primero 'Alsasua → Crear Animator Controller Mixamo'");
                todoOK = false;
            }
        }
        if (!todoOK) return;

        AssetDatabase.StartAssetEditing();
        try
        {
            // ── 2. Configurar modelo como Humanoid ───────────────────────────
            ConfigurarRigHumanoide(RUTA_MODELO);

            // ── 3. Configurar animaciones como Humanoid + Loop ───────────────
            ConfigurarAnimacion(RUTA_ANIM_IDLE,   loop: true,  inPlace: true);
            ConfigurarAnimacion(RUTA_ANIM_CORRER, loop: true,  inPlace: true);
            ConfigurarAnimacion(RUTA_ANIM_SALTAR, loop: false, inPlace: true);
        }
        finally
        {
            AssetDatabase.StopAssetEditing();
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
        }

        // ── 4. Crear materiales y prefabs (después del Refresh) ──────────────
        System.IO.Directory.CreateDirectory(RUTA_PREFABS);
        System.IO.Directory.CreateDirectory(RUTA_MATERIALES);
        AssetDatabase.Refresh();

        GameObject modelo = AssetDatabase.LoadAssetAtPath<GameObject>(RUTA_MODELO);
        if (modelo == null)
        {
            Debug.LogError("[Personajes] No se pudo cargar PersonajeBase.fbx tras configurar el rig.");
            return;
        }

        int prefabsCreados = 0;
        foreach (var (archivo, nombre, descripcion) in SKINS)
        {
            string rutaTextura = $"{RUTA_TEXTURAS}/{archivo}";
            if (!File.Exists(rutaTextura))
            {
                Debug.LogWarning($"[Personajes] Textura no encontrada: {rutaTextura} — saltando {nombre}");
                continue;
            }

            Material mat = CrearMaterialURP(nombre, rutaTextura);
            if (mat == null) continue;

            CrearPrefab(modelo, mat, nombre, descripcion);
            prefabsCreados++;
        }

        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();

        // ── 5. Resumen en consola ────────────────────────────────────────────
        Debug.Log(
            "════════════════════════════════════════════════════════\n" +
            $"  [Alsasua] ✓ {prefabsCreados} prefabs de personaje creados en:\n" +
            $"  {RUTA_PREFABS}/\n" +
            "  \n" +
            "  PREFABS DISPONIBLES:\n" +
            "  · Manifestante  → skin oscuro, activista\n" +
            "  · CivilMasculino → transeúnte masculino\n" +
            "  · CivilFemenino  → transeúnte femenino\n" +
            "  · GuardiaCivil   → agente de orden\n" +
            "  · Jarrai          → joven activista vasco\n" +
            "  \n" +
            "  CÓMO USAR:\n" +
            "  1. Abre el Animator Controller (Alsasua → Crear Animator Controller Mixamo)\n" +
            "  2. Arrastra los clips desde Assets/Personajes/Animaciones/ a los estados\n" +
            "  3. En el Inspector del Jugador:\n" +
            "       · Prefab Personaje → arrastra el prefab que quieras\n" +
            "       · Controlador Animaciones → AnimatorMixamo.controller\n" +
            "       · Escala Modelo → 1 (Kenney ya viene a escala correcta)\n" +
            "════════════════════════════════════════════════════════"
        );

        if (!silencioso) EditorUtility.DisplayDialog(
            $"✓ {prefabsCreados} personajes configurados",
            $"Prefabs creados en Assets/Personajes/Prefabs/\n\n" +
            "Arrastra cualquier prefab al campo 'Prefab Personaje' " +
            "del ControladorJugador en el Inspector.\n\n" +
            "El Animator Controller se crea automáticamente al abrir Unity.",
            "OK"
        );
    }

    // ────────────────────────────────────────────────────────────────────────
    //  HELPERS
    // ────────────────────────────────────────────────────────────────────────

    private static void ConfigurarRigHumanoide(string ruta)
    {
        var importer = AssetImporter.GetAtPath(ruta) as ModelImporter;
        if (importer == null) return;

        bool cambio = false;

        if (importer.animationType != ModelImporterAnimationType.Human)
        {
            importer.animationType = ModelImporterAnimationType.Human;
            cambio = true;
        }

        // Normalizar escala si el FBX viene en otras unidades
        if (importer.useFileScale && Mathf.Abs(importer.globalScale - 1f) > 0.001f)
        {
            importer.globalScale = 1f;
            cambio = true;
        }

        // Desactivar la importación de cámara/luces embebidas (reducen el tamaño del asset)
        if (importer.importCameras || importer.importLights)
        {
            importer.importCameras = false;
            importer.importLights  = false;
            cambio = true;
        }

        if (cambio)
        {
            importer.SaveAndReimport();
            Debug.Log($"[Personajes] ✓ Rig Humanoid configurado en: {ruta}");
        }
    }

    private static void ConfigurarAnimacion(string ruta, bool loop, bool inPlace)
    {
        var importer = AssetImporter.GetAtPath(ruta) as ModelImporter;
        if (importer == null) return;

        // Las animaciones deben ser Humanoid para el retargeting
        importer.animationType = ModelImporterAnimationType.Human;

        // Usar el avatar del modelo base para retargeting
        var avatarModel = AssetDatabase.LoadAssetAtPath<GameObject>(RUTA_MODELO);
        if (avatarModel != null)
        {
            var avatarImporter = AssetImporter.GetAtPath(RUTA_MODELO) as ModelImporter;
            if (avatarImporter?.sourceAvatar != null)
                importer.sourceAvatar = avatarImporter.sourceAvatar;
        }

        var clips = importer.clipAnimations;
        if (clips == null || clips.Length == 0)
        {
            // Crear la definición del clip desde las animaciones por defecto del FBX
            clips = importer.defaultClipAnimations;
        }

        for (int i = 0; i < clips.Length; i++)
        {
            clips[i].loopTime       = loop;
            clips[i].loopPose       = loop;
            clips[i].lockRootRotY   = inPlace;  // sin rotación del root en Y
            clips[i].lockRootHeightY = inPlace; // sin traslación vertical del root
            clips[i].lockRootPositionXZ = inPlace; // SIN desplazamiento horizontal del root
            // In Place: el personaje anima en el sitio; CharacterController lo mueve
        }

        importer.clipAnimations = clips;
        importer.SaveAndReimport();
        Debug.Log($"[Personajes] ✓ Animación configurada (loop={loop}, inPlace={inPlace}): {ruta}");
    }

    private static Material CrearMaterialURP(string nombre, string rutaTextura)
    {
        // Buscar shader URP
        var shader = Shader.Find("Universal Render Pipeline/Lit")
                  ?? Shader.Find("Universal Render Pipeline/Simple Lit")
                  ?? Shader.Find("Standard");

        if (shader == null)
        {
            Debug.LogError("[Personajes] No se encontró shader URP. El material saldrá magenta.");
            return null;
        }

        var textura = AssetDatabase.LoadAssetAtPath<Texture2D>(rutaTextura);
        if (textura == null)
        {
            Debug.LogWarning($"[Personajes] Textura no cargada: {rutaTextura}");
            return null;
        }

        string rutaMat = $"{RUTA_MATERIALES}/Mat_{nombre}.mat";

        // Reutilizar material si ya existe
        var mat = AssetDatabase.LoadAssetAtPath<Material>(rutaMat);
        if (mat == null)
        {
            mat = new Material(shader);
            AssetDatabase.CreateAsset(mat, rutaMat);
        }
        else
        {
            mat.shader = shader;
        }

        // Asignar textura al canal correcto según el shader
        if (mat.HasProperty("_BaseMap"))       mat.SetTexture("_BaseMap", textura);   // URP
        else if (mat.HasProperty("_MainTex"))  mat.SetTexture("_MainTex", textura);   // Standard

        // Sin metalicidad ni smoothness para aspecto de tela/ropa
        if (mat.HasProperty("_Metallic"))    mat.SetFloat("_Metallic",   0f);
        if (mat.HasProperty("_Smoothness"))  mat.SetFloat("_Smoothness", 0.2f);

        EditorUtility.SetDirty(mat);
        return mat;
    }

    private static void CrearPrefab(GameObject modeloFuente, Material material,
                                    string nombre, string descripcion)
    {
        string rutaPrefab = $"{RUTA_PREFABS}/{nombre}.prefab";

        // Instanciar temporalmente en escena para configurar el prefab
        var instancia = Object.Instantiate(modeloFuente);
        instancia.name = nombre;

        // Aplicar material a todos los Renderers del personaje
        foreach (var renderer in instancia.GetComponentsInChildren<SkinnedMeshRenderer>(true))
            renderer.sharedMaterial = material;
        foreach (var renderer in instancia.GetComponentsInChildren<MeshRenderer>(true))
            renderer.sharedMaterial = material;

        // Guardar como prefab (sobrescribir si ya existe)
        PrefabUtility.SaveAsPrefabAssetAndConnect(instancia, rutaPrefab,
                                                   InteractionMode.AutomatedAction);

        // Destruir la instancia temporal
        Object.DestroyImmediate(instancia);

        Debug.Log($"[Personajes] ✓ Prefab creado: {nombre} — {descripcion}");
    }
}

#endif
