// Assets/Scripts/GestorEscena.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Orquestador de la escena de Alsasua.
//  Instancia y configura todos los sistemas de simulación:
//
//    · SistemaMultitud    — manifestación 500-1000 personas
//    · SistemaPersonajes  — GC, Policía Foral, civiles, portadores banderas
//    · SistemaTrafico     — tráfico en autovías y calles urbanas
//    · SistemaFerroviario — tren Pamplona–Donostia por la estación
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
#if UNITY_EDITOR
using UnityEditor;
#endif

[DefaultExecutionOrder(-10)]   // ejecutar ANTES que los sistemas
public sealed class GestorEscena : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR — referencias opcionales (GestorEscena las busca/crea si faltan)
    // ───────────────────────────────────────────────────────────────────────
    [Header("═══ SISTEMAS (auto-creados si están vacíos) ═══")]
    [SerializeField] private SistemaMultitud    sistemaMultitud;
    [SerializeField] private SistemaPersonajes  sistemaPersonajes;
    [SerializeField] private SistemaTrafico     sistemaTrafico;
    [SerializeField] private SistemaFerroviario sistemaFerroviario;
    [SerializeField] private SistemaVegetacion  sistemaVegetacion;

    [Header("═══ ACTIVOS ═══")]
    [SerializeField] private bool activarMultitud    = true;
    [SerializeField] private bool activarPersonajes  = true;
    [SerializeField] private bool activarTrafico     = true;
    [SerializeField] private bool activarTren        = true;
    [SerializeField] private bool activarVegetacion  = true;

    [Header("═══ CONFIGURACIÓN ALSASUA ═══")]
    [Tooltip("Punto de origen del mapa Unity (debe coincidir con CesiumGeoreference)")]
    [SerializeField] private Vector3 origenMundo = Vector3.zero;

    // ─── Posiciones reales de Alsasua (en metros locales desde el centro) ─
    // Centro del pueblo ≈ (0, 0, 0)
    // Norte:  +Z, Sur: -Z, Este: +X, Oeste: -X
    // 1 grado lat ≈ 111 km, 1 grado lon ≈ 80 km en esa latitud

    // Cuartel Guardia Civil: calle real aproximada
    private static readonly Vector3[] RUTA_GC = new Vector3[]
    {
        new Vector3(-180f,  0f,   60f),
        new Vector3(-120f,  0f,   80f),
        new Vector3( -60f,  0f,   40f),
        new Vector3(  20f,  0f,   20f),
        new Vector3(  60f,  0f,  -10f),
        new Vector3(   0f,  0f,  -50f),
        new Vector3(-100f,  0f,  -30f),
        new Vector3(-180f,  0f,   60f),   // cierre del bucle
    };

    // Comisaría Policía Foral: zona norte-este
    private static readonly Vector3[] RUTA_PF = new Vector3[]
    {
        new Vector3( 100f,  0f,  120f),
        new Vector3( 150f,  0f,   80f),
        new Vector3( 200f,  0f,   20f),
        new Vector3( 160f,  0f,  -40f),
        new Vector3(  80f,  0f,  -80f),
        new Vector3(  20f,  0f,   40f),
        new Vector3(  80f,  0f,  100f),
        new Vector3( 100f,  0f,  120f),
    };

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
        if (activarMultitud   && sistemaMultitud    == null) sistemaMultitud    = CrearSistema<SistemaMultitud>("SistemaMultitud");
        if (activarTrafico    && sistemaTrafico     == null) sistemaTrafico     = CrearSistema<SistemaTrafico>("SistemaTrafico");
        if (activarTren       && sistemaFerroviario == null) sistemaFerroviario = CrearSistema<SistemaFerroviario>("SistemaFerroviario");
        if (activarVegetacion && sistemaVegetacion  == null) sistemaVegetacion  = CrearSistema<SistemaVegetacion>("SistemaVegetacion");

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
            Debug.LogWarning("[GestorEscena] ConfigurarWaypointsPersonajes: sistemaPersonajes es null.");
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
        Debug.Log(
            "╔══════════════════════════════════════════════════════════╗\n" +
            "║  ALSASUA SIMULATOR — Sistemas activos                    ║\n" +
            "╠══════════════════════════════════════════════════════════╣\n" +
           $"║  Multitud          : {(activarMultitud   ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Personajes        : {(activarPersonajes ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Tráfico           : {(activarTrafico    ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Ferroviario       : {(activarTren       ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
           $"║  Vegetación        : {(activarVegetacion ? "✓ ACTIVO" : "— desactivado"),-30} ║\n" +
            "╚══════════════════════════════════════════════════════════╝"
        );
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
            Debug.Log("[GestorEscena] ⚠ ALERTA ACTIVADA — GC y Policía Foral en patrulla intensiva.");
        }
        else
        {
            Debug.LogWarning("[GestorEscena] ActivarAlerta: SistemaPersonajes no disponible.");
        }
    }

    /// <summary>Cancela la alerta.</summary>
    public void DesactivarAlerta()
    {
        if (sistemaPersonajes != null)
            sistemaPersonajes.SetAlerta(false);
        else
            Debug.LogWarning("[GestorEscena] DesactivarAlerta: SistemaPersonajes no disponible.");
    }

    /// <summary>Referencia al sistema de personajes activo.</summary>
    public SistemaPersonajes Personajes => sistemaPersonajes;
}
