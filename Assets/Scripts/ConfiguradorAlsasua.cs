using UnityEngine;
using CesiumForUnity;
using Unity.Mathematics;

/// <summary>
/// Configurador principal del escenario de Alsasua.
/// Lee los tokens desde Assets/Resources/ConfiguracionTokens.asset
/// y carga los tilesets fotorrealistas automáticamente al hacer Play.
/// </summary>
public class ConfiguradorAlsasua : MonoBehaviour
{
    // ============================================================
    //  COORDENADAS DE ALSASUA (Nafarroa / Navarra, España)
    // ============================================================
    private const double ALSASUA_LATITUD   =  42.9037;  // 42°54'13" N
    private const double ALSASUA_LONGITUD  =  -2.1668;  // 2°10'0" W
    private const double ALSASUA_ALTURA    = 530.0;     // ~530 m sobre el mar

    // ============================================================
    //  INSPECTOR
    // ============================================================
    [Header("═══ TOKENS DE ACCESO ═══")]
    [Tooltip("Token de Cesium Ion — https://cesium.com/ion/tokens")]
    [SerializeField] private string tokenCesiumIon = "";

    [Tooltip("API Key de Google Maps Platform (para fachadas reales)")]
    [SerializeField] private string apiKeyGoogle = "";

    // ScriptableObject con los tokens (se carga automáticamente de Resources)
    private ConfiguracionTokens configTokens;

    [Header("═══ TILESETS ═══")]
    [Tooltip("Activa Google Photorealistic 3D Tiles (fachadas y tejados reales)")]
    [SerializeField] private bool usarGooglePhotorealistic = true;

    [Tooltip("Activa terreno Cesium World Terrain (relieve real)")]
    [SerializeField] private bool usarCesiumWorldTerrain = true;

    [Tooltip("Fallback: edificios OSM si no hay Google API Key")]
    [SerializeField] private bool usarOSMEdificiosFallback = true;

    [Header("═══ CÁMARA INICIAL ═══")]
    [Tooltip("Altura inicial de la cámara sobre el suelo (metros)")]
    [SerializeField] private float alturaInicialCamara = 1500f;

    [Header("═══ CALIDAD ═══")]
    [Range(4, 32)]
    [Tooltip("Error de pantalla para tilesets — menor = más detalle (más GPU)")]
    [SerializeField] private float maximumScreenSpaceError = 8f;

    // Referencias privadas
    private CesiumGeoreference georeference;
    private GameObject tilesetTerreno;
    private GameObject tilesetEdificios;
    private GameObject tilesetOSM;

    // ============================================================
    //  INICIALIZACIÓN
    // ============================================================

    private void Start()
    {
        // Cargar tokens desde el ScriptableObject (Resources)
        CargarTokensDesdeConfig();

        ConfigurarGeoreferenciaAlsasua();
        CargarTilesets();
        PosicionarCamaraInicial();
    }

    /// <summary>
    /// Carga los tokens desde Assets/Resources/ConfiguracionTokens.asset.
    /// Si los campos del Inspector están vacíos, usa los del asset automáticamente.
    /// </summary>
    private void CargarTokensDesdeConfig()
    {
        configTokens = Resources.Load<ConfiguracionTokens>("ConfiguracionTokens");

        if (configTokens == null)
        {
            Debug.LogWarning("[Alsasua] No se encontró ConfiguracionTokens.asset en Resources. " +
                             "Usa el menú Alsasua → Abrir Configuración de Tokens.");
            return;
        }

        // Usar valores del asset si los campos del Inspector están vacíos
        if (string.IsNullOrEmpty(apiKeyGoogle) && !string.IsNullOrEmpty(configTokens.apiKeyGoogle))
        {
            apiKeyGoogle = configTokens.apiKeyGoogle;
            Debug.Log("[Alsasua] ✓ API Key de Google cargada desde ConfiguracionTokens.asset");
        }

        if (string.IsNullOrEmpty(tokenCesiumIon) && !string.IsNullOrEmpty(configTokens.tokenCesiumIon))
        {
            tokenCesiumIon = configTokens.tokenCesiumIon;
            Debug.Log("[Alsasua] ✓ Token de Cesium Ion cargado desde ConfiguracionTokens.asset");
        }
    }

    // ============================================================
    //  GEORREFERENCIA
    // ============================================================

    /// <summary>
    /// Posiciona el origen del mundo en el centro de Alsasua.
    /// Esto garantiza precisión métrica en toda la escena.
    /// </summary>
    private void ConfigurarGeoreferenciaAlsasua()
    {
        // Buscar o crear el CesiumGeoreference
        georeference = Object.FindFirstObjectByType<CesiumGeoreference>();
        if (georeference == null)
        {
            GameObject geoObj = new GameObject("CesiumGeoreference");
            georeference = geoObj.AddComponent<CesiumGeoreference>();
            Debug.Log("[Alsasua] CesiumGeoreference creado.");
        }

        // Establecer origen en Alsasua
        georeference.latitude  = ALSASUA_LATITUD;
        georeference.longitude = ALSASUA_LONGITUD;
        georeference.height    = ALSASUA_ALTURA;

        Debug.Log($"[Alsasua] Georreferencia establecida → Lat: {ALSASUA_LATITUD}, Lon: {ALSASUA_LONGITUD}, Alt: {ALSASUA_ALTURA}m");
    }

    // ============================================================
    //  CARGA DE TILESETS
    // ============================================================

    private void CargarTilesets()
    {
        // Eliminar TODOS los tilesets previos (evitar duplicados por nombres variantes)
        string[] nombresEliminar = {
            "Terreno_CesiumWorld",
            "Google_Photorealistic3DTiles",
            "Google Photorealistic 3D Tiles",  // variante con espacios (ionAssetID 2275207)
            "OSM_Buildings_Fallback",
            "Cesium OSM Buildings",             // variante añadida desde el panel Cesium
            "Satellite_ImageTileset"            // creado por TexturizadorEdificiosReales
        };
        foreach (string nombre in nombresEliminar)
            EliminarTilesetsDuplicados(nombre);

        // 1. Terreno mundial (relieve real)
        if (usarCesiumWorldTerrain)
            CargarTilesetTerreno();

        // 2. Edificios con fachadas reales (Google Photorealistic 3D Tiles)
        if (usarGooglePhotorealistic && !string.IsNullOrEmpty(apiKeyGoogle))
            CargarGooglePhotorealistic3DTiles();
        else if (usarOSMEdificiosFallback)
        {
            Debug.LogWarning("[Alsasua] API Key de Google no configurada. Usando edificios OSM como fallback.");
            CargarEdificiosOSM();
        }
    }

    /// <summary>
    /// Elimina todos los GameObjects con el nombre dado para evitar duplicados.
    /// </summary>
    private void EliminarTilesetsDuplicados(string nombre)
    {
        foreach (GameObject obj in Object.FindObjectsByType<GameObject>(FindObjectsSortMode.None))
        {
            if (obj.name == nombre)
            {
                Destroy(obj);
                Debug.Log($"[Alsasua] Eliminado duplicado: {nombre}");
            }
        }
    }

    /// <summary>
    /// Carga el terreno real de Cesium World Terrain.
    /// Proporciona el relieve montañoso de los alrededores de Alsasua.
    /// </summary>
    private void CargarTilesetTerreno()
    {
        tilesetTerreno = new GameObject("Terreno_CesiumWorld");
        if (georeference != null)
            tilesetTerreno.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetTerreno.AddComponent<Cesium3DTileset>();

        // Cesium World Terrain — asset ID 1 en Cesium Ion
        tileset.ionAssetID = 1;
        if (!string.IsNullOrEmpty(tokenCesiumIon))
            tileset.ionAccessToken = tokenCesiumIon;

        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.preloadAncestors         = true;
        tileset.preloadSiblings          = true;
        tileset.createPhysicsMeshes      = false;  // Evita warning de triángulos grandes en PhysX

        Debug.Log("[Alsasua] Terreno Cesium World cargado (asset ID: 1).");
    }

    /// <summary>
    /// Carga Google Photorealistic 3D Tiles — la fuente más realista disponible.
    /// Incluye:
    ///   - Fachadas de edificios con textura fotogramétrica real
    ///   - Tejados con su color, forma y material real
    ///   - Árboles, puentes, mobiliario urbano
    ///   - Cobertura total de Alsasua
    ///
    /// Requiere Google Maps Platform API Key con "Map Tiles API" activado.
    /// Primer año gratuito: https://cloud.google.com/maps-platform/
    /// </summary>
    private void CargarGooglePhotorealistic3DTiles()
    {
        tilesetEdificios = new GameObject("Google_Photorealistic3DTiles");
        if (georeference != null)
            tilesetEdificios.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetEdificios.AddComponent<Cesium3DTileset>();

        // URL directa de Google Map Tiles API — 3D Photorealistic Tiles
        tileset.url = $"https://tile.googleapis.com/v1/3dtiles/root.json?key={apiKeyGoogle}";
        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.preloadAncestors         = true;
        tileset.preloadSiblings          = true;
        tileset.createPhysicsMeshes      = false;  // Evita warning de triángulos grandes en PhysX

        // Configuración de renderizado
        tileset.showCreditsOnScreen = true;  // Obligatorio por licencia de Google

        Debug.Log("[Alsasua] ✓ Google Photorealistic 3D Tiles cargados — fachadas y tejados reales activados.");
        Debug.Log("[Alsasua] Fuente: https://tile.googleapis.com/v1/3dtiles/root.json");
    }

    /// <summary>
    /// Fallback: edificios 3D de OpenStreetMap vía Cesium Ion (asset ID: 96188).
    /// Sin texturas fotográficas, pero con geometría correcta.
    /// </summary>
    private void CargarEdificiosOSM()
    {
        tilesetOSM = new GameObject("OSM_Buildings_Fallback");
        if (georeference != null)
            tilesetOSM.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetOSM.AddComponent<Cesium3DTileset>();

        // OSM Buildings en Cesium Ion — asset ID 96188 (gratuito)
        tileset.ionAssetID = 96188;
        if (!string.IsNullOrEmpty(tokenCesiumIon))
            tileset.ionAccessToken = tokenCesiumIon;

        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.createPhysicsMeshes     = false;  // Evita warning de triángulos grandes en PhysX

        Debug.Log("[Alsasua] Edificios OSM cargados (fallback). Para ver fachadas reales configura la API Key de Google.");
    }

    // ============================================================
    //  CÁMARA INICIAL
    // ============================================================

    /// <summary>
    /// Posiciona la cámara sobre el centro de Alsasua al iniciar la escena.
    /// </summary>
    private void PosicionarCamaraInicial()
    {
        Camera camara = Camera.main;
        if (camara == null)
        {
            Debug.LogWarning("[Alsasua] No se encontró la cámara principal.");
            return;
        }

        // La posición relativa al georeference origin es (0,0,0) = centro de Alsasua
        // Colocar la cámara a alturaInicialCamara metros por encima
        camara.transform.position = new Vector3(0f, alturaInicialCamara, 0f);
        camara.transform.rotation = Quaternion.Euler(60f, 0f, 0f);  // Mirando hacia el suelo en ángulo

        // Ajustar plano de recorte lejano para escenas de escala urbana
        camara.farClipPlane  = 300000f;   // 300 km — cubre todo el horizonte
        camara.nearClipPlane = 0.5f;      // 50 cm

        Debug.Log($"[Alsasua] Cámara posicionada a {alturaInicialCamara}m sobre el centro de Alsasua.");
    }

    // ============================================================
    //  DIAGNÓSTICO
    // ============================================================

    private void OnGUI()
    {
        // HUD de diagnóstico — visible mientras los tiles cargan
        CesiumForUnity.Cesium3DTileset[] tilesets =
            Object.FindObjectsByType<CesiumForUnity.Cesium3DTileset>(FindObjectsSortMode.None);

        int cargando = 0;
        foreach (var t in tilesets)
        {
            if (!t.enabled || !t.gameObject.activeSelf) continue;
            // Cesium3DTileset no expone un estado de carga directamente;
            // usamos la presencia del tileset como indicador
            cargando++;
        }

        float hudAltura = tilesets.Length * 20f + 52f;
        GUI.color = new Color(0f, 0f, 0f, 0.55f);
        GUI.DrawTexture(new Rect(8f, 8f, 360f, hudAltura), Texture2D.whiteTexture);

        // Línea 1: ubicación geográfica
        GUI.color = Color.yellow;
        GUI.Label(new Rect(12f, 10f, 350f, 20f),
            $"Altsasu / Alsasua  |  Lat: {ALSASUA_LATITUD:F4}  Lon: {ALSASUA_LONGITUD:F4}");

        // Línea 2: tilesets y altitud cámara
        GUI.color = Color.cyan;
        GUI.Label(new Rect(12f, 30f, 350f, 20f),
            $"Tilesets activos: {cargando}  |  Cámara Y: {(Camera.main != null ? Camera.main.transform.position.y.ToString("F0") : "?")} m");

        // Lista de tilesets
        GUI.color = Color.white;
        for (int i = 0; i < tilesets.Length; i++)
        {
            var t = tilesets[i];
            if (!t.gameObject.activeSelf) continue;
            string src = !string.IsNullOrEmpty(t.url) ? "URL" : $"Ion:{t.ionAssetID}";
            GUI.Label(new Rect(12f, 50f + i * 20f, 350f, 20f), $"  {t.gameObject.name} [{src}]");
        }
    }

    // ============================================================
    //  UTILIDADES INSPECTOR
    // ============================================================

    [ContextMenu("Reconfigurar escena")]
    public void ReconfigurarDesdeInspector()
    {
        // Eliminar tilesets existentes
        if (tilesetTerreno  != null) DestroyImmediate(tilesetTerreno);
        if (tilesetEdificios != null) DestroyImmediate(tilesetEdificios);
        if (tilesetOSM       != null) DestroyImmediate(tilesetOSM);

        ConfigurarGeoreferenciaAlsasua();
        CargarTilesets();
        Debug.Log("[Alsasua] Escena reconfigurada.");
    }

    [ContextMenu("Mostrar instrucciones API Key Google")]
    private void MostrarInstruccionesAPIKey()
    {
        Debug.Log("════════════════════════════════════════════════");
        Debug.Log("  CÓMO OBTENER API KEY DE GOOGLE MAPS PLATFORM  ");
        Debug.Log("════════════════════════════════════════════════");
        Debug.Log("1. Ve a: https://console.cloud.google.com/");
        Debug.Log("2. Crea un proyecto nuevo");
        Debug.Log("3. Activa 'Map Tiles API' en APIs y servicios");
        Debug.Log("4. Ve a Credenciales → Crear credencial → API Key");
        Debug.Log("5. Pega la clave en el campo 'Api Key Google' del Inspector");
        Debug.Log("════════════════════════════════════════════════");
        Debug.Log("  COBERTURA DE ALSASUA:");
        Debug.Log("  Google tiene fotogrametría 3D de toda España");
        Debug.Log("  incluyendo Alsasua — fachadas y tejados reales");
        Debug.Log("════════════════════════════════════════════════");
    }
}
