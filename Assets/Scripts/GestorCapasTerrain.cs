using UnityEngine;
using CesiumForUnity;

/// <summary>
/// Gestiona qué fuente de terreno está activa en cada momento.
///
/// MODOS DISPONIBLES:
///
///   Cesium   — tiles Cesium en streaming (Cesium World Terrain + raster overlay Bing Maps
///              o Google Photorealistic 3D Tiles). Requiere conexión a internet y tokens.
///              Idóneo para visualización fotorrealista o cuando no hay asset IGN disponible.
///
///   TerrainIGN — Unity Terrain estático generado desde el MDT del IGN.
///              No requiere internet. Física estándar de Unity (TerrainCollider).
///              Idóneo para builds offline, tests locales o builds de plataformas
///              sin soporte de Cesium (WebGL, consolas).
///
///   Ambos   — Cesium para fondos lejanos (LOD) + TerrainIGN para la zona central.
///              Útil para combinar calidad de la zona jugable con contexto global.
///              NOTA: ajustar el CullingMask de la cámara y los Near/Far Clip Planes
///              para evitar z-fighting entre ambas capas en la transición.
///
/// REGLAS DE COMPATIBILIDAD (igual que en ConfiguradorAlsasua):
///   · Google Photorealistic + CWT → desactivar CWT (z-fighting)
///   · Google Photorealistic + OSM → desactivar OSM (duplicados)
///   · TerrainIGN + CWT            → OK (capas independientes, sin solapamiento de malla)
///   · TerrainIGN + Google         → OK si Google no cubre la zona, o desactivar TerrainIGN
/// </summary>
public class GestorCapasTerrain : MonoBehaviour
{
    // ── Modos ─────────────────────────────────────────────────────────────────
    public enum ModoTerrain { Cesium, TerrainIGN, Ambos }

    // ── Inspector ─────────────────────────────────────────────────────────────
    [Header("═══ MODO DE TERRENO ═══")]
    [Tooltip("Cesium = streaming online | TerrainIGN = asset estático offline | Ambos = combinado")]
    [SerializeField] private ModoTerrain modo = ModoTerrain.TerrainIGN;

    [Header("Referencias (opcional — se auto-detectan si están vacías)")]
    [Tooltip("El Unity Terrain generado desde el MDT del IGN")]
    [SerializeField] private Terrain terrainIGN;

    [Tooltip("El tileset de Cesium World Terrain (ionAssetID=1)")]
    [SerializeField] private Cesium3DTileset tilesetTerreno;

    [Tooltip("El tileset de Google Photorealistic (URL con googleapis)")]
    [SerializeField] private Cesium3DTileset tilesetGoogle;

    [Tooltip("El tileset de edificios OSM (ionAssetID=96188)")]
    [SerializeField] private Cesium3DTileset tilesetOSM;

    [Header("Modo 'Ambos' — distancia de transición")]
    [Tooltip("Distancia (m Unity) a partir de la cual se activa Cesium World Terrain\n" +
             "como fondo lejano. El TerrainIGN cubre la zona cercana.")]
    [SerializeField] private float distanciaFondoCesium = 8000f;

    // ── Estado interno ────────────────────────────────────────────────────────
    private ModoTerrain modoActivo;

    // ─────────────────────────────────────────────────────────────────────────
    //  CICLO DE VIDA
    // ─────────────────────────────────────────────────────────────────────────
    private void Start()
    {
        AutoDetectarReferencias();
        AplicarModo(modo);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  AUTO-DETECCIÓN DE REFERENCIAS
    // ─────────────────────────────────────────────────────────────────────────
    private void AutoDetectarReferencias()
    {
        // TerrainIGN: componente Terrain en este mismo GameObject
        if (terrainIGN == null)
            terrainIGN = GetComponent<Terrain>();

        if (terrainIGN == null)
            Debug.LogWarning("[GestorCapasTerrain] No se encontró un componente Terrain en este GameObject. " +
                             "Asígnalo manualmente en el Inspector.");

        // Tilesets Cesium
        if (tilesetTerreno == null || tilesetGoogle == null || tilesetOSM == null)
        {
            var todos = FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
            foreach (var t in todos)
            {
                bool esGoogle = !string.IsNullOrEmpty(t.url) && t.url.Contains("googleapis");
                bool esGoogleIon = t.ionAssetID == 2275207;

                if (tilesetGoogle  == null && (esGoogle || esGoogleIon)) tilesetGoogle  = t;
                else if (tilesetTerreno == null && t.ionAssetID == 1)    tilesetTerreno = t;
                else if (tilesetOSM    == null && t.ionAssetID == 96188) tilesetOSM    = t;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  APLICAR MODO
    // ─────────────────────────────────────────────────────────────────────────
    /// <summary>
    /// Activa / desactiva las capas de terreno según el modo elegido.
    /// Llama a este método desde código o desde botones de UI en runtime.
    /// </summary>
    public void AplicarModo(ModoTerrain nuevoModo)
    {
        modoActivo = nuevoModo;

        switch (nuevoModo)
        {
            case ModoTerrain.TerrainIGN:
                SetTerrainIGN(true);
                SetCesiumTerreno(false);
                SetGoogle(false);
                SetOSM(false);
                Debug.Log("[GestorCapasTerrain] Modo: Unity Terrain IGN (offline estático)");
                break;

            case ModoTerrain.Cesium:
                SetTerrainIGN(false);
                // La lógica Google vs CWT es responsabilidad de ConfiguradorAlsasua/GestionTilesets.
                // Aquí solo activamos lo que existe; ellos ya habrán resuelto el conflicto.
                SetCesiumTerreno(true);
                SetGoogle(true);
                SetOSM(tilesetGoogle == null || !tilesetGoogle.gameObject.activeSelf);
                Debug.Log("[GestorCapasTerrain] Modo: Cesium (streaming online)");
                break;

            case ModoTerrain.Ambos:
                SetTerrainIGN(true);
                // CWT activo como fondo lejano; Google puede estar activo si tiene cobertura.
                // Para evitar z-fighting en la zona central, el TerrainIGN debe tener
                // renderQueue más alto o usar capas de renderizado separadas.
                SetCesiumTerreno(true);
                SetGoogle(true);
                SetOSM(tilesetGoogle == null || !tilesetGoogle.gameObject.activeSelf);
                Debug.Log("[GestorCapasTerrain] Modo: Ambos (TerrainIGN central + Cesium fondo)");
                break;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  HELPERS DE ACTIVACIÓN
    // ─────────────────────────────────────────────────────────────────────────
    private void SetTerrainIGN(bool activo)
    {
        if (terrainIGN != null)
            terrainIGN.gameObject.SetActive(activo);
    }

    private void SetCesiumTerreno(bool activo)
    {
        if (tilesetTerreno != null)
            tilesetTerreno.gameObject.SetActive(activo);
    }

    private void SetGoogle(bool activo)
    {
        if (tilesetGoogle != null)
            tilesetGoogle.gameObject.SetActive(activo);
    }

    private void SetOSM(bool activo)
    {
        if (tilesetOSM != null)
            tilesetOSM.gameObject.SetActive(activo);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ─────────────────────────────────────────────────────────────────────────
    /// <summary>Activa únicamente el Unity Terrain estático (sin internet requerido).</summary>
    public void UsarTerrainIGN()   => AplicarModo(ModoTerrain.TerrainIGN);

    /// <summary>Activa únicamente los tiles Cesium en streaming.</summary>
    public void UsarCesium()       => AplicarModo(ModoTerrain.Cesium);

    /// <summary>Alterna entre TerrainIGN y Cesium.</summary>
    public void AlternarModo()
    {
        AplicarModo(modoActivo == ModoTerrain.TerrainIGN ? ModoTerrain.Cesium : ModoTerrain.TerrainIGN);
    }

    /// <returns>El modo de terreno actualmente activo.</returns>
    public ModoTerrain ModoActual => modoActivo;

    // ─────────────────────────────────────────────────────────────────────────
    //  TECLA DE DESARROLLO (solo en Editor / Debug builds)
    // ─────────────────────────────────────────────────────────────────────────
#if UNITY_EDITOR || DEVELOPMENT_BUILD
    private void Update()
    {
        // F9 → alternar entre TerrainIGN y Cesium mientras se prueba en Play Mode
        if (Input.GetKeyDown(KeyCode.F9))
        {
            AlternarModo();
            Debug.Log($"[GestorCapasTerrain] F9 → modo: {modoActivo}");
        }
    }
#endif
}
