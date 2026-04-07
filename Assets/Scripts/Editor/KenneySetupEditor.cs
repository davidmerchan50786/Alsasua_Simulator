// Assets/Scripts/Editor/KenneySetupEditor.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Herramienta de editor: prepara los assets Kenney del proyecto para que
//  SistemaPersonajes pueda cargarlos en tiempo de ejecución.
//
//  ACCESO: menú Unity → Alsasua → Configurar Assets Kenney
//
//  Qué hace:
//  1. Crea Assets/Resources/Personajes/  si no existe.
//  2. Copia characterMedium.fbx de Kenney → Resources/Personajes/CharacterMesh.fbx
//  3. Genera materiales URP con las skins .png de Kenney para cada tipo de
//     personaje (GuardiaCivil, PoliciaForal, Manifestante, Civilian...) y
//     los guarda en Assets/Resources/Personajes/Materials/.
//  4. Mueve las texturas de skins a Assets/Resources/Personajes/Textures/ para
//     que Resources.Load<Texture2D>() las encuentre en builds.
//
//  Tras ejecutar este menú, asigna en el Inspector de SistemaPersonajes el
//  campo "Mesh Personaje" → Assets/Resources/Personajes/CharacterMesh.fbx
// ═══════════════════════════════════════════════════════════════════════════

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;
using System.IO;

public static class KenneySetupEditor
{
    // ─── Rutas dentro del proyecto ──────────────────────────────────────
    private const string KENNEY_MODEL_PATH = "Assets/Personajes/Kenney/animated-characters-2/Model/characterMedium.fbx";
    private const string KENNEY_ANIM_IDLE  = "Assets/Personajes/Kenney/animated-characters-2/Animations/idle.fbx";
    private const string KENNEY_ANIM_RUN   = "Assets/Personajes/Kenney/animated-characters-2/Animations/run.fbx";

    private const string KENNEY_SKIN_SKATER_MALE   = "Assets/Personajes/Kenney/animated-characters-2/Skins/skaterMaleA.png";
    private const string KENNEY_SKIN_SKATER_FEMALE = "Assets/Personajes/Kenney/animated-characters-2/Skins/skaterFemaleA.png";
    private const string KENNEY_SKIN_CRIMINAL      = "Assets/Personajes/Kenney/animated-characters-2/Skins/criminalMaleA.png";
    private const string KENNEY_SKIN_CYBORG        = "Assets/Personajes/Kenney/animated-characters-2/Skins/cyborgFemaleA.png";

    private const string OUT_ROOT      = "Assets/Resources/Personajes";
    private const string OUT_MATERIALS = "Assets/Resources/Personajes/Materials";
    private const string OUT_TEXTURES  = "Assets/Resources/Personajes/Textures";

    // ─── Shader URP ─────────────────────────────────────────────────────
    private const string SHADER_LIT = "Universal Render Pipeline/Lit";

    // ───────────────────────────────────────────────────────────────────
    [MenuItem("Alsasua/Configurar Assets Kenney")]
    public static void ConfigurarAssetsKenney()
    {
        CrearCarpetasResources();

        bool meshOk   = CopiarMeshPersonaje();
        bool matsOk   = CrearMaterialesPorTipo();

        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();

        if (meshOk && matsOk)
            EditorUtility.DisplayDialog(
                "Kenney Setup — OK",
                "Assets Kenney configurados correctamente.\n\n" +
                "Ahora asigna en Inspector de SistemaPersonajes:\n" +
                "  • Mesh Personaje → Resources/Personajes/CharacterMesh\n" +
                "  • Anim Walk       → Resources/Personajes/AnimWalk\n" +
                "  • Anim Idle       → Resources/Personajes/AnimIdle",
                "OK");
        else
            EditorUtility.DisplayDialog(
                "Kenney Setup — Avisos",
                "Algunos assets no se encontraron (ver Console).\n\n" +
                "El simulador usará mallas procedurales como fallback.",
                "OK");
    }

    // ───────────────────────────────────────────────────────────────────
    [MenuItem("Alsasua/Abrir carpeta de assets Kenney")]
    public static void AbrirCarpetaKenney()
    {
        var obj = AssetDatabase.LoadAssetAtPath<Object>("Assets/Personajes/Kenney");
        if (obj != null) Selection.activeObject = obj;
        else Debug.LogWarning("[Kenney] Carpeta Assets/Personajes/Kenney no encontrada.");
    }

    // ───────────────────────────────────────────────────────────────────
    [MenuItem("Alsasua/Comprobar estado de todos los sistemas")]
    public static void ComprobarSistemas()
    {
        int errores = 0;

        Check(ref errores, "SistemaMultitud.cs",    "Assets/Scripts/SistemaMultitud.cs");
        Check(ref errores, "SistemaPersonajes.cs",  "Assets/Scripts/SistemaPersonajes.cs");
        Check(ref errores, "SistemaTrafico.cs",     "Assets/Scripts/SistemaTrafico.cs");
        Check(ref errores, "SistemaFerroviario.cs", "Assets/Scripts/SistemaFerroviario.cs");
        Check(ref errores, "SistemaVegetacion.cs",  "Assets/Scripts/SistemaVegetacion.cs");
        Check(ref errores, "GestorEscena.cs",       "Assets/Scripts/GestorEscena.cs");
        Check(ref errores, "BarricadaFuego.cs",     "Assets/Scripts/BarricadaFuego.cs");
        Check(ref errores, "ConfiguradorAlsasua.cs","Assets/Scripts/ConfiguradorAlsasua.cs");

        if (errores == 0)
            EditorUtility.DisplayDialog("Estado de sistemas — OK",
                "✓ Todos los scripts del simulador están presentes.\n\n" +
                "Compila con Ctrl+R para verificar errores C#.", "OK");
        else
            EditorUtility.DisplayDialog("Estado de sistemas — Faltan archivos",
                $"⚠ {errores} script(s) no encontrados. Revisa la Console.", "OK");
    }

    // ───────────────────────────────────────────────────────────────────
    //  PRIVADOS
    // ───────────────────────────────────────────────────────────────────
    private static void CrearCarpetasResources()
    {
        CrearCarpetaSiNoExiste(OUT_ROOT);
        CrearCarpetaSiNoExiste(OUT_MATERIALS);
        CrearCarpetaSiNoExiste(OUT_TEXTURES);
    }

    private static void CrearCarpetaSiNoExiste(string path)
    {
        if (!AssetDatabase.IsValidFolder(path))
        {
            string parent = Path.GetDirectoryName(path).Replace('\\', '/');
            string name   = Path.GetFileName(path);
            AssetDatabase.CreateFolder(parent, name);
            Debug.Log($"[KenneySetup] Carpeta creada: {path}");
        }
    }

    private static bool CopiarMeshPersonaje()
    {
        // Copiar FBX a Resources para que sea accesible con Resources.Load
        if (!File.Exists(Path.GetFullPath(KENNEY_MODEL_PATH)))
        {
            Debug.LogWarning($"[KenneySetup] FBX no encontrado: {KENNEY_MODEL_PATH}\n" +
                             "Descarga Kenney Animated Characters 2 de kenney.nl/assets");
            return false;
        }

        string destMesh = $"{OUT_ROOT}/CharacterMesh.fbx";
        if (!File.Exists(Path.GetFullPath(destMesh)))
        {
            AssetDatabase.CopyAsset(KENNEY_MODEL_PATH, destMesh);
            Debug.Log($"[KenneySetup] ✓ Mesh copiado a {destMesh}");
        }

        // Animaciones
        CopiarSiExiste(KENNEY_ANIM_IDLE, $"{OUT_ROOT}/AnimIdle.fbx");
        CopiarSiExiste(KENNEY_ANIM_RUN,  $"{OUT_ROOT}/AnimWalk.fbx");

        return true;
    }

    private static bool CrearMaterialesPorTipo()
    {
        var shader = Shader.Find(SHADER_LIT) ?? Shader.Find("Standard");
        if (shader == null)
        {
            Debug.LogError("[KenneySetup] Shader URP/Lit no encontrado. Asegúrate de tener URP instalado.");
            return false;
        }

        bool ok = true;

        // Guardia Civil — verde oliva + skin criminal (ropa oscura como proxy)
        ok &= CrearMaterial("Mat_GuardiaCivil",    shader, new Color(0.50f, 0.53f, 0.28f), KENNEY_SKIN_CRIMINAL);
        // Policía Foral  — azul marino
        ok &= CrearMaterial("Mat_PoliciaForal",    shader, new Color(0.10f, 0.14f, 0.34f), KENNEY_SKIN_CRIMINAL);
        // Manifestante oscuro — ropa negra + skin criminal
        ok &= CrearMaterial("Mat_Manifestante",    shader, new Color(0.12f, 0.12f, 0.14f), KENNEY_SKIN_CRIMINAL);
        // Civilian masculino — ropa casual
        ok &= CrearMaterial("Mat_CivilMale",       shader, new Color(0.52f, 0.46f, 0.40f), KENNEY_SKIN_SKATER_MALE);
        // Civilian femenino
        ok &= CrearMaterial("Mat_CivilFemale",     shader, new Color(0.60f, 0.42f, 0.48f), KENNEY_SKIN_SKATER_FEMALE);

        return ok;
    }

    private static bool CrearMaterial(string nombre, Shader shader, Color color, string texPath)
    {
        string outPath = $"{OUT_MATERIALS}/{nombre}.mat";

        // No sobreescribir si ya existe
        if (AssetDatabase.LoadAssetAtPath<Material>(outPath) != null)
        {
            Debug.Log($"[KenneySetup] Material existente conservado: {outPath}");
            return true;
        }

        var mat = new Material(shader) { name = nombre, color = color };
        mat.enableInstancing = true;

        // Intentar asignar skin si existe en el proyecto
        if (File.Exists(Path.GetFullPath(texPath)))
        {
            // Copiar textura a Resources/Personajes/Textures/
            string texName  = Path.GetFileName(texPath);
            string texDest  = $"{OUT_TEXTURES}/{texName}";
            if (!File.Exists(Path.GetFullPath(texDest)))
                AssetDatabase.CopyAsset(texPath, texDest);

            var tex = AssetDatabase.LoadAssetAtPath<Texture2D>(texDest);
            if (tex != null)
            {
                mat.mainTexture = tex;
                Debug.Log($"[KenneySetup] Skin asignada a {nombre}: {texName}");
            }
        }
        else
        {
            Debug.LogWarning($"[KenneySetup] Skin no encontrada: {texPath} — material con color plano.");
        }

        AssetDatabase.CreateAsset(mat, outPath);
        Debug.Log($"[KenneySetup] ✓ Material creado: {outPath}");
        return true;
    }

    private static void CopiarSiExiste(string origen, string destino)
    {
        if (File.Exists(Path.GetFullPath(origen)) &&
            !File.Exists(Path.GetFullPath(destino)))
        {
            AssetDatabase.CopyAsset(origen, destino);
            Debug.Log($"[KenneySetup] ✓ Copiado: {destino}");
        }
    }

    private static void Check(ref int errores, string nombre, string path)
    {
        if (!File.Exists(Path.GetFullPath(path)))
        {
            Debug.LogError($"[Alsasua] ❌ Falta: {nombre} ({path})");
            errores++;
        }
        else
            Debug.Log($"[Alsasua] ✓ OK: {nombre}");
    }
}
#endif
