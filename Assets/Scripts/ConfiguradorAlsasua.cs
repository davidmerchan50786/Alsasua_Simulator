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
    [Range(2, 32)]
    [Tooltip("Error de pantalla para tilesets — menor = más detalle (más GPU). 4 = equilibrio calidad/rendimiento, 2 = máximo detalle")]
    [SerializeField] private float maximumScreenSpaceError = 4f;

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
        CorregirCesiumOriginShift();
        CorregirCamaras();          // CRÍTICO: Far Clipping = 1M m para Cesium
        CorregirAudioListeners();
        // NOTA: PosicionarCamaraInicial() eliminado — posicionaba la cámara a Y=1500
        // en cada Play, haciendo que Cesium cargara tiles aéreos de baja resolución.
        // La cámara ya queda posicionada correctamente en el editor.
    }

    /// <summary>
    /// Mueve CesiumOriginShift de la Main Camera (Y=1500, incorrecto) al Jugador
    /// (Y=1, nivel de suelo). Esto garantiza que Cesium cargue tiles de ALTA RESOLUCIÓN
    /// desde la perspectiva del jugador, no desde 1500m de altitud.
    /// </summary>
    private void CorregirCesiumOriginShift()
    {
        // 1. Quitar CesiumOriginShift de cualquier cámara (estaba en Main Camera a Y=1500)
        foreach (var cam in Object.FindObjectsByType<Camera>(FindObjectsSortMode.None))
        {
            var cos = cam.GetComponent<CesiumOriginShift>();
            if (cos != null)
            {
                Destroy(cos);
                Debug.Log($"[Alsasua] CesiumOriginShift eliminado de '{cam.gameObject.name}' (estaba a Y={cam.transform.position.y:F0})");
            }
        }

        // 2. CRÍTICO: re-emparenar todos los objetos con CesiumGlobeAnchor bajo
        //    CesiumGeoreference ANTES de cualquier AddComponent de Cesium.
        //    CesiumGlobeAnchor y CesiumOriginShift exigen estar en la jerarquía del
        //    CesiumGeoreference o sus OnEnable emiten warnings y no funcionan.
        if (georeference != null)
        {
            foreach (var anchor in Object.FindObjectsByType<CesiumGlobeAnchor>(FindObjectsSortMode.None))
            {
                if (!anchor.transform.IsChildOf(georeference.transform))
                {
                    anchor.transform.SetParent(georeference.transform, worldPositionStays: true);
                    Debug.Log($"[Alsasua] ✓ '{anchor.gameObject.name}' re-emparentado bajo CesiumGeoreference (CesiumGlobeAnchor).");
                }
            }
        }

        // 3. Añadir CesiumOriginShift al Jugador (Y=1 = nivel de suelo).
        //    Re-emparenar primero para que OnEnable no emita el warning.
        var controlador = Object.FindFirstObjectByType<ControladorJugador>();
        if (controlador != null)
        {
            if (georeference != null && !controlador.transform.IsChildOf(georeference.transform))
            {
                controlador.transform.SetParent(georeference.transform, worldPositionStays: true);
                Debug.Log("[Alsasua] ✓ Jugador re-emparentado bajo CesiumGeoreference.");
            }

            if (controlador.GetComponent<CesiumOriginShift>() == null)
            {
                controlador.gameObject.AddComponent<CesiumOriginShift>();
                Debug.Log("[Alsasua] ✓ CesiumOriginShift añadido al Jugador — tiles a nivel de suelo activados.");
            }
            else
                Debug.Log("[Alsasua] ✓ CesiumOriginShift ya estaba en el Jugador.");
        }
        else
            Debug.LogWarning("[Alsasua] ⚠ Jugador no encontrado — CesiumOriginShift no asignado. Ejecuta 'Configurar Gameplay'.");
    }

    /// <summary>
    /// Configura el Far/Near Clipping Plane de todas las cámaras para Cesium for Unity.
    ///
    /// CAUSA del pantalla negra:
    ///   Unity crea cámaras con Far = 1.000 m por defecto.
    ///   Cesium renderiza tiles del terreno real que pueden estar a decenas de km.
    ///   Con Far=1000m, todo lo que está más lejos es invisible → pantalla negra.
    ///
    /// Cesium for Unity recomienda Far = 1.000.000 m (1.000 km).
    /// Near = 0.1 m para no cortar la geometría cercana al suelo.
    /// </summary>
    private void CorregirCamaras()
    {
        const float FAR_CESIUM  = 1_000_000f;   // 1.000 km — recomendado por Cesium
        const float NEAR_SUELO  = 0.1f;         // 10 cm — evita corte al nivel del suelo

        // ── Skybox: si no hay ninguno asignado, la cámara ve el cielo NEGRO ────
        // Cesium For Unity no configura skybox automáticamente.
        // Usamos el skybox procedimental de Unity (shader built-in, compatible con URP).
        if (RenderSettings.skybox == null)
        {
            var skyShader = Shader.Find("Skybox/Procedural");
            if (skyShader != null)
            {
                var skyMat = new Material(skyShader);
                skyMat.SetFloat("_AtmosphereThickness", 1.1f);
                skyMat.SetColor("_SkyTint", new Color(0.50f, 0.65f, 0.82f));  // azul cielo País Vasco
                skyMat.SetColor("_GroundColor", new Color(0.37f, 0.35f, 0.32f)); // tierra/suelo
                skyMat.SetFloat("_Exposure", 1.3f);
                RenderSettings.skybox = skyMat;
                DynamicGI.UpdateEnvironment();
                Debug.Log("[Alsasua] Skybox procedimental activado — el cielo ya no es negro.");
            }
        }

        // ── Clipping planes ─────────────────────────────────────────────────────
        var camaras = Object.FindObjectsByType<Camera>(FindObjectsSortMode.None);
        foreach (var cam in camaras)
        {
            cam.farClipPlane  = FAR_CESIUM;
            cam.nearClipPlane = NEAR_SUELO;
            cam.clearFlags    = CameraClearFlags.Skybox;  // Forzar skybox, nunca fondo negro
            Debug.Log($"[Alsasua] Cámara '{cam.gameObject.name}': Near={NEAR_SUELO} Far={FAR_CESIUM} clearFlags=Skybox.");
        }

        if (camaras.Length == 0)
            Debug.LogWarning("[Alsasua] ⚠ No se encontraron cámaras para configurar el clipping plane.");
    }

    /// <summary>
    /// Garantiza que solo haya UN AudioListener activo en la escena.
    /// Unity da warning "There are 2 audio listeners in the scene" si hay más de uno.
    /// La Main Camera (cámara dron a Y=1500) no necesita AudioListener para gameplay.
    /// CamaraFPS (la cámara de seguimiento del jugador) es la que debe tener uno.
    /// </summary>
    private void CorregirAudioListeners()
    {
        var listeners = Object.FindObjectsByType<AudioListener>(FindObjectsSortMode.None);
        if (listeners.Length <= 1) return;

        // Hay más de un AudioListener: deshabilitar el de Main Camera (cámara dron)
        foreach (var al in listeners)
        {
            var cam = al.GetComponent<Camera>();
            if (cam != null && cam.CompareTag("MainCamera"))
            {
                al.enabled = false;
                Debug.Log($"[Alsasua] AudioListener desactivado en '{cam.gameObject.name}' (cámara dron). " +
                          "CamaraFPS conserva el suyo para gameplay.");
                return; // Solo desactivar uno, el primero que encontremos en Main Camera
            }
        }
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
        // ── Comprobar si ya hay tilesets configurados en la escena ─────────────
        // Un tileset es "funcional" si tiene:
        //   - ionAccessToken: token explicit en el componente
        //   - url: URL directa (p.ej. Google Photorealistic)
        //   - ionAssetID > 0: asset de Cesium Ion usando el TOKEN GLOBAL del login
        //     (cuando el usuario añade tiles desde el panel de Cesium, NO se guarda
        //      ionAccessToken — usa el token global del CesiumIonServer. Sin este
        //      check, nuestro código borraba esos tiles en cada Play → PANTALLA NEGRA)
        var tilesetsExistentes = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        bool hayTilesetsFuncionales = false;
        foreach (var t in tilesetsExistentes)
        {
            bool tieneToken = !string.IsNullOrEmpty(t.ionAccessToken);
            bool tieneURL   = !string.IsNullOrEmpty(t.url);
            bool tieneAsset = t.ionAssetID > 0;   // ← BUG FIX: token global de Cesium Ion login
            if (tieneToken || tieneURL || tieneAsset)
            {
                hayTilesetsFuncionales = true;
                Debug.Log($"[Alsasua] Tileset detectado: '{t.gameObject.name}' " +
                          $"(ionAssetID={t.ionAssetID}, url={!string.IsNullOrEmpty(t.url)}, token={tieneToken}) — conservando.");
            }
        }

        if (hayTilesetsFuncionales)
        {
            Debug.Log($"[Alsasua] ✓ {tilesetsExistentes.Length} tileset(s) existentes — conservando todos.");
            return;
        }

        // ── Sin tilesets: intentar crear desde Inspector/Resources tokens ───────
        Debug.Log("[Alsasua] No hay tilesets en la escena. Intentando crear desde tokens del Inspector...");

        // SOLO eliminar los nombres PROPIOS de este script (no los del panel de Cesium).
        // "Google Photorealistic 3D Tiles" y "Cesium OSM Buildings" son nombres del
        // panel de Cesium — NO los eliminamos para no borrar config del usuario.
        string[] nombresEliminar = {
            "Terreno_CesiumWorld",
            "Google_Photorealistic3DTiles",
            "OSM_Buildings_Fallback",
            "Satellite_ImageTileset"
        };
        foreach (string nombre in nombresEliminar)
            EliminarTilesetsDuplicados(nombre);

        // 1. Terreno mundial (relieve real)
        if (usarCesiumWorldTerrain)
        {
            if (!string.IsNullOrEmpty(tokenCesiumIon))
                CargarTilesetTerreno();
            else
                Debug.LogWarning("[Alsasua] ⚠ Sin token Cesium Ion — terreno no cargado. " +
                    "Opciones: (A) Pon el token en Inspector > ManagerAlsasua > Token Cesium Ion, " +
                    "(B) usa el panel Cesium → 'Add Cesium World Terrain'.");
        }

        // 2. Edificios con fachadas reales (Google Photorealistic 3D Tiles)
        if (usarGooglePhotorealistic && !string.IsNullOrEmpty(apiKeyGoogle))
            CargarGooglePhotorealistic3DTiles();
        else if (usarOSMEdificiosFallback && !string.IsNullOrEmpty(tokenCesiumIon))
        {
            Debug.LogWarning("[Alsasua] API Key de Google no configurada. Usando edificios OSM como fallback.");
            CargarEdificiosOSM();
        }

        // ── Diagnóstico final: avisar si no hay NINGÚN tileset ─────────────────
        var tilesetsFinal = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        if (tilesetsFinal.Length == 0)
        {
            Debug.LogError(
                "╔══════════════════════════════════════════════════════════════╗\n" +
                "║  [Alsasua] ❌ SIN TILES — ESCENARIO COMPLETAMENTE NEGRO      ║\n" +
                "║                                                              ║\n" +
                "║  No hay ningún tileset de Cesium en la escena.               ║\n" +
                "║  El terreno de Alsasua NO puede cargarse sin tokens.         ║\n" +
                "║                                                              ║\n" +
                "║  SOLUCIÓN RÁPIDA (2 pasos):                                  ║\n" +
                "║  1. Menú Unity: Cesium → Connect to Cesium Ion               ║\n" +
                "║     (crea cuenta gratuita en cesium.com/ion si no tienes)    ║\n" +
                "║  2. Menú Unity: Cesium → Add Cesium World Terrain            ║\n" +
                "║     (añade el terreno real directamente a la escena)         ║\n" +
                "║                                                              ║\n" +
                "║  O pon tu token en Inspector > ManagerAlsasua > Token Cesium ║\n" +
                "╚══════════════════════════════════════════════════════════════╝"
            );
        }
        else
        {
            Debug.Log($"[Alsasua] ✓ {tilesetsFinal.Length} tileset(s) creados desde tokens del Inspector.");
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
    //  CÁMARA INICIAL — ELIMINADO
    // ============================================================
    // PosicionarCamaraInicial() fue eliminada porque movía Camera.main a Y=1500
    // en cada Play. Esto hacía que Cesium cargara tiles de baja resolución
    // (vista aérea) en vez de tiles de alta resolución al nivel del suelo.
    // La cámara queda posicionada correctamente desde el editor.

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
