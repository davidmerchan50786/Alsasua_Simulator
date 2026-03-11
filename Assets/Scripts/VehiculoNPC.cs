// Assets/Scripts/VehiculoNPC.cs
// Vehículo NPC que sigue waypoints, frena ante obstáculos y recibe daño

using UnityEngine;
using System.Collections.Generic;

[RequireComponent(typeof(Rigidbody))]
public class VehiculoNPC : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  MOVIMIENTO
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ MOVIMIENTO ═══")]
    [SerializeField] private float velocidadMax    = 8f;    // m/s ≈ 30 km/h ciudad
    [SerializeField] private float aceleracion     = 4f;
    [SerializeField] private float velocidadGiro   = 120f;  // grados/seg
    [SerializeField] private float distanciaWP     = 4f;    // distancia para cambiar waypoint
    [SerializeField] private bool  bucleWaypoints  = true;

    [Header("═══ WAYPOINTS ═══")]
    [Tooltip("Arrastra aquí los GameObjects que marcan la ruta del coche")]
    [SerializeField] private List<Transform> waypoints = new List<Transform>();

    [Header("═══ OBSTÁCULOS ═══")]
    [SerializeField] private float distanciaFreno  = 6f;
    [SerializeField] private float anguloDeteccion = 30f;
    [SerializeField] private LayerMask capasObstaculo;

    // ═══════════════════════════════════════════════════════════════════════
    //  SALUD
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ SALUD ═══")]
    [SerializeField] private int vida    = 80;
    [SerializeField] private int vidaMax = 80;
    private bool destruido = false;

    // ═══════════════════════════════════════════════════════════════════════
    //  COLOR DEL COCHE (aleatorio)
    // ═══════════════════════════════════════════════════════════════════════

    private static Color[] coloresCoche =
    {
        new Color(0.8f, 0.1f, 0.1f),   // rojo
        new Color(0.1f, 0.2f, 0.7f),   // azul
        new Color(0.1f, 0.5f, 0.2f),   // verde
        new Color(0.9f, 0.9f, 0.9f),   // blanco
        new Color(0.15f, 0.15f, 0.15f),// negro
        new Color(0.7f, 0.6f, 0.1f),   // amarillo oscuro
        new Color(0.5f, 0.5f, 0.5f),   // gris
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private Rigidbody rb;
    private int       wpActual    = 0;
    private float     velocidadActual = 0f;
    private bool      frenando    = false;

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        rb = GetComponent<Rigidbody>();
        rb.constraints = RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationZ;
        rb.linearDamping        = 1.5f;

        if (capasObstaculo == 0) capasObstaculo = ~0;

        // Color aleatorio
        var renderer = GetComponentInChildren<Renderer>();
        if (renderer != null)
            renderer.material.color = coloresCoche[Random.Range(0, coloresCoche.Length)];
    }

    private void FixedUpdate()
    {
        if (destruido || waypoints.Count == 0) return;

        DetectarObstaculos();
        MoverHaciaWaypoint();
        CambiarWaypointSiLlegamos();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MOVIMIENTO
    // ═══════════════════════════════════════════════════════════════════════

    private void MoverHaciaWaypoint()
    {
        if (wpActual >= waypoints.Count) return;

        Transform destino = waypoints[wpActual];
        if (destino == null) { wpActual++; return; }

        Vector3 dir = (destino.position - transform.position);
        dir.y = 0f;

        // Girar hacia el destino
        if (dir.magnitude > 0.1f)
        {
            Quaternion rotObjetivo = Quaternion.LookRotation(dir.normalized);
            transform.rotation = Quaternion.RotateTowards(
                transform.rotation, rotObjetivo,
                velocidadGiro * Time.fixedDeltaTime);
        }

        // Acelerar / frenar
        float velObjetivo = frenando ? 0f : velocidadMax;
        velocidadActual = Mathf.MoveTowards(velocidadActual, velObjetivo,
                                             aceleracion * Time.fixedDeltaTime);

        rb.MovePosition(rb.position + transform.forward * velocidadActual * Time.fixedDeltaTime);
    }

    private void CambiarWaypointSiLlegamos()
    {
        if (wpActual >= waypoints.Count) return;

        float dist = Vector3.Distance(
            new Vector3(transform.position.x, 0, transform.position.z),
            new Vector3(waypoints[wpActual].position.x, 0, waypoints[wpActual].position.z));

        if (dist < distanciaWP)
        {
            wpActual++;
            if (wpActual >= waypoints.Count)
            {
                if (bucleWaypoints) wpActual = 0;
                else { velocidadActual = 0f; enabled = false; }
            }
        }
    }

    private void DetectarObstaculos()
    {
        // Raycast frontal
        frenando = Physics.Raycast(transform.position + Vector3.up * 0.5f,
                                   transform.forward,
                                   distanciaFreno, capasObstaculo);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DAÑO
    // ═══════════════════════════════════════════════════════════════════════

    public void RecibirDano(int cantidad)
    {
        if (destruido) return;
        vida -= cantidad;

        // Cambiar color al recibir daño (oscurecer)
        var rend = GetComponentInChildren<Renderer>();
        if (rend != null)
            rend.material.color = Color.Lerp(rend.material.color, Color.black,
                                             0.3f * ((float)(vidaMax - vida) / vidaMax));

        if (vida <= 0) Destruir();
    }

    private void Destruir()
    {
        destruido = true;
        velocidadActual = 0f;

        // Explosión del vehículo
        SistemaExplosion.Explotar(transform.position + Vector3.up, 8f, 400f, 80);

        // Oscurecer completamente
        foreach (var r in GetComponentsInChildren<Renderer>())
            r.material.color = new Color(0.08f, 0.06f, 0.05f);

        // Desactivar física controlada
        rb.constraints = RigidbodyConstraints.None;
        Destroy(gameObject, 15f);
        enabled = false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  UTILIDAD ESTÁTICA: crear coche con forma básica por código
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Crea un VehiculoNPC en la posición indicada con waypoints de prueba.
    /// Útil para instanciar desde SetupEscenaAlsasua.
    /// </summary>
    public static GameObject CrearCocheBasico(Vector3 posicion, List<Transform> ruta)
    {
        var go = new GameObject("CocheNPC");
        go.transform.position = posicion;

        // Carrocería
        var cuerpo = GameObject.CreatePrimitive(PrimitiveType.Cube);
        cuerpo.transform.SetParent(go.transform);
        cuerpo.transform.localPosition = new Vector3(0f, 0.45f, 0f);
        cuerpo.transform.localScale    = new Vector3(1.8f, 0.8f, 4.2f);
        Destroy(cuerpo.GetComponent<Collider>());

        // Techo
        var techo = GameObject.CreatePrimitive(PrimitiveType.Cube);
        techo.transform.SetParent(go.transform);
        techo.transform.localPosition = new Vector3(0f, 1.05f, -0.2f);
        techo.transform.localScale    = new Vector3(1.6f, 0.55f, 2.4f);
        Destroy(techo.GetComponent<Collider>());

        // Ruedas (4)
        float[] posX = { -1.0f, 1.0f, -1.0f, 1.0f };
        float[] posZ = { 1.3f, 1.3f, -1.3f, -1.3f };
        for (int i = 0; i < 4; i++)
        {
            var rueda = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            rueda.transform.SetParent(go.transform);
            rueda.transform.localPosition = new Vector3(posX[i], 0.22f, posZ[i]);
            rueda.transform.localScale    = new Vector3(0.35f, 0.22f, 0.35f);
            rueda.transform.localRotation = Quaternion.Euler(0f, 0f, 90f);
            rueda.GetComponent<Renderer>().material.color = new Color(0.1f, 0.1f, 0.1f);
            Destroy(rueda.GetComponent<Collider>());
        }

        // Colisionador principal
        var boxCol = go.AddComponent<BoxCollider>();
        boxCol.center = new Vector3(0, 0.45f, 0);
        boxCol.size   = new Vector3(1.8f, 1.4f, 4.2f);

        // Rigidbody y script
        var vehiculo = go.AddComponent<VehiculoNPC>();
        vehiculo.waypoints = ruta;
        vehiculo.vida      = vehiculo.vidaMax = 80;

        return go;
    }
}
