// Assets/Scripts/GestorEscena.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Orquestador de la escena de Alsasua.
//  Instancia y configura todos los sistemas de simulación:
//
//    · SistemaMultitud    — manifestación 500-1000 personas
//    · SistemaPersonajes  — GC, Policía Foral, civiles, portadores banderas
//    · SistemaTrafico     — tráfico en autovías y calles urbanas
//    · SistemaFerroviario — tren Madrid–Irún por la estación de Alsasua
//    · SistemaVegetacion  — bosques procedurales alrededor de Alsasua
//
//  Carga automáticamente los assets Kenney presentes en el proyecto
//  (characterMedium.fbx, idle.fbx, run.fbx) si no se asignan en Inspector.
//  En builds, asigna los prefabs/meshes directamente desde Inspector.
//
//  Waypoints de patrulla GC/PF calculados desde las coordenadas reales
//  de Alsasua convertidas a espacio local Unity.
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;

[DefaultExecutionOrder(-10)]   // ejecutar ANTES que los sistemas
public sealed class GestorEscena : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR — referencias opcionales (GestorEscena las busca/crea si faltan)
    // ───────────────────────────────────────────────────────────────────────
    [Header("═══ SISTEMAS (auto-creados si están vacíos) ═══")]
    [SerializeField] private SistemaMultitud        sistemaMultitud;
    [SerializeField] private SistemaPersonajes      sistemaPersonajes;
    [SerializeField] private SistemaTrafico         sistemaTrafico;
    [SerializeField] private SistemaFerroviario     sistemaFerroviario;
    [SerializeField] private SistemaVegetacion      sistemaVegetacion;
    [SerializeField] private SistemaBarricadas      sistemaBarricadas;
    [SerializeField] private SistemaFarolas         sistemaFarolas;
    [SerializeField] private SistemaEdificios       sistemaEdificios;
    [SerializeField] private SistemaTerrenoAlsasua  sistemaTerrenoAlsasua;
    [SerializeField] private SistemaFauna           sistemaFauna;

    [Header("═══ ACTIVOS ═══")]
    [Tooltip("Activa la simulación de la manifestación (500-1000 personas en movimiento).")]
    [SerializeField] private bool activarMultitud    = true;
    [Tooltip("Activa los personajes individuales: Guardia Civil, Policía Foral y civiles con rutas de patrulla.")]
    [SerializeField] private bool activarPersonajes  = true;
    [Tooltip("Activa el tráfico de vehículos NPC con DrawMeshInstanced.")]
    [SerializeField] private bool activarTrafico     = true;
    [Tooltip("Activa la simulación del tren en la línea Madrid–Irún (Alsasua).")]
    [SerializeField] private bool activarTren        = true;
    [Tooltip("Activa los bosques procedurales de pinos y robles alrededor de Alsasua.")]
    [SerializeField] private bool activarVegetacion  = true;
    [Tooltip("Activa las barricadas en las entradas del pueblo " +
             "(assets: Abandoned World Barriers + BarrierPack + VFX fuego).")]
    [SerializeField] private bool activarBarricadas  = true;
    [Tooltip("Activa las farolas urbanas a lo largo de las calles " +
             "(asset: SpaceZeta_StreetLamps2).")]
    [SerializeField] private bool activarFarolas     = true;
    [Tooltip("Activa los edificios del pueblo Alsasua " +
             "(assets: Village Houses Pack + House Pack + Modular Fence).")]
    [SerializeField] private bool activarEdificios   = true;
    [Tooltip("Activa el terreno procedural del Valle de Burunda con los montes de alrededor. " +
             "Proporciona physics collision y heightmap antes de que carguen los Cesium tiles.")]
    [SerializeField] private bool activarTerreno     = true;
    [Tooltip("Activa la fauna del valle: caballos, ovejas, conejos, lobos, gallinas y gallos. " +
             "Assets desde Downloads/Animals si están importados, fallback procedural si no.")]
    [SerializeField] private bool activarFauna       = true;

    [Header("═══ CONFIGURACIÓN ALSASUA ═══")]
    [Tooltip("Punto de origen del mapa Unity (debe coincidir con CesiumGeoreference)")]
    [SerializeField] private Vector3 origenMundo = Vector3.zero;

    // ─── Rutas de patrulla — delegadas a GeoDataAlsasua (fuente única de verdad) ─
    // Los waypoints reales siguen las calles de Alsasua: Calle Ameztia, Navarra,
    // Erdikale, Plaza de los Fueros y acceso al polígono Ondarria.
    // Para cambiar los waypoints, editar GeoDataAlsasua.cs.

    private static Vector3[] RUTA_GC => GeoDataAlsasua.PatrullaGuardiaCivil;
    private static Vector3[] RUTA_PF => GeoDataAlsasua.PatrullaPolForal;

    // ───────────────────────────────────────────────────────────────────────
    //  UNITY
    // ───────────────────────────────────────────────────────────────────────
    private void Awake()
    {
        // Crear sistemas que falten.
        // ORDEN IMPORTA: SistemaPersonajes se crea el ÚLTIMO para que
        // ConfigurarWaypointsPersonajes() se llame antes de su Awake().
        // [DefaultExecutionOrder(-10)] garantiza que este Awake() corre antes que
        // SistemaPersonajes (order 0) cuando ambos están pre-asignados en el Inspector.
        // En el caso de creación dinámica (AddComponent), Awake() del nuevo componente
        // se dispara de forma síncrona, por lo que llamamos a ConfigurarWaypointsPersonajes()
        // INMEDIATAMENTE después para que las rutas estén listas antes del primer TickCaminar.
        if (activarMultitud    && sistemaMultitud    == null) sistemaMultitud    = CrearSistema<SistemaMultitud>("SistemaMultitud");
        if (activarTrafico     && sistemaTrafico     == null) sistemaTrafico     = CrearSistema<SistemaTrafico>("SistemaTrafico");
        if (activarTren        && sistemaFerroviario == null) sistemaFerroviario = CrearSistema<SistemaFerroviario>("SistemaFerroviario");
        if (activarVegetacion  && sistemaVegetacion  == null) sistemaVegetacion  = CrearSistema<SistemaVegetacion>("SistemaVegetacion");
        // Nuevos sistemas (assets Abandoned World + SpaceZeta)
        if (activarBarricadas  && sistemaBarricadas  == null) sistemaBarricadas  = CrearSistema<SistemaBarricadas>("SistemaBarricadas");
        if (activarFarolas     && sistemaFarolas     == null) sistemaFarolas     = CrearSistema<SistemaFarolas>("SistemaFarolas");
        if (activarEdificios   && sistemaEdificios       == null) sistemaEdificios       = CrearSistema<SistemaEdificios>("SistemaEdificios");
        if (activarTerreno     && sistemaTerrenoAlsasua == null) sistemaTerrenoAlsasua  = CrearSistema<SistemaTerrenoAlsasua>("SistemaTerrenoAlsasua");
        if (activarFauna       && sistemaFauna          == null) sistemaFauna           = CrearSistema<SistemaFauna>("SistemaFauna");

        // FIX RACE CONDITION: las rutas deben inyectarse en SistemaPersonajes ANTES de que
        // su Awake() llame a SpawnTodos(). Con el orden de ejecución (-10 vs 0) esto se cumple
        // tanto si el componente está en Inspector como si se crea aquí dinámicamente.
        if (activarPersonajes && sistemaPersonajes  == null) sistemaPersonajes  = CrearSistema<SistemaPersonajes>("SistemaPersonajes");
        if (activarPersonajes && sistemaPersonajes  != null) ConfigurarWaypointsPersonajes();

        // Desactivar los que no se usen
        if (!activarMultitud   && sistemaMultitud    != null) sistemaMultitud.gameObject.SetActive(false);
        if (!activarPersonajes && sistemaPersonajes  != null) sistemaPersonajes.gameObject.SetActive(false);
        if (!activarTrafico    && sistemaTrafico     != null) sistemaTrafico.gameObject.SetActive(false);
        if (!activarTren       && sistemaFerroviario != null) sistemaFerroviario.gameObject.SetActive(false);
        if (!activarVegetacion && sistemaVegetacion  != null) sistemaVegetacion.gameObject.SetActive(false);
        if (!activarBarricadas && sistemaBarricadas  != null) sistemaBarricadas.gameObject.SetActive(false);
        if (!activarFarolas    && sistemaFarolas     != null) sistemaFarolas.gameObject.SetActive(false);
        if (!activarEdificios  && sistemaEdificios       != null) sistemaEdificios.gameObject.SetActive(false);
        if (!activarTerreno    && sistemaTerrenoAlsasua != null) sistemaTerrenoAlsasua.gameObject.SetActive(false);
        if (!activarFauna      && sistemaFauna          != null) sistemaFauna.gameObject.SetActive(false);
    }

    private void Start()
    {
        LogEstado();
    }

    // ───────────────────────────────────────────────────────────────────────
    //  CONFIGURACIÓN WAYPOINTS
    // ───────────────────────────────────────────────────────────────────────
    private void ConfigurarWaypointsPersonajes()
    {
        // FIX DEFENSA: aunque Start() ya comprueba sistemaPersonajes != null antes de llamar
        // aquí, añadimos guard explícito por si este método se invoca desde otro contexto.
        if (sistemaPersonajes == null)
        {
            AlsasuaLogger.Warn("GestorEscena", "ConfigurarWaypointsPersonajes: sistemaPersonajes es null.");
            return;
        }

        // Crear Transform[] desde las rutas estáticas e inyectar vía método público.
        var waypointsGC = CrearWaypoints("WaypointsGC", RUTA_GC);
        var waypointsPF = CrearWaypoints("WaypointsPF", RUTA_PF);

        sistemaPersonajes.AsignarRutas(waypointsGC, waypointsPF);
    }

    private Transform[] CrearWaypoints(string nombrePadre, Vector3[] posiciones)
    {
        var padre = new GameObject(nombrePadre);
        padre.transform.SetParent(transform);

        var lista = new Transform[posiciones.Length];
        for (int i = 0; i < posiciones.Length; i++)
        {
            var wp = new GameObject($"WP_{i:D2}");
            wp.transform.SetParent(padre.transform);
            wp.transform.position = origenMundo + posiciones[i];
            lista[i] = wp.transform;
        }
        return lista;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  HELPER CREACIÓN SISTEMAS
    // ───────────────────────────────────────────────────────────────────────

    // FIX: no static — necesita acceder a transform para emparejar los GOs de sistemas
    // como hijos directos de GestorEscena. Sin parent, quedan huérfanos en la raíz de la
    // jerarquía y no se destruyen automáticamente al destruir GestorEscena.
    private T CrearSistema<T>(string nombre) where T : MonoBehaviour
    {
        var go = new GameObject(nombre);
        go.transform.SetParent(transform);
        return go.AddComponent<T>();
    }

    // ───────────────────────────────────────────────────────────────────────
    //  LOG DE ESTADO
    // ───────────────────────────────────────────────────────────────────────
    private void LogEstado()
    {
        AlsasuaLogger.Info("GestorEscena",
            "╔══════════════════════════════════════════════════════════╗\n" +
            "║  ALSASUA SIMULATOR — Sistemas activos                    ║\n" +
            "╠══════════════════════════════════════════════════════════╣\n" +
           $"║  Multitud          : {(activarMultitud    ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Personajes        : {(activarPersonajes  ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Tráfico           : {(activarTrafico     ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Ferroviario       : {(activarTren        ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Vegetación        : {(activarVegetacion  ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Barricadas        : {(activarBarricadas  ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Farolas           : {(activarFarolas     ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Edificios         : {(activarEdificios   ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Terreno Valle     : {(activarTerreno    ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Fauna Valle       : {(activarFauna      ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
            "╚══════════════════════════════════════════════════════════╝");
    }

    // ───────────────────────────────────────────────────────────────────────
    //  GIZMOS EDITOR
    // ───────────────────────────────────────────────────────────────────────
#if UNITY_EDITOR
    private void OnDrawGizmos()
    {
        // Ruta GC en verde
        Gizmos.color = new Color(0.2f, 0.8f, 0.2f, 0.8f);
        DrawRutaGizmo(RUTA_GC);

        // Ruta PF en azul
        Gizmos.color = new Color(0.2f, 0.4f, 0.9f, 0.8f);
        DrawRutaGizmo(RUTA_PF);
    }

    private void DrawRutaGizmo(Vector3[] ruta)
    {
        // FIX: guard para array vacío — ruta.Length == 0 causaría módulo-por-cero
        if (ruta == null || ruta.Length == 0) return;

        for (int i = 0; i < ruta.Length; i++)
        {
            Vector3 a = origenMundo + ruta[i];
            Vector3 b = origenMundo + ruta[(i + 1) % ruta.Length];
            Gizmos.DrawSphere(a, 3f);
            Gizmos.DrawLine(a, b);
        }
    }
#endif

    // ───────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ───────────────────────────────────────────────────────────────────────

    /// <summary>Activa alerta general: GC y PF aceleran patrulla.</summary>
    public void ActivarAlerta()
    {
        if (sistemaPersonajes != null)
        {
            sistemaPersonajes.SetAlerta(true);
            AlsasuaLogger.Info("GestorEscena", "⚠ ALERTA ACTIVADA — GC y Policía Foral en patrulla intensiva.");
        }
        else
        {
            AlsasuaLogger.Warn("GestorEscena", "ActivarAlerta: SistemaPersonajes no disponible.");
        }
    }

    /// <summary>Cancela la alerta.</summary>
    public void DesactivarAlerta()
    {
        if (sistemaPersonajes != null)
            sistemaPersonajes.SetAlerta(false);
        else
            AlsasuaLogger.Warn("GestorEscena", "DesactivarAlerta: SistemaPersonajes no disponible.");
    }

    /// <summary>Referencia al sistema de personajes activo.</summary>
    public SistemaPersonajes Personajes   => sistemaPersonajes;

    /// <summary>Referencia al sistema de barricadas activo.</summary>
    public SistemaBarricadas Barricadas   => sistemaBarricadas;

    /// <summary>Referencia al sistema de farolas activo.</summary>
    public SistemaFarolas    Farolas      => sistemaFarolas;

    /// <summary>Referencia al sistema de edificios activo.</summary>
    public SistemaEdificios  Edificios    => sistemaEdificios;

    /// <summary>Referencia al sistema de fauna activo.</summary>
    public SistemaFauna      Fauna        => sistemaFauna;

    /// <summary>
    /// Activa el fuego en todas las barricadas (escalada de la manifestación).
    /// </summary>
    public void PrenderBarricadas()
    {
        if (sistemaBarricadas != null)
        {
            sistemaBarricadas.PrenderTodas();
            AlsasuaLogger.Info("GestorEscena", "🔥 Barricadas encendidas por escalada.");
        }
        else
        {
            AlsasuaLogger.Warn("GestorEscena", "PrenderBarricadas: SistemaBarricadas no disponible.");
        }
    }
}
