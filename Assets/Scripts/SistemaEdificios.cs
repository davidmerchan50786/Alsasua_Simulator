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

    [Header("═══ LAYOUT REAL (GeoDataCalles) ═══")]
    [Tooltip("Si es true, usa el layout real de manzanas y edificios de GeoDataCalles.ManzanasAlsasua. " +
             "Si es false, cae al modo cuadrícula genérico de abajo.")]
    [SerializeField] private bool usarLayoutReal = true;
    [Tooltip("Spawner edificios singulares: iglesia, ayuntamiento, GC, estación, etc.")]
    [SerializeField] private bool spawnearSingulares = true;

    [Header("═══ MANZANAS / BLOQUES (solo si usarLayoutReal = false) ═══")]
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
        if (usarLayoutReal)
        {
            SpawnearEdificiosLayoutReal();
            if (spawnearSingulares) SpawnearEdificiosSingulares();
        }
        else
        {
            SpawnearEdificios();   // modo cuadrícula genérica (legacy)
        }
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
    //  LAYOUT REAL — manzanas de GeoDataCalles
    // ───────────────────────────────────────────────────────────────────────

    /// <summary>
    /// Coloca edificios en el perímetro de cada manzana real de GeoDataCalles.ManzanasAlsasua.
    /// Los edificios se alinean a la fachada exterior (cara de la calle), reproduciendo
    /// la tipología de construcción cerrada propia de los cascos vascos.
    /// </summary>
    private void SpawnearEdificiosLayoutReal()
    {
        var padre = new GameObject("Edificios_Alsasua_Real");
        padre.transform.SetParent(transform);

        foreach (var m in GeoDataCalles.ManzanasAlsasua)
        {
            // Saltar si la manzana está en zona de exclusión (manifestación)
            if (Vector3.Distance(m.Centro, centroManifestacion) < radioExclusion)
                continue;

            SpawnearManzanaReal(m, padre.transform);
        }

        AlsasuaLogger.Info("SistemaEdificios",
            $"✓ {_totalEdificios} edificios colocados en {GeoDataCalles.ManzanasAlsasua.Length} " +
            "manzanas reales de Alsasua (GeoDataCalles).");
    }

    /// <summary>
    /// Coloca edificios en el perímetro de una manzana real.
    /// Estrategia: dividir cada lado del rectángulo en parcelas de fachada ~8-12m
    /// y spawnear un edificio por parcela mirando hacia la calle.
    /// </summary>
    private void SpawnearManzanaReal(GeoDataCalles.ManzanaData m, Transform padre)
    {
        float margenCalle = 0.5f;   // retranqueo mínimo desde el borde (m)
        float fachadaMin  = 7f;     // anchura mínima de parcela (m)
        float fachadaMax  = 14f;    // anchura máxima de parcela (m)

        // Altura: plantar ~ numPlantas * 3.2 metros
        float alturaEdificio = m.NumPlantas * 3.2f;

        Quaternion rotManzana = Quaternion.Euler(0f, m.RotacionY, 0f);

        // Los 4 lados del rectángulo (en espacio local de la manzana, sin rotar):
        //   Norte (+Z), Sur (-Z): fachada en X, profundidad en Z
        //   Este  (+X), Oeste(-X): fachada en Z, profundidad en X

        var lados = new (Vector3 inicio, Vector3 fin, float rotFachada, float profundidad)[]
        {
            // lado Norte: de SW a SE
            ( new Vector3(-m.TamanoX*0.5f + margenCalle, 0f,  m.TamanoZ*0.5f - margenCalle),
              new Vector3( m.TamanoX*0.5f - margenCalle, 0f,  m.TamanoZ*0.5f - margenCalle),
              0f, m.TamanoZ * 0.25f ),
            // lado Sur: de SE a SW
            ( new Vector3( m.TamanoX*0.5f - margenCalle, 0f, -m.TamanoZ*0.5f + margenCalle),
              new Vector3(-m.TamanoX*0.5f + margenCalle, 0f, -m.TamanoZ*0.5f + margenCalle),
              180f, m.TamanoZ * 0.25f ),
            // lado Este: de NE a SE
            ( new Vector3( m.TamanoX*0.5f - margenCalle, 0f,  m.TamanoZ*0.5f - margenCalle),
              new Vector3( m.TamanoX*0.5f - margenCalle, 0f, -m.TamanoZ*0.5f + margenCalle),
              90f, m.TamanoX * 0.25f ),
            // lado Oeste: de SW a NW
            ( new Vector3(-m.TamanoX*0.5f + margenCalle, 0f, -m.TamanoZ*0.5f + margenCalle),
              new Vector3(-m.TamanoX*0.5f + margenCalle, 0f,  m.TamanoZ*0.5f - margenCalle),
              270f, m.TamanoX * 0.25f ),
        };

        foreach (var (inicio, fin, rotFachada, profEdf) in lados)
        {
            float longitud   = Vector3.Distance(inicio, fin);
            float fachadaObj = Random.Range(fachadaMin, fachadaMax);
            int   numParc    = Mathf.Max(1, Mathf.RoundToInt(longitud / fachadaObj));
            float fachadaReal = longitud / numParc;

            for (int p = 0; p < numParc; p++)
            {
                float t = (p + 0.5f) / numParc;
                Vector3 posLocal = Vector3.Lerp(inicio, fin, t);

                // Pasar a espacio de mundo
                Vector3 posWorld = m.Centro + rotManzana * posLocal;

                // Alineación de la fachada: la cara mira hacia fuera del bloque
                float rotWorld = m.RotacionY + rotFachada;

                // Pequeña variación de posición para no quedar en cuadrícula perfecta
                posWorld.x += Random.Range(-0.5f, 0.5f);
                posWorld.z += Random.Range(-0.5f, 0.5f);

                bool esCascoViejo = (m.Tipo == GeoDataCalles.TipoEdificio.CascoAntiguo);
                bool esIndustrial = (m.Tipo == GeoDataCalles.TipoEdificio.Industrial);

                if (esIndustrial)
                    CrearNaveIndustrial(posWorld, rotWorld, fachadaReal, profEdf, alturaEdificio * 0.6f, padre);
                else
                    SpawnearEdificioFachada(posWorld, rotWorld, fachadaReal, profEdf,
                                            alturaEdificio, esCascoViejo, padre);
            }
        }
    }

    /// <summary>
    /// Spawnea un edificio de fachada (alineado a calle) en la posición dada.
    /// Usa prefabs si están disponibles, si no crea geometría procedural.
    /// </summary>
    private void SpawnearEdificioFachada(Vector3 posicion, float rotacionY,
        float anchoFachada, float profundidad, float altura,
        bool cascoViejo, Transform padre)
    {
        // Selección de prefab según tipología
        GameObject prefab = SeleccionarPrefab(cascoViejo);

        if (prefab != null)
        {
            var go = Instantiate(prefab, posicion, Quaternion.Euler(0f, rotacionY, 0f));
            go.name = $"Edf_{_totalEdificios:D3}";
            go.transform.SetParent(padre);
            NormalizarEscalaEdificio(go, anchoFachada);
        }
        else
        {
            // Fallback procedural con dimensiones reales
            CrearEdificioProcedural(posicion, rotacionY, anchoFachada, profundidad, altura, cascoViejo, padre);
        }
        _totalEdificios++;
    }

    private GameObject SeleccionarPrefab(bool cascoViejo)
    {
        // 30% probabilidad de usar assets de Downloads
        if (Random.value < 0.30f)
        {
            if (cascoViejo && _prefabsRuinas.Length > 0)
                return _prefabsRuinas[Random.Range(0, _prefabsRuinas.Length)];
            if (_prefabsCasasAdicionales.Length > 0)
                return _prefabsCasasAdicionales[Random.Range(0, _prefabsCasasAdicionales.Length)];
        }
        if (cascoViejo)
            return prefabCasaPueblo ?? prefabCasaUrbana;
        return (Random.value < fraccionPueblo)
            ? (prefabCasaPueblo ?? prefabCasaUrbana)
            : (prefabCasaUrbana ?? prefabCasaPueblo);
    }

    private void CrearEdificioProcedural(Vector3 posicion, float rotacionY,
        float ancho, float prof, float alto, bool cascoViejo, Transform padre)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = cascoViejo ? $"CascoViejo_{_totalEdificios:D3}" : $"Residencial_{_totalEdificios:D3}";
        go.transform.SetParent(padre);
        go.transform.position   = posicion + Vector3.up * alto * 0.5f;
        go.transform.rotation   = Quaternion.Euler(0f, rotacionY, 0f);
        go.transform.localScale = new Vector3(ancho, alto, prof);

        // Paleta cromática según tipología
        Color color;
        if (cascoViejo)
            color = new Color(
                Random.Range(0.68f, 0.82f),
                Random.Range(0.60f, 0.72f),
                Random.Range(0.46f, 0.58f));   // piedra beige/ocre
        else
            color = new Color(
                Random.Range(0.72f, 0.88f),
                Random.Range(0.70f, 0.85f),
                Random.Range(0.65f, 0.80f));   // enfoscado blanco/crema

        AsignarMaterial(go.GetComponent<Renderer>(), color);
        Object.Destroy(go.GetComponent<Collider>());
    }

    private void CrearNaveIndustrial(Vector3 posicion, float rotacionY,
        float ancho, float prof, float alto, Transform padre)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = $"Nave_{_totalEdificios:D3}";
        go.transform.SetParent(padre);
        go.transform.position   = posicion + Vector3.up * alto * 0.5f;
        go.transform.rotation   = Quaternion.Euler(0f, rotacionY, 0f);
        go.transform.localScale = new Vector3(ancho, alto, prof);

        // Naves: gris metalizado
        Color color = new Color(
            Random.Range(0.58f, 0.70f),
            Random.Range(0.60f, 0.72f),
            Random.Range(0.62f, 0.74f));
        AsignarMaterial(go.GetComponent<Renderer>(), color);
        Object.Destroy(go.GetComponent<Collider>());
    }

    /// <summary>
    /// Spawna los edificios singulares definidos en GeoDataCalles.EdificiosSingulares:
    /// ayuntamiento, iglesia, cuartel GC, comisaría PF, estación de tren, etc.
    /// </summary>
    private void SpawnearEdificiosSingulares()
    {
        var padre = new GameObject("Edificios_Singulares");
        padre.transform.SetParent(transform);

        foreach (var e in GeoDataCalles.EdificiosSingulares)
        {
            // Intentar usar un prefab especial si está disponible
            GameObject prefab = _prefabsEdificiosEspeciales.Length > 0
                ? _prefabsEdificiosEspeciales[Random.Range(0, _prefabsEdificiosEspeciales.Length)]
                : null;

            if (prefab != null)
            {
                var go = Instantiate(prefab, e.Centro, Quaternion.identity);
                go.name = e.Nombre;
                go.transform.SetParent(padre);
                NormalizarEscalaEdificio(go, e.TamanoX);
            }
            else
            {
                // Fallback procedural con geometría real del edificio
                CrearEdificioSingularProcedural(e, padre.transform);
            }
            _totalEdificios++;
        }

        AlsasuaLogger.Info("SistemaEdificios",
            $"✓ {GeoDataCalles.EdificiosSingulares.Length} edificios singulares colocados " +
            "(iglesia, ayuntamiento, GC, PF, estación, polideportivo...).");
    }

    private void CrearEdificioSingularProcedural(GeoDataCalles.EdificioSingular e, Transform padre)
    {
        // Cuerpo principal
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = e.Nombre;
        go.transform.SetParent(padre);
        go.transform.position   = e.Centro + Vector3.up * e.Altura * 0.5f;
        go.transform.localScale = new Vector3(e.TamanoX, e.Altura, e.TamanoZ);

        Color color;
        switch (e.Tipo)
        {
            case GeoDataCalles.TipoEdificio.Institucional:
                color = new Color(0.80f, 0.78f, 0.68f);  // piedra arenisca gris
                break;
            case GeoDataCalles.TipoEdificio.Deportivo:
                color = new Color(0.60f, 0.75f, 0.82f);  // azul deportivo
                break;
            case GeoDataCalles.TipoEdificio.Industrial:
                color = new Color(0.62f, 0.64f, 0.66f);  // gris metálico
                break;
            default:
                color = new Color(0.75f, 0.70f, 0.62f);
                break;
        }
        AsignarMaterial(go.GetComponent<Renderer>(), color);
        Object.Destroy(go.GetComponent<Collider>());

        // Torre/campanario para la iglesia
        if (e.Nombre.Contains("Iglesia") || e.Nombre.Contains("iglesia"))
        {
            float campanarioRelH = 1f + 4f / e.Altura;  // más alto que el cuerpo principal
            var torre = GameObject.CreatePrimitive(PrimitiveType.Cube);
            torre.name = "Campanario";
            torre.transform.SetParent(go.transform);
            torre.transform.localPosition = new Vector3(-0.4f, campanarioRelH * 0.5f, 0f);
            torre.transform.localScale    = new Vector3(
                0.22f,
                campanarioRelH,
                0.22f * e.TamanoX / Mathf.Max(0.1f, e.TamanoZ));
            AsignarMaterial(torre.GetComponent<Renderer>(), new Color(0.78f, 0.75f, 0.65f));
            Object.Destroy(torre.GetComponent<Collider>());
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  SPAWN DE EDIFICIOS (modo cuadrícula genérica — legacy)
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
                go.transform.SetParent(padre.transform);
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
        if (usarLayoutReal)
        {
            // Manzanas reales de GeoDataCalles
            foreach (var m in GeoDataCalles.ManzanasAlsasua)
            {
                // Color según tipología
                switch (m.Tipo)
                {
                    case GeoDataCalles.TipoEdificio.CascoAntiguo:   Gizmos.color = new Color(0.85f, 0.70f, 0.30f, 0.45f); break;
                    case GeoDataCalles.TipoEdificio.Residencial:    Gizmos.color = new Color(0.40f, 0.70f, 0.90f, 0.40f); break;
                    case GeoDataCalles.TipoEdificio.Comercial:      Gizmos.color = new Color(0.90f, 0.40f, 0.60f, 0.40f); break;
                    case GeoDataCalles.TipoEdificio.Industrial:     Gizmos.color = new Color(0.60f, 0.60f, 0.65f, 0.40f); break;
                    case GeoDataCalles.TipoEdificio.Institucional:  Gizmos.color = new Color(1.00f, 0.90f, 0.20f, 0.50f); break;
                    default:                                        Gizmos.color = new Color(0.50f, 0.80f, 0.50f, 0.40f); break;
                }
                // Dibujar rectángulo de la manzana
                var rot = Quaternion.Euler(0f, m.RotacionY, 0f);
                var corners = new Vector3[]
                {
                    m.Centro + rot * new Vector3(-m.TamanoX*0.5f, 0f, -m.TamanoZ*0.5f),
                    m.Centro + rot * new Vector3( m.TamanoX*0.5f, 0f, -m.TamanoZ*0.5f),
                    m.Centro + rot * new Vector3( m.TamanoX*0.5f, 0f,  m.TamanoZ*0.5f),
                    m.Centro + rot * new Vector3(-m.TamanoX*0.5f, 0f,  m.TamanoZ*0.5f),
                };
                for (int i = 0; i < 4; i++)
                    Gizmos.DrawLine(corners[i], corners[(i+1)%4]);
                // Altura indicativa
                Gizmos.DrawWireCube(m.Centro + Vector3.up * m.NumPlantas * 1.6f,
                    new Vector3(m.TamanoX, m.NumPlantas * 3.2f, m.TamanoZ));
            }

            // Edificios singulares en amarillo
            Gizmos.color = new Color(1f, 0.9f, 0f, 0.6f);
            foreach (var e in GeoDataCalles.EdificiosSingulares)
            {
                Gizmos.DrawWireCube(e.Centro + Vector3.up * e.Altura * 0.5f,
                    new Vector3(e.TamanoX, e.Altura, e.TamanoZ));
                Gizmos.DrawSphere(e.Centro, 2f);
            }
        }
        else
        {
            // Cuadrícula genérica (legacy)
            Gizmos.color = new Color(0.6f, 0.8f, 1f, 0.25f);
            float w = columnas * tamanoManzana;
            float h = filas    * tamanoManzana;
            Gizmos.DrawWireCube(Vector3.zero, new Vector3(w, 2f, h));
        }

        // Zona de exclusión (manifestación)
        Gizmos.color = new Color(1f, 0.5f, 0f, 0.35f);
        DrawCircleGizmo(centroManifestacion, radioExclusion, 24);

        // Zona acordonada (policial)
        Gizmos.color = new Color(0.2f, 0.4f, 1f, 0.45f);
        DrawCircleGizmo(centroAcordonamiento, radioAcordonamiento, 24);

        // Calles principales (en el Editor, verde suave)
        Gizmos.color = new Color(0.3f, 0.9f, 0.4f, 0.50f);
        foreach (var c in GeoDataCalles.CallesPrincipales)
        {
            for (int i = 0; i < c.Puntos.Length - 1; i++)
                Gizmos.DrawLine(c.Puntos[i], c.Puntos[i+1]);
        }
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
