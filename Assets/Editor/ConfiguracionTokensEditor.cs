using UnityEngine;
using UnityEditor;
using System.IO;

/// <summary>
/// Editor script que crea automáticamente el asset ConfiguracionTokens
/// al abrir el proyecto por primera vez.
/// </summary>
[InitializeOnLoad]
public static class ConfiguracionTokensEditor
{
    static ConfiguracionTokensEditor()
    {
        EditorApplication.delayCall += CrearAssetSiNoExiste;
    }

    private static void CrearAssetSiNoExiste()
    {
        string carpetaResources = "Assets/Resources";
        string rutaAsset        = carpetaResources + "/ConfiguracionTokens.asset";

        // Si ya existe, no hacer nada
        if (File.Exists(Application.dataPath + "/../" + rutaAsset))
            return;

        // Crear la carpeta Resources si no existe
        if (!AssetDatabase.IsValidFolder(carpetaResources))
            AssetDatabase.CreateFolder("Assets", "Resources");

        // Crear el ScriptableObject con los tokens por defecto
        ConfiguracionTokens config = ScriptableObject.CreateInstance<ConfiguracionTokens>();
        AssetDatabase.CreateAsset(config, rutaAsset);
        AssetDatabase.SaveAssets();

        Debug.Log("[Alsasua] ✓ ConfiguracionTokens.asset creado en Assets/Resources/");
        Debug.Log("[Alsasua] API Key de Google configurada. Añade también tu token de Cesium Ion.");
    }

    [MenuItem("Alsasua/Abrir Configuración de Tokens")]
    public static void AbrirConfiguracion()
    {
        ConfiguracionTokens config = Resources.Load<ConfiguracionTokens>("ConfiguracionTokens");
        if (config != null)
        {
            Selection.activeObject = config;
            EditorGUIUtility.PingObject(config);
        }
        else
        {
            Debug.LogWarning("No se encontró ConfiguracionTokens.asset. Abre el proyecto y se creará automáticamente.");
        }
    }
}
