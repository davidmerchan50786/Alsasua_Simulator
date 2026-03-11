using UnityEngine;
using CesiumForUnity;

/// <summary>
/// Aplica texturas de fachada realistas a los edificios de Cesium OSM Buildings.
///
/// FUNCIONAMIENTO:
///   Los edificios OSM (ionAssetID=96188) se cargan como polígonos 3D sin textura
///   (geometría blanca/gris plana). Este script les asigna un material con el
///   shader "Alsasua/FachadasEdificios" que usa mapeo triplanar en world-space,
///   de modo que el número de ventanas escala con el tamaño real del edificio.
///
/// TEXTURAS PROCEDURALES:
///   Se generan en código, sin necesidad de assets externos:
///   · Fachada residencial — ventanas rectangulares en pared beige cálida
///   · Fachada comercial   — ventanas panorámicas en pared gris clara
///   · Tejado              — superficie plana marrón grisáceo
///
///   El tileset OSM usa 'opaqueMaterial' de Cesium3DTileset para aplicar el
///   material a todos los tiles del tileset automáticamente.
/// </summary>
[AddComponentMenu("Alsasua/Texturizador Fachadas OSM")]
public class TexturizadorFachadasOSM : MonoBehaviour
{
    // ================================================================
    //  INSPECTOR
    // ================================================================

    [Header("═══ TIPO DE FACHADA ═══")]
    [Tooltip("Variante de textura para las paredes de los edificios")]
    [SerializeField] private TipoFachada tipoFachada = TipoFachada.Residencial;

    public enum TipoFachada
    {
        Residencial,   // Bloques de viviendas — ventanas medianas, pared beige
        Comercial,     // Locales y oficinas  — ventanas panorámicas, pared gris
        Industrial,    // Naves y fábricas    — ventanas pequeñas, pared ocre
    }

    [Header("═══ ESCALA DE TEXTURA ═══")]
    [Tooltip("Metros de edificio real que ocupa cada repetición de la textura. " +
             "Valores típicos: 3–5 m para pisos residenciales, 4–6 m para oficinas.")]
    [Range(2f, 10f)]
    [SerializeField] private float metrosPorTile = 4f;

    [Header("═══ COLORES (ajuste fino) ═══")]
    [SerializeField] private Color colorPared    = new Color(0.92f, 0.87f, 0.78f, 1f);
    [SerializeField] private Color colorVentana  = new Color(0.16f, 0.22f, 0.32f, 1f);
    [SerializeField] private Color colorMarco    = new Color(0.80f, 0.75f, 0.68f, 1f);
    [SerializeField] private Color colorTejado   = new Color(0.52f, 0.47f, 0.42f, 1f);

    [Header("═══ ILUMINACIÓN ═══")]
    [Tooltip("Porcentaje de luz ambiente (0 = solo luz solar, 1 = uniforme sin sombras)")]
    [Range(0.1f, 0.8f)]
    [SerializeField] private float luzAmbiente = 0.40f;

    [Header("═══ DEBUG ═══")]
    [Tooltip("Activa para ver en Consola qué tilesets se encontraron y texturizaron")]
    [SerializeField] private bool logDetallado = false;

    // ================================================================
    //  INTERNOS
    // ================================================================

    private Material materialFachadas;
    private Texture2D texFachada;
    private Texture2D texTejado;

    // Shader en Assets/Shaders/FachadasEdificios.shader
    private const string SHADER_NAME = "Alsasua/FachadasEdificios";

    // ================================================================
    //  CICLO DE VIDA
    // ================================================================

    private void Start()
    {
        // Buscar el shader. Si no compila todavía (primer import), avisa y cancela.
        var shader = Shader.Find(SHADER_NAME);
        if (shader == null)
        {
            Debug.LogError(
                "[FachadasOSM] Shader '" + SHADER_NAME + "' no encontrado.\n" +
                "Comprueba que Assets/Shaders/FachadasEdificios.shader esté en el proyecto " +
                "y que Unity haya re-importado los assets (menú Assets → Refresh).");
            return;
        }

        texFachada = GenerarTexturaPared();
        texTejado  = GenerarTexturaTejado();
        materialFachadas = ConstruirMaterial(shader);

        AplicarAEdificiosOSM();
    }

    private void OnDestroy()
    {
        // Limpiar texturas procedurales para evitar memory leaks
        if (texFachada  != null) Destroy(texFachada);
        if (texTejado   != null) Destroy(texTejado);
        if (materialFachadas != null) Destroy(materialFachadas);
    }

    // ================================================================
    //  ASIGNACIÓN AL TILESET OSM
    // ================================================================

    private void AplicarAEdificiosOSM()
    {
        int aplicados = 0;

        foreach (var tileset in Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None))
        {
            bool esOSM    = tileset.ionAssetID == 96188;
            bool esGoogle = !string.IsNullOrEmpty(tileset.url) && tileset.url.Contains("googleapis");

            // Solo aplicar a OSM (ionAssetID=96188) y a cualquier tileset Ion de edificios
            // que NO sea terreno (assetID=1) ni Google (URL con googleapis).
            bool esEdificio = esOSM || (!esGoogle && tileset.ionAssetID > 1 && tileset.ionAssetID != 2);

            if (!esEdificio) continue;

            // Asignar el material de fachada al tileset.
            // opaqueMaterial sobreescribe el material usado para todos los tiles opacos.
            tileset.opaqueMaterial = materialFachadas;
            aplicados++;

            if (logDetallado)
                Debug.Log($"[FachadasOSM] ✓ Material de fachada aplicado a '{tileset.gameObject.name}' " +
                          $"(assetID={tileset.ionAssetID}, tipo={tipoFachada}, tile={metrosPorTile}m).");
        }

        if (aplicados == 0)
            Debug.LogWarning(
                "[FachadasOSM] ⚠ No se encontraron tilesets de edificios OSM (ionAssetID=96188).\n" +
                "  · Si estás usando Google Photorealistic, los edificios ya tienen texturas reales.\n" +
                "  · Para OSM como fallback, activa 'Usar OSM Edificios Fallback' en ConfiguradorAlsasua.");
        else
            Debug.Log($"[FachadasOSM] ✓ Texturas de fachada aplicadas a {aplicados} tileset(s) de edificios.");
    }

    // ================================================================
    //  CONSTRUCCIÓN DEL MATERIAL
    // ================================================================

    private Material ConstruirMaterial(Shader shader)
    {
        var mat = new Material(shader);
        mat.name = "FachadasOSM_" + tipoFachada;

        mat.SetTexture("_WallTex",    texFachada);
        mat.SetTexture("_RoofTex",    texTejado);
        mat.SetFloat("_TileMetros",   metrosPorTile);
        mat.SetColor("_WallTint",     Color.white);   // Tinte ya está en la textura
        mat.SetColor("_RoofTint",     colorTejado);
        mat.SetFloat("_Ambient",      luzAmbiente);
        mat.SetFloat("_RoofBlend",    0.90f);

        return mat;
    }

    // ================================================================
    //  GENERACIÓN PROCEDURAL DE TEXTURAS
    // ================================================================

    /// <summary>
    /// Genera una textura 512×512 con patrón de fachada (pared + ventanas).
    /// El patrón varía según el TipoFachada seleccionado en el Inspector.
    /// </summary>
    private Texture2D GenerarTexturaPared()
    {
        const int W = 512, H = 512;
        Color[] px = new Color[W * H];

        // Parámetros por tipo de fachada
        int   ventX, ventW, ventH, marcoGrosor;
        float propVentY;
        Color pared, ventana, marco;

        switch (tipoFachada)
        {
            case TipoFachada.Comercial:
                ventX       = 2;     // 2 módulos horizontales (ventanas anchas)
                ventW       = 68;    // porcentaje ancho ventana
                ventH       = 58;
                propVentY   = 3.5f;  // 3.5 módulos verticales (piso comercial alto)
                marcoGrosor = 6;
                pared   = new Color(0.75f, 0.75f, 0.78f);  // gris azulado
                ventana = new Color(0.10f, 0.18f, 0.30f);
                marco   = new Color(0.62f, 0.62f, 0.65f);
                break;

            case TipoFachada.Industrial:
                ventX       = 3;
                ventW       = 40;
                ventH       = 28;
                propVentY   = 5f;    // pisos altos de nave industrial
                marcoGrosor = 4;
                pared   = new Color(0.82f, 0.78f, 0.68f);  // ocre
                ventana = new Color(0.20f, 0.24f, 0.26f);
                marco   = new Color(0.70f, 0.66f, 0.58f);
                break;

            default: // Residencial
                ventX       = 3;
                ventW       = 42;
                ventH       = 50;
                propVentY   = 4f;
                marcoGrosor = 5;
                pared   = colorPared;
                ventana = colorVentana;
                marco   = colorMarco;
                break;
        }

        // Tamaño de módulo (espacio por ventana)
        int modX = W / ventX;
        int modY = Mathf.RoundToInt(H / propVentY);

        // Tamaño real de ventana en píxeles
        int vw = Mathf.RoundToInt(modX * ventW / 100f);
        int vh = Mathf.RoundToInt(modY * ventH / 100f);

        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                int lx = x % modX;
                int ly = y % modY;

                // Centrar ventana en el módulo
                int ox = (modX - vw) / 2;
                int oy = (modY - vh) / 2;

                bool enMarco   = lx >= ox - marcoGrosor && lx <= ox + vw + marcoGrosor - 1
                              && ly >= oy - marcoGrosor && ly <= oy + vh + marcoGrosor - 1;
                bool enVentana = lx >= ox && lx < ox + vw
                              && ly >= oy && ly < oy + vh;

                Color c;
                if (enVentana)
                {
                    // Añadir reflejo sutil en la parte superior de la ventana
                    float gradV   = (float)(ly - oy) / vh;
                    float reflejo = Mathf.Lerp(0.20f, 0f, gradV * 3f);
                    c = Color.Lerp(ventana, Color.white, Mathf.Clamp01(reflejo));
                }
                else if (enMarco)
                    c = marco;
                else
                {
                    // Variación sutil de la pared para evitar efecto "plastificado"
                    float ruido = (Mathf.Sin(x * 0.31f) * Mathf.Cos(y * 0.27f)) * 0.03f;
                    c = new Color(
                        Mathf.Clamp01(pared.r + ruido),
                        Mathf.Clamp01(pared.g + ruido * 0.8f),
                        Mathf.Clamp01(pared.b + ruido * 0.6f));
                }

                px[y * W + x] = c;
            }
        }

        return BuildTexture(px, W, H, "FachadaPared");
    }

    /// <summary>
    /// Genera una textura 256×256 para el tejado plano (dirección Y en triplanar).
    /// </summary>
    private Texture2D GenerarTexturaTejado()
    {
        const int W = 256, H = 256;
        Color[] px = new Color[W * H];

        Color base   = new Color(0.45f, 0.40f, 0.36f);   // marrón grisáceo
        Color junta  = new Color(0.35f, 0.31f, 0.28f);   // junta más oscura
        int bloqueSz = 32;

        for (int y = 0; y < H; y++)
        {
            for (int x = 0; x < W; x++)
            {
                bool esJuntaX = (x % bloqueSz) < 2;
                bool esJuntaY = (y % bloqueSz) < 2;

                float ruido = (Mathf.Sin(x * 0.17f + y * 0.23f)) * 0.025f;
                Color c = (esJuntaX || esJuntaY) ? junta : base;
                c = new Color(
                    Mathf.Clamp01(c.r + ruido),
                    Mathf.Clamp01(c.g + ruido),
                    Mathf.Clamp01(c.b + ruido));

                px[y * W + x] = c;
            }
        }

        return BuildTexture(px, W, H, "TejadoPlano");
    }

    private static Texture2D BuildTexture(Color[] pixels, int w, int h, string nombre)
    {
        var tex = new Texture2D(w, h, TextureFormat.RGBA32, mipChain: true);
        tex.name       = nombre;
        tex.wrapMode   = TextureWrapMode.Repeat;
        tex.filterMode = FilterMode.Bilinear;
        tex.SetPixels(pixels);
        tex.Apply(updateMipmaps: true, makeNoLongerReadable: true);
        return tex;
    }

    // ================================================================
    //  API PÚBLICA
    // ================================================================

    /// <summary>Actualiza el material con los valores actuales del Inspector.</summary>
    [ContextMenu("Actualizar material ahora")]
    public void ActualizarMaterial()
    {
        if (materialFachadas == null) { Start(); return; }

        materialFachadas.SetFloat("_TileMetros", metrosPorTile);
        materialFachadas.SetColor("_RoofTint",   colorTejado);
        materialFachadas.SetFloat("_Ambient",     luzAmbiente);
        Debug.Log("[FachadasOSM] Material actualizado (metros/tile=" + metrosPorTile + ").");
    }
}
