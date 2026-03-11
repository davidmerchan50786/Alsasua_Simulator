// Assets/Scripts/EnemigoPatrulla.cs
// Enemigo genérico (mercenario/soldado ficticio) con IA de patrulla
// Estados: Patrullando → Alertado → Persiguiendo → Atacando → Muerto

using UnityEngine;
using System.Collections;

public class EnemigoPatrulla : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  ENUM DE ESTADOS
    // ═══════════════════════════════════════════════════════════════════════

    public enum EstadoIA { Patrullando, Alertado, Persiguiendo, Atacando, Muerto }

    // ═══════════════════════════════════════════════════════════════════════
    //  CONFIGURACIÓN
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ VIDA ═══")]
    [SerializeField] private int vida    = 100;
    [SerializeField] private int vidaMax = 100;

    [Header("═══ VISIÓN ═══")]
    [SerializeField] private float radioVision     = 25f;
    [SerializeField] private float anguloVision    = 110f;
    [SerializeField] private float radioEscucha    = 8f;   // detecta sin línea de visión

    [Header("═══ MOVIMIENTO ═══")]
    [SerializeField] private float velocidadPatrulla  = 2.5f;
    [SerializeField] private float velocidadPerseguir = 5.5f;
    [SerializeField] private float velocidadGiro      = 180f;

    [Header("═══ ATAQUE ═══")]
    [SerializeField] private float radioAtaque        = 18f;
    [SerializeField] private int   danoPorDisparo     = 15;
    [SerializeField] private float cadenciaAtaque     = 0.8f;
    [SerializeField] private float precisionEnemy     = 0.05f;  // dispersión

    [Header("═══ WAYPOINTS DE PATRULLA ═══")]
    [SerializeField] private Transform[] waypointsPatrulla;
    [SerializeField] private float tiempoEsperaWP = 2f;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    public  EstadoIA Estado  { get; private set; } = EstadoIA.Patrullando;
    private Transform jugador;
    private ControladorJugador controlJugador;
    private int    wpActual         = 0;
    private float  timerAtaque      = 0f;
    private float  timerAlerta      = 0f;
    private bool   esperandoWP      = false;
    private Vector3 ultimaPosJugador;

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Start()
    {
        // Buscar jugador
        var jp = Object.FindFirstObjectByType<ControladorJugador>();
        if (jp != null)
        {
            jugador        = jp.transform;
            controlJugador = jp;
        }

        // Crear cuerpo visual básico si no tiene hijos
        if (transform.childCount == 0)
            CrearCuerpoBasico();
    }

    private void Update()
    {
        if (Estado == EstadoIA.Muerto) return;

        timerAtaque -= Time.deltaTime;

        switch (Estado)
        {
            case EstadoIA.Patrullando:  ActualizarPatrulla();   break;
            case EstadoIA.Alertado:     ActualizarAlerta();     break;
            case EstadoIA.Persiguiendo: ActualizarPerseguir();  break;
            case EstadoIA.Atacando:     ActualizarAtacando();   break;
        }

        // Comprobar visión siempre
        if (Estado != EstadoIA.Atacando)
            ComprobarVision();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADOS DE IA
    // ═══════════════════════════════════════════════════════════════════════

    private void ActualizarPatrulla()
    {
        // BUG FIX: evitar NullReferenceException si el array no está asignado en el Inspector
        if (waypointsPatrulla == null || waypointsPatrulla.Length == 0 || esperandoWP) return;

        var wp = waypointsPatrulla[wpActual];
        if (wp == null) { wpActual = 0; return; }

        MoverHacia(wp.position, velocidadPatrulla);

        if (Vector3.Distance(transform.position, wp.position) < 1.5f)
            StartCoroutine(EsperarEnWP());
    }

    private void ActualizarAlerta()
    {
        timerAlerta -= Time.deltaTime;

        // Girar buscando al jugador
        transform.Rotate(Vector3.up, 60f * Time.deltaTime);

        if (timerAlerta <= 0f)
            CambiarEstado(EstadoIA.Patrullando);
    }

    private void ActualizarPerseguir()
    {
        if (jugador == null) { CambiarEstado(EstadoIA.Alertado); return; }

        float dist = Vector3.Distance(transform.position, jugador.position);

        if (dist <= radioAtaque)
        {
            CambiarEstado(EstadoIA.Atacando);
            return;
        }

        if (dist > radioVision * 1.5f)
        {
            ultimaPosJugador = jugador.position;
            CambiarEstado(EstadoIA.Alertado);
            return;
        }

        MoverHacia(jugador.position, velocidadPerseguir);
    }

    private void ActualizarAtacando()
    {
        if (jugador == null || controlJugador == null || controlJugador.EstaMuerto)
        {
            CambiarEstado(EstadoIA.Patrullando);
            return;
        }

        float dist = Vector3.Distance(transform.position, jugador.position);

        // Si el jugador se aleja, perseguir
        if (dist > radioAtaque * 1.3f)
        {
            CambiarEstado(EstadoIA.Persiguiendo);
            return;
        }

        // Apuntar al jugador
        var dir = (jugador.position - transform.position);
        dir.y   = 0;
        if (dir != Vector3.zero)
            transform.rotation = Quaternion.RotateTowards(
                transform.rotation,
                Quaternion.LookRotation(dir.normalized),
                velocidadGiro * Time.deltaTime);

        // Disparar
        if (timerAtaque <= 0f)
        {
            Disparar();
            timerAtaque = cadenciaAtaque;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  VISIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private void ComprobarVision()
    {
        if (jugador == null) return;

        float dist  = Vector3.Distance(transform.position, jugador.position);
        bool  cerca = dist < radioEscucha;

        bool enCampo = false;
        if (dist < radioVision)
        {
            var dirJugador = (jugador.position - transform.position).normalized;
            float angulo   = Vector3.Angle(transform.forward, dirJugador);
            if (angulo < anguloVision / 2f)
            {
                // Raycast de línea de visión
                if (!Physics.Raycast(transform.position + Vector3.up,
                                     dirJugador, dist,
                                     LayerMask.GetMask("Default")))
                    enCampo = true;
            }
        }

        if ((enCampo || cerca) && Estado == EstadoIA.Patrullando)
            CambiarEstado(EstadoIA.Persiguiendo);
        else if (enCampo && Estado == EstadoIA.Alertado)
            CambiarEstado(EstadoIA.Persiguiendo);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DISPARO ENEMIGO
    // ═══════════════════════════════════════════════════════════════════════

    private void Disparar()
    {
        if (jugador == null) return;

        Vector3 origen    = transform.position + Vector3.up * 1.5f;
        Vector3 dir       = (jugador.position + Vector3.up * 1f - origen).normalized;

        // Dispersión perpendicular a la dirección de disparo
        Vector3 perpH = Vector3.Cross(dir, Vector3.up).normalized;
        if (perpH == Vector3.zero) perpH = Vector3.right;
        Vector3 perpV = Vector3.Cross(dir, perpH).normalized;
        dir += perpH * Random.Range(-precisionEnemy, precisionEnemy)
             + perpV * Random.Range(-precisionEnemy, precisionEnemy);

        // Flash visual
        SistemaDisparo_Flash(origen, dir);

        // Impacto
        if (Physics.Raycast(origen, dir, out RaycastHit hit, radioAtaque * 1.5f))
        {
            var jugadorHit = hit.collider.GetComponentInParent<ControladorJugador>();
            if (jugadorHit != null)
                jugadorHit.RecibirDano(danoPorDisparo);
        }
    }

    private void SistemaDisparo_Flash(Vector3 origen, Vector3 dir)
    {
        var go = new GameObject("FlashEnemigo");
        go.transform.position = origen;
        var ps = go.AddComponent<ParticleSystem>();
        var main = ps.main;
        main.startLifetime = 0.05f;
        main.startSpeed    = 5f;
        main.startSize     = 0.08f;
        main.startColor    = new Color(1f, 0.9f, 0.4f);
        main.maxParticles  = 5;
        var em = ps.emission;
        em.rateOverTime = 0;
        em.SetBursts(new[] { new ParticleSystem.Burst(0f, 5) });
        ps.Play();
        Destroy(go, 0.15f);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MOVIMIENTO
    // ═══════════════════════════════════════════════════════════════════════

    private void MoverHacia(Vector3 destino, float velocidad)
    {
        var dir = destino - transform.position;
        dir.y   = 0f;

        if (dir.magnitude > 0.1f)
        {
            transform.rotation = Quaternion.RotateTowards(
                transform.rotation,
                Quaternion.LookRotation(dir.normalized),
                velocidadGiro * Time.deltaTime);

            transform.position += transform.forward * velocidad * Time.deltaTime;
        }
    }

    private IEnumerator EsperarEnWP()
    {
        esperandoWP = true;
        yield return new WaitForSeconds(tiempoEsperaWP);
        wpActual    = (wpActual + 1) % waypointsPatrulla.Length;
        esperandoWP = false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CAMBIO DE ESTADO
    // ═══════════════════════════════════════════════════════════════════════

    private void CambiarEstado(EstadoIA nuevo)
    {
        Estado = nuevo;
        if (nuevo == EstadoIA.Alertado)
        {
            timerAlerta = 5f;
            if (jugador != null) ultimaPosJugador = jugador.position;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DAÑO Y MUERTE
    // ═══════════════════════════════════════════════════════════════════════

    public void RecibirDano(int cantidad)
    {
        if (Estado == EstadoIA.Muerto) return;

        vida -= cantidad;

        // Flash rojo en el cuerpo
        StartCoroutine(FlashDano());

        // Alerta al recibir disparo
        if (Estado == EstadoIA.Patrullando)
            CambiarEstado(EstadoIA.Persiguiendo);

        if (vida <= 0) Morir();
    }

    private IEnumerator FlashDano()
    {
        foreach (var r in GetComponentsInChildren<Renderer>())
            r.material.color = Color.red;
        yield return new WaitForSeconds(0.15f);
        // BUG FIX: no restaurar color si el enemigo ya ha muerto durante el flash
        if (Estado != EstadoIA.Muerto)
        {
            foreach (var r in GetComponentsInChildren<Renderer>())
                r.material.color = colorUniforme;
        }
    }

    private void Morir()
    {
        Estado = EstadoIA.Muerto;

        // BUG FIX: detener todas las coroutines al morir para evitar que
        // FlashDano() restaure el color verde encima del color de "muerto"
        StopAllCoroutines();

        // Caer
        transform.rotation = Quaternion.Euler(90f, transform.eulerAngles.y, 0f);

        foreach (var r in GetComponentsInChildren<Renderer>())
            r.material.color = new Color(0.2f, 0.1f, 0.1f);

        Destroy(gameObject, 8f);
        Debug.Log("[Enemigo] Derribado.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CUERPO VISUAL BÁSICO
    // ═══════════════════════════════════════════════════════════════════════

    private Color colorUniforme = new Color(0.25f, 0.28f, 0.22f); // verde militar

    // Crea un Material compatible con URP (evita el magenta por shader incorrecto)
    private static Material MatURP(Color color)
    {
        var shader = Shader.Find("Universal Render Pipeline/Lit")
                  ?? Shader.Find("Standard");
        var mat = new Material(shader);
        mat.color = color;
        return mat;
    }

    private void CrearCuerpoBasico()
    {
        // Cuerpo
        var cuerpo = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        cuerpo.transform.SetParent(transform);
        cuerpo.transform.localPosition = new Vector3(0f, 1f, 0f);
        cuerpo.transform.localScale    = new Vector3(0.6f, 0.9f, 0.6f);
        cuerpo.GetComponent<Renderer>().material = MatURP(colorUniforme);
        Destroy(cuerpo.GetComponent<Collider>());

        // Cabeza
        var cabeza = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        cabeza.transform.SetParent(transform);
        cabeza.transform.localPosition = new Vector3(0f, 2.0f, 0f);
        cabeza.transform.localScale    = new Vector3(0.4f, 0.4f, 0.4f);
        cabeza.GetComponent<Renderer>().material = MatURP(new Color(0.75f, 0.62f, 0.48f));
        Destroy(cabeza.GetComponent<Collider>());

        // Casco
        var casco = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        casco.transform.SetParent(transform);
        casco.transform.localPosition = new Vector3(0f, 2.15f, 0f);
        casco.transform.localScale    = new Vector3(0.45f, 0.35f, 0.45f);
        casco.GetComponent<Renderer>().material = MatURP(colorUniforme);
        Destroy(casco.GetComponent<Collider>());

        // Colisionador
        var col = gameObject.AddComponent<CapsuleCollider>();
        col.height = 1.8f;
        col.radius = 0.3f;
        col.center = Vector3.up * 0.9f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  GIZMOS (waypoints visibles en editor)
    // ═══════════════════════════════════════════════════════════════════════

    private void OnDrawGizmosSelected()
    {
        Gizmos.color = Color.yellow;
        Gizmos.DrawWireSphere(transform.position, radioVision);
        Gizmos.color = Color.red;
        Gizmos.DrawWireSphere(transform.position, radioAtaque);
        Gizmos.color = Color.cyan;
        Gizmos.DrawWireSphere(transform.position, radioEscucha);

        if (waypointsPatrulla == null) return;
        Gizmos.color = Color.green;
        for (int i = 0; i < waypointsPatrulla.Length; i++)
        {
            if (waypointsPatrulla[i] == null) continue;
            Gizmos.DrawSphere(waypointsPatrulla[i].position, 0.4f);
            if (i + 1 < waypointsPatrulla.Length && waypointsPatrulla[i + 1] != null)
                Gizmos.DrawLine(waypointsPatrulla[i].position, waypointsPatrulla[i + 1].position);
        }
    }
}
