using UnityEngine;

/// <summary>
/// ScriptableObject que almacena los tokens de acceso del proyecto.
/// Se guarda en Assets/Resources/ConfiguracionTokens.asset
///
/// ⚠️ IMPORTANTE: No subas este archivo a GitHub/repositorios públicos.
/// Añade "ConfiguracionTokens.asset" a tu .gitignore.
/// </summary>
[CreateAssetMenu(fileName = "ConfiguracionTokens", menuName = "Alsasua/Configuración de Tokens")]
public class ConfiguracionTokens : ScriptableObject
{
    [Header("Google Maps Platform")]
    [Tooltip("API Key de Google Maps Platform — Map Tiles API")]
    public string apiKeyGoogle = "AIzaSyAIXBPGknxIOof5odgqXI4_mHYDtO93esY";

    [Header("Cesium Ion")]
    [Tooltip("Token de Cesium Ion — https://cesium.com/ion/tokens")]
    public string tokenCesiumIon = "";
}
