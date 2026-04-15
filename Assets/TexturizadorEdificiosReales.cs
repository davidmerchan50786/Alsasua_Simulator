using UnityEngine;
using CesiumForUnity;

/// <summary>
/// Script para cargar imágenes de satélite de alta resolución y gestionar
/// la fuente de edificios 3D. Integra Cesium Ion (Bing Maps) y Mapbox.
/// </summary>
public class TexturizadorEdificiosReales : MonoBehaviour
{
    [Header("Imágenes de Satélite")]
    [Tooltip("IMPORTANTE: el tileset satélite solo funciona si este GameObject está bajo CesiumGeoreference.")]
    [SerializeField] private bool cargarSateliteAlResolucion = false;
    [SerializeField] private TipoImagenSatelite tipoImagenSatelite = TipoImagenSatelite.CesiumIonBingMaps;

    [Header("Tokens y Credenciales")]
    [Tooltip("Token de Mapbox (requerido solo para TipoImagenSatelite.MapboxSatellite)")]
    [SerializeField] private string tokenMapbox = "";

    [Tooltip("Token de Cesium Ion — se carga automáticamente desde ConfiguracionTokens.asset si está vacío")]
    [SerializeField] private string tokenCesiumIon = "";

    [Header("Edificios 3D")]
    [SerializeField] private bool cargarEdificiosOSM = false;  // Solo si no hay Google ni OSM ya en escena

    [Header("Calidad")]
    [Range(4f, 32f)]
    [SerializeField] private float screenSpaceError = 8f;

    // CesiumIonRasterOverlay se aplica SOBRE el tileset de terreno existente.
    // No es un Cesium3DTileset independiente — ese error causa el "terreno blanco".
    private CesiumIonRasterOverlay overlayImagenSatelite;
    private Cesium3DTileset tilesetEdificiosOSM;

    public enum TipoImagenSatelite
    {
        CesiumIonBingMaps,    // Bing Maps vía Cesium Ion — asset ID 2 (recomendado, gratis)
        MapboxSatellite,      // Mapbox Satellite (requiere token de Mapbox)
        SentinelEsriLandsat   // ESRI World Imagery WMTS (baja res, sin autenticación)
    }

    private void Start()
    {
        // Cargar token desde ConfiguracionTokens.asset si el campo está vacío
        if (string.IsNullOrEmpty(tokenCesiumIon))
        {
            ConfiguracionTokens config = Resources.Load<ConfiguracionTokens>("ConfiguracionTokens");
            if (config != null && !string.IsNullOrEmpty(config.tokenCesiumIon))
            {
                tokenCesiumIon = config.tokenCesiumIon;
                Debug.Log("[Texturizador] Token Cesium Ion cargado desde ConfiguracionTokens.asset");
            }
        }

        if (cargarSateliteAlResolucion)
            CargarImagenSatelite();

        if (cargarEdificiosOSM)
            CargarTilesetEdificiosOSM();
    }

    /// <summary>
    /// Carga la capa de imagen de satélite según el tipo seleccionado.
    ///
    /// IMPORTANTE — "terreno blanco":
    /// Bing Maps y otras imágenes de satélite son RASTER OVERLAYS, no Cesium3DTileset.
    /// Deben aplicarse como CesiumIonRasterOverlay sobre el tileset de terreno existente.
    /// Crearlos como Cesium3DTileset independiente provoca un plano blanco sin textura
    /// porque el tileset de imagen 2D no sabe cómo draparse sobre el terreno 3D.
    /// </summary>
    private void CargarImagenSatelite()
    {
        Debug.Log($"[Texturizador] Cargando imagen satélite: {tipoImagenSatelite}");

        // Buscar el tileset de terreno (Cesium World Terrain, ionAssetID=1)
        // El overlay debe aplicarse sobre él para que el terreno tenga textura.
        Cesium3DTileset tilesetTerreno = null;
        foreach (var t in Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None))
        {
            if (t.ionAssetID == 1) { tilesetTerreno = t; break; }
        }

        if (tilesetTerreno == null)
        {
            Debug.LogWarning("[Texturizador] Cesium World Terrain (ionAssetID=1) no encontrado. " +
                             "Añade el terreno primero desde el panel de Cesium o ejecuta 'Configurar Escena Completa'.");
            return;
        }

        switch (tipoImagenSatelite)
        {
            case TipoImagenSatelite.CesiumIonBingMaps:
                // Bing Maps Aerial (assetID=2) — se drapa sobre el terreno como raster overlay.
                // NO usar Cesium3DTileset para esto — causa terreno blanco.
                if (tilesetTerreno.GetComponent<CesiumIonRasterOverlay>() != null)
                {
                    Debug.Log("[Texturizador] Bing Maps overlay ya presente en el tileset de terreno.");
                    overlayImagenSatelite = tilesetTerreno.GetComponent<CesiumIonRasterOverlay>();
                    return;
                }
                overlayImagenSatelite = tilesetTerreno.gameObject.AddComponent<CesiumIonRasterOverlay>();
                overlayImagenSatelite.ionAssetID = 2;
                if (!string.IsNullOrEmpty(tokenCesiumIon))
                    overlayImagenSatelite.ionAccessToken = tokenCesiumIon;
                else
                    Debug.LogWarning("[Texturizador] Token de Cesium Ion vacío. Bing Maps puede no cargar.");
                Debug.Log("[Texturizador] ✓ Bing Maps Aerial añadido como RasterOverlay al terreno.");
                break;

            case TipoImagenSatelite.MapboxSatellite:
                if (string.IsNullOrEmpty(tokenMapbox))
                {
                    Debug.LogError("[Texturizador] Token de Mapbox no configurado. Imagen satélite no cargada.");
                    return;
                }
                // Mapbox Satellite: tiles XYZ raster — usar CesiumWebMapTileServiceRasterOverlay
                Debug.LogWarning("[Texturizador] Mapbox Satellite requiere CesiumWebMapTileServiceRasterOverlay. " +
                                 "Añade ese componente manualmente al tileset de terreno con la URL de Mapbox.");
                return;

            case TipoImagenSatelite.SentinelEsriLandsat:
                // ESRI World Imagery — baja resolución, sin autenticación
                Debug.LogWarning("[Texturizador] ESRI World Imagery debe configurarse como " +
                                 "'Cesium Web Map Tile Service Raster Overlay' sobre el tileset de terreno. " +
                                 "Añade ese componente manualmente con la URL pública de ESRI.");
                return;
        }
    }

    /// <summary>
    /// Carga edificios OSM desde Cesium Ion (asset 96188) como fallback.
    /// Úsalo solo si ConfiguradorAlsasua no ha cargado ya este tileset.
    /// </summary>
    private void CargarTilesetEdificiosOSM()
    {
        // BUG 31 FIX: detectar duplicados por ionAssetID == 96188, no por nombre del GameObject.
        // Un tileset OSM existente puede tener cualquier nombre (p.ej. "Cesium OSM Buildings").
        Cesium3DTileset[] existentes = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        foreach (var t in existentes)
        {
            if (t.ionAssetID == 96188)
            {
                Debug.Log("[Texturizador] Tileset OSM (assetID 96188) ya existe en escena. No se crea duplicado.");
                return;
            }
        }

        // BUG 32 FIX: parenterlo bajo CesiumGeoreference (igual que CargarImagenSatelite).
        var georefOSM = Object.FindFirstObjectByType<CesiumGeoreference>();
        Transform parentOSM = georefOSM != null ? georefOSM.transform : transform;
        if (georefOSM == null)
            Debug.LogWarning("[Texturizador] CesiumGeoreference no encontrado — el tileset OSM puede aparecer desplazado.");

        GameObject osmObj = new GameObject("OSM_Buildings_Texturizado");
        osmObj.transform.parent = parentOSM;

        Cesium3DTileset tileset = osmObj.AddComponent<Cesium3DTileset>();

        // Cesium OSM Buildings — asset ID 96188 (gratuito en Cesium Ion)
        tileset.ionAssetID = 96188;
        if (!string.IsNullOrEmpty(tokenCesiumIon))
            tileset.ionAccessToken = tokenCesiumIon;
        else
            Debug.LogWarning("[Texturizador] Token de Cesium Ion vacío. OSM Buildings puede no cargar.");

        tileset.maximumScreenSpaceError = screenSpaceError;
        tilesetEdificiosOSM = tileset;

        Debug.Log("[Texturizador] Edificios OSM (Cesium Ion asset 96188) cargados.");
    }

    [ContextMenu("Recargar Tilesets")]
    public void RecargarTexturas()
    {
        // El overlay de imagen satélite es un componente, no un GameObject aparte
        if (overlayImagenSatelite != null) Destroy(overlayImagenSatelite);
        if (tilesetEdificiosOSM  != null) Destroy(tilesetEdificiosOSM.gameObject);

        overlayImagenSatelite = null;

        if (cargarSateliteAlResolucion) CargarImagenSatelite();
        if (cargarEdificiosOSM)         CargarTilesetEdificiosOSM();

        Debug.Log("[Texturizador] Tilesets recargados.");
    }
}
