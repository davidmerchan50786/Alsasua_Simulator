// Assets/Scripts/SistemaVegetacion.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Sistema de vegetación procedural para el entorno de Alsasua:
//
//  · Pinos y robles de poca poligonización (mesh procedural), pintados con
//    GPU Instancing (DrawMeshInstanced, 1023 instancias por lote).
//  · Distribución natural via ruido Perlin + densidad configurable por zona.
//  · Cuatro zonas de bosque alrededor de Alsasua (norte, sur, este, oeste).
//  · Viento simulado en shader (oscilación cíclica de _WindStrength en
//    MaterialPropertyBlock — no requiere shader custom; actúa como color
//    tint animado en URP/Lit como fallback visual si el shader no lo soporta).
//  · LOD implícito: sólo se renderizan árboles dentro de _rangoRender metros.
//  · OnDestroy() destruye todos los new Material() y new Mesh().
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;
using System.Collections.Generic;
using Unity.Profiling;

public sealed class SistemaVegetacion : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR
    // ───────────────────────────────────────────────────────────────────────
    [Header("═══ ZONAS DE BOSQUE ═══")]
    [Tooltip("Centro de cada zona forestal")]
    [SerializeField] private Vector3[] centrosZona = new Vector3[]
    {
        new Vector3(   0f, 0f,  500f),  // norte
        new Vector3(   0f, 0f, -500f),  // sur
        new Vector3( 500f, 0f,    0f),  // este
        new Vector3(-500f, 0f,    0f),  // oeste
    };
    [Tooltip("Radio de cada zona (m)")]
    [SerializeField] private float[] radiosZona = new float[] { 250f, 200f, 220f, 180f };

    [Header("═══ PARÁMETROS ═══")]
    [Tooltip("Número máximo de árboles generados por zona forestal.")]
    [SerializeField] private int   densidadArbolesZona = 300;  // árboles por zona
    [Tooltip("Distancia máxima de renderizado de árboles desde la cámara (m). Árboles más lejanos se descartan.")]
    [SerializeField] private float rangoRender          = 600f; // distancia máx visible
    [Tooltip("Altura Y base para el raycast de terreno. Aumentar si los árboles flotan o se entierran.")]
    [SerializeField] private float alturaTerreno        = 0f;   // altura base si no hay raycast
    [Tooltip("Escala mínima de los pinos (multiplicador de tamaño del mesh).")]
    [SerializeField] private float escalaMinPino        = 0.8f;
    [Tooltip("Escala máxima de los pinos (multiplicador de tamaño del mesh).")]
    [SerializeField] private float escalaMaxPino        = 2.2f;
    [Tooltip("Escala mínima de los robles (multiplicador de tamaño del mesh).")]
    [SerializeField] private float escalaMinRoble       = 0.7f;
    [Tooltip("Escala máxima de los robles (multiplicador de tamaño del mesh).")]
    [SerializeField] private float escalaMaxRoble       = 1.8f;
    [Tooltip("Proporción de pinos respecto al total de árboles. 0 = todo robles, 1 = todo pinos.")]
    [SerializeField] [Range(0f,1f)] private float fraccionPinos = 0.65f; // 65 % pinos, resto robles

    [Header("═══ VIENTO ═══")]
    [Tooltip("Frecuencia del ciclo de balanceo del viento (ciclos/segundo).")]
    [SerializeField] private float velocidadViento = 0.8f;   // ciclos/s
    [Tooltip("Amplitud de la oscilación de viento en escala X del árbol. 0 = sin viento.")]
    [SerializeField] private float fuerzaViento    = 0.04f;  // amplitud tono oscilación

    // ───────────────────────────────────────────────────────────────────────
    //  ESTADO INTERNO
    // ───────────────────────────────────────────────────────────────────────
    private struct ArbolData
    {
        public Vector3 posicion;
        public float   escala;
        public float   rotacionY;
        public bool    esPino;
        public float   faseViento;   // offset de fase 0-2π
    }

    private ArbolData[] _arboles;

    // Meshes procedurales
    private Mesh _meshPino;     // tronco + 3 conos
    private Mesh _meshRoble;    // tronco + esfera copa

    // Materiales (uno por tipo de árbol; tronco compartido en el mismo mesh)
    private Material _matPino;
    private Material _matRoble;

    private readonly List<Material> _matsCreados  = new List<Material>();
    private readonly List<Mesh>     _meshesCreados = new List<Mesh>();

    // Buffers de render (pre-alloc, zero-GC per frame).
    // IMPORTANTE: pinos y robles usan buffers SEPARADOS para evitar overwrites
    // cuando DrawMeshInstanced se llama con ambos tipos en el mismo frame.
    private const int MAX_LOTE = 1023;
    private readonly Matrix4x4[] _lotesPino  = new Matrix4x4[MAX_LOTE];
    private readonly Matrix4x4[] _lotesRoble = new Matrix4x4[MAX_LOTE];

    private Camera   _camPrincipal;
    private float    _timerViento = 0f;

    // (sin _idBaseColor/_idBaseMap — vegetación no usa per-instance color override)

    // ── Profiler marker ──────────────────────────────────────────────────
    private static readonly ProfilerMarker _markerRender =
        new ProfilerMarker("SistemaVegetacion.RenderizarArboles");

    // ───────────────────────────────────────────────────────────────────────
    //  UNITY
    // ───────────────────────────────────────────────────────────────────────
    private void Awake()
    {
        CrearMeshes();
        CrearMateriales();
        GenerarArboles();
        _camPrincipal = Camera.main;
    }

    private void Update()
    {
        _timerViento += Time.deltaTime * velocidadViento;
        if (_timerViento > Mathf.PI * 2f) _timerViento -= Mathf.PI * 2f;

        if (_camPrincipal == null) _camPrincipal = Camera.main;
        if (_camPrincipal == null) return;

        RenderizarArboles();
    }

    private void OnDestroy()
    {
        foreach (var m    in _matsCreados)   if (m    != null) Object.Destroy(m);
        foreach (var mesh in _meshesCreados) if (mesh != null) Object.Destroy(mesh);
        _matsCreados.Clear();
        _meshesCreados.Clear();
    }

    // ───────────────────────────────────────────────────────────────────────
    //  GENERACIÓN DE ÁRBOLES
    // ───────────────────────────────────────────────────────────────────────
    private void GenerarArboles()
    {
        int totalMax = centrosZona.Length * densidadArbolesZona;
        var lista    = new List<ArbolData>(totalMax);

        for (int z = 0; z < centrosZona.Length; z++)
        {
            float radio = z < radiosZona.Length ? radiosZona[z] : 200f;
            for (int i = 0; i < densidadArbolesZona; i++)
            {
                // Distribución en disco con ruido Perlin para clarear
                Vector2 offset = Random.insideUnitCircle * radio;
                Vector3 pos = centrosZona[z] + new Vector3(offset.x, 0f, offset.y);

                // Perlin: eliminar árboles en claros (valor < umbral)
                float ruido = Mathf.PerlinNoise(pos.x * 0.01f + z * 10f,
                                                pos.z * 0.01f + z * 7f);
                if (ruido < 0.30f) continue;   // claro natural

                // Altura real del terreno vía raycast
                float alturaY = MuestrearTerreno(pos);

                bool esPino   = Random.value < fraccionPinos;
                float escMin  = esPino ? escalaMinPino  : escalaMinRoble;
                float escMax  = esPino ? escalaMaxPino  : escalaMaxRoble;
                float escala  = Mathf.Lerp(escMin, escMax, ruido);   // más ruido → más alto

                lista.Add(new ArbolData
                {
                    posicion  = new Vector3(pos.x, alturaY, pos.z),
                    escala    = escala,
                    rotacionY = Random.Range(0f, 360f),
                    esPino    = esPino,
                    faseViento = Random.Range(0f, Mathf.PI * 2f),
                });
            }
        }

        _arboles = lista.ToArray();
        Debug.Log($"[SistemaVegetacion] Generados {_arboles.Length} árboles.");
    }

    private float MuestrearTerreno(Vector3 pos)
    {
        if (Physics.Raycast(new Vector3(pos.x, alturaTerreno + 100f, pos.z),
                            Vector3.down, out RaycastHit hit, 200f, ~0))
            return hit.point.y;
        return alturaTerreno;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  RENDER GPU INSTANCING
    // ───────────────────────────────────────────────────────────────────────
    private void RenderizarArboles()
    {
        using var _prof = _markerRender.Auto();
        if (_arboles == null || _arboles.Length == 0) return;

        Vector3 camPos = _camPrincipal.transform.position;
        float rango2   = rangoRender * rangoRender;

        int lPino = 0, lRoble = 0;

        for (int i = 0; i < _arboles.Length; i++)
        {
            ref var a = ref _arboles[i];

            // Culling por distancia
            float dx = a.posicion.x - camPos.x;
            float dz = a.posicion.z - camPos.z;
            if (dx * dx + dz * dz > rango2) continue;

            // Viento: pequeña oscilación en escala X para simular balanceo
            float viento = Mathf.Sin(_timerViento + a.faseViento) * fuerzaViento;
            Vector3 escVec = new Vector3(a.escala * (1f + viento), a.escala, a.escala);
            Matrix4x4 mat  = Matrix4x4.TRS(
                a.posicion,
                Quaternion.Euler(0f, a.rotacionY, 0f),
                escVec);

            if (a.esPino)
            {
                // Usar buffer dedicado de pinos — nunca colisiona con robles
                _lotesPino[lPino++] = mat;
                if (lPino == MAX_LOTE)
                {
                    Graphics.DrawMeshInstanced(_meshPino, 0, _matPino, _lotesPino, lPino);
                    lPino = 0;
                }
            }
            else
            {
                // Buffer dedicado de robles
                _lotesRoble[lRoble++] = mat;
                if (lRoble == MAX_LOTE)
                {
                    Graphics.DrawMeshInstanced(_meshRoble, 0, _matRoble, _lotesRoble, lRoble);
                    lRoble = 0;
                }
            }
        }

        // Flush de los lotes restantes
        if (lPino  > 0) Graphics.DrawMeshInstanced(_meshPino,  0, _matPino,  _lotesPino,  lPino);
        if (lRoble > 0) Graphics.DrawMeshInstanced(_meshRoble, 0, _matRoble, _lotesRoble, lRoble);
    }

    // ───────────────────────────────────────────────────────────────────────
    //  MESHES PROCEDURALES
    // ───────────────────────────────────────────────────────────────────────
    private void CrearMeshes()
    {
        _meshPino  = BuildPino();
        _meshRoble = BuildRoble();
        _meshesCreados.Add(_meshPino);
        _meshesCreados.Add(_meshRoble);
    }

    // Pino: tronco cilíndrico + 3 capas de conos superpuestos
    private static Mesh BuildPino()
    {
        var verts = new List<Vector3>();
        var norms = new List<Vector3>();
        var uvs   = new List<Vector2>();
        var tris  = new List<int>();

        // Tronco: cilindro 0.18 m radio, 2 m alto
        AppendCilindro(verts, norms, uvs, tris, 0.18f, 2.0f, 8, new Vector3(0, 0, 0));

        // Tres capas de cono: base grande abajo, pequeña arriba
        // Cono 0: y=0.8 – 2.8, radio base 1.0
        AppendCono(verts, norms, uvs, tris, 1.0f, 2.0f, 8, new Vector3(0, 0.8f, 0));
        // Cono 1: y=1.6 – 3.4, radio base 0.75
        AppendCono(verts, norms, uvs, tris, 0.75f, 1.8f, 8, new Vector3(0, 1.6f, 0));
        // Cono 2: y=2.4 – 4.0, radio base 0.50
        AppendCono(verts, norms, uvs, tris, 0.50f, 1.6f, 8, new Vector3(0, 2.4f, 0));

        var mesh = new Mesh { name = "MeshPino" };
        mesh.SetVertices(verts); mesh.SetNormals(norms); mesh.SetUVs(0, uvs);
        mesh.SetTriangles(tris, 0); mesh.RecalculateBounds();
        return mesh;
    }

    // Roble: tronco + esfera de copa
    private static Mesh BuildRoble()
    {
        var verts = new List<Vector3>();
        var norms = new List<Vector3>();
        var uvs   = new List<Vector2>();
        var tris  = new List<int>();

        // Tronco: cilindro 0.22 m radio, 2.2 m alto
        AppendCilindro(verts, norms, uvs, tris, 0.22f, 2.2f, 8, new Vector3(0, 0, 0));

        // Copa esférica: centro y=3.1, radio 1.3 m
        AppendEsfera(verts, norms, uvs, tris, 1.3f, 8, 6, new Vector3(0, 3.1f, 0));

        var mesh = new Mesh { name = "MeshRoble" };
        mesh.SetVertices(verts); mesh.SetNormals(norms); mesh.SetUVs(0, uvs);
        mesh.SetTriangles(tris, 0); mesh.RecalculateBounds();
        return mesh;
    }

    // ── Primitivas geométricas ────────────────────────────────────────────

    private static void AppendCilindro(List<Vector3> v, List<Vector3> n, List<Vector2> uv,
                                        List<int> t, float r, float h, int seg, Vector3 offset)
    {
        int base0 = v.Count;
        int rings  = 2;
        for (int ri = 0; ri <= rings; ri++)
        {
            float y = offset.y + h * ((float)ri / rings);
            for (int si = 0; si <= seg; si++)
            {
                float a = si * Mathf.PI * 2f / seg;
                float x = Mathf.Cos(a) * r + offset.x;
                float z = Mathf.Sin(a) * r + offset.z;
                v.Add(new Vector3(x, y, z));
                n.Add(new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a)));
                uv.Add(new Vector2((float)si / seg, (float)ri / rings));
            }
        }
        for (int ri = 0; ri < rings; ri++)
        for (int si = 0; si < seg;   si++)
        {
            int b = base0 + ri * (seg + 1) + si;
            t.Add(b); t.Add(b + seg + 1); t.Add(b + 1);
            t.Add(b + 1); t.Add(b + seg + 1); t.Add(b + seg + 2);
        }
    }

    private static void AppendCono(List<Vector3> v, List<Vector3> n, List<Vector2> uv,
                                    List<int> t, float rBase, float h, int seg, Vector3 offset)
    {
        int base0 = v.Count;
        float apex = offset.y + h;

        // Vértice del ápice
        int apexIdx = v.Count;
        v.Add(new Vector3(offset.x, apex, offset.z));
        n.Add(Vector3.up);
        uv.Add(new Vector2(0.5f, 1f));

        // Anillo base
        for (int si = 0; si <= seg; si++)
        {
            float a = si * Mathf.PI * 2f / seg;
            float x = Mathf.Cos(a) * rBase + offset.x;
            float z = Mathf.Sin(a) * rBase + offset.z;
            // Normal inclinada hacia fuera+arriba
            Vector3 lateral = new Vector3(Mathf.Cos(a), 0f, Mathf.Sin(a));
            Vector3 up = Vector3.up;
            v.Add(new Vector3(x, offset.y, z));
            n.Add((lateral + up * (rBase / h)).normalized);
            uv.Add(new Vector2((float)si / seg, 0f));
        }

        // Triángulos
        for (int si = 0; si < seg; si++)
        {
            int b = base0 + 1 + si;
            t.Add(apexIdx); t.Add(b + 1); t.Add(b);
        }

        // Tapa base (disco)
        int centroDisco = v.Count;
        v.Add(new Vector3(offset.x, offset.y, offset.z));
        n.Add(Vector3.down); uv.Add(new Vector2(0.5f, 0f));
        for (int si = 0; si <= seg; si++)
        {
            float a = si * Mathf.PI * 2f / seg;
            v.Add(new Vector3(Mathf.Cos(a) * rBase + offset.x, offset.y,
                              Mathf.Sin(a) * rBase + offset.z));
            n.Add(Vector3.down);
            uv.Add(new Vector2(0.5f + 0.5f * Mathf.Cos(a), 0.5f + 0.5f * Mathf.Sin(a)));
        }
        for (int si = 0; si < seg; si++)
        {
            int b = centroDisco + 1 + si;
            t.Add(centroDisco); t.Add(b); t.Add(b + 1);
        }
    }

    private static void AppendEsfera(List<Vector3> v, List<Vector3> n, List<Vector2> uv,
                                      List<int> t, float r, int seg, int rings, Vector3 offset)
    {
        int base0 = v.Count;
        for (int ri = 0; ri <= rings; ri++)
        {
            float phi = Mathf.PI * ((float)ri / rings - 0.5f);
            float cosP = Mathf.Cos(phi), sinP = Mathf.Sin(phi);
            float y = offset.y + r * sinP;
            float rLayer = r * cosP;
            for (int si = 0; si <= seg; si++)
            {
                float a = si * Mathf.PI * 2f / seg;
                float x = Mathf.Cos(a) * rLayer + offset.x;
                float z = Mathf.Sin(a) * rLayer + offset.z;
                v.Add(new Vector3(x, y, z));
                n.Add(new Vector3(Mathf.Cos(a) * cosP, sinP, Mathf.Sin(a) * cosP).normalized);
                uv.Add(new Vector2((float)si / seg, (float)ri / rings));
            }
        }
        for (int ri = 0; ri < rings; ri++)
        for (int si = 0; si < seg;   si++)
        {
            int b = base0 + ri * (seg + 1) + si;
            t.Add(b); t.Add(b + seg + 1); t.Add(b + 1);
            t.Add(b + 1); t.Add(b + seg + 1); t.Add(b + seg + 2);
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  MATERIALES
    // ───────────────────────────────────────────────────────────────────────
    private void CrearMateriales()
    {
        // Verde oscuro para pinos, verde amarillento para robles.
        // Tronco incluido en el mismo mesh → mismo material por árbol (sin sub-mesh split).
        _matPino  = MatURP(new Color(0.10f, 0.28f, 0.10f));   // verde conífero
        _matRoble = MatURP(new Color(0.18f, 0.38f, 0.08f));   // verde caducifolio

        // GPU instancing obligatorio para DrawMeshInstanced
        _matPino.enableInstancing  = true;
        _matRoble.enableInstancing = true;
    }

    private Material MatURP(Color col)
    {
        var sh = Shader.Find("Universal Render Pipeline/Lit")
              ?? Shader.Find("Universal Render Pipeline/Unlit")
              ?? Shader.Find("Standard");
        if (sh == null)
        {
            Debug.LogError("[SistemaVegetacion] Shader no encontrado. " +
                           "Incluye 'Universal Render Pipeline/Lit' en Always Included Shaders.");
            sh = Shader.Find("Hidden/InternalErrorShader");
            if (sh == null) return null;
        }
        var mat = new Material(sh) { color = col };
        _matsCreados.Add(mat);
        return mat;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ───────────────────────────────────────────────────────────────────────

    // ───────────────────────────────────────────────────────────────────────
    //  GIZMOS — visualización de zonas en el Editor
    // ───────────────────────────────────────────────────────────────────────
    private void OnDrawGizmosSelected()
    {
        if (centrosZona == null) return;
        for (int z = 0; z < centrosZona.Length; z++)
        {
            float radio = z < radiosZona?.Length ? radiosZona[z] : 200f;
            // Color degradado verde por zona (para diferenciarlas fácilmente)
            float hue = (float)z / Mathf.Max(1, centrosZona.Length);
            Gizmos.color = Color.HSVToRGB(0.33f + hue * 0.15f, 0.7f, 0.9f);
            Gizmos.DrawWireSphere(centrosZona[z], radio);
            // Punto central
            Gizmos.DrawSphere(centrosZona[z], 3f);
        }
    }

    /// <summary>Número total de árboles generados.</summary>
    public int TotalArboles => _arboles?.Length ?? 0;

    /// <summary>Añade árboles en una zona adicional en tiempo de ejecución.</summary>
    public void AñadirZona(Vector3 centro, float radio, int cantidad)
    {
        if (_arboles == null) { Debug.LogWarning("[SistemaVegetacion] AñadirZona llamado antes de Awake."); return; }
        var lista = new List<ArbolData>(_arboles.Length + cantidad);
        lista.AddRange(_arboles);

        for (int i = 0; i < cantidad; i++)
        {
            Vector2 off = Random.insideUnitCircle * radio;
            Vector3 pos = centro + new Vector3(off.x, 0f, off.y);
            float altY  = MuestrearTerreno(pos);
            bool pino   = Random.value < fraccionPinos;
            float ruidoE = Random.Range(0.4f, 1.0f);
            float esc   = pino ? Mathf.Lerp(escalaMinPino, escalaMaxPino, ruidoE)
                               : Mathf.Lerp(escalaMinRoble, escalaMaxRoble, ruidoE);
            lista.Add(new ArbolData
            {
                posicion   = new Vector3(pos.x, altY, pos.z),
                escala     = esc,
                rotacionY  = Random.Range(0f, 360f),
                esPino     = pino,
                faseViento = Random.Range(0f, Mathf.PI * 2f),
            });
        }

        _arboles = lista.ToArray();
    }
}
