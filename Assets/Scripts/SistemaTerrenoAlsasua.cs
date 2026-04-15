// Assets/Scripts/SistemaTerrenoAlsasua.cs
// ═══════════════════════════════════════════════════════════════════════════
//  GENERADOR DE TERRENO — VALLE DE BURUNDA / ALSASUA
//
//  Crea un Unity Terrain que reproduce la topografía real del Valle de Burunda:
//
//    · Fondo de valle plano a Y=0 (536 m s.n.m. en el mundo real)
//    · Crestas montañosas derivadas de los datos de GeoDataAlsasua:
//        – Monte Artia / ladera O    ~200 m sobre el valle
//        – Peña Blanca / Askiz NE    ~250 m sobre el valle
//        – Corredor norte (Bidasoa)  ~120 m (suave, abierto)
//        – Cierre sur (Lizarraga)    ~100 m (suave, corredor N-1)
//    · Ruido Perlin multicapa para irregularidades naturales
//    · El renderer del Terrain puede deshabilitarse cuando Cesium 3D Tiles
//      cubre la zona, manteniendo solo el collider para physics.
//
//  INTEGRACIÓN CON CESIUM:
//    · El terreno se coloca en Y = -1 para NO interferir con los meshes
//      de colisión de Cesium cuando estén cargados.
//    · Activar 'soloPhysics' para ocultar el renderer (Cesium provee visual).
//    · Activar 'soloVisual' para usar como fallback cuando los tiles no cargan.
//
//  ESCALA:
//    · Tamaño: 4096 m × 4096 m (cubre todo el valle + montañas visibles)
//    · Resolución heightmap: 513 × 513 (defecto Unity, buena calidad)
//    · Altura máxima: 600 m (suficiente para Peña Blanca + margen)
//
//  USO EN EDITOR:
//    Añadir este componente a un GO vacío. En Start() genera el terreno.
//    Para regenerarlo: deshabilita y vuelve a habilitar el componente.
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;
using System.IO;
#if UNITY_EDITOR
using UnityEditor;
#endif

[AddComponentMenu("Alsasua/Sistema Terreno Alsasua")]
public sealed class SistemaTerrenoAlsasua : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  INSPECTOR
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ TAMAÑO DEL TERRENO ═══")]
    [Tooltip("Extensión del terreno en X y Z (metros). 4096 cubre todo el valle de Burunda.")]
    [SerializeField] private float tamanoTerreno     = 4096f;
    [Tooltip("Altura máxima del terreno en metros. 600 m basta para los montes del entorno.")]
    [SerializeField] private float alturaMaxima       = 600f;
    [Tooltip("Resolución del heightmap (debe ser 2^n+1). 513 = buena calidad, 257 = rápido.")]
    [SerializeField] private int   resolucionHeightmap = 513;

    [Header("═══ MONTES Y RELIEVES ═══")]
    [Tooltip("Intensidad de las montañas (0=plano, 1=altura máxima real).")]
    [Range(0f, 1f)]
    [SerializeField] private float intensidadMontes  = 1f;
    [Tooltip("Anchura del fondo plano del valle (metros). El terreno es completamente plano dentro de este radio.")]
    [SerializeField] private float radioValleFlat    = 300f;
    [Tooltip("Suavizado de la transición valle-montaña. Mayor = transición más gradual.")]
    [Range(0.5f, 3f)]
    [SerializeField] private float suavizadoFalda    = 1.4f;

    [Header("═══ RUIDO PERLIN ═══")]
    [Tooltip("Amplitud del ruido Perlin de baja frecuencia (colinas suaves).")]
    [Range(0f, 0.15f)]
    [SerializeField] private float amplitudRuidoBajo  = 0.06f;
    [Tooltip("Amplitud del ruido Perlin de alta frecuencia (irregularidades pequeñas).")]
    [Range(0f, 0.05f)]
    [SerializeField] private float amplitudRuidoAlto  = 0.015f;
    [Tooltip("Semilla para el ruido Perlin (0 = aleatorio).")]
    [SerializeField] private int   semillaRuido       = 42;

    [Header("═══ TEXTURIZACIÓN ═══")]
    [Tooltip("Número de capas de textura: 0 = sin texturizar (solo heightmap).")]
    [SerializeField] private bool  texturizarTerreno  = true;
    [Tooltip("Color del fondo del valle (pradera).")]
    [SerializeField] private Color colorValle    = new Color(0.40f, 0.55f, 0.18f);
    [Tooltip("Color de las laderas (hierba y roca).")]
    [SerializeField] private Color colorLadera   = new Color(0.35f, 0.45f, 0.15f);
    [Tooltip("Color de las cimas y crestas (roca).")]
    [SerializeField] private Color colorCima     = new Color(0.50f, 0.48f, 0.38f);

    [Header("═══ HEIGHTMAP RAW (IDENA / EXTERNO) ═══")]
    [Tooltip("Si es true, intenta cargar el heightmap desde 'rutaRAW' antes de generar proceduralmente. " +
             "Útil cuando ya se ha ejecutado DescargadorIDENA.py.")]
    [SerializeField] private bool   cargarDesdeRAW  = true;
    [Tooltip("Ruta relativa desde Application.dataPath al archivo .raw (16-bit, little-endian). " +
             "Defecto: Terrain/alsasua_heightmap.raw  →  Assets/Terrain/alsasua_heightmap.raw")]
    [SerializeField] private string rutaRAW         = "Terrain/alsasua_heightmap.raw";
    [Tooltip("Si es true y el RAW se cargó correctamente, omite completamente el cálculo procedural " +
             "(más rápido en Start). Si es false, suma el ruido Perlin al RAW.")]
    [SerializeField] private bool   usarRawPuro     = true;

    [Header("═══ INTEGRACIÓN CESIUM ═══")]
    [Tooltip("Solo mantener physics collider (deshabilita renderer). " +
             "Usar cuando Cesium 3D Tiles provee el visual.")]
    [SerializeField] private bool  soloPhysics        = false;
    [Tooltip("Desactivar el collider del terreno (útil cuando Cesium ya genera mesh collision).")]
    [SerializeField] private bool  deshabilitarCollider = false;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO
    // ═══════════════════════════════════════════════════════════════════════

    private Terrain     _terrain;
    private TerrainData _terrainData;
    private float       _semillaX, _semillaZ;   // offsets para Perlin
    internal bool       _rawCargado;             // true si el heightmap vino de un archivo RAW (leído por Inspector nested class)

    // ═══════════════════════════════════════════════════════════════════════
    //  DEFINICIÓN DE RELIEVES (crestas del valle de Burunda)
    //
    //  Cada cresta se modela como una "loma gaussiana" en el mapa de alturas.
    //  Parámetros:
    //    centro   — posición Unity del pico
    //    altRel   — altura sobre el valle en metros
    //    rx, rz   — radio de influencia en cada eje (metros)
    //    angulo   — rotación de la elipse en grados (si la cresta es oblicua)
    // ═══════════════════════════════════════════════════════════════════════

    private struct Cresta
    {
        public Vector2 Centro;   // X, Z en metros Unity
        public float   AltRel;   // metros sobre el valle
        public float   Rx;       // radio E-O
        public float   Rz;       // radio N-S
        public float   Angulo;   // rotación elipse en grados (0 = ortogonal)
    }

    // Crestas derivadas de GeoDataAlsasua + topografía real del Valle de Burunda.
    // Posiciones en metros locales desde Herriko Plaza.
    private static readonly Cresta[] CrestasBurunda = new Cresta[]
    {
        // ── OESTE — Monte Artia, ladera Ibarrea (bloquea el O del valle)
        new Cresta { Centro = new Vector2(-700f, 200f),  AltRel = 220f, Rx = 550f, Rz = 700f, Angulo = -15f },
        new Cresta { Centro = new Vector2(-900f, -200f), AltRel = 180f, Rx = 400f, Rz = 550f, Angulo =  10f },

        // ── NORESTE — Peña Blanca / Askiz (primera cresta visible desde pueblo)
        new Cresta { Centro = new Vector2( 750f,  650f), AltRel = 260f, Rx = 500f, Rz = 700f, Angulo =  20f },

        // ── NORTE — Corredor Bidasoa, suave elevación (valle se estrecha)
        new Cresta { Centro = new Vector2(  50f, 1400f), AltRel = 130f, Rx = 900f, Rz = 600f, Angulo =   0f },

        // ── NORNOROESTE — Ladera norte cierre O (hacia Arakil)
        new Cresta { Centro = new Vector2(-600f,  900f), AltRel = 200f, Rx = 500f, Rz = 600f, Angulo = -25f },

        // ── SUR — Cierre sur del valle (hacia Puerto Lizarraga / Navarra)
        new Cresta { Centro = new Vector2(   0f,-1600f), AltRel = 120f, Rx = 800f, Rz = 500f, Angulo =   0f },

        // ── SUROESTE — Ladera sur-oeste (hacia polígono Isasia, cota alta)
        new Cresta { Centro = new Vector2(-1100f,-1400f), AltRel = 200f, Rx = 600f, Rz = 700f, Angulo =  30f },

        // ── ESTE — Cierre este antes de la vía del tren (colina suave)
        new Cresta { Centro = new Vector2( 900f, -400f), AltRel = 160f, Rx = 450f, Rz = 600f, Angulo =  10f },

        // ── SURESTE — Hacia Aralar (alejado, cota baja en este sector)
        new Cresta { Centro = new Vector2(1200f, 1200f), AltRel = 190f, Rx = 700f, Rz = 900f, Angulo =  35f },
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Start()
    {
        GenerarTerreno();
    }

    private void OnDestroy()
    {
        if (_terrainData != null)
        {
            if (Application.isPlaying) Destroy(_terrainData);
            else                       DestroyImmediate(_terrainData);
            _terrainData = null;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  GENERACIÓN PRINCIPAL
    // ═══════════════════════════════════════════════════════════════════════

    public void GenerarTerreno()
    {
        // Semillas Perlin (usadas aunque se cargue RAW, para el modo blend)
        int s = semillaRuido == 0 ? (int)System.DateTime.Now.Ticks : semillaRuido;
        _semillaX = (s * 0.137f) % 1000f;
        _semillaZ = (s * 0.317f) % 1000f;

        // ── Crear TerrainData ──────────────────────────────────────────────
        _terrainData = new TerrainData();
        _terrainData.heightmapResolution = resolucionHeightmap;
        _terrainData.size = new Vector3(tamanoTerreno, alturaMaxima, tamanoTerreno);

        // ── Intentar cargar RAW (IDENA o analítico exportado) ──────────────
        float[,] heights = null;
        _rawCargado = false;

        if (cargarDesdeRAW)
        {
            heights = IntentarCargarRAW(resolucionHeightmap);
            if (heights != null) _rawCargado = true;
        }

        // ── Si no hay RAW (o no se quiso), generar proceduralmente ─────────
        if (heights == null)
        {
            heights = GenerarHeightmap(resolucionHeightmap);
        }
        else if (!usarRawPuro)
        {
            // Modo blend: sumar ruido Perlin suave sobre el RAW (añade naturalidad
            // sin destruir la topografía real importada de IDENA).
            AñadirRuidoSobreRAW(heights, resolucionHeightmap);
        }

        _terrainData.SetHeights(0, 0, heights);

        // ── Texturizar (opcional) ──────────────────────────────────────────
        if (texturizarTerreno)
            AplicarTexturas(heights);

        // ── Crear o reusar el componente Terrain ──────────────────────────
        _terrain = GetComponent<Terrain>();
        if (_terrain == null)
            _terrain = gameObject.AddComponent<Terrain>();

        _terrain.terrainData = _terrainData;

        // El terreno de Unity va desde (0,0,0) a (size.x, size.y, size.z).
        // Lo centramos en el origen del mundo: esquina SW en (-half, -1, -half).
        // Y=-1 evita z-fighting con los meshes de Cesium a nivel de calle.
        float half = tamanoTerreno * 0.5f;
        transform.position = new Vector3(-half, -1f, -half);

        // ── Collider ──────────────────────────────────────────────────────
        var tc = GetComponent<TerrainCollider>() ?? gameObject.AddComponent<TerrainCollider>();
        tc.terrainData = _terrainData;
        tc.enabled = !deshabilitarCollider;

        // ── Modo physics-only ─────────────────────────────────────────────
        _terrain.enabled = !soloPhysics;

        string fuenteStr = _rawCargado
            ? $"RAW '{rutaRAW}'{(usarRawPuro ? "" : " + Perlin blend")}"
            : "procedural (Gaussianas + Perlin)";

        AlsasuaLogger.Info("SistemaTerrenoAlsasua",
            $"✓ Terreno Valle de Burunda: {tamanoTerreno:F0}m × {tamanoTerreno:F0}m, " +
            $"alt max {alturaMaxima:F0}m, res {resolucionHeightmap}², " +
            $"fuente={fuenteStr}, " +
            $"{(soloPhysics ? "solo-physics" : "renderer activo")}.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CARGA DE HEIGHTMAP RAW (16-bit little-endian, formato Unity)
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Lee el archivo .raw de 16 bits (unsigned, little-endian) generado por
    /// DescargadorIDENA.py y lo convierte a float[res,res] normalizado [0,1].
    ///
    /// Convenciones de Unity Terrain → Import Raw:
    ///   Depth: 16 bit   |  Byte order: Little Endian
    ///   Flip Vertically: NO  (Python escribe fila 0 = norte = Unity Z-max)
    ///
    /// Devuelve null si el archivo no existe o su tamaño no coincide.
    /// </summary>
    private float[,] IntentarCargarRAW(int res)
    {
        string rutaCompleta = Path.Combine(Application.dataPath, rutaRAW);

        if (!File.Exists(rutaCompleta))
        {
            AlsasuaLogger.Info("SistemaTerrenoAlsasua",
                $"RAW no encontrado en '{rutaCompleta}'. Usando generación procedural.");
            return null;
        }

        byte[] bytes;
        try
        {
            bytes = File.ReadAllBytes(rutaCompleta);
        }
        catch (System.Exception ex)
        {
            AlsasuaLogger.Warn("SistemaTerrenoAlsasua",
                $"Error leyendo RAW '{rutaCompleta}': {ex.Message}. Usando generación procedural.");
            return null;
        }

        // ── Validar tamaño ──────────────────────────────────────────────
        // El res del TerrainData es heightmapResolution = N+1 (e.g. 513),
        // pero el RAW tiene exactamente res×res muestras de 2 bytes.
        int muestrasEsperadas = res * res;
        int bytesEsperados    = muestrasEsperadas * 2;

        // Si el RAW fue generado con una resolución distinta, intentar autodetectar.
        if (bytes.Length != bytesEsperados)
        {
            int rawRes = Mathf.RoundToInt(Mathf.Sqrt(bytes.Length / 2f));
            if (rawRes * rawRes * 2 == bytes.Length && rawRes >= 33)
            {
                AlsasuaLogger.Warn("SistemaTerrenoAlsasua",
                    $"RAW tiene resolución {rawRes}² (esperado {res}²). " +
                    $"Se remuestreará bilinealmente al cargar.");
                return CargarYRemuestreaRaw(bytes, rawRes, res);
            }
            AlsasuaLogger.Warn("SistemaTerrenoAlsasua",
                $"RAW tiene {bytes.Length} bytes, esperado {bytesEsperados} para {res}². " +
                $"Usando generación procedural.");
            return null;
        }

        // ── Decodificar ─────────────────────────────────────────────────
        float[,] h    = new float[res, res];
        float    inv  = 1f / 65535f;  // uint16 max

        // DescargadorIDENA.py escribe fila 0 = latitud norte = Unity Z alto (zi máximo).
        // El loop Unity es h[zi, xi] donde zi=0 es el borde sur del Terrain.
        // → hay que invertir el eje Z al leer.
        for (int zi = 0; zi < res; zi++)
        {
            int filaRAW = (res - 1) - zi;  // invertir Z para alinear con Unity
            for (int xi = 0; xi < res; xi++)
            {
                int idx    = (filaRAW * res + xi) * 2;
                ushort val = (ushort)(bytes[idx] | (bytes[idx + 1] << 8));  // little-endian
                h[zi, xi]  = val * inv;
            }
        }

        AlsasuaLogger.Info("SistemaTerrenoAlsasua",
            $"✓ Heightmap RAW cargado desde '{rutaCompleta}' ({res}² muestras, {bytes.Length / 1024} KB).");
        return h;
    }

    /// <summary>
    /// Remuestrea un heightmap RAW de resolución srcRes a dstRes usando
    /// interpolación bilineal. Permite usar el mismo .raw aunque la resolución
    /// del Terrain Inspector se cambie a 257 o 1025.
    /// </summary>
    private float[,] CargarYRemuestreaRaw(byte[] bytes, int srcRes, int dstRes)
    {
        // Decodificar a array lineal
        var src  = new float[srcRes * srcRes];
        float inv = 1f / 65535f;
        for (int i = 0; i < src.Length; i++)
        {
            int idx = i * 2;
            src[i]  = (ushort)(bytes[idx] | (bytes[idx + 1] << 8)) * inv;
        }

        float[,] dst = new float[dstRes, dstRes];
        float  scale = (srcRes - 1f) / (dstRes - 1f);

        for (int zi = 0; zi < dstRes; zi++)
        {
            // Invertir Z (mismo motivo que en IntentarCargarRAW)
            float szRaw = ((dstRes - 1 - zi) * scale);
            int   sz0   = Mathf.FloorToInt(szRaw);
            int   sz1   = Mathf.Min(sz0 + 1, srcRes - 1);
            float tz    = szRaw - sz0;

            for (int xi = 0; xi < dstRes; xi++)
            {
                float sxRaw = xi * scale;
                int   sx0   = Mathf.FloorToInt(sxRaw);
                int   sx1   = Mathf.Min(sx0 + 1, srcRes - 1);
                float tx    = sxRaw - sx0;

                float v00 = src[sz0 * srcRes + sx0];
                float v10 = src[sz0 * srcRes + sx1];
                float v01 = src[sz1 * srcRes + sx0];
                float v11 = src[sz1 * srcRes + sx1];

                dst[zi, xi] = Mathf.Lerp(Mathf.Lerp(v00, v10, tx),
                                          Mathf.Lerp(v01, v11, tx), tz);
            }
        }
        return dst;
    }

    /// <summary>
    /// Añade ruido Perlin suave (sólo alta frecuencia, amplitud reducida) sobre
    /// un heightmap ya cargado desde RAW. Preserva la topografía real de IDENA
    /// pero añade pequeñas irregularidades naturales que los LiDAR de 5m suavizan.
    /// </summary>
    private void AñadirRuidoSobreRAW(float[,] h, int res)
    {
        float half   = tamanoTerreno * 0.5f;
        float invMax = 1f / alturaMaxima;
        float esc    = 0.0018f;   // ~100 m de longitud de onda
        float amp    = amplitudRuidoAlto * 0.5f;  // mitad de la amplitud estándar

        for (int zi = 0; zi < res; zi++)
        {
            float wz = (zi / (float)(res - 1)) * tamanoTerreno - half;
            for (int xi = 0; xi < res; xi++)
            {
                float wx     = (xi / (float)(res - 1)) * tamanoTerreno - half;
                float dist   = Mathf.Sqrt(wx * wx + wz * wz);
                float mask   = Mathf.Clamp01(dist / (radioValleFlat * 2f));
                float perlin = Mathf.PerlinNoise(wx * esc + _semillaX + 500f,
                                                  wz * esc + _semillaZ + 300f);
                h[zi, xi] = Mathf.Clamp01(h[zi, xi] + (perlin - 0.5f) * amp * invMax * mask);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HEIGHTMAP — función de alturas
    // ═══════════════════════════════════════════════════════════════════════

    private float[,] GenerarHeightmap(int res)
    {
        float[,] h = new float[res, res];
        float half = tamanoTerreno * 0.5f;
        float invMax = 1f / alturaMaxima;

        for (int zi = 0; zi < res; zi++)
        {
            for (int xi = 0; xi < res; xi++)
            {
                // Posición en metros Unity (centrada en Herriko Plaza)
                float wx = (xi / (float)(res - 1)) * tamanoTerreno - half;
                float wz = (zi / (float)(res - 1)) * tamanoTerreno - half;

                float altMetros = AlturaEnPunto(wx, wz);

                // Normalizar a [0,1] para TerrainData
                h[zi, xi] = Mathf.Clamp01(altMetros * invMax);
            }
        }
        return h;
    }

    /// <summary>
    /// Calcula la altura en metros en el punto (wx, wz) del mundo Unity.
    /// Combina lomas gaussianas de montaña + zona plana del valle + ruido Perlin.
    /// </summary>
    private float AlturaEnPunto(float wx, float wz)
    {
        // ── 1. Suma de lomas gaussianas de montaña ─────────────────────
        float altMonte = 0f;
        foreach (var c in CrestasBurunda)
        {
            float dx = wx - c.Centro.x;
            float dz = wz - c.Centro.y;

            // Rotar si la cresta es oblicua
            if (c.Angulo != 0f)
            {
                float rad = c.Angulo * Mathf.Deg2Rad;
                float cosA = Mathf.Cos(rad), sinA = Mathf.Sin(rad);
                float dx2 = dx * cosA - dz * sinA;
                float dz2 = dx * sinA + dz * cosA;
                dx = dx2; dz = dz2;
            }

            float exponent = (dx * dx) / (c.Rx * c.Rx) + (dz * dz) / (c.Rz * c.Rz);
            float gauss    = c.AltRel * Mathf.Exp(-exponent);
            altMonte += gauss;
        }

        // ── 2. Zona plana del valle (anular de radio radioValleFlat) ───
        float distCentro = Mathf.Sqrt(wx * wx + wz * wz);
        float factorFlat = 1f;
        if (distCentro < radioValleFlat)
        {
            factorFlat = 0f;
        }
        else
        {
            // Transición suave entre valle plano y ladera
            float t = (distCentro - radioValleFlat) / (radioValleFlat * suavizadoFalda);
            factorFlat = Mathf.SmoothStep(0f, 1f, Mathf.Clamp01(t));
        }

        altMonte *= factorFlat;

        // ── 3. Ruido Perlin multicapa (terreno natural) ────────────────
        float escala1 = 0.00035f;   // baja frecuencia (colinas kilométricas)
        float escala2 = 0.0018f;    // alta frecuencia (irregularidades ~100m)

        float perlin1 = Mathf.PerlinNoise(wx * escala1 + _semillaX, wz * escala1 + _semillaZ);
        float perlin2 = Mathf.PerlinNoise(wx * escala2 + _semillaX + 500f, wz * escala2 + _semillaZ + 300f);

        float ruido = (perlin1 - 0.5f) * amplitudRuidoBajo * alturaMaxima
                    + (perlin2 - 0.5f) * amplitudRuidoAlto * alturaMaxima;

        // El ruido se escala según la distancia al centro (sin ruido en el valle)
        ruido *= Mathf.Clamp01(distCentro / (radioValleFlat * 2f));

        float altTotal = (altMonte + ruido) * intensidadMontes;
        return Mathf.Max(0f, altTotal);   // el valle no puede bajar de 0
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TEXTURIZACIÓN PROCEDURAL
    // ═══════════════════════════════════════════════════════════════════════

    private void AplicarTexturas(float[,] heights)
    {
        int res = heights.GetLength(0);

        // Crear un shader de textura simple (color sólido por capa)
        var shaderName = "Universal Render Pipeline/Terrain/Lit";
        var shLit = Shader.Find(shaderName)
                 ?? Shader.Find("Universal Render Pipeline/Lit")
                 ?? Shader.Find("Standard");

        if (shLit == null)
        {
            AlsasuaLogger.Warn("SistemaTerrenoAlsasua",
                "Shader de terreno no encontrado — skipping texturización.");
            return;
        }

        // Capa 0: pradera del valle
        var tex0 = CrearTextura(colorValle, "TexValle");
        // Capa 1: ladera
        var tex1 = CrearTextura(colorLadera, "TexLadera");
        // Capa 2: cima
        var tex2 = CrearTextura(colorCima, "TexCima");

#pragma warning disable CS0618  // TerrainLayer reemplaza SplatPrototype en Unity 2019+
        var layers = new TerrainLayer[3];
        layers[0] = new TerrainLayer { diffuseTexture = tex0, tileSize = new Vector2(30, 30) };
        layers[1] = new TerrainLayer { diffuseTexture = tex1, tileSize = new Vector2(20, 20) };
        layers[2] = new TerrainLayer { diffuseTexture = tex2, tileSize = new Vector2(15, 15) };
        _terrainData.terrainLayers = layers;
#pragma warning restore CS0618

        // Alphamap (mezcla de capas)
        int aRes = _terrainData.alphamapResolution;
        var alpha = new float[aRes, aRes, 3];

        float invMax  = 1f / alturaMaxima;
        float invRes  = 1f / (aRes - 1);

        for (int zi = 0; zi < aRes; zi++)
        {
            for (int xi = 0; xi < aRes; xi++)
            {
                // Samplear la altura en este punto
                int hi = Mathf.RoundToInt(zi * (res - 1) * invRes);
                int hj = Mathf.RoundToInt(xi * (res - 1) * invRes);
                float h01 = heights[hi, hj];  // [0,1]
                float hm  = h01 * alturaMaxima;

                // Mezcla por altura: <40m=valle, 40-140m=ladera, >140m=cima
                float wValle  = 1f - Mathf.Clamp01((hm - 10f) / 50f);
                float wCima   = Mathf.Clamp01((hm - 140f) / 60f);
                float wLadera = 1f - wValle - wCima;

                float total = wValle + wLadera + wCima;
                if (total < 0.001f) { wValle = 1f; total = 1f; }
                alpha[zi, xi, 0] = wValle  / total;
                alpha[zi, xi, 1] = wLadera / total;
                alpha[zi, xi, 2] = wCima   / total;
            }
        }

        _terrainData.SetAlphamaps(0, 0, alpha);
    }

    private Texture2D CrearTextura(Color col, string nombre)
    {
        var tex = new Texture2D(4, 4, TextureFormat.RGBA32, false);
        tex.name = nombre;
        var pixels = new Color[16];
        for (int i = 0; i < 16; i++) pixels[i] = col;
        tex.SetPixels(pixels);
        tex.Apply();
        return tex;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  API PÚBLICA
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>
    /// True si el heightmap activo fue cargado desde el archivo RAW (IDENA).
    /// False si se generó proceduralmente.
    /// </summary>
    public bool HeightmapDesdeRAW => _rawCargado;

    /// <summary>
    /// Fuerza regeneración del terreno (útil desde Editor o tras cambios en el .raw).
    /// </summary>
    public void Regenerar()
    {
        OnDestroy();
        GenerarTerreno();
    }

    /// <summary>
    /// Devuelve la altura del terreno en el punto Unity (wx, wz), en metros.
    /// Útil para SistemaVegetacion, SistemaEdificios, etc.
    /// Si el Terrain aún no está generado, usa la función analítica directamente.
    /// </summary>
    public float ObtenerAltura(float wx, float wz)
    {
        if (_terrain != null && _terrain.terrainData != null)
        {
            // Samplear del TerrainData ya generado
            float half = tamanoTerreno * 0.5f;
            float u = (wx + half) / tamanoTerreno;
            float v = (wz + half) / tamanoTerreno;
            return _terrain.terrainData.GetInterpolatedHeight(u, v) - 1f;  // -1 offset
        }
        // Fallback: calcular analíticamente
        return Mathf.Max(0f, AlturaEnPunto(wx, wz));
    }

    /// <summary>
    /// Devuelve la normal del terreno en el punto Unity (wx, wz).
    /// Útil para orientar vegetación y edificios sobre la ladera.
    /// </summary>
    public Vector3 ObtenerNormal(float wx, float wz)
    {
        if (_terrain != null && _terrain.terrainData != null)
        {
            float half = tamanoTerreno * 0.5f;
            float u = (wx + half) / tamanoTerreno;
            float v = (wz + half) / tamanoTerreno;
            return _terrain.terrainData.GetInterpolatedNormal(u, v);
        }
        return Vector3.up;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  GIZMOS — visualización en editor
    // ═══════════════════════════════════════════════════════════════════════
#if UNITY_EDITOR
    // ───────────────────────────────────────────────────────────────────────
    //  CUSTOM INSPECTOR — botón Regenerar en el Inspector
    // ───────────────────────────────────────────────────────────────────────
    [CustomEditor(typeof(SistemaTerrenoAlsasua))]
    private sealed class Inspector : Editor
    {
        public override void OnInspectorGUI()
        {
            DrawDefaultInspector();

            var t = (SistemaTerrenoAlsasua)target;
            EditorGUILayout.Space(6);

            GUI.color = new Color(0.6f, 1f, 0.7f);
            if (GUILayout.Button("⛰  Regenerar Terreno Ahora", GUILayout.Height(32)))
            {
                if (Application.isPlaying) t.Regenerar();
                else EditorUtility.DisplayDialog("Regenerar Terreno",
                    "La regeneración procedural requiere Play Mode.\n\n" +
                    "Para usar el heightmap RAW en Editor:\n" +
                    "  Terrain → Import Raw → selecciona el .raw\n" +
                    "  (Depth: 16bit, Byte order: Little Endian)", "OK");
            }
            GUI.color = Color.white;

            if (!Application.isPlaying) return;

            EditorGUILayout.Space(4);
            string fuenteStr = t._rawCargado ? "📦 RAW (IDENA)" : "🔧 Procedural";
            EditorGUILayout.HelpBox(
                $"Fuente activa: {fuenteStr}", MessageType.Info);
        }
    }

    private void OnDrawGizmos()
    {
        // Mostrar el footprint del terreno
        float half = tamanoTerreno * 0.5f;
        Gizmos.color = new Color(0.4f, 0.8f, 0.2f, 0.25f);
        Gizmos.DrawCube(new Vector3(0, -1f + alturaMaxima * 0.5f, 0),
                        new Vector3(tamanoTerreno, alturaMaxima, tamanoTerreno));

        // Mostrar centro de cada cresta
        Gizmos.color = new Color(0.9f, 0.5f, 0.1f, 0.8f);
        foreach (var c in CrestasBurunda)
        {
            Vector3 pos = new Vector3(c.Centro.x, c.AltRel, c.Centro.y);
            Gizmos.DrawSphere(pos, 20f);
            // Radio de influencia
            Gizmos.color = new Color(0.9f, 0.5f, 0.1f, 0.2f);
            Gizmos.DrawWireSphere(pos, (c.Rx + c.Rz) * 0.5f);
            Gizmos.color = new Color(0.9f, 0.5f, 0.1f, 0.8f);
        }

        // Valle plano
        Gizmos.color = new Color(0.2f, 0.8f, 0.4f, 0.3f);
        Gizmos.DrawWireSphere(Vector3.zero, radioValleFlat);
    }
#endif
}
