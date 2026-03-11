// Assets/Scripts/SistemaExplosion.cs
// Explosión con físicas, daño en radio, fuego y humo

using UnityEngine;
using System.Collections.Generic;

public class SistemaExplosion : MonoBehaviour
{
    [Header("═══ EXPLOSIÓN ═══")]
    [SerializeField] public float radio        = 15f;
    [SerializeField] public float fuerzaFisica = 800f;
    [SerializeField] public int   danoMaximo   = 150;
    [SerializeField] public float duracionFuego = 6f;

    /// <summary>
    /// Crea una explosión en la posición indicada.
    /// Llamar estáticamente: SistemaExplosion.Explotar(pos, radio, fuerza, dano)
    /// </summary>
    public static void Explotar(Vector3 posicion, float radio = 12f,
                                 float fuerza = 600f, int dano = 120)
    {
        var go = new GameObject("Explosion");
        go.transform.position = posicion;
        var se = go.AddComponent<SistemaExplosion>();
        se.radio        = radio;
        se.fuerzaFisica = fuerza;
        se.danoMaximo   = dano;
        se.Detonar();
    }

    public void Detonar()
    {
        AplicarFisicasYDano();
        EfectoBolaFuego();
        EfectoHumo();
        EfectoRescoldo();
        EfectoLlamas();
        EfectoOnda();

        // Sacudir cámara si el jugador está cerca
        SacudirCamara();

        Destroy(gameObject, duracionFuego + 2f);
    }

    // ─── Físicas y daño ──────────────────────────────────────────────────

    private void AplicarFisicasYDano()
    {
        Collider[] afectados = Physics.OverlapSphere(transform.position, radio);

        // BUG FIX: usar HashSets para evitar aplicar daño múltiple a la misma entidad
        // cuando tiene varios colliders hijos (ej. enemigos con cuerpo + cabeza + arma).
        var rbYaDanados       = new HashSet<Rigidbody>();
        var enemigosYaDanados = new HashSet<EnemigoPatrulla>();
        var jugadoresYaDanados= new HashSet<ControladorJugador>();
        var vehiculosYaDanados= new HashSet<VehiculoNPC>();

        foreach (var col in afectados)
        {
            // Fuerza de explosión a Rigidbodies (una sola vez por objeto físico)
            var rb = col.GetComponent<Rigidbody>();
            if (rb != null && rbYaDanados.Add(rb))
                rb.AddExplosionForce(fuerzaFisica, transform.position, radio, 1f);

            // Daño basado en la distancia al centro del objeto raíz
            float dist   = Vector3.Distance(transform.position, col.transform.position);
            float factor = 1f - Mathf.Clamp01(dist / radio);
            int   dano   = Mathf.RoundToInt(danoMaximo * factor);

            var enemigo  = col.GetComponentInParent<EnemigoPatrulla>();
            if (enemigo != null && enemigosYaDanados.Add(enemigo))
                enemigo.RecibirDano(dano);

            var jugador  = col.GetComponentInParent<ControladorJugador>();
            if (jugador != null && jugadoresYaDanados.Add(jugador))
                jugador.RecibirDano(dano);

            var vehiculo = col.GetComponentInParent<VehiculoNPC>();
            if (vehiculo != null && vehiculosYaDanados.Add(vehiculo))
                vehiculo.RecibirDano(dano);
        }
    }

    // ─── Efectos visuales ────────────────────────────────────────────────

    private void EfectoBolaFuego()
    {
        var go = new GameObject("BolaDeFuego");
        go.transform.position = transform.position;
        var ps = go.AddComponent<ParticleSystem>();

        var main          = ps.main;
        main.startLifetime = new ParticleSystem.MinMaxCurve(0.4f, 0.9f);
        main.startSpeed    = new ParticleSystem.MinMaxCurve(4f, 14f);
        main.startSize     = new ParticleSystem.MinMaxCurve(radio * 0.5f, radio * 1.2f);
        main.startColor    = new ParticleSystem.MinMaxGradient(
                                 new Color(1f, 0.6f, 0.1f, 0.9f),
                                 new Color(1f, 0.2f, 0f,   0.7f));
        main.gravityModifier = -0.3f;
        main.maxParticles    = 40;

        var em = ps.emission;
        em.rateOverTime = 0;
        em.SetBursts(new[] { new ParticleSystem.Burst(0f, 35) });

        var shape      = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        shape.radius   = radio * 0.3f;

        ps.Play();
        Destroy(go, 2f);
    }

    private void EfectoHumo()
    {
        var go = new GameObject("HumoExplosion");
        go.transform.position = transform.position;
        var ps = go.AddComponent<ParticleSystem>();

        var main           = ps.main;
        main.startLifetime  = new ParticleSystem.MinMaxCurve(3f, 6f);
        main.startSpeed     = new ParticleSystem.MinMaxCurve(2f, 6f);
        main.startSize      = new ParticleSystem.MinMaxCurve(radio * 0.8f, radio * 2.5f);
        main.startColor     = new ParticleSystem.MinMaxGradient(
                                  new Color(0.15f, 0.12f, 0.10f, 0.8f),
                                  new Color(0.35f, 0.30f, 0.25f, 0.5f));
        main.gravityModifier = -0.5f;
        main.maxParticles    = 25;

        var em = ps.emission;
        em.rateOverTime = 0;
        em.SetBursts(new[] { new ParticleSystem.Burst(0f, 20) });

        var shape      = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        shape.radius   = radio * 0.4f;

        ps.Play();
        Destroy(go, 8f);
    }

    private void EfectoRescoldo()
    {
        var go = new GameObject("Rescoldos");
        go.transform.position = transform.position;
        var ps = go.AddComponent<ParticleSystem>();

        var main           = ps.main;
        main.startLifetime  = new ParticleSystem.MinMaxCurve(1f, 2.5f);
        main.startSpeed     = new ParticleSystem.MinMaxCurve(5f, 20f);
        main.startSize      = new ParticleSystem.MinMaxCurve(0.05f, 0.15f);
        main.startColor     = new ParticleSystem.MinMaxGradient(
                                  new Color(1f, 0.8f, 0.2f),
                                  new Color(1f, 0.4f, 0f));
        main.gravityModifier = 0.6f;
        main.maxParticles    = 80;

        var em = ps.emission;
        em.rateOverTime = 0;
        em.SetBursts(new[] { new ParticleSystem.Burst(0f, 60) });

        var shape      = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        shape.radius   = 0.5f;

        ps.Play();
        Destroy(go, 3f);
    }

    private void EfectoLlamas()
    {
        // Llamas persistentes en el suelo
        var go = new GameObject("LlamasExplosion");
        go.transform.position = transform.position + Vector3.up * 0.5f;
        var ps = go.AddComponent<ParticleSystem>();

        var main           = ps.main;
        main.duration       = duracionFuego;
        main.loop           = true;
        main.startLifetime  = new ParticleSystem.MinMaxCurve(0.5f, 1.2f);
        main.startSpeed     = new ParticleSystem.MinMaxCurve(1f, 3f);
        main.startSize      = new ParticleSystem.MinMaxCurve(0.5f, 2f);
        main.startColor     = new ParticleSystem.MinMaxGradient(
                                  new Color(1f, 0.5f, 0.1f, 0.85f),
                                  new Color(1f, 0.2f, 0.0f, 0.65f));
        main.gravityModifier = -0.2f;
        main.maxParticles    = 60;

        var em = ps.emission;
        em.rateOverTime = 30f;

        var shape      = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Circle;
        shape.radius   = radio * 0.25f;

        ps.Play();
        Destroy(go, duracionFuego + 1f);
    }

    private void EfectoOnda()
    {
        // Onda expansiva visual (esfera que crece y desaparece)
        var go   = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.transform.position   = transform.position;
        go.transform.localScale = Vector3.zero;
        Destroy(go.GetComponent<Collider>());

        var rend     = go.GetComponent<Renderer>();
        var mat      = new Material(Shader.Find("Universal Render Pipeline/Unlit")
                                 ?? Shader.Find("Unlit/Color")
                                 ?? Shader.Find("Standard"));
        mat.color    = new Color(1f, 0.8f, 0.5f, 0.3f);
        rend.material = mat;

        float t       = 0f;
        float dur     = 0.3f;
        float maxScale = radio * 2.5f;

        // Animar la onda
        var lerper = go.AddComponent<OndaExplosionAnim>();
        lerper.Init(maxScale, dur);
    }

    private void SacudirCamara()
    {
        Camera cam = Camera.main;
        if (cam == null) return;

        float dist = Vector3.Distance(transform.position, cam.transform.position);
        if (dist > radio * 4f) return;

        float intensidad = 1f - dist / (radio * 4f);
        var sacudida = cam.GetComponent<SacudidaCamara>();
        if (sacudida == null) sacudida = cam.gameObject.AddComponent<SacudidaCamara>();
        sacudida.Sacudir(intensidad * 0.5f, 0.6f);
    }
}

// ─── Componente auxiliar para animar la onda ─────────────────────────────────
public class OndaExplosionAnim : MonoBehaviour
{
    private float    maxScale, duracion, timer;
    private Renderer rend;  // cacheado para no llamar GetComponent cada frame

    public void Init(float max, float dur)
    {
        maxScale = max;
        // Guardia: duración mínima para evitar división por cero
        duracion = Mathf.Max(dur, 0.01f);
        rend     = GetComponent<Renderer>();
    }

    private void Update()
    {
        timer += Time.deltaTime;
        // Clamp01: evita que t > 1 cuando timer sobrepasa duracion en el último frame
        float t = Mathf.Clamp01(timer / duracion);

        transform.localScale = Vector3.one * Mathf.Lerp(0f, maxScale, t);

        if (rend != null)
            rend.material.color = new Color(1f, 0.8f, 0.5f, Mathf.Lerp(0.4f, 0f, t));

        if (timer >= duracion) Destroy(gameObject);
    }
}

// ─── Sacudida de cámara por explosión ────────────────────────────────────────
public class SacudidaCamara : MonoBehaviour
{
    private float intensidad, duracion, timer;
    private Vector3 posOriginal;

    public void Sacudir(float intens, float dur)
    {
        // BUG FIX: solo capturar posOriginal cuando NO se está sacudiendo ya.
        // Si se llama mientras timer > 0, la posición actual ya está desplazada
        // y capturarla causaría que la cámara derive respecto a su posición real.
        if (timer <= 0f)
            posOriginal = transform.localPosition;

        // Acumular la sacudida más intensa de las dos (nueva vs actual)
        intensidad = Mathf.Max(intensidad, intens);
        duracion   = Mathf.Max(duracion, dur);
        timer      = Mathf.Max(timer, dur);
    }

    private void Update()
    {
        if (timer <= 0) return;

        timer -= Time.deltaTime;
        float t = timer / duracion;
        transform.localPosition = posOriginal + Random.insideUnitSphere * intensidad * t;

        if (timer <= 0)
            transform.localPosition = posOriginal;
    }
}
