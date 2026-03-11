using UnityEngine;
using CesiumForUnity;
using Unity.Mathematics;

/// <summary>
/// Configurador principal del escenario de Alsasua.
/// Lee los tokens desde Assets/Resources/ConfiguracionTokens.asset
/// y carga los tilesets fotorrealistas automáticamente al hacer Play.
///
/// JERARQUÍA REQUERIDA POR CESIUM FOR UNITY:
///   CesiumGeoreference (raíz)
///   ├── Jugador  (CesiumOriginShift + CesiumGlobeAnchor)
///   ├── CamaraTP (CesiumGlobeAnchor, gestionada por ControladorJugador)
///   └── Tilesets (Cesium3DTileset)
///
/// Este script crea esa jerarquía en tiempo de ejecución cuando la escena
/// no la tiene configurada correctamente desde el editor.
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
        CargarTokensDesdeConfig();
        ConfigurarGeoreferenciaAlsasua();
        CorregirTilesetsExistentes();       // Arreglar tilesets YA en escena (physics, SSE)
        CargarTilesets();
        CorregirCesiumOriginShift();        // Re-parenta Jugador y añade CesiumOriginShift
        CorregirCamaras();                  // Far Clipping = 1M m, skybox
        // BUG FIX: CorregirAudioListeners() estaba definido pero nunca llamado →
        // si había múltiples AudioListeners en escena, Unity emitía warnings de audio.
        CorregirAudioListeners();

        // Invoke(0): se ejecuta al FINAL del primer frame, después de que TODOS los
        // Start() hayan corrido (incluyendo ControladorJugador que desacopla la cámara).
        // Necesario para re-emparenar la cámara DESPUÉS de que ControladorJugador la mueva.
        Invoke(nameof(FixCamerasPostStart), 0f);
    }

    // ============================================================
    //  CORRECCIÓN POST-START (se ejecuta tras todos los Start())
    // ============================================================

    /// <summary>
    /// Re-emparenta todos los objetos con CesiumGlobeAnchor bajo CesiumGeoreference
    /// y garantiza el Far Clipping correcto, ejecutándose DESPUÉS de que
    /// ControladorJugador.Start() haya desacoplado su cámara.
    /// </summary>
    private void FixCamerasPostStart()
    {
        if (georeference == null) return;

        // Re-emparenar cualquier CesiumGlobeAnchor que esté fuera de la jerarquía
        foreach (var anchor in Object.FindObjectsByType<CesiumGlobeAnchor>(FindObjectsSortMode.None))
        {
            if (!anchor.transform.IsChildOf(georeference.transform))
            {
                anchor.transform.SetParent(georeference.transform, worldPositionStays: true);
                Debug.Log($"[Alsasua] ✓ '{anchor.gameObject.name}' re-emparentado bajo CesiumGeoreference (post-start).");
            }
        }

        // Asegurar Far Clipping en todas las cámaras (ControladorJugador puede haberlo sobreescrito)
        foreach (var cam in Object.FindObjectsByType<Camera>(FindObjectsSortMode.None))
        {
            if (cam.farClipPlane < 500_000f)
            {
                cam.farClipPlane = 1_000_000f;
                Debug.Log($"[Alsasua] ✓ Far clip corregido en '{cam.gameObject.name}' post-start.");
            }
        }
    }

    // ============================================================
    //  CORRECCIÓN DE CESIUM ORIGIN SHIFT
    // ============================================================

    /// <summary>
    /// Mueve CesiumOriginShift de la Main Camera (Y=1500, incorrecto) al Jugador
    /// (Y=1, nivel de suelo). Esto garantiza que Cesium cargue tiles de ALTA RESOLUCIÓN
    /// desde la perspectiva del jugador, no desde 1500m de altitud.
    ///
    /// CRÍTICO: el Jugador es re-emparentado bajo CesiumGeoreference ANTES de añadir
    /// CesiumOriginShift, para que CesiumGlobeAnchor.OnEnable() no emita warnings.
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

    // ============================================================
    //  CORRECCIÓN DE TILESETS EXISTENTES EN ESCENA
    // ============================================================

    /// <summary>
    /// Arregla los tilesets que YA están en la escena (añadidos desde el panel de Cesium).
    /// Aplica la configuración correcta de física y calidad a cada tileset.
    ///
    /// REGLA DE FÍSICA:
    ///   - Cesium World Terrain (ionAssetID=1): createPhysicsMeshes=true  → el jugador puede caminar
    ///   - Google Photorealistic (URL):          createPhysicsMeshes=true  → colisión con edificios y terreno
    ///   - OSM Buildings (ionAssetID=96188):     createPhysicsMeshes=false → solo visual
    ///
    /// El warning de PhysX sobre triángulos >500 unidades es NORMAL para tiles de terreno
    /// real y no afecta al juego — solo es una advertencia de rendimiento de PhysX.
    /// </summary>
    private void CorregirTilesetsExistentes()
    {
        var todos = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        if (todos.Length == 0) return;

        foreach (var t in todos)
        {
            bool esTerreno  = t.ionAssetID == 1;
            bool esGoogle   = !string.IsNullOrEmpty(t.url) && t.url.Contains("googleapis");
            bool esOSM      = t.ionAssetID == 96188;

            // Física: terreno y Google necesitan colisión para que el jugador camine
            bool necesitaFisica = esTerreno || esGoogle || (!esOSM && t.ionAssetID > 0);
            if (t.createPhysicsMeshes != necesitaFisica)
            {
                t.createPhysicsMeshes = necesitaFisica;
                Debug.Log($"[Alsasua] Tileset '{t.gameObject.name}': createPhysicsMeshes={necesitaFisica}");
            }

            // Calidad SSE
            if (t.maximumScreenSpaceError > maximumScreenSpaceError)
                t.maximumScreenSpaceError = maximumScreenSpaceError;

            // Mostrar créditos de Google (obligatorio por licencia)
            if (esGoogle) t.showCreditsOnScreen = true;

            // Ensamblar bajo CesiumGeoreference si el tileset está fuera
            if (georeference != null && !t.transform.IsChildOf(georeference.transform))
            {
                t.transform.SetParent(georeference.transform, worldPositionStays: true);
                Debug.Log($"[Alsasua] Tileset '{t.gameObject.name}' movido bajo CesiumGeoreference.");
            }
        }

        Debug.Log($"[Alsasua] ✓ {todos.Length} tileset(s) existentes corregidos.");
    }

    // ============================================================
    //  CORRECCIÓN DE CÁMARAS
    // ============================================================

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
        if (RenderSettings.skybox == null)
        {
            var skyShader = Shader.Find("Skybox/Procedural");
            if (skyShader != null)
            {
                var skyMat = new Material(skyShader);
                skyMat.SetFloat("_AtmosphereThickness", 1.1f);
                skyMat.SetColor("_SkyTint", new Color(0.50f, 0.65f, 0.82f));
                skyMat.SetColor("_GroundColor", new Color(0.37f, 0.35f, 0.32f));
                skyMat.SetFloat("_Exposure", 1.3f);
                RenderSettings.skybox = skyMat;
                DynamicGI.UpdateEnvironment();
                Debug.Log("[Alsasua] Skybox procedimental activado.");
            }
        }

        // ── Clipping planes ─────────────────────────────────────────────────────
        var camaras = Object.FindObjectsByType<Camera>(FindObjectsSortMode.None);
        foreach (var cam in camaras)
        {
            cam.farClipPlane  = FAR_CESIUM;
            cam.nearClipPlane = NEAR_SUELO;
            cam.clearFlags    = CameraClearFlags.Skybox;
            Debug.Log($"[Alsasua] Cámara '{cam.gameObject.name}': Near={NEAR_SUELO} Far={FAR_CESIUM}.");
        }

        if (camaras.Length == 0)
            Debug.LogWarning("[Alsasua] ⚠ No se encontraron cámaras para configurar el clipping plane.");
    }

    // ============================================================
    //  AUDIO LISTENERS
    // ============================================================

    private void CorregirAudioListeners()
    {
        var listeners = Object.FindObjectsByType<AudioListener>(FindObjectsSortMode.None);
        if (listeners.Length <= 1) return;

        foreach (var al in listeners)
        {
            var cam = al.GetComponent<Camera>();
            if (cam != null && cam.CompareTag("MainCamera"))
            {
                al.enabled = false;
                Debug.Log($"[Alsasua] AudioListener desactivado en '{cam.gameObject.name}'.");
                return;
            }
        }
    }

    // ============================================================
    //  TOKENS
    // ============================================================

    private void CargarTokensDesdeConfig()
    {
        configTokens = Resources.Load<ConfiguracionTokens>("ConfiguracionTokens");

        if (configTokens == null)
        {
            Debug.LogWarning("[Alsasua] No se encontró ConfiguracionTokens.asset en Resources.");
            return;
        }

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

    private void ConfigurarGeoreferenciaAlsasua()
    {
        georeference = Object.FindFirstObjectByType<CesiumGeoreference>();
        if (georeference == null)
        {
            GameObject geoObj = new GameObject("CesiumGeoreference");
            georeference = geoObj.AddComponent<CesiumGeoreference>();
            Debug.Log("[Alsasua] CesiumGeoreference creado.");
        }

        georeference.latitude  = ALSASUA_LATITUD;
        georeference.longitude = ALSASUA_LONGITUD;
        georeference.height    = ALSASUA_ALTURA;

        Debug.Log($"[Alsasua] Georreferencia → Lat: {ALSASUA_LATITUD}, Lon: {ALSASUA_LONGITUD}, Alt: {ALSASUA_ALTURA}m");
    }

    // ============================================================
    //  CARGA DE TILESETS
    // ============================================================

    private void CargarTilesets()
    {
        var tilesetsExistentes = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        bool hayTilesetsFuncionales = false;
        foreach (var t in tilesetsExistentes)
        {
            bool tieneToken = !string.IsNullOrEmpty(t.ionAccessToken);
            bool tieneURL   = !string.IsNullOrEmpty(t.url);
            bool tieneAsset = t.ionAssetID > 0;
            if (tieneToken || tieneURL || tieneAsset)
            {
                hayTilesetsFuncionales = true;
                Debug.Log($"[Alsasua] Tileset detectado: '{t.gameObject.name}' — conservando.");
            }
        }

        if (hayTilesetsFuncionales)
        {
            Debug.Log($"[Alsasua] ✓ {tilesetsExistentes.Length} tileset(s) existentes — conservando todos.");
            return;
        }

        Debug.Log("[Alsasua] No hay tilesets en la escena. Creando desde tokens...");

        string[] nombresEliminar = {
            "Terreno_CesiumWorld",
            "Google_Photorealistic3DTiles",
            "OSM_Buildings_Fallback"
        };
        foreach (string nombre in nombresEliminar)
            EliminarTilesetsDuplicados(nombre);

        // 1. Terreno (relieve real) — necesario para colisión de suelo
        if (usarCesiumWorldTerrain && !string.IsNullOrEmpty(tokenCesiumIon))
            CargarTilesetTerreno();
        else if (usarCesiumWorldTerrain)
            Debug.LogWarning("[Alsasua] ⚠ Sin token Cesium Ion — terreno no cargado. " +
                "Usa Cesium → Connect to Cesium Ion → Add Cesium World Terrain.");

        // 2. Google Photorealistic (fachadas + tejados reales)
        if (usarGooglePhotorealistic && !string.IsNullOrEmpty(apiKeyGoogle))
            CargarGooglePhotorealistic3DTiles();
        else if (usarOSMEdificiosFallback && !string.IsNullOrEmpty(tokenCesiumIon))
        {
            Debug.LogWarning("[Alsasua] Sin API Key Google → fallback OSM.");
            CargarEdificiosOSM();
        }

        var tilesetsFinal = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        if (tilesetsFinal.Length == 0)
        {
            Debug.LogError(
                "╔══════════════════════════════════════════════════════════════╗\n" +
                "║  [Alsasua] ❌ SIN TILES — ESCENARIO COMPLETAMENTE NEGRO      ║\n" +
                "║                                                              ║\n" +
                "║  SOLUCIÓN (2 pasos):                                         ║\n" +
                "║  1. Menú Unity: Cesium → Connect to Cesium Ion               ║\n" +
                "║  2. Menú Unity: Cesium → Add Cesium World Terrain            ║\n" +
                "║     + Cesium → Add Google Photorealistic 3D Tiles            ║\n" +
                "╚══════════════════════════════════════════════════════════════╝"
            );
        }
        else
            Debug.Log($"[Alsasua] ✓ {tilesetsFinal.Length} tileset(s) creados.");
    }

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
    /// Física habilitada para que el jugador pueda caminar sobre el suelo.
    /// El warning de PhysX sobre triángulos >500 unidades es NORMAL y esperado
    /// para tiles de terreno real — no afecta al gameplay.
    /// </summary>
    private void CargarTilesetTerreno()
    {
        tilesetTerreno = new GameObject("Terreno_CesiumWorld");
        tilesetTerreno.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetTerreno.AddComponent<Cesium3DTileset>();
        tileset.ionAssetID = 1;
        if (!string.IsNullOrEmpty(tokenCesiumIon))
            tileset.ionAccessToken = tokenCesiumIon;

        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.preloadAncestors        = true;
        tileset.preloadSiblings         = true;
        tileset.createPhysicsMeshes     = true;   // NECESARIO: el jugador camina sobre el terreno

        Debug.Log("[Alsasua] Cesium World Terrain cargado (ionAssetID=1). Física habilitada para colisión de suelo.");
    }

    /// <summary>
    /// Carga Google Photorealistic 3D Tiles.
    /// Incluye fachadas, tejados, árboles y terreno fotogramétrico de Alsasua.
    /// Física habilitada para colisión tanto con edificios como con el suelo.
    /// Requiere API Key de Google Maps Platform con "Map Tiles API" activado.
    /// </summary>
    private void CargarGooglePhotorealistic3DTiles()
    {
        tilesetEdificios = new GameObject("Google_Photorealistic3DTiles");
        tilesetEdificios.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetEdificios.AddComponent<Cesium3DTileset>();
        tileset.url = $"https://tile.googleapis.com/v1/3dtiles/root.json?key={apiKeyGoogle}";
        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.preloadAncestors        = true;
        tileset.preloadSiblings         = true;
        tileset.createPhysicsMeshes     = true;   // Colisión con edificios y terreno Google
        tileset.showCreditsOnScreen     = true;   // Obligatorio por licencia de Google

        Debug.Log("[Alsasua] ✓ Google Photorealistic 3D Tiles cargados — fachadas y tejados reales.");
    }

    /// <summary>
    /// Fallback: edificios 3D de OpenStreetMap (sin texturas fotográficas).
    /// </summary>
    private void CargarEdificiosOSM()
    {
        tilesetOSM = new GameObject("OSM_Buildings_Fallback");
        tilesetOSM.transform.parent = georeference.transform;

        Cesium3DTileset tileset = tilesetOSM.AddComponent<Cesium3DTileset>();
        tileset.ionAssetID = 96188;
        if (!string.IsNullOrEmpty(tokenCesiumIon))
            tileset.ionAccessToken = tokenCesiumIon;

        tileset.maximumScreenSpaceError = maximumScreenSpaceError;
        tileset.createPhysicsMeshes     = false;  // Solo visual — física proviene del terreno

        Debug.Log("[Alsasua] Edificios OSM cargados (fallback).");
    }

    // ============================================================
    //  HUD DE DIAGNÓSTICO
    // ============================================================

    private void OnGUI()
    {
        CesiumForUnity.Cesium3DTileset[] tilesets =
            Object.FindObjectsByType<CesiumForUnity.Cesium3DTileset>(FindObjectsSortMode.None);

        int activos = 0;
        foreach (var t in tilesets)
            if (t.enabled && t.gameObject.activeSelf) activos++;

        float hudAltura = tilesets.Length * 20f + 52f;
        GUI.color = new Color(0f, 0f, 0f, 0.55f);
        GUI.DrawTexture(new Rect(8f, 8f, 380f, hudAltura), Texture2D.whiteTexture);

        GUI.color = Color.yellow;
        GUI.Label(new Rect(12f, 10f, 370f, 20f),
            $"Altsasu / Alsasua  |  Lat: {ALSASUA_LATITUD:F4}  Lon: {ALSASUA_LONGITUD:F4}");

        GUI.color = Color.cyan;
        GUI.Label(new Rect(12f, 30f, 370f, 20f),
            $"Tilesets activos: {activos}  |  Cámara Y: {(Camera.main != null ? Camera.main.transform.position.y.ToString("F0") : "?")} m");

        GUI.color = Color.white;
        for (int i = 0; i < tilesets.Length; i++)
        {
            var t = tilesets[i];
            if (!t.gameObject.activeSelf) continue;
            string src = !string.IsNullOrEmpty(t.url) ? "Google" : $"Ion:{t.ionAssetID}";
            string fisica = t.createPhysicsMeshes ? "[physics]" : "";
            GUI.Label(new Rect(12f, 50f + i * 20f, 370f, 20f),
                $"  {t.gameObject.name} [{src}] {fisica}");
        }
    }

    // ============================================================
    //  UTILIDADES INSPECTOR
    // ============================================================

    [ContextMenu("Reconfigurar escena")]
    public void ReconfigurarDesdeInspector()
    {
        if (tilesetTerreno   != null) DestroyImmediate(tilesetTerreno);
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
        Debug.Log("  Google tiene fotogrametría 3D de toda España");
        Debug.Log("  incluyendo Alsasua — fachadas y tejados reales");
        Debug.Log("════════════════════════════════════════════════");
    }
}
