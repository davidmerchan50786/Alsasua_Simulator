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

    private Cesium3DTileset tilesetImagenSatelite;
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
    /// </summary>
    private void CargarImagenSatelite()
    {
        Debug.Log($"[Texturizador] Cargando imagen satélite: {tipoImagenSatelite}");

        GameObject tilesetObj = new GameObject("Satellite_ImageTileset");
        tilesetObj.transform.parent = transform;

        Cesium3DTileset tileset = tilesetObj.AddComponent<Cesium3DTileset>();
        tileset.maximumScreenSpaceError = screenSpaceError;
        tileset.createPhysicsMeshes     = false;  // Evita warning PhysX de triángulos grandes

        switch (tipoImagenSatelite)
        {
            case TipoImagenSatelite.CesiumIonBingMaps:
                // Bing Maps Aerial — asset ID 2 en Cesium Ion (incluido gratis)
                tileset.ionAssetID = 2;
                if (!string.IsNullOrEmpty(tokenCesiumIon))
                    tileset.ionAccessToken = tokenCesiumIon;
                else
                    Debug.LogWarning("[Texturizador] Token de Cesium Ion vacío. Bing Maps puede no cargar.");
                break;

            case TipoImagenSatelite.MapboxSatellite:
                if (string.IsNullOrEmpty(tokenMapbox))
                {
                    Debug.LogError("[Texturizador] Token de Mapbox no configurado. Imagen satélite no cargada.");
                    Destroy(tilesetObj);
                    return;
                }
                // Mapbox Satellite: endpoint de tiles raster XYZ (no es un 3D Tileset — solo para referencia)
                Debug.LogWarning("[Texturizador] Mapbox Satellite usa tiles XYZ, no 3D Tileset. " +
                                 "Para integración completa usa CesiumRasterOverlay con URL personalizada.");
                Destroy(tilesetObj);
                return;

            case TipoImagenSatelite.SentinelEsriLandsat:
                // ESRI World Imagery — baja resolución, sin autenticación
                // Usar como raster overlay sobre el terreno, no como 3DTileset
                Debug.LogWarning("[Texturizador] ESRI World Imagery debe usarse como CesiumRasterOverlay, " +
                                 "no como 3DTileset. Añade un componente 'Cesium Web Map Tile Service Raster Overlay' " +
                                 "al tileset de terreno con la URL de ESRI.");
                Destroy(tilesetObj);
                return;
        }

        tilesetImagenSatelite = tileset;
        Debug.Log("[Texturizador] Imagen satélite cargada correctamente.");
    }

    /// <summary>
    /// Carga edificios OSM desde Cesium Ion (asset 96188) como fallback.
    /// Úsalo solo si ConfiguradorAlsasua no ha cargado ya este tileset.
    /// </summary>
    private void CargarTilesetEdificiosOSM()
    {
        // Evitar duplicados
        if (Object.FindFirstObjectByType<Cesium3DTileset>() != null)
        {
            Cesium3DTileset[] existentes = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
            foreach (var t in existentes)
            {
                if (t.name.Contains("OSM"))
                {
                    Debug.Log("[Texturizador] Tileset OSM ya existe en escena. No se crea duplicado.");
                    return;
                }
            }
        }

        GameObject osmObj = new GameObject("OSM_Buildings_Texturizado");
        osmObj.transform.parent = transform;

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
        if (tilesetImagenSatelite != null) Destroy(tilesetImagenSatelite.gameObject);
        if (tilesetEdificiosOSM  != null) Destroy(tilesetEdificiosOSM.gameObject);

        if (cargarSateliteAlResolucion) CargarImagenSatelite();
        if (cargarEdificiosOSM)         CargarTilesetEdificiosOSM();

        Debug.Log("[Texturizador] Tilesets recargados.");
    }
}
