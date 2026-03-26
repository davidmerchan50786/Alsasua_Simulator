// Assets/Scripts/SistemaAssets.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Cargador centralizado de assets externos para el simulador de Alsasua.
//
//  FLUJO DE USO:
//  1. Descarga los modelos desde los enlaces en menú Unity → Alsasua → Descargar Assets Externos
//  2. Importa cada modelo en la carpeta indicada dentro de Assets/
//  3. SistemaAssets los carga automáticamente en Awake() via Resources.Load
//     y los propaga a SistemaPersonajes, SistemaTrafico y SistemaVegetacion.
//
//  ESTRUCTURA DE CARPETAS ESPERADA (crear manualmente):
//    Assets/Resources/Personajes/GuardiaCivil/   ← model FBX o GLB
//    Assets/Resources/Personajes/Keffiyeh/        ← Palestinian Scarf FBX
//    Assets/Resources/Personajes/Civiles/         ← City People FBX
//    Assets/Resources/Vehiculos/Patrulla/         ← Land Cruiser GC FBX
//    Assets/Resources/Vehiculos/Civil/            ← Seat Ibiza FBX
//    Assets/Resources/Vegetacion/                 ← Forest Sample prefabs
//
//  Si un asset no se encuentra, el sistema usa el fallback procedural del
//  script correspondiente (cápsula para personajes, mesh simple para vehículos).
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;

[DefaultExecutionOrder(-20)]    // ejecutar ANTES que GestorEscena (-10) y sistemas
public sealed class SistemaAssets : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  SINGLETON
    // ───────────────────────────────────────────────────────────────────────
    public static SistemaAssets Instancia { get; private set; }

    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR — override manual (arrastra los assets directamente)
    // ───────────────────────────────────────────────────────────────────────
    [Header("═══ PERSONAJES — override Inspector (opcional) ═══")]
    [Tooltip("FBX del modelo Guardia Civil (Sketchfab)")]
    [SerializeField] private Mesh meshGuardiaCivil;
    [Tooltip("FBX del modelo Policía Foral (genérico policeman re-skinned)")]
    [SerializeField] private Mesh meshPoliciaForal;
    [Tooltip("FBX keffiyeh / pañuelo (Palestinian Scarf Sketchfab)")]
    [SerializeField] private Mesh meshKeffiyeh;
    [Tooltip("FBX personaje civil masculino (City People FREE)")]
    [SerializeField] private Mesh meshCivilMale;
    [Tooltip("FBX personaje civil femenino (City People FREE)")]
    [SerializeField] private Mesh meshCivilFemale;

    [Header("═══ TEXTURAS BANDERAS (SVG convertido a PNG 512×320) ═══")]
    [Tooltip("Ikurriña PNG (Wikimedia Commons — dominio público)")]
    [SerializeField] private Texture2D texIkurriña;
    [Tooltip("Bandera de Navarra PNG (Wikimedia Commons — dominio público)")]
    [SerializeField] private Texture2D texNavarra;

    [Header("═══ VEHÍCULOS ═══")]
    [Tooltip("FBX Toyota Land Cruiser Guardia Civil (Sketchfab CC-BY)")]
    [SerializeField] private GameObject prefabPatrullaGC;
    [Tooltip("FBX Seat Ibiza civil (CGTrader)")]
    [SerializeField] private GameObject prefabCocheCivil;

    [Header("═══ VEGETACIÓN ═══")]
    [Tooltip("Prefab árbol pino (Environment Pack Free Forest)")]
    [SerializeField] private GameObject prefabPino;
    [Tooltip("Prefab árbol roble/caducifolio (Environment Pack Free Forest)")]
    [SerializeField] private GameObject prefabRoble;

    [Header("═══ VFX ═══")]
    [Tooltip("Prefab de explosión (Mirza Beig — Cinematic Explosions FREE). " +
             "Asignar 'Explosion FREE 1 Variant.prefab'. Se propaga a SistemaExplosion.PrefabExplosion.")]
    [SerializeField] private GameObject prefabExplosion;
    [Tooltip("Prefab de fuego suelo (Free Fire VFX — VFX_Fire_Floor_01.prefab). " +
             "Se propaga a BarricadaFuego.prefabVFXFuego vía SistemaAssets.")]
    [SerializeField] private GameObject prefabVFXFuego;

    [Header("═══ AUDIO ═══")]
    [Tooltip("Sonido ambiente multitud (loop)")]
    [SerializeField] private AudioClip audioMultitud;
    [Tooltip("Sonido sirena policía (loop)")]
    [SerializeField] private AudioClip audioSirena;
    [Tooltip("Sonido tren pasando")]
    [SerializeField] private AudioClip audioTren;

    // ───────────────────────────────────────────────────────────────────────
    //  RUTAS RESOURCES (fallback si los campos Inspector están vacíos)
    // ───────────────────────────────────────────────────────────────────────
    private const string PATH_GC_MESH         = "Personajes/GuardiaCivil/CharacterGC";
    private const string PATH_PF_MESH         = "Personajes/PoliciaForal/CharacterPF";
    private const string PATH_KEFFIYEH        = "Personajes/Keffiyeh/KeffiyehMesh";
    private const string PATH_CIVIL_MALE      = "Personajes/Civiles/CharacterMale";
    private const string PATH_CIVIL_FEMALE    = "Personajes/Civiles/CharacterFemale";
    private const string PATH_TEX_IKURRINA    = "Banderas/Ikurrina";
    private const string PATH_TEX_NAVARRA     = "Banderas/Navarra";
    private const string PATH_PATRULLA_GC     = "Vehiculos/Patrulla/PatrullaGC";
    private const string PATH_COCHE_CIVIL     = "Vehiculos/Civil/CocheCivil";
    private const string PATH_PREFAB_PINO     = "Vegetacion/Pino";
    private const string PATH_PREFAB_ROBLE    = "Vegetacion/Roble";
    private const string PATH_EXPLOSION       = "VFX/Explosion";
    private const string PATH_VFX_FUEGO       = "VFX/FuegoSuelo";

    // ───────────────────────────────────────────────────────────────────────
    //  PROPIEDADES PÚBLICAS (usadas por GestorEscena y los sistemas)
    // ───────────────────────────────────────────────────────────────────────
    public Mesh        MeshGuardiaCivil   => meshGuardiaCivil;
    public Mesh        MeshPoliciaForal   => meshPoliciaForal;
    public Mesh        MeshKeffiyeh       => meshKeffiyeh;
    public Mesh        MeshCivilMale      => meshCivilMale;
    public Mesh        MeshCivilFemale    => meshCivilFemale;
    public Texture2D   TexIkurriña        => texIkurriña;
    public Texture2D   TexNavarra         => texNavarra;
    public GameObject  PrefabPatrullaGC   => prefabPatrullaGC;
    public GameObject  PrefabCocheCivil   => prefabCocheCivil;
    public GameObject  PrefabPino         => prefabPino;
    public GameObject  PrefabRoble        => prefabRoble;
    public GameObject  PrefabExplosion    => prefabExplosion;
    public GameObject  PrefabVFXFuego     => prefabVFXFuego;
    public AudioClip   AudioMultitud      => audioMultitud;
    public AudioClip   AudioSirena        => audioSirena;
    public AudioClip   AudioTren          => audioTren;

    /// <summary>True si los assets críticos (al menos un personaje) están cargados.</summary>
    public bool AssetsListos => meshGuardiaCivil != null || meshCivilMale != null;

    // ───────────────────────────────────────────────────────────────────────
    //  UNITY
    // ───────────────────────────────────────────────────────────────────────
    private void Awake()
    {
        if (Instancia != null && Instancia != this)
        {
            Destroy(gameObject);
            return;
        }
        Instancia = this;

        CargarDesdeResources();
        PropagarAssets();
        LogEstadoAssets();
    }

    private void OnDestroy()
    {
        // BUG FIX: limpiar referencia estática al destruirse.
        // Sin esto, si la escena se recarga, Instancia apunta a un objeto destruido
        // y cualquier acceso posterior lanza MissingReferenceException.
        if (Instancia == this) Instancia = null;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  CARGA DESDE RESOURCES
    // ───────────────────────────────────────────────────────────────────────
    private void CargarDesdeResources()
    {
        // Meshes de personajes
        meshGuardiaCivil ??= CargarMesh(PATH_GC_MESH);
        meshPoliciaForal ??= CargarMesh(PATH_PF_MESH);
        meshKeffiyeh     ??= CargarMesh(PATH_KEFFIYEH);
        meshCivilMale    ??= CargarMesh(PATH_CIVIL_MALE);
        meshCivilFemale  ??= CargarMesh(PATH_CIVIL_FEMALE);

        // Texturas de banderas
        texIkurriña ??= Resources.Load<Texture2D>(PATH_TEX_IKURRINA);
        texNavarra  ??= Resources.Load<Texture2D>(PATH_TEX_NAVARRA);

        // Vehículos (Prefab con MeshFilter)
        if (prefabPatrullaGC == null) prefabPatrullaGC = Resources.Load<GameObject>(PATH_PATRULLA_GC);
        if (prefabCocheCivil == null) prefabCocheCivil = Resources.Load<GameObject>(PATH_COCHE_CIVIL);

        // Vegetación
        if (prefabPino  == null) prefabPino  = Resources.Load<GameObject>(PATH_PREFAB_PINO);
        if (prefabRoble == null) prefabRoble = Resources.Load<GameObject>(PATH_PREFAB_ROBLE);

        // VFX
        if (prefabExplosion == null) prefabExplosion = Resources.Load<GameObject>(PATH_EXPLOSION);
        if (prefabVFXFuego  == null) prefabVFXFuego  = Resources.Load<GameObject>(PATH_VFX_FUEGO);
    }

    // ───────────────────────────────────────────────────────────────────────
    //  PROPAGACIÓN DE ASSETS A SISTEMAS
    // ───────────────────────────────────────────────────────────────────────
    /// <summary>
    /// Propaga los assets cargados a los sistemas que no pueden recibir
    /// references directas desde el Inspector (campos estáticos, prefabs
    /// instanciados en runtime, etc.).
    /// Llamado desde Awake() justo después de CargarDesdeResources().
    /// </summary>
    private void PropagarAssets()
    {
        // SistemaExplosion usa un campo estático para el prefab de explosión.
        if (prefabExplosion != null)
            SistemaExplosion.PrefabExplosion = prefabExplosion;
    }

    // Intenta cargar el primer Mesh de un asset FBX/GLB desde Resources
    private static Mesh CargarMesh(string path)
    {
        // Resources.Load<Mesh> funciona si el FBX está en Resources y tiene un solo mesh.
        // Para FBX con múltiples meshes, usar Resources.LoadAll<Mesh>.
        var mesh = Resources.Load<Mesh>(path);
        if (mesh == null)
        {
            // Intento con LoadAll (FBX con sub-meshes)
            var todos = Resources.LoadAll<Mesh>(path);
            if (todos != null && todos.Length > 0) mesh = todos[0];
        }
        return mesh;   // null si no se encuentra → fallback procedural en el sistema
    }

    // ───────────────────────────────────────────────────────────────────────
    //  LOG DE ESTADO
    // ───────────────────────────────────────────────────────────────────────
    private void LogEstadoAssets()
    {
        int cargados = 0, fallbacks = 0;

        Cuenta("Guardia Civil mesh",  meshGuardiaCivil  != null, ref cargados, ref fallbacks);
        Cuenta("Policía Foral mesh",  meshPoliciaForal  != null, ref cargados, ref fallbacks);
        Cuenta("Keffiyeh mesh",       meshKeffiyeh      != null, ref cargados, ref fallbacks);
        Cuenta("Civil Male mesh",     meshCivilMale     != null, ref cargados, ref fallbacks);
        Cuenta("Civil Female mesh",   meshCivilFemale   != null, ref cargados, ref fallbacks);
        Cuenta("Textura ikurriña",    texIkurriña       != null, ref cargados, ref fallbacks);
        Cuenta("Textura Navarra",     texNavarra        != null, ref cargados, ref fallbacks);
        Cuenta("Prefab patrulla GC",  prefabPatrullaGC  != null, ref cargados, ref fallbacks);
        Cuenta("Prefab coche civil",  prefabCocheCivil  != null, ref cargados, ref fallbacks);
        Cuenta("Prefab pino",         prefabPino        != null, ref cargados, ref fallbacks);
        Cuenta("Prefab roble",        prefabRoble       != null, ref cargados, ref fallbacks);
        Cuenta("Prefab explosión",    prefabExplosion   != null, ref cargados, ref fallbacks);
        Cuenta("Prefab VFX fuego",    prefabVFXFuego    != null, ref cargados, ref fallbacks);

        if (fallbacks > 0)
        {
            AlsasuaLogger.Warn("SistemaAssets",
                $"{cargados} assets externos cargados, {fallbacks} usarán fallback procedural.\n" +
                "Para cargar los assets reales: menú Unity → Alsasua → Descargar Assets Externos");
        }
        else
        {
            AlsasuaLogger.Info("SistemaAssets", $"✓ Todos los assets externos cargados ({cargados}/{cargados + fallbacks}).");
        }
    }

    private static void Cuenta(string nombre, bool ok, ref int cargados, ref int fallbacks)
    {
        if (ok) cargados++;
        else
        {
            fallbacks++;
            AlsasuaLogger.Verbose("SistemaAssets", $"Asset no encontrado (fallback procedural): {nombre}");
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ───────────────────────────────────────────────────────────────────────

    /// <summary>
    /// Extrae el Mesh del primer MeshFilter de un Prefab importado desde Sketchfab.
    /// Útil cuando el Prefab tiene varios sub-meshes anidados (ej. vehículos).
    /// </summary>
    public static Mesh ExtraerMeshDePrefab(GameObject prefab)
    {
        if (prefab == null) return null;
        var mf = prefab.GetComponentInChildren<MeshFilter>();
        return mf != null ? mf.sharedMesh : null;
    }

    /// <summary>
    /// Extrae el Material del primer Renderer de un Prefab.
    /// </summary>
    public static Material ExtraerMaterialDePrefab(GameObject prefab)
    {
        if (prefab == null) return null;
        var mr = prefab.GetComponentInChildren<MeshRenderer>();
        return mr != null ? mr.sharedMaterial : null;
    }

    /// <summary>
    /// Crea una instancia del Prefab de patrulla en la posición dada,
    /// correctamente escalado a ~4.5m de largo.
    /// </summary>
    public GameObject InstanciarPatrullaGC(Vector3 posicion, Quaternion rotacion)
    {
        if (prefabPatrullaGC == null) return null;
        var go = Instantiate(prefabPatrullaGC, posicion, rotacion);
        go.name = "PatrullaGC";
        // Normalizar escala (Sketchfab suele exportar en cm → * 0.01)
        NormalizarEscalaVehiculo(go, 4.5f);
        return go;
    }

    /// <summary>
    /// Crea una instancia del Prefab de coche civil escalado a ~4m.
    /// </summary>
    public GameObject InstanciarCocheCivil(Vector3 posicion, Quaternion rotacion)
    {
        if (prefabCocheCivil == null) return null;
        var go = Instantiate(prefabCocheCivil, posicion, rotacion);
        go.name = "CocheCivil";
        NormalizarEscalaVehiculo(go, 4.0f);
        return go;
    }

    private static void NormalizarEscalaVehiculo(GameObject go, float longitudObjetivo)
    {
        // Calcular la escala necesaria para que el bounding box X mida longitudObjetivo
        var renderers = go.GetComponentsInChildren<Renderer>();
        if (renderers.Length == 0) return;

        var bounds = renderers[0].bounds;
        foreach (var r in renderers) bounds.Encapsulate(r.bounds);

        float escalaActual = bounds.size.x;
        if (escalaActual < 0.001f) return;

        float factor = longitudObjetivo / escalaActual;
        go.transform.localScale *= factor;
    }
}
