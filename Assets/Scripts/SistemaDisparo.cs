// Assets/Scripts/SistemaDisparo.cs
// Sistema de disparo con raycast, cadencia, munición e impactos

using UnityEngine;
using UnityEngine.InputSystem;

public class SistemaDisparo : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  CONFIGURACIÓN
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ ARMA ═══")]
    [SerializeField] private float alcanceDisparo  = 200f;
    [SerializeField] private int   danoPorBala     = 25;
    [SerializeField] private float cadencia         = 0.12f;   // segundos entre disparos
    [SerializeField] private float dispersionMax    = 0.015f;  // dispersión del arma

    [Header("═══ MUNICIÓN ═══")]
    [SerializeField] private int   balas          = 30;
    [SerializeField] private int   balasMaxCargador = 30;
    [SerializeField] private int   balasReserva   = 120;
    [SerializeField] private float tiempoRecarga  = 2.0f;

    [Header("═══ CAPAS ═══")]
    [SerializeField] private LayerMask capasImpacto;   // qué capas reciben impacto

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private float              tiempoUltimoDisparo = 0f;
    private bool               estaCargando        = false;
    private float              timerRecarga        = 0f;
    private Camera             camara;
    private ControladorJugador controlJugador; // BUG 6 FIX: cachear en Awake(), no buscar cada disparo

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        camara = GetComponentInChildren<Camera>();
        if (camara == null) camara = Camera.main;

        // BUG 6 FIX: cachear la referencia al ControladorJugador en Awake().
        // Antes se llamaba GetComponentInParent() EN CADA DISPARO (hasta 8 disparos/seg),
        // que es una búsqueda de árbol de componentes innecesariamente cara.
        controlJugador = GetComponentInParent<ControladorJugador>()
                      ?? GetComponent<ControladorJugador>();

        // Por defecto: todas las capas EXCEPTO "Ignore Raycast" (capa 2).
        // El jugador se coloca en capa 2 para no bloquearse sus propios disparos.
        if (capasImpacto == 0)
            capasImpacto = ~(1 << 2);
    }

    private void Update()
    {
        // Recarga
        if (estaCargando)
        {
            timerRecarga -= Time.deltaTime;
            if (timerRecarga <= 0f)
                FinRecarga();
        }

        // Tecla R = recargar manualmente
        if (Keyboard.current != null && Keyboard.current.rKey.wasPressedThisFrame)
            IniciarRecarga();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  API PÚBLICA
    // ═══════════════════════════════════════════════════════════════════════

    public void Disparar()
    {
        if (estaCargando) return;
        if (Time.time - tiempoUltimoDisparo < cadencia) return;
        if (balas <= 0) { IniciarRecarga(); return; }
        if (camara == null) return;  // cámara no disponible

        tiempoUltimoDisparo = Time.time;
        balas--;

        // ── Técnica 3ª persona: two-step aiming ─────────────────────────────
        // 1. Ray desde la cámara para encontrar el "punto de mira" en el mundo.
        //    (El jugador está en capa 2 = IgnoreRaycast → el ray lo atraviesa.)
        // 2. Disparar desde la posición del arma HACIA ese punto de mira.
        // Esto da alineación perfecta entre crosshair y punto de impacto,
        // independientemente del ángulo de la cámara en 3ª persona.

        Vector3 origen    = camara.transform.position;
        Vector3 dirCamara = camara.transform.forward;

        // Step 1 — Punto de mira (ignora capa 2 donde está el jugador)
        Vector3 puntoMira = Physics.Raycast(origen, dirCamara, out RaycastHit aimHit,
                                            alcanceDisparo, capasImpacto)
                          ? aimHit.point
                          : origen + dirCamara * alcanceDisparo;

        // Step 2 — Posición del arma (mano derecha del jugador)
        // BUG 6 FIX: usar la referencia cacheada en Awake() en vez de buscar cada disparo.
        Vector3 posArma = controlJugador != null
            ? controlJugador.transform.position
              + controlJugador.transform.forward * 0.38f
              + Vector3.up * 1.30f
              + controlJugador.transform.right * 0.25f
            : origen;   // FPS fallback: disparar desde la cámara

        // Dirección final + dispersión perpendicular al disparo
        Vector3 direccion = (puntoMira - posArma).normalized;
        Vector3 perpH = Vector3.Cross(direccion, Vector3.up).normalized;
        if (perpH == Vector3.zero) perpH = Vector3.right;
        Vector3 perpV = Vector3.Cross(direccion, perpH).normalized;
        direccion += perpH * Random.Range(-dispersionMax, dispersionMax)
                   + perpV * Random.Range(-dispersionMax, dispersionMax);

        EfectoDisparo(posArma, direccion);

        // Raycast definitivo desde el arma
        if (Physics.Raycast(posArma, direccion, out RaycastHit hit, alcanceDisparo, capasImpacto))
            ProcesarImpacto(hit);

        // Auto-recargar cuando se acaba el cargador
        if (balas <= 0) IniciarRecarga();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  IMPACTO
    // ═══════════════════════════════════════════════════════════════════════

    private void ProcesarImpacto(RaycastHit hit)
    {
        // ¿Es un enemigo?
        var enemigo = hit.collider.GetComponentInParent<EnemigoPatrulla>();
        if (enemigo != null)
        {
            enemigo.RecibirDano(danoPorBala);
            EfectoSangre(hit.point, hit.normal);
            return;
        }

        // ¿Es un vehículo NPC?
        var vehiculo = hit.collider.GetComponentInParent<VehiculoNPC>();
        if (vehiculo != null)
        {
            vehiculo.RecibirDano(danoPorBala);
            EfectoChispa(hit.point, hit.normal);
            return;
        }

        // Superficie genérica
        EfectoImpactoSuelo(hit.point, hit.normal);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  RECARGA
    // ═══════════════════════════════════════════════════════════════════════

    private void IniciarRecarga()
    {
        if (estaCargando) return;
        if (balasReserva <= 0) return;
        if (balas == balasMaxCargador) return;

        estaCargando  = true;
        timerRecarga  = tiempoRecarga;
        Debug.Log("[Disparo] Recargando...");
    }

    private void FinRecarga()
    {
        int necesarias  = balasMaxCargador - balas;
        int disponibles = Mathf.Min(necesarias, balasReserva);
        balas          += disponibles;
        balasReserva   -= disponibles;
        estaCargando    = false;
        Debug.Log($"[Disparo] Recargado: {balas}/{balasMaxCargador}  Reserva: {balasReserva}");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  EFECTOS VISUALES (partículas por código, sin assets externos)
    // ═══════════════════════════════════════════════════════════════════════

    private void EfectoDisparo(Vector3 origen, Vector3 direccion)
    {
        // Flash del cañón
        CrearBurst(origen + direccion * 0.5f, new Color(1f, 0.8f, 0.3f), 0.05f, 8, 0.08f);
    }

    private void EfectoImpactoSuelo(Vector3 punto, Vector3 normal)
    {
        // Polvo / chispas en pared/suelo
        CrearBurst(punto, new Color(0.7f, 0.6f, 0.5f), 0.06f, 12, 0.3f);

        // Marca de bala (pequeña esfera negra)
        // FIX: usar MaterialPropertyBlock en lugar de renderer.material para no crear
        // una instancia de Material per-decal. A 8 disparos/seg, renderer.material
        // generaría ~480 instancias en 60 seg → presión sobre el GC innecesaria.
        var marca = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        marca.transform.position   = punto + normal * 0.01f;
        marca.transform.localScale = Vector3.one * 0.04f;
        var rend = marca.GetComponent<Renderer>();
        var pb   = new MaterialPropertyBlock();
        pb.SetColor("_BaseColor", new Color(0.1f, 0.1f, 0.1f));
        pb.SetColor("_Color",     new Color(0.1f, 0.1f, 0.1f));
        rend.SetPropertyBlock(pb);
        Destroy(marca.GetComponent<Collider>());
        Destroy(marca, 8f);
    }

    private void EfectoSangre(Vector3 punto, Vector3 normal)
    {
        CrearBurst(punto, new Color(0.6f, 0f, 0f), 0.06f, 15, 0.25f);
    }

    private void EfectoChispa(Vector3 punto, Vector3 normal)
    {
        CrearBurst(punto, new Color(1f, 0.9f, 0.3f), 0.04f, 20, 0.15f);
    }

    private static void CrearBurst(Vector3 pos, Color color, float tamano, int cantidad, float duracion)
    {
        var go = new GameObject("Burst");
        go.transform.position = pos;
        var ps = go.AddComponent<ParticleSystem>();

        var main = ps.main;
        main.startLifetime  = duracion;
        main.startSpeed     = new ParticleSystem.MinMaxCurve(1f, 4f);
        main.startSize      = tamano;
        main.startColor     = color;
        main.gravityModifier = 0.4f;
        main.maxParticles   = cantidad;

        var em  = ps.emission;
        em.rateOverTime = 0f;
        em.SetBursts(new[] { new ParticleSystem.Burst(0f, cantidad) });

        var shape = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        shape.radius    = 0.05f;

        ps.Play();
        Destroy(go, duracion + 0.5f);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HUD
    // ═══════════════════════════════════════════════════════════════════════

    private void OnGUI()
    {
        float x = Screen.width - 160f;
        float y = Screen.height - 40f;

        GUI.color = new Color(0, 0, 0, 0.55f);
        GUI.DrawTexture(new Rect(x - 4, y - 4, 155, 28), Texture2D.whiteTexture);

        GUI.color = estaCargando ? Color.yellow : Color.white;
        string txt = estaCargando
            ? $"RECARGANDO... {timerRecarga:F1}s"
            : $"🔫 {balas} / {balasMaxCargador}   [{balasReserva}]";
        GUI.Label(new Rect(x, y, 150, 24), txt);
    }
}
