using UnityEngine;
using CesiumForUnity;
using System.Collections;

/// <summary>
/// Gestor de tilesets fotorrealistas para Alsasua.
/// Controla en tiempo real la calidad, visibilidad y LOD de los tilesets
/// para mantener el balance entre rendimiento y realismo visual.
///
/// FACHADAS REALES: Se consiguen con Google Photorealistic 3D Tiles.
/// Este script ajusta automáticamente el nivel de detalle según
/// la distancia de la cámara para maximizar la calidad en el área visible.
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
    [Tooltip("Activar ajuste automático de LOD según la altura de la cámara")]
    [SerializeField] private bool calidadDinamica = true;

    [Tooltip("SSE cuando la cámara está cerca del suelo (< 100m) — mayor detalle")]
    [Range(2f, 16f)]
    [SerializeField] private float sseCercano = 4f;   // Alta calidad

    [Tooltip("SSE cuando la cámara está lejos (> 500m) — menor detalle")]
    [Range(8f, 64f)]
    [SerializeField] private float sseLejano = 24f;   // Menor calidad

    [Header("═══ UMBRALES DE ALTURA (metros) ═══")]
    [SerializeField] private float alturaVistaCercana = 100f;
    [SerializeField] private float alturaVistaLejana  = 500f;

    // ============================================================
    //  OPCIONES DE VISUALIZACIÓN
    // ============================================================
    [Header("═══ VISUALIZACIÓN ═══")]
    [Tooltip("Mostrar overlay de debug (altura de cámara y SSE actual)")]
    [SerializeField] private bool mostrarDebugOverlay = false;

    [Tooltip("Mostrar créditos de Google en pantalla (obligatorio por licencia)")]
    [SerializeField] private bool mostrarCreditosGoogle = true;

    // Referencia a la cámara principal
    private Camera camaraPrincipal;
    private float alturaAnterior = -1f;

    // ============================================================
    //  INICIALIZACIÓN
    // ============================================================

    private void Start()
    {
        camaraPrincipal = Camera.main;
        ConfigurarTilesetsIniciales();
        StartCoroutine(ActualizarCalidadDinamica());
    }

    private void ConfigurarTilesetsIniciales()
    {
        // Configurar Google Photorealistic
        if (tilesetGooglePhotorealistic != null)
        {
            tilesetGooglePhotorealistic.maximumScreenSpaceError = sseCercano;
            tilesetGooglePhotorealistic.showCreditsOnScreen     = mostrarCreditosGoogle;
            tilesetGooglePhotorealistic.preloadAncestors        = true;
            tilesetGooglePhotorealistic.preloadSiblings         = true;
            Debug.Log("[GestionTilesets] Google Photorealistic configurado.");
        }

        // Configurar terreno
        if (tilesetTerreno != null)
        {
            tilesetTerreno.maximumScreenSpaceError = sseCercano;
            tilesetTerreno.preloadAncestors        = true;
            Debug.Log("[GestionTilesets] Terreno configurado.");
        }

        // Ocultar OSM si tenemos Google (evitar doble geometría)
        if (tilesetOSM != null && tilesetGooglePhotorealistic != null)
        {
            tilesetOSM.gameObject.SetActive(false);
            Debug.Log("[GestionTilesets] OSM desactivado — usando Google Photorealistic.");
        }
    }

    // ============================================================
    //  CALIDAD DINÁMICA
    // ============================================================

    /// <summary>
    /// Coroutine que cada 0.5s ajusta el SSE según la altura de la cámara.
    /// Más cerca del suelo = más detalle = fachadas más nítidas.
    /// </summary>
    private IEnumerator ActualizarCalidadDinamica()
    {
        while (true)
        {
            yield return new WaitForSeconds(0.5f);

            if (!calidadDinamica || camaraPrincipal == null)
                continue;

            float alturaCamara = camaraPrincipal.transform.position.y;

            // Solo actualizar si la altura cambió significativamente
            if (Mathf.Abs(alturaCamara - alturaAnterior) < 5f)
                continue;

            alturaAnterior = alturaCamara;

            // Calcular SSE interpolado según altura
            float t = Mathf.InverseLerp(alturaVistaCercana, alturaVistaLejana, alturaCamara);
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
    //  API PÚBLICA
    // ============================================================

    /// <summary>
    /// Forzar máxima calidad de fachadas — útil al tomar capturas.
    /// </summary>
    public void ForzarMaximaCalidad()
    {
        calidadDinamica = false;
        AplicarSSE(2f);
        Debug.Log("[GestionTilesets] Calidad máxima activada (SSE = 2).");
    }

    /// <summary>
    /// Restaurar calidad dinámica automática.
    /// </summary>
    public void RestaurarCalidadDinamica()
    {
        calidadDinamica = true;
        Debug.Log("[GestionTilesets] Calidad dinámica restaurada.");
    }

    /// <summary>
    /// Alternar entre Google Photorealistic y OSM.
    /// Útil para comparar cobertura.
    /// </summary>
    public void AlternarFuenteEdificios()
    {
        bool googleActivo = tilesetGooglePhotorealistic != null &&
                            tilesetGooglePhotorealistic.gameObject.activeSelf;

        if (tilesetGooglePhotorealistic != null)
            tilesetGooglePhotorealistic.gameObject.SetActive(!googleActivo);

        if (tilesetOSM != null)
            tilesetOSM.gameObject.SetActive(googleActivo);

        string fuente = googleActivo ? "OSM (fallback)" : "Google Photorealistic";
        Debug.Log($"[GestionTilesets] Fuente de edificios: {fuente}");
    }

    // ============================================================
    //  GIZMOS DE DEBUG
    // ============================================================

    private void OnGUI()
    {
        if (!mostrarDebugOverlay)
            return;

        float alturaCam = camaraPrincipal != null ? camaraPrincipal.transform.position.y : 0f;
        float t         = Mathf.InverseLerp(alturaVistaCercana, alturaVistaLejana, alturaCam);
        float sseActual = Mathf.Lerp(sseCercano, sseLejano, t);

        GUI.color = Color.yellow;
        GUI.Label(new Rect(10, 10, 400, 20),
            $"[Tilesets] Altura: {alturaCam:F0} m  |  SSE: {sseActual:F1}  |  Calidad dinámica: {calidadDinamica}");
    }
}
