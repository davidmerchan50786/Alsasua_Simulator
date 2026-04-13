// Assets/Scripts/SistemaEdificios.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Spawner de edificios para el pueblo de Alsasua.
//
//  Usa los nuevos asset packs:
//    · Village Houses Pack (GBAndrewGB)    → casas de pueblo / manzanas
//    · House Pack (Mehdi Rabiee)           → casas urbanas / bloques
//    · Modular self-stand fence (Aleksey)  → vallas de acordonamiento policial
//
//  Coloca los edificios en una cuadrícula de manzanas alrededor del centro
//  del pueblo, respetando las rutas de patrulla de GC/PF y la zona de
//  la manifestación.
//
//  Si los prefabs no están importados, crea edificios procedurales simples
//  (cubos con color) para que la escena no quede vacía.
//
//  SistemaAssets inyecta los prefabs vía AsignarPrefabs() antes de Start().
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;
using System.Collections.Generic;

[AddComponentMenu("Alsasua/Sistema Edificios")]
public sealed class SistemaEdificios : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR
    // ───────────────────────────────────────────────────────────────────────

    [Header("═══ PREFABS (asignados por SistemaAssets) ═══")]
    [Tooltip("Prefab casa de pueblo (Village Houses Pack). Null → procedural.")]
    [SerializeField] private GameObject prefabCasaPueblo;
    [Tooltip("Prefab casa urbana (House Pack). Null → usa casa pueblo.")]
    [SerializeField] private GameObject prefabCasaUrbana;
    [Tooltip("Prefab valla modular (Modular self-stand fence). Null → sin vallas.")]
    [SerializeField] private GameObject prefabValla;

    // Arrays para variedad de edificios desde Downloads
    private GameObject[] _prefabsRuinas = new GameObject[0];
    private GameObject[] _prefabsCasasAdicionales = new GameObject[0];
    private GameObject[] _prefabsEdificiosEspeciales = new GameObject[0];

    [Header("═══ MANZANAS / BLOQUES ═══")]
    [Tooltip("Número de manzanas en cada eje (X y Z). Total = filas × columnas.")]
    [Range(2, 8)]
    [SerializeField] private int filas    = 4;
    [Range(2, 8)]
    [SerializeField] private int columnas = 4;

    [Tooltip("Tamaño de cada manzana (metros). Incluye calle perimetral.")]
    [Range(20f, 80f)]
    [SerializeField] private float tamanoManzana = 40f;

    [Tooltip("Número máximo de casas por manzana.")]
    [Range(1, 8)]
    [SerializeField] private int casasPorManzana = 4;

    [Tooltip("Porcentaje de manzanas de pueblo vs urbanas. 1 = todo pueblo, 0 = todo urbano.")]
    [Range(0f, 1f)]
    [SerializeField] private float fraccionPueblo = 0.7f;

    [Header("═══ ZONA ACORDONADA (POLICIAL) ═══")]
    [Tooltip("Centro de la zona acordonada con vallas policiales.")]
    [SerializeField] private Vector3 centroAcordonamiento = new Vector3(80f, 0f, 0f);
    [Tooltip("Radio de la zona acordonada (metros).")]
    [Range(10f, 80f)]
    [SerializeField] private float radioAcordonamiento = 30f;
    [Tooltip("Colocar vallas alrededor del acordonamiento policial.")]
    [SerializeField] private bool colocarVallas = true;
    [Tooltip("Separación entre postes de valla (metros).")]
    [Range(1f, 6f)]
    [SerializeField] private float separacionValla = 3f;

    [Header("═══ ZONA EXCLUIDA (manifestación) ═══")]
    [Tooltip("Centro de la manifestación — no se colocan edificios aquí.")]
    [SerializeField] private Vector3 centroManifestacion = new Vector3(-50f, 0f, 0f);
    [Tooltip("Radio libre alrededor de la manifestación (metros).")]
    [Range(20f, 100f)]
    [SerializeField] private float radioExclusion = 55f;

    // ───────────────────────────────────────────────────────────────────────
    //  ESTADO INTERNO
    // ───────────────────────────────────────────────────────────────────────
    private readonly List<Material> _matsCreados = new List<Material>();
    private int _totalEdificios = 0;
    private int _totalVallas    = 0;

    // ───────────────────────────────────────────────────────────────────────
    //  UNITY
    // ───────────────────────────────────────────────────────────────────────
    private void Start()
    {
        SpawnearEdificios();
        if (colocarVallas) SpawnearVallas();
    }

    private void OnDestroy()
    {
        foreach (var m in _matsCreados)
            if (m != null) Object.Destroy(m);
        _matsCreados.Clear();
    }

    // ───────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ───────────────────────────────────────────────────────────────────────

    /// <summary>
    /// Inyectado por SistemaAssets.PropagarAssets() con los prefabs importados.
    /// </summary>
    public void AsignarPrefabs(GameObject casaPueblo, GameObject casaUrbana, GameObject valla)
    {
        prefabCasaPueblo = casaPueblo;
        prefabCasaUrbana = casaUrbana;
        prefabValla      = valla;

        AlsasuaLogger.Info("SistemaEdificios",
            $"Prefabs asignados → pueblo: {NombreO(casaPueblo)}, " +
            $"urbana: {NombreO(casaUrbana)}, valla: {NombreO(valla)}");
    }

    /// <summary>
    /// Asigna múltiples prefabs de edificios variados desde paquetes Downloads.
    /// Proporciona variedad visual: ruinas, casas adicionales, edificios especiales.
    /// </summary>
    public void AsignarPrefabsDescargas(GameObject[] ruinas, GameObject[] casasAdicionales, GameObject[] edificiosEspeciales)
    {
        _prefabsRuinas = ruinas ?? new GameObject[0];
        _prefabsCasasAdicionales = casasAdicionales ?? new GameObject[0];
        _prefabsEdificiosEspeciales = edificiosEspeciales ?? new GameObject[0];

        int totalExtra = _prefabsRuinas.Length + _prefabsCasasAdicionales.Length + _prefabsEdificiosEspeciales.Length;
        if (totalExtra > 0)
            AlsasuaLogger.Info("SistemaEdificios",
                $"✓ {totalExtra} prefabs de edificios Downloads asignados " +
                $"(ruinas: {_prefabsRuinas.Length}, casas: {_prefabsCasasAdicionales.Length}, especiales: {_prefabsEdificiosEspeciales.Length})");
    }

    // ───────────────────────────────────────────────────────────────────────
    //  SPAWN DE EDIFICIOS
    // ───────────────────────────────────────────────────────────────────────
    private void SpawnearEdificios()
    {
        var padre = new GameObject("Edificios_Alsasua");
        padre.transform.SetParent(transform);

        // Calcular esquina inferior izquierda de la cuadrícula centrada en (0,0,0)
        float anchoTotal = columnas * tamanoManzana;
        float altoTotal  = filas    * tamanoManzana;
        var   origen     = new Vector3(-anchoTotal * 0.5f, 0f, -altoTotal * 0.5f);

        for (int fila = 0; fila < filas; fila++)
        {
            for (int col = 0; col < columnas; col++)
            {
                var centroCelda = origen + new Vector3(
                    (col + 0.5f) * tamanoManzana,
                    0f,
                    (fila + 0.5f) * tamanoManzana);

                // Saltar si la manzana está en la zona de la manifestación
                if (Vector3.Distance(centroCelda, centroManifestacion) < radioExclusion)
                    continue;

                bool esPueblo = Random.value < fraccionPueblo;
                SpawnearManzana(centroCelda, esPueblo, padre.transform);
            }
        }

        AlsasuaLogger.Info("SistemaEdificios",
            $"✓ {_totalEdificios} edificios colocados en {filas}×{columnas} manzanas. " +
            $"Assets: {(prefabCasaPueblo != null ? prefabCasaPueblo.name : "procedural")}, " +
            $"{(prefabCasaUrbana != null ? prefabCasaUrbana.name : "procedural")}.");
    }

    private void SpawnearManzana(Vector3 centro, bool esPueblo, Transform padre)
    {
        float margen     = tamanoManzana * 0.15f;  // margen de calle
        float areaCasa   = tamanoManzana - margen * 2f;

        for (int i = 0; i < casasPorManzana; i++)
        {
            // Distribuir casas en cuadrícula interior de la manzana
            int   cols    = Mathf.CeilToInt(Mathf.Sqrt(casasPorManzana));
            int   fila    = i / cols;
            int   col     = i % cols;
            float paso    = areaCasa / cols;

            var posLocal  = new Vector3(
                -areaCasa * 0.5f + col * paso + paso * 0.5f + Random.Range(-paso * 0.15f, paso * 0.15f),
                0f,
                -areaCasa * 0.5f + fila * paso + paso * 0.5f + Random.Range(-paso * 0.15f, paso * 0.15f));

            var posicion  = centro + posLocal;
            float rotY    = Mathf.RoundToInt(Random.Range(0f, 3f)) * 90f; // alineado a cuadrícula

            SpawnearEdificio(posicion, rotY, esPueblo, padre);
        }
    }

    private void SpawnearEdificio(Vector3 posicion, float rotacionY, bool esPueblo, Transform padre)
    {
        // Probabilidad de usar assets Downloads en lugar de default
        float probDownloads = 0.3f;  // 30% chance de usar nuevos assets
        GameObject prefab = null;
        string tipoEdificio = "";

        if (Random.value < probDownloads)
        {
            // Intentar usar assets de Downloads
            float selector = Random.value;
            if (selector < 0.5f && _prefabsRuinas.Length > 0)
            {
                prefab = _prefabsRuinas[Random.Range(0, _prefabsRuinas.Length)];
                tipoEdificio = "Ruina";
            }
            else if (selector < 0.75f && _prefabsCasasAdicionales.Length > 0)
            {
                prefab = _prefabsCasasAdicionales[Random.Range(0, _prefabsCasasAdicionales.Length)];
                tipoEdificio = "CasaExtra";
            }
            else if (_prefabsEdificiosEspeciales.Length > 0)
            {
                prefab = _prefabsEdificiosEspeciales[Random.Range(0, _prefabsEdificiosEspeciales.Length)];
                tipoEdificio = "EdificioEspecial";
            }
        }

        // Fallback a prefabs default si no tenemos Downloads
        if (prefab == null)
        {
            prefab = esPueblo
                ? (prefabCasaPueblo ?? prefabCasaUrbana)
                : (prefabCasaUrbana ?? prefabCasaPueblo);
            tipoEdificio = esPueblo ? "CasaPueblo" : "CasaUrbana";
        }

        if (prefab != null)
        {
            var go = Instantiate(prefab, posicion, Quaternion.Euler(0f, rotacionY, 0f));
            go.name = $"{tipoEdificio}_{_totalEdificios}";
            go.transform.SetParent(padre);

            // Normalizar escala: casas deben medir entre 5 y 12m de fachada
            NormalizarEscalaEdificio(go, Random.Range(5f, 10f));
        }
        else
        {
            // Fallback procedural: caja con color
            CrearEdificioProcedural(posicion, rotacionY, esPueblo, padre);
        }

        _totalEdificios++;
    }

    private void NormalizarEscalaEdificio(GameObject go, float fachadaObjetivo)
    {
        var renderers = go.GetComponentsInChildren<Renderer>();
        if (renderers.Length == 0) return;

        var bounds = renderers[0].bounds;
        foreach (var r in renderers) bounds.Encapsulate(r.bounds);

        float tamanoActual = Mathf.Max(bounds.size.x, bounds.size.z);
        if (tamanoActual < 0.001f) return;

        go.transform.localScale *= fachadaObjetivo / tamanoActual;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  SPAWN DE VALLAS (acordonamiento policial)
    // ───────────────────────────────────────────────────────────────────────
    private void SpawnearVallas()
    {
        if (prefabValla == null && radioAcordonamiento <= 0f) return;

        var padre = new GameObject("Vallas_Policia");
        padre.transform.SetParent(transform);

        int segmentos = Mathf.Max(4, Mathf.RoundToInt(
            2f * Mathf.PI * radioAcordonamiento / separacionValla));

        for (int i = 0; i < segmentos; i++)
        {
            float angulo = (float)i / segmentos * Mathf.PI * 2f;
            var posicion = centroAcordonamiento + new Vector3(
                Mathf.Cos(angulo) * radioAcordonamiento,
                0f,
                Mathf.Sin(angulo) * radioAcordonamiento);

            float rotY = angulo * Mathf.Rad2Deg + 90f;

            if (prefabValla != null)
            {
                var go = Instantiate(prefabValla, posicion, Quaternion.Euler(0f, rotY, 0f));
                go.name = $"Valla_{i}";
                go.transform.SetParent(padre);
            }
            else
            {
                CrearVallaProcedural(posicion, rotY, padre.transform);
            }

            _totalVallas++;
        }

        AlsasuaLogger.Info("SistemaEdificios",
            $"✓ {_totalVallas} vallas colocadas alrededor del acordonamiento policial. " +
            $"Asset: {(prefabValla != null ? prefabValla.name : "procedural")}.");
    }

    // ───────────────────────────────────────────────────────────────────────
    //  FALLBACKS PROCEDURALES
    // ───────────────────────────────────────────────────────────────────────
    private void CrearEdificioProcedural(Vector3 posicion, float rotacionY,
        bool esPueblo, Transform padre)
    {
        float ancho = Random.Range(5f, 10f);
        float prof  = Random.Range(4f, 8f);
        float alto  = Random.Range(4f, 12f);

        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = esPueblo ? "CasaPueblo_proc" : "CasaUrbana_proc";
        go.transform.SetParent(padre);
        go.transform.position     = posicion + Vector3.up * alto * 0.5f;
        go.transform.rotation     = Quaternion.Euler(0f, rotacionY, 0f);
        go.transform.localScale   = new Vector3(ancho, alto, prof);

        // Color fachada: tonos tierra para pueblo, gris para urbano
        Color color = esPueblo
            ? new Color(Random.Range(0.70f, 0.85f), Random.Range(0.58f, 0.72f), Random.Range(0.45f, 0.60f))
            : new Color(Random.Range(0.55f, 0.70f), Random.Range(0.55f, 0.70f), Random.Range(0.55f, 0.70f));

        AsignarMaterial(go.GetComponent<Renderer>(), color);
        Object.Destroy(go.GetComponent<Collider>());
    }

    private void CrearVallaProcedural(Vector3 posicion, float rotacionY, Transform padre)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = "Valla_proc";
        go.transform.SetParent(padre);
        go.transform.position   = posicion + Vector3.up * 0.5f;
        go.transform.rotation   = Quaternion.Euler(0f, rotacionY, 0f);
        go.transform.localScale = new Vector3(separacionValla * 0.9f, 1f, 0.12f);
        AsignarMaterial(go.GetComponent<Renderer>(), new Color(0.85f, 0.85f, 0.87f));
        Object.Destroy(go.GetComponent<Collider>());
    }

    private void AsignarMaterial(Renderer r, Color color)
    {
        var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
        if (shader == null) return;
        var mat = new Material(shader) { color = color };
        r.sharedMaterial = mat;
        _matsCreados.Add(mat);
    }

    // ───────────────────────────────────────────────────────────────────────
    //  GIZMOS
    // ───────────────────────────────────────────────────────────────────────
#if UNITY_EDITOR
    private void OnDrawGizmos()
    {
        // Cuadrícula de manzanas
        Gizmos.color = new Color(0.6f, 0.8f, 1f, 0.25f);
        float w = columnas * tamanoManzana;
        float h = filas    * tamanoManzana;
        Gizmos.DrawWireCube(Vector3.zero, new Vector3(w, 2f, h));

        // Zona de exclusión (manifestación)
        Gizmos.color = new Color(1f, 0.5f, 0f, 0.35f);
        DrawCircleGizmo(centroManifestacion, radioExclusion, 24);

        // Zona acordonada (policial)
        Gizmos.color = new Color(0.2f, 0.4f, 1f, 0.45f);
        DrawCircleGizmo(centroAcordonamiento, radioAcordonamiento, 24);
    }

    private static void DrawCircleGizmo(Vector3 c, float r, int seg)
    {
        for (int i = 0; i < seg; i++)
        {
            float a1 = (float)i       / seg * Mathf.PI * 2f;
            float a2 = (float)(i + 1) / seg * Mathf.PI * 2f;
            Gizmos.DrawLine(
                c + new Vector3(Mathf.Cos(a1), 0f, Mathf.Sin(a1)) * r,
                c + new Vector3(Mathf.Cos(a2), 0f, Mathf.Sin(a2)) * r);
        }
    }
#endif

    // ───────────────────────────────────────────────────────────────────────
    //  HELPERS
    // ───────────────────────────────────────────────────────────────────────
    private static string NombreO(Object o) => o != null ? o.name : "null";
}
