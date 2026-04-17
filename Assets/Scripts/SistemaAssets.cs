// Assets/Scripts/SistemaAssets.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Cargador centralizado de assets externos para el simulador de Alsasua.
//
//  FLUJO DE USO:
//  1. Descarga los modelos desde los enlaces en menú Unity → Alsasua → Descargar Assets Externos
//  2. Importa cada modelo en la carpeta indicada dentro de Assets/
//  3. SistemaAssets los carga automáticamente en Awake() via Resources.Load
//     y los propaga a SistemaPersonajes, SistemaTrafico, SistemaBarricadas y SistemaFarolas.
//
//  ESTRUCTURA DE CARPETAS ESPERADA (crear manualmente):
//    Assets/Resources/Personajes/GuardiaCivil/    ← model FBX o GLB
//    Assets/Resources/Personajes/Keffiyeh/         ← Palestinian Scarf FBX
//    Assets/Resources/Personajes/Civiles/          ← City People FBX
//    Assets/Resources/Personajes/Soldado/          ← LowPolySoldiers Soldier_demo.FBX
//    Assets/Resources/Vehiculos/Patrulla/          ← Police Car Interceptor.prefab
//    Assets/Resources/Vehiculos/Helicoptero/       ← Police Helicopter.prefab
//    Assets/Resources/Vehiculos/Civil/             ← Hot Rod LOD0.FBX
//    Assets/Resources/Vegetacion/                  ← Forest Sample prefabs
//    Assets/Resources/Barricadas/Hormigon/         ← Concrete_Barrier_1.prefab
//    Assets/Resources/Barricadas/Metal/            ← Metal_Barrier_1.prefab
//    Assets/Resources/Barricadas/BarricadaPack/    ← Barricada Concreto.FBX
//    Assets/Resources/Farolas/                     ← StreetLampRound1A.prefab
//    Assets/Resources/VFX/FuegoLite/               ← LiteFireEffect BaseFire000.prefab
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

    [Header("═══ PERSONAJES — SOLDADO ═══")]
    [Tooltip("Prefab o FBX LowPolySoldiers (Soldier_demo.FBX). " +
             "Se usa para Guardia Civil y Policía Foral si meshGuardiaCivil/meshPoliciaForal están vacíos.")]
    [SerializeField] private GameObject prefabSoldado;

    [Header("═══ VEHÍCULOS ═══")]
    [Tooltip("Prefab Police Car Interceptor (Police Car & Helicopter/Prefabs/Interceptor.prefab). " +
             "Reemplaza al antiguo Land Cruiser GC. Se propaga a SistemaTrafico.")]
    [SerializeField] private GameObject prefabPatrullaGC;
    [Tooltip("FBX / Prefab vehículo civil (Hot Rod LOD0.FBX o similar)")]
    [SerializeField] private GameObject prefabCocheCivil;
    [Tooltip("Prefab Police Helicopter (Police Car & Helicopter/Prefabs/Helicopter.prefab). " +
             "Usado por SistemaTrafico para vigilancia aérea.")]
    [SerializeField] private GameObject prefabHelicoptero;

    [Header("═══ BARRICADAS ═══")]
    [Tooltip("Prefab barricada de hormigón (Abandoned World/Metal and Concrete Barrier/Prefabs/Concrete_Barrier_1.prefab). " +
             "Se propaga a SistemaBarricadas.")]
    [SerializeField] private GameObject prefabBarricadaHormigon;
    [Tooltip("Prefab barricada metálica (Abandoned World/Metal and Concrete Barrier/Prefabs/Metal_Barrier_1.prefab). " +
             "Se propaga a SistemaBarricadas.")]
    [SerializeField] private GameObject prefabBarricadaMetal;
    [Tooltip("Prefab barricada de hormigón pack alternativo (BarrierPack/Assets/Barricada Concreto.FBX). " +
             "Fallback si prefabBarricadaHormigon está vacío.")]
    [SerializeField] private GameObject prefabBarricadaPack;

    [Header("═══ FAROLAS ═══")]
    [Tooltip("Prefab farola urbana (SpaceZeta_StreetLamps2/Prefabs/StreetLampRound1A.prefab). " +
             "Se propaga a SistemaFarolas.")]
    [SerializeField] private GameObject prefabFarola;

    [Header("═══ VEGETACIÓN ═══")]
    [Tooltip("Prefab árbol pino (Environment Pack Free Forest)")]
    [SerializeField] private GameObject prefabPino;
    [Tooltip("Prefab árbol roble/caducifolio (Environment Pack Free Forest)")]
    [SerializeField] private GameObject prefabRoble;
    [Tooltip("Prefab arbusto (Yughues Free Bushes). Se propaga a SistemaVegetacion.")]
    [SerializeField] private GameObject prefabArbusto;
    [Tooltip("Prefab árbol low poly alternativo (Free Low Poly Nature Forest o Mobile Tree Package). " +
             "Se propaga a SistemaVegetacion como árbol extra.")]
    [SerializeField] private GameObject prefabArbolExtra;

    [Header("═══ EDIFICIOS ═══")]
    [Tooltip("Prefab casa pueblo (Village Houses Pack — GBAndrewGB). " +
             "Se propaga a SistemaEdificios.")]
    [SerializeField] private GameObject prefabCasaPueblo;
    [Tooltip("Prefab casa urbana (House Pack — Mehdi Rabiee). " +
             "Se propaga a SistemaEdificios.")]
    [SerializeField] private GameObject prefabCasaUrbana;
    [Tooltip("Prefab valla/cerco modular (Modular self-stand fence — Aleksey Kozhemyakin). " +
             "Usado por SistemaEdificios para acordonar zonas policiales.")]
    [SerializeField] private GameObject prefabValla;

    [Header("═══ VFX ═══")]
    [Tooltip("Prefab de explosión (Mirza Beig — Cinematic Explosions FREE). " +
             "Asignar 'Explosion FREE 1 Variant.prefab'. Se propaga a SistemaExplosion.PrefabExplosion.")]
    [SerializeField] private GameObject prefabExplosion;
    [Tooltip("Prefab de fuego suelo (Free Fire VFX — VFX_Fire_Floor_01.prefab). " +
             "Se propaga a SistemaBarricadas y BarricadaFuego vía SistemaAssets.")]
    [SerializeField] private GameObject prefabVFXFuego;
    [Tooltip("Prefab de fuego alternativo (LiteFireEffect/Prafab/BaseFire000.prefab). " +
             "Usado como VFX secundario de llamas en barricadas y escena.")]
    [SerializeField] private GameObject prefabVFXFuegoLite;

    [Header("═══ AUDIO ═══")]
    [Tooltip("Sonido ambiente multitud (loop). " +
             "Asignar 'Crowd - Cheering - Ambience.wav' (Gregor Quendel).")]
    [SerializeField] private AudioClip audioMultitud;
    [Tooltip("Sonido multitud coreando rítmico (loop). " +
             "Asignar 'Crowd - Cheering - Rhythmic cheering.wav' (Gregor Quendel).")]
    [SerializeField] private AudioClip audioMultitudRitmico;
    [Tooltip("Sonido sirena policía (loop)")]
    [SerializeField] private AudioClip audioSirena;
    [Tooltip("Sonido tren pasando")]
    [SerializeField] private AudioClip audioTren;
    [Tooltip("Sonido tráfico ciudad (loop). " +
             "Asignar 'City Ambience - Traffic - Street - Cars and tram.wav' (Gregor Quendel).")]
    [SerializeField] private AudioClip audioTraficoAmbiente;
    [Tooltip("Sonido explosión (SFX). " +
             "Asignar 'Explosion 1.wav' (Free Pack / Epic Game Hits SFX).")]
    [SerializeField] private AudioClip audioExplosion;

    // ───────────────────────────────────────────────────────────────────────
    //  RUTAS RESOURCES (fallback si los campos Inspector están vacíos)
    // ───────────────────────────────────────────────────────────────────────
    private const string PATH_GC_MESH              = "Personajes/GuardiaCivil/CharacterGC";
    private const string PATH_PF_MESH              = "Personajes/PoliciaForal/CharacterPF";
    private const string PATH_KEFFIYEH             = "Personajes/Keffiyeh/KeffiyehMesh";
    private const string PATH_CIVIL_MALE           = "Personajes/Civiles/CharacterMale";
    private const string PATH_CIVIL_FEMALE         = "Personajes/Civiles/CharacterFemale";
    private const string PATH_SOLDADO              = "Personajes/Soldado/Soldier";
    private const string PATH_TEX_IKURRINA         = "Banderas/Ikurrina";
    private const string PATH_TEX_NAVARRA          = "Banderas/Navarra";
    private const string PATH_PATRULLA_GC          = "Vehiculos/Patrulla/PatrullaGC";
    private const string PATH_COCHE_CIVIL          = "Vehiculos/Civil/CocheCivil";
    private const string PATH_HELICOPTERO          = "Vehiculos/Helicoptero/Helicoptero";
    private const string PATH_PREFAB_PINO          = "Vegetacion/Pino";
    private const string PATH_PREFAB_ROBLE         = "Vegetacion/Roble";
    private const string PATH_PREFAB_ARBUSTO       = "Vegetacion/Arbusto";
    private const string PATH_PREFAB_ARBOL_EXTRA   = "Vegetacion/ArbolExtra";
    private const string PATH_CASA_PUEBLO          = "Edificios/CasaPueblo";
    private const string PATH_CASA_URBANA          = "Edificios/CasaUrbana";
    private const string PATH_VALLA               = "Edificios/Valla";
    private const string PATH_EXPLOSION            = "VFX/Explosion";
    private const string PATH_VFX_FUEGO            = "VFX/FuegoSuelo";
    private const string PATH_VFX_FUEGO_LITE       = "VFX/FuegoLite/BaseFire";
    private const string PATH_BARRICADA_HORMIGON   = "Barricadas/Hormigon/Concrete_Barrier";
    private const string PATH_BARRICADA_METAL      = "Barricadas/Metal/Metal_Barrier";
    private const string PATH_BARRICADA_PACK       = "Barricadas/BarricadaPack/BarricadaConcreto";
    private const string PATH_FAROLA               = "Farolas/StreetLamp";

    // ───────────────────────────────────────────────────────────────────────
    //  PROPIEDADES PÚBLICAS (usadas por GestorEscena y los sistemas)
    // ───────────────────────────────────────────────────────────────────────
    public Mesh        MeshGuardiaCivil        => meshGuardiaCivil;
    public Mesh        MeshPoliciaForal        => meshPoliciaForal;
    public Mesh        MeshKeffiyeh            => meshKeffiyeh;
    public Mesh        MeshCivilMale           => meshCivilMale;
    public Mesh        MeshCivilFemale         => meshCivilFemale;
    public Texture2D   TexIkurriña             => texIkurriña;
    public Texture2D   TexNavarra              => texNavarra;
    // Vehículos
    public GameObject  PrefabPatrullaGC        => prefabPatrullaGC;
    public GameObject  PrefabCocheCivil        => prefabCocheCivil;
    public GameObject  PrefabHelicoptero       => prefabHelicoptero;
    // Personaje soldado (LowPolySoldiers)
    public GameObject  PrefabSoldado           => prefabSoldado;
    // Vegetación
    public GameObject  PrefabPino              => prefabPino;
    public GameObject  PrefabRoble             => prefabRoble;
    public GameObject  PrefabArbusto           => prefabArbusto;
    public GameObject  PrefabArbolExtra        => prefabArbolExtra;
    // Edificios
    public GameObject  PrefabCasaPueblo        => prefabCasaPueblo;
    public GameObject  PrefabCasaUrbana        => prefabCasaUrbana;
    public GameObject  PrefabValla             => prefabValla;
    // VFX
    public GameObject  PrefabExplosion         => prefabExplosion;
    public GameObject  PrefabVFXFuego          => prefabVFXFuego;
    public GameObject  PrefabVFXFuegoLite      => prefabVFXFuegoLite;
    // Barricadas
    public GameObject  PrefabBarricadaHormigon => prefabBarricadaHormigon;
    public GameObject  PrefabBarricadaMetal    => prefabBarricadaMetal;
    public GameObject  PrefabBarricadaPack     => prefabBarricadaPack;
    /// <summary>Devuelve el mejor prefab de barricada de hormigón disponible (Abandoned World → BarrierPack → null).</summary>
    public GameObject  MejorPrefabBarricadaHormigon => prefabBarricadaHormigon ?? prefabBarricadaPack;
    // Farolas
    public GameObject  PrefabFarola            => prefabFarola;
    // Audio
    public AudioClip   AudioMultitud           => audioMultitud;
    public AudioClip   AudioMultitudRitmico    => audioMultitudRitmico;
    public AudioClip   AudioSirena             => audioSirena;
    public AudioClip   AudioTren               => audioTren;
    public AudioClip   AudioTraficoAmbiente    => audioTraficoAmbiente;
    public AudioClip   AudioExplosion          => audioExplosion;

    /// <summary>True si los assets críticos (al menos un personaje) están cargados.</summary>
    public bool AssetsListos => meshGuardiaCivil != null || meshCivilMale != null || prefabSoldado != null;

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

        // Soldado (LowPolySoldiers — nuevo asset)
        if (prefabSoldado == null) prefabSoldado = Resources.Load<GameObject>(PATH_SOLDADO);

        // Texturas de banderas
        texIkurriña ??= Resources.Load<Texture2D>(PATH_TEX_IKURRINA);
        texNavarra  ??= Resources.Load<Texture2D>(PATH_TEX_NAVARRA);

        // Vehículos
        if (prefabPatrullaGC  == null) prefabPatrullaGC  = Resources.Load<GameObject>(PATH_PATRULLA_GC);
        if (prefabCocheCivil  == null) prefabCocheCivil  = Resources.Load<GameObject>(PATH_COCHE_CIVIL);
        if (prefabHelicoptero == null) prefabHelicoptero = Resources.Load<GameObject>(PATH_HELICOPTERO);

        // Vegetación
        if (prefabPino       == null) prefabPino       = Resources.Load<GameObject>(PATH_PREFAB_PINO);
        if (prefabRoble      == null) prefabRoble      = Resources.Load<GameObject>(PATH_PREFAB_ROBLE);
        if (prefabArbusto    == null) prefabArbusto    = Resources.Load<GameObject>(PATH_PREFAB_ARBUSTO);
        if (prefabArbolExtra == null) prefabArbolExtra = Resources.Load<GameObject>(PATH_PREFAB_ARBOL_EXTRA);

        // Edificios (Village Houses + House Pack + Modular Fence)
        if (prefabCasaPueblo == null) prefabCasaPueblo = Resources.Load<GameObject>(PATH_CASA_PUEBLO);
        if (prefabCasaUrbana == null) prefabCasaUrbana = Resources.Load<GameObject>(PATH_CASA_URBANA);
        if (prefabValla      == null) prefabValla      = Resources.Load<GameObject>(PATH_VALLA);

        // VFX
        if (prefabExplosion    == null) prefabExplosion    = Resources.Load<GameObject>(PATH_EXPLOSION);
        if (prefabVFXFuego     == null) prefabVFXFuego     = Resources.Load<GameObject>(PATH_VFX_FUEGO);
        if (prefabVFXFuegoLite == null) prefabVFXFuegoLite = Resources.Load<GameObject>(PATH_VFX_FUEGO_LITE);

        // Barricadas (Abandoned World + BarrierPack — nuevos assets)
        if (prefabBarricadaHormigon == null) prefabBarricadaHormigon = Resources.Load<GameObject>(PATH_BARRICADA_HORMIGON);
        if (prefabBarricadaMetal    == null) prefabBarricadaMetal    = Resources.Load<GameObject>(PATH_BARRICADA_METAL);
        if (prefabBarricadaPack     == null) prefabBarricadaPack     = Resources.Load<GameObject>(PATH_BARRICADA_PACK);

        // Farolas (SpaceZeta StreetLamps2 — nuevo asset)
        if (prefabFarola == null) prefabFarola = Resources.Load<GameObject>(PATH_FAROLA);
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

        // SistemaBarricadas: propagar prefabs de barricadas y VFX fuego.
        // Usamos el mejor disponible: Abandoned World → BarrierPack → null (fallback procedural).
        var barricadas = Object.FindFirstObjectByType<SistemaBarricadas>();
        if (barricadas != null)
        {
            barricadas.AsignarPrefabs(
                MejorPrefabBarricadaHormigon,
                prefabBarricadaMetal,
                prefabVFXFuego ?? prefabVFXFuegoLite);
        }

        // SistemaFarolas: propagar prefab de farola.
        var farolas = Object.FindFirstObjectByType<SistemaFarolas>();
        if (farolas != null && prefabFarola != null)
            farolas.AsignarPrefab(prefabFarola);

        // SistemaEdificios: propagar prefabs de casas y vallas.
        var edificios = Object.FindFirstObjectByType<SistemaEdificios>();
        if (edificios != null)
        {
            edificios.AsignarPrefabs(prefabCasaPueblo, prefabCasaUrbana, prefabValla);

            // Cargar edificios variados de Downloads
            var ruinasDescargas = CargarPrefabsDescargas("Assets/Downloads/Buildings/FantasyRuins");
            var casasDescargas = CargarPrefabsDescargas("Assets/Downloads/Buildings/House");
            var edificiosEspecialesDescargas = CargarPrefabsDescargas("Assets/Downloads/Buildings/HQBuilding");

            if (ruinasDescargas.Length > 0 || casasDescargas.Length > 0 || edificiosEspecialesDescargas.Length > 0)
                edificios.AsignarPrefabsDescargas(ruinasDescargas, casasDescargas, edificiosEspecialesDescargas);
        }

        // SistemaVegetacion: propagar arbustos y árbol extra si están disponibles.
        var vegetacion = Object.FindFirstObjectByType<SistemaVegetacion>();
        if (vegetacion != null)
        {
            vegetacion.AsignarPrefabsExtra(prefabArbusto, prefabArbolExtra);

            // Cargar plantas de Downloads si están disponibles
            var plantasDescargas = CargarPrefabsDescargas("Assets/Downloads/Vegetation/PlantPack");
            var bushesDescargas = CargarPrefabsDescargas("Assets/Downloads/Vegetation/StylishPlants");
            var arbolesDescargas = CargarPrefabsDescargas("Assets/Downloads/Vegetation/OakTree");

            if (plantasDescargas.Length > 0 || bushesDescargas.Length > 0 || arbolesDescargas.Length > 0)
                vegetacion.AsignarPrefabsDescargas(plantasDescargas, bushesDescargas, arbolesDescargas);
        }

        // SistemaFauna: propagar prefabs de animales reales desde Downloads/Animals.
        // Los subdirectorios coinciden 1:1 con los tipos de SistemaFauna:
        //   Horse → Caballo, Wolf → Lobo, Rabbit → Conejo,
        //   Chicken → Pollo, Rooster → Gallo, Sheep → Oveja
        var fauna = Object.FindFirstObjectByType<SistemaFauna>();
        if (fauna != null)
        {
            var caballo = PrimerPrefabDescargas("Assets/Downloads/Animals/Horse");
            var lobo    = PrimerPrefabDescargas("Assets/Downloads/Animals/Wolf");
            var conejo  = PrimerPrefabDescargas("Assets/Downloads/Animals/Rabbit");
            var pollo   = PrimerPrefabDescargas("Assets/Downloads/Animals/Chicken");
            var gallo   = PrimerPrefabDescargas("Assets/Downloads/Animals/Rooster");
            var oveja   = PrimerPrefabDescargas("Assets/Downloads/Animals/Sheep");

            int cargados = (caballo != null ? 1 : 0) + (lobo != null ? 1 : 0) +
                           (conejo  != null ? 1 : 0) + (pollo != null ? 1 : 0) +
                           (gallo   != null ? 1 : 0) + (oveja != null ? 1 : 0);
            if (cargados > 0)
            {
                fauna.AsignarPrefabs(caballo, lobo, conejo, pollo, gallo, oveja);
                AlsasuaLogger.Info("SistemaAssets", $"SistemaFauna: {cargados}/6 animales reales asignados desde Downloads.");
            }
            else
            {
                AlsasuaLogger.Warn("SistemaAssets", "SistemaFauna: sin prefabs en Downloads/Animals → usando fallback procedural (cápsulas).");
            }
        }
    }

    /// <summary>
    /// Devuelve el primer prefab encontrado en una carpeta de Assets/Downloads.
    /// Útil cuando solo se necesita una variante de un tipo de asset.
    /// </summary>
    private static GameObject PrimerPrefabDescargas(string carpeta)
    {
        var todos = CargarPrefabsDescargas(carpeta);
        return todos.Length > 0 ? todos[0] : null;
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

        // Personajes
        Cuenta("Guardia Civil mesh",       meshGuardiaCivil       != null, ref cargados, ref fallbacks);
        Cuenta("Policía Foral mesh",       meshPoliciaForal       != null, ref cargados, ref fallbacks);
        Cuenta("Keffiyeh mesh",            meshKeffiyeh           != null, ref cargados, ref fallbacks);
        Cuenta("Civil Male mesh",          meshCivilMale          != null, ref cargados, ref fallbacks);
        Cuenta("Civil Female mesh",        meshCivilFemale        != null, ref cargados, ref fallbacks);
        Cuenta("Prefab soldado",           prefabSoldado          != null, ref cargados, ref fallbacks);
        // Banderas
        Cuenta("Textura ikurriña",         texIkurriña            != null, ref cargados, ref fallbacks);
        Cuenta("Textura Navarra",          texNavarra             != null, ref cargados, ref fallbacks);
        // Vehículos
        Cuenta("Prefab patrulla GC",       prefabPatrullaGC       != null, ref cargados, ref fallbacks);
        Cuenta("Prefab coche civil",       prefabCocheCivil       != null, ref cargados, ref fallbacks);
        Cuenta("Prefab helicóptero",       prefabHelicoptero      != null, ref cargados, ref fallbacks);
        // Vegetación
        Cuenta("Prefab pino",              prefabPino             != null, ref cargados, ref fallbacks);
        Cuenta("Prefab roble",             prefabRoble            != null, ref cargados, ref fallbacks);
        // VFX
        Cuenta("Prefab explosión",         prefabExplosion        != null, ref cargados, ref fallbacks);
        Cuenta("Prefab VFX fuego",         prefabVFXFuego         != null, ref cargados, ref fallbacks);
        Cuenta("Prefab VFX fuego lite",    prefabVFXFuegoLite     != null, ref cargados, ref fallbacks);
        // Barricadas
        Cuenta("Prefab barricada hormigón",prefabBarricadaHormigon!= null, ref cargados, ref fallbacks);
        Cuenta("Prefab barricada metal",   prefabBarricadaMetal   != null, ref cargados, ref fallbacks);
        Cuenta("Prefab barricada pack",    prefabBarricadaPack    != null, ref cargados, ref fallbacks);
        // Farolas
        Cuenta("Prefab farola",            prefabFarola           != null, ref cargados, ref fallbacks);
        // Vegetación extra (nuevos paquetes)
        Cuenta("Prefab arbusto",           prefabArbusto          != null, ref cargados, ref fallbacks);
        Cuenta("Prefab árbol extra",       prefabArbolExtra       != null, ref cargados, ref fallbacks);
        // Edificios (nuevos paquetes)
        Cuenta("Prefab casa pueblo",       prefabCasaPueblo       != null, ref cargados, ref fallbacks);
        Cuenta("Prefab casa urbana",       prefabCasaUrbana       != null, ref cargados, ref fallbacks);
        Cuenta("Prefab valla",             prefabValla            != null, ref cargados, ref fallbacks);

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
        // Police Car Interceptor exportado en escala correcta (~4.5m).
        // Si las unidades están en cm (valor bounds.x > 100), normalizar automáticamente.
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

    /// <summary>
    /// Crea una instancia del helicóptero policial escalado a ~8m de longitud.
    /// </summary>
    public GameObject InstanciarHelicoptero(Vector3 posicion, Quaternion rotacion)
    {
        if (prefabHelicoptero == null) return null;
        var go = Instantiate(prefabHelicoptero, posicion, rotacion);
        go.name = "HelicopteroPolicia";
        NormalizarEscalaVehiculo(go, 8.0f);
        return go;
    }

    /// <summary>
    /// Crea una instancia del prefab soldado (LowPolySoldiers) escalado a ~1.8m.
    /// </summary>
    public GameObject InstanciarSoldado(Vector3 posicion, Quaternion rotacion)
    {
        if (prefabSoldado == null) return null;
        var go = Instantiate(prefabSoldado, posicion, rotacion);
        go.name = "Soldado";
        // Escalar el soldado a altura humana (1.8m)
        var renderers = go.GetComponentsInChildren<Renderer>();
        if (renderers.Length > 0)
        {
            var bounds = renderers[0].bounds;
            foreach (var r in renderers) bounds.Encapsulate(r.bounds);
            float alturaActual = bounds.size.y;
            if (alturaActual > 0.001f)
                go.transform.localScale *= 1.8f / alturaActual;
        }
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

    /// <summary>
    /// Carga todos los prefabs de una carpeta de Assets/Downloads.
    /// Útil para cargar múltiples variantes de plantas, arbustos, árboles, etc.
    /// </summary>
    private static GameObject[] CargarPrefabsDescargas(string carpeta)
    {
        #if UNITY_EDITOR
        var guids = UnityEditor.AssetDatabase.FindAssets("t:Prefab t:GameObject", new[] { carpeta });
        var prefabs = new System.Collections.Generic.List<GameObject>();
        foreach (var guid in guids)
        {
            string path = UnityEditor.AssetDatabase.GUIDToAssetPath(guid);
            var prefab = UnityEditor.AssetDatabase.LoadAssetAtPath<GameObject>(path);
            if (prefab != null) prefabs.Add(prefab);
        }
        return prefabs.ToArray();
        #else
        // En runtime, usar Resources si están en Assets/Resources/Downloads
        var allPrefabs = Resources.LoadAll<GameObject>(carpeta);
        return allPrefabs ?? new GameObject[0];
        #endif
    }
}
