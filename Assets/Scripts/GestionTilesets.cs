using UnityEngine;
using CesiumForUnity;
using System.Collections;

/// <summary>
/// Gestor de tilesets fotorrealistas para Pamplona.
/// Controla en tiempo real la calidad, visibilidad y LOD de los tilesets
/// para mantener el balance entre rendimiento y realismo visual.
///
/// FACHADAS REALES: Google Photorealistic 3D Tiles cubre Pamplona con
/// fotogrametría aérea completa — edificios, calles, vegetación y terreno.
/// OsmEdificioLoader NO es necesario; Google 3D es la única fuente de edificios.
///
/// Este script ajusta automáticamente el nivel de detalle según
/// la distancia del JUGADOR (no la cámara dron) para maximizar
/// la calidad en el área visible desde el suelo.
/// </summary>
public class GestionTilesets : MonoBehaviour
{
    // ============================================================
    //  REFERENCIAS A TILESETS
    // ============================================================
    [Header("═══ TILESETS ═══")]
    [Tooltip("Google Photorealistic 3D Tiles (fachadas y tejados reales)")]
    [SerializeField] private Cesium3DTileset tilesetGooglePhotorealistic;

    [Tooltip("Cesium World Terrain (relieve real)")]
    [SerializeField] private Cesium3DTileset tilesetTerreno;

    [Tooltip("Edificios OSM (fallback sin texturas reales)")]
    [SerializeField] private Cesium3DTileset tilesetOSM;

    // ============================================================
    //  CONFIGURACIÓN DE CALIDAD
    // ============================================================
    [Header("═══ CALIDAD DINÁMICA ═══")]
    [Tooltip("Activar ajuste automático de LOD según la altura del observador")]
    [SerializeField] private bool calidadDinamica = true;

    [Tooltip("SSE cuando el observador está cerca del suelo (< 100m).\n" +
             "Pamplona: Google 3D Tiles tiene cobertura completa — SSE 2 da máxima resolución de fachadas.")]
    [Range(2f, 16f)]
    [SerializeField] private float sseCercano = 2f;   // Pamplona: Google 3D disponible → máxima calidad

    [Tooltip("SSE cuando el observador está lejos (> 500m).\n" +
             "Pamplona es ciudad grande — mantener 12 (vs 16 de Alsasua) para ver más tiles a distancia.")]
    [Range(8f, 64f)]
    [SerializeField] private float sseLejano = 12f;   // Pamplona: rango urbano mayor, más tiles visibles

    [Header("═══ UMBRALES DE ALTURA (metros) ═══")]
    [SerializeField] private float alturaVistaCercana = 100f;
    [SerializeField] private float alturaVistaLejana  = 500f;

    // ============================================================
    //  OPCIONES DE VISUALIZACIÓN
    // ============================================================
    [Header("═══ VISUALIZACIÓN ═══")]
    [Tooltip("Mostrar overlay de debug (altura y SSE actual)")]
    [SerializeField] private bool mostrarDebugOverlay = false;

    // LICENCIA: showCreditsOnScreen es OBLIGATORIO por los Términos de Servicio de
    // Google Maps Platform (sección "Attribution requirements").
    // NO se expone como campo serializable para evitar que se desactive accidentalmente
    // desde el Inspector — siempre forzado a true en ConfigurarTilesetsIniciales().
    // Ref: https://developers.google.com/maps/documentation/tile/policies
    private const bool CREDITOS_GOOGLE_OBLIGATORIOS = true;

    // BUG FIX: usamos el Transform del Jugador, NO de Camera.main.
    // Camera.main es la cámara dron a Y=1500 → InverseLerp(100,500,1500)=1 → sseLejano siempre
    // → calidad mínima aunque el jugador esté al nivel del suelo.
    // Con el Jugador a Y≈1: InverseLerp(100,500,1)≈0 → sseCercano → máxima calidad al suelo.
    private Transform observador;
    private float     alturaAnterior = -1f;

    // ============================================================
    //  INICIALIZACIÓN
    // ============================================================

    private void Start()
    {
        // Buscar el Jugador (la referencia correcta para altura)
        var controlador = Object.FindFirstObjectByType<ControladorJugador>();
        if (controlador != null)
        {
            observador = controlador.transform;
            AlsasuaLogger.Info("GestionTilesets", "✓ Usando posición del Jugador (Y≈1) para calidad dinámica — máxima resolución al nivel del suelo.");
        }
        else
        {
            // Fallback: cualquier cámara activa (mejor que Camera.main a Y=1500)
            observador = transform;
            AlsasuaLogger.Warn("GestionTilesets", "Jugador no encontrado — usando posición del Manager para calidad dinámica.");
        }

        ConfigurarTilesetsIniciales();
        StartCoroutine(ActualizarCalidadDinamica());
    }

    private void ConfigurarTilesetsIniciales()
    {
        // Buscar tilesets dinámicamente si no están asignados en Inspector
        if (tilesetGooglePhotorealistic == null || tilesetTerreno == null)
        {
            var todos = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
            foreach (var t in todos)
            {
                if (tilesetGooglePhotorealistic == null &&
                    (!string.IsNullOrEmpty(t.url) || t.ionAssetID == 2275207))
                    tilesetGooglePhotorealistic = t;
                else if (tilesetTerreno == null && t.ionAssetID == 1)
                    tilesetTerreno = t;
                else if (tilesetOSM == null && t.ionAssetID == 96188)
                    tilesetOSM = t;
            }
        }

        // Configurar Google Photorealistic
        // Pamplona: cobertura completa con fotogrametría aérea → precargar vecinos para scroll suave
        if (tilesetGooglePhotorealistic != null)
        {
            tilesetGooglePhotorealistic.maximumScreenSpaceError = sseCercano;
            tilesetGooglePhotorealistic.showCreditsOnScreen     = CREDITOS_GOOGLE_OBLIGATORIOS;
            tilesetGooglePhotorealistic.preloadAncestors        = true;
            tilesetGooglePhotorealistic.preloadSiblings         = true;
            AlsasuaLogger.Info("GestionTilesets", $"Google Photorealistic 3D (Pamplona) configurado — SSE inicial: {sseCercano}. " +
                               "Fotogrametría completa: edificios, calles y vegetación.");
        }
        else
        {
            AlsasuaLogger.Error("GestionTilesets",
                "╔══════════════════════════════════════════════════════════════╗\n" +
                "║  ❌ GOOGLE PHOTOREALISTIC 3D TILES NO ENCONTRADO             ║\n" +
                "║  Pamplona tiene cobertura Google 3D — configura la API Key:  ║\n" +
                "║  Inspector → ConfiguradorAlsasua → API Key Google           ║\n" +
                "╚══════════════════════════════════════════════════════════════╝");
        }

        // Configurar terreno
        if (tilesetTerreno != null)
        {
            tilesetTerreno.maximumScreenSpaceError = sseCercano;
            tilesetTerreno.preloadAncestors        = true;
            tilesetTerreno.preloadSiblings         = true;
            AlsasuaLogger.Info("GestionTilesets", "Cesium World Terrain configurado (colisión de suelo).");
        }

        // Ocultar OSM si tenemos Google (evitar doble geometría)
        if (tilesetOSM != null && tilesetGooglePhotorealistic != null)
        {
            tilesetOSM.gameObject.SetActive(false);
            AlsasuaLogger.Info("GestionTilesets", "OSM desactivado — usando Google Photorealistic.");
        }
    }

    // ============================================================
    //  CALIDAD DINÁMICA
    // ============================================================

    /// <summary>
    /// Coroutine que cada 0.5s ajusta el SSE según la altura del observador.
    /// Más cerca del suelo = más detalle = fachadas más nítidas.
    /// </summary>
    private IEnumerator ActualizarCalidadDinamica()
    {
        while (true)
        {
            yield return new WaitForSeconds(0.5f);

            // FIX: guard post-yield — si el GO fue destruido durante el sleep, salir limpiamente.
            // Sin esto, la coroutine intenta acceder a tilesets ya destruidos y lanza MissingReferenceException.
            if (!this) yield break;

            if (!calidadDinamica || observador == null)
                continue;

            float alturaObservador = observador.position.y;

            // Solo actualizar si la altura cambió significativamente
            if (Mathf.Abs(alturaObservador - alturaAnterior) < 5f)
                continue;

            alturaAnterior = alturaObservador;

            // Guardia: evitar división por cero en InverseLerp si ambos umbrales son iguales
            if (Mathf.Approximately(alturaVistaCercana, alturaVistaLejana))
            {
                AplicarSSE(sseCercano);
                continue;
            }

            // Calcular SSE interpolado según altura (clampeado 0-1 automáticamente)
            float t       = Mathf.InverseLerp(alturaVistaCercana, alturaVistaLejana, alturaObservador);
            float sseActual = Mathf.Lerp(sseCercano, sseLejano, t);

            AplicarSSE(sseActual);
        }
    }

    private void AplicarSSE(float sse)
    {
        if (tilesetGooglePhotorealistic != null)
            tilesetGooglePhotorealistic.maximumScreenSpaceError = sse;

        if (tilesetTerreno != null)
            tilesetTerreno.maximumScreenSpaceError = sse;

        if (tilesetOSM != null && tilesetOSM.gameObject.activeSelf)
            tilesetOSM.maximumScreenSpaceError = sse;
    }

    // ============================================================
    //  CICLO DE VIDA
    // ============================================================

    private void OnDestroy()
    {
        // BUG 28 FIX: detener la coroutine al destruir el componente.
        // Sin esto, la coroutine sigue ejecutándose un frame más después de la destrucción
        // y puede intentar acceder a referencias nulas (tilesets ya destruidos),
        // generando MissingReferenceException en Unity.
        StopAllCoroutines();
    }

    // ============================================================
    //  API PÚBLICA
    // ============================================================

    /// <summary>Forzar máxima calidad de fachadas — útil al tomar capturas.</summary>
    public void ForzarMaximaCalidad()
    {
        calidadDinamica = false;
        AplicarSSE(2f);
        AlsasuaLogger.Info("GestionTilesets", "Calidad máxima activada (SSE = 2).");
    }

    /// <summary>Restaurar calidad dinámica automática.</summary>
    public void RestaurarCalidadDinamica()
    {
        calidadDinamica = true;
        AlsasuaLogger.Info("GestionTilesets", "Calidad dinámica restaurada.");
    }

    /// <summary>Alternar entre Google Photorealistic y OSM.</summary>
    public void AlternarFuenteEdificios()
    {
        bool googleActivo = tilesetGooglePhotorealistic != null &&
                            tilesetGooglePhotorealistic.gameObject.activeSelf;

        if (tilesetGooglePhotorealistic != null)
            tilesetGooglePhotorealistic.gameObject.SetActive(!googleActivo);

        if (tilesetOSM != null)
            tilesetOSM.gameObject.SetActive(googleActivo);

        string fuente = googleActivo ? "OSM (fallback)" : "Google Photorealistic";
        AlsasuaLogger.Info("GestionTilesets", $"Fuente de edificios: {fuente}");
    }

    // ============================================================
    //  DEBUG OVERLAY
    // ============================================================

    private void OnGUI()
    {
        if (!mostrarDebugOverlay) return;

        float alturaObs = observador != null ? observador.position.y : 0f;
        float t = Mathf.Approximately(alturaVistaCercana, alturaVistaLejana) ? 0f
            : Mathf.InverseLerp(alturaVistaCercana, alturaVistaLejana, alturaObs);
        float sseActual = Mathf.Lerp(sseCercano, sseLejano, t);

        GUI.color = Color.yellow;
        GUI.Label(new Rect(10, 10, 500, 20),
            $"[Tilesets] Observador Y: {alturaObs:F0} m  |  SSE: {sseActual:F1}  |  Calidad dinámica: {calidadDinamica}");
    }
}
