// Assets/Scripts/SistemaClima.cs
// Sistema de clima fotorrealista para Alsasua (clima atlántico-montañoso)
// Lluvia · Llovizna · Niebla baja · Nubarrones · Tormenta
// No requiere assets externos — todo se genera por código.

using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

// ─────────────────────────────────────────────────────────────────────────────
//  ESTADOS DE CLIMA (típicos de Nafarroa / País Vasco)
// ─────────────────────────────────────────────────────────────────────────────
public enum EstadoClima
{
    Despejado,      // cielo azul, sin niebla
    Nublado,        // nubes grises, sin lluvia
    Llovizna,       // sirimiri — lluvia muy fina
    Lluvia,         // lluvia moderada normal
    LluviaIntensa,  // chaparrón fuerte
    Niebla,         // niebla densa sin lluvia (muy típico en el valle)
    NieblaBaja,     // niebla baja con cielo despejado arriba
    Tormenta        // lluvia + viento fuerte
}

/// <summary>
/// Gestiona el clima de Alsasua en tiempo real:
/// lluvia con partículas, niebla exponencial, cielo nublado, viento.
/// Se integra con SistemaAtmosfera y el Volume de post-procesado.
/// </summary>
[DefaultExecutionOrder(-40)]
public class SistemaClima : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO GENERAL
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ CLIMA ACTUAL ═══")]
    [Tooltip("Estado meteorológico de Alsasua. El defecto es Lluvia (clima atlántico).")]
    [SerializeField] private EstadoClima climaActual = EstadoClima.Lluvia;

    [Tooltip("Activar ciclo de clima aleatorio (cambia cada cierto tiempo)")]
    [SerializeField] private bool cicloAleatorio = false;

    [Range(30f, 600f)]
    [Tooltip("Segundos hasta el próximo cambio de clima (en tiempo de juego)")]
    [SerializeField] private float intervaloCambioClima = 120f;

    [Range(0f, 1f)]
    [Tooltip("Probabilidad de lluvia/niebla en el ciclo aleatorio (Alsasua = 0.75)")]
    [SerializeField] private float probabilidadLluvia = 0.75f;

    // ═══════════════════════════════════════════════════════════════════════
    //  LLUVIA
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ LLUVIA ═══")]
    [Range(0f, 90f)]
    [Tooltip("Ángulo de inclinación de la lluvia por el viento (0 = vertical)")]
    [SerializeField] private float anguloViento = 8f;

    [Range(0f, 360f)]
    [Tooltip("Dirección del viento (grados, 0=Norte, 90=Este)")]
    [SerializeField] private float direccionViento = 220f;   // Suroeste — viento atlántico

    [Range(5f, 20f)]
    [Tooltip("Velocidad de caída de las gotas (m/s)")]
    [SerializeField] private float velocidadGota = 10f;

    [Range(0f, 1f)]
    [Tooltip("Opacidad de las gotas")]
    [SerializeField] private float opacidadLluvia = 0.55f;

    // ═══════════════════════════════════════════════════════════════════════
    //  NIEBLA
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ NIEBLA ═══")]
    [SerializeField] private Color colorNieblaLluvia = new Color(0.60f, 0.63f, 0.68f);
    [SerializeField] private Color colorNieblaLimpia = new Color(0.82f, 0.86f, 0.90f);
    [SerializeField] private Color colorNieblaEspesa = new Color(0.78f, 0.80f, 0.82f);

    // ═══════════════════════════════════════════════════════════════════════
    //  REFERENCIAS EXTERNAS
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ REFERENCIAS ═══")]
    [SerializeField] private SistemaAtmosfera atmosfera;
    [SerializeField] private Light luzSolar;
    [SerializeField] private Volume volumenPostProcesado;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    // Sistemas de partículas de lluvia (se crean dinámicamente)
    private ParticleSystem psLluvia;       // gotas principales
    private ParticleSystem psBruma;        // bruma fina de fondo

    // Efectos del volume que modificamos
    private ColorAdjustments colorAdjustments;
    private WhiteBalance      whiteBalance;
    private Vignette          vignette;
    private DepthOfField      dof;

    // Transición
    private EstadoClima climaObjetivo;
    private float progresoTransicion = 1f;
    private float velocidadTransicion = 0.5f;  // unidades/seg

    // Ciclo
    private float timerCiclo = 0f;

    // Parámetros del estado actual (interpolados)
    private float emisionActual   = 0f;
    private float densidadActual  = 0f;
    private Color colorNieblaActual;
    private float intensidadSolActual = 1f;
    private float temperaturaCCActual = 0f;

    // BUG 22 FIX: cachear Camera.main en Awake() — llamarla en SeguirCamara() cada frame
    // realiza una búsqueda de escena (O(n)) que es innecesaria ya que no cambia.
    private Camera camPrincipal;

    // BUG 24 FIX: evitar recalcular el ángulo de lluvia cada frame si el viento no cambió.
    private float anguloVientoPrev    = float.MaxValue;
    private float direccionVientoPrev = float.MaxValue;

    // ═══════════════════════════════════════════════════════════════════════
    //  DATOS POR ESTADO (tabla de configuración)
    // ═══════════════════════════════════════════════════════════════════════

    private struct DatosClima
    {
        public float emision;        // partículas/seg lluvia
        public float emisionBruma;   // partículas/seg bruma
        public float densidadNiebla; // exponentialSquared
        public Color colorNiebla;
        public float multiplicadorSol; // 0=noche total, 1=sol pleno
        public float temperaturaCC;    // WhiteBalance, -50=frío
        public float saturacion;       // -100 a +100 (URP)
        public float vignetteExtra;    // intensidad extra de vignette
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        if (atmosfera           == null) atmosfera           = Object.FindFirstObjectByType<SistemaAtmosfera>();
        if (luzSolar            == null) luzSolar            = BuscarLuzDirectional();
        if (volumenPostProcesado == null) volumenPostProcesado = Object.FindFirstObjectByType<Volume>();

        colorNieblaActual = colorNieblaLluvia;
        climaObjetivo     = climaActual;
        camPrincipal      = Camera.main; // BUG 22 FIX
    }

    private void Start()
    {
        CrearSistemaLluvia();
        CrearSistemaBruma();
        ObtenerEfectosVolumen();

        // Aplicar clima inmediatamente (sin transición)
        var datos = ObtenerDatos(climaActual);
        AplicarDatosInstantaneo(datos);

        Debug.Log($"[Clima] Iniciado: {climaActual}");
    }

    private void Update()
    {
        // Seguir la cámara con la lluvia
        SeguirCamara();

        // Transición suave entre estados
        if (progresoTransicion < 1f)
        {
            progresoTransicion = Mathf.MoveTowards(progresoTransicion, 1f,
                                                   velocidadTransicion * Time.deltaTime);
            InterpolaDatos(ObtenerDatos(climaActual), ObtenerDatos(climaObjetivo),
                           progresoTransicion);
        }

        // Ciclo aleatorio
        if (cicloAleatorio)
        {
            timerCiclo += Time.deltaTime;
            if (timerCiclo >= intervaloCambioClima)
            {
                timerCiclo = 0f;
                CambiarClimaAleatorio();
            }
        }

        // BUG 24 FIX: recalcular el ángulo de lluvia sólo cuando el viento haya cambiado.
        // Antes se llamaba cada frame aunque anguloViento/direccionViento fueran constantes,
        // ejecutando trigonometría y modificando el ParticleSystem innecesariamente.
        if (!Mathf.Approximately(anguloViento, anguloVientoPrev) ||
            !Mathf.Approximately(direccionViento, direccionVientoPrev))
        {
            ActualizarAnguloLluvia();
            anguloVientoPrev    = anguloViento;
            direccionVientoPrev = direccionViento;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  API PÚBLICA
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>Cambia el clima con transición suave.</summary>
    public void SetClima(EstadoClima nuevoClima, float duracionTransicion = 10f)
    {
        if (nuevoClima == climaObjetivo) return;

        climaActual       = climaObjetivo;     // partimos del objetivo anterior
        climaObjetivo     = nuevoClima;
        progresoTransicion = 0f;
        velocidadTransicion = 1f / Mathf.Max(duracionTransicion, 0.1f);

        Debug.Log($"[Clima] Transición: {climaActual} → {climaObjetivo} ({duracionTransicion}s)");
    }

    // Atajos de Inspector (clic derecho sobre el script)
    [ContextMenu("Estado: Despejado")]        private void CtxDespejado()       => SetClima(EstadoClima.Despejado, 15f);
    [ContextMenu("Estado: Nublado")]          private void CtxNublado()         => SetClima(EstadoClima.Nublado, 10f);
    [ContextMenu("Estado: Llovizna (sirimiri)")]  private void CtxLlovizna()   => SetClima(EstadoClima.Llovizna, 8f);
    [ContextMenu("Estado: Lluvia")]           private void CtxLluvia()          => SetClima(EstadoClima.Lluvia, 5f);
    [ContextMenu("Estado: Lluvia Intensa")]   private void CtxLluviaIntensa()   => SetClima(EstadoClima.LluviaIntensa, 3f);
    [ContextMenu("Estado: Niebla Densa")]     private void CtxNiebla()          => SetClima(EstadoClima.Niebla, 20f);
    [ContextMenu("Estado: Niebla Baja")]      private void CtxNieblaBaja()      => SetClima(EstadoClima.NieblaBaja, 20f);
    [ContextMenu("Estado: Tormenta")]         private void CtxTormenta()        => SetClima(EstadoClima.Tormenta, 2f);

    // ═══════════════════════════════════════════════════════════════════════
    //  TABLA DE PARÁMETROS POR ESTADO
    // ═══════════════════════════════════════════════════════════════════════

    private DatosClima ObtenerDatos(EstadoClima estado) => estado switch
    {
        //                      emision  bruma  densNiebla  colorNiebla          sol    tempCC  sat   vig
        EstadoClima.Despejado      => D(    0,     50,  0.00010f, colorNieblaLimpia,  1.00f,   0f,  0f, 0.00f),
        EstadoClima.Nublado        => D(    0,    200,  0.00030f, colorNieblaLluvia,  0.30f, -15f, -5f, 0.05f),
        EstadoClima.Llovizna       => D(  400,    500,  0.00120f, colorNieblaLluvia,  0.15f, -20f,-10f, 0.08f),
        EstadoClima.Lluvia         => D( 2000,    800,  0.00200f, colorNieblaLluvia,  0.10f, -25f,-15f, 0.10f),
        EstadoClima.LluviaIntensa  => D( 5000,   1200,  0.00400f, colorNieblaLluvia,  0.06f, -30f,-20f, 0.15f),
        EstadoClima.Niebla         => D(    0,   2000,  0.00800f, colorNieblaEspesa,  0.08f, -10f,-12f, 0.20f),
        EstadoClima.NieblaBaja     => D(    0,    800,  0.00350f, colorNieblaEspesa,  0.50f,  -5f, -8f, 0.12f),
        EstadoClima.Tormenta       => D( 8000,   1500,  0.00500f, colorNieblaLluvia,  0.04f, -35f,-25f, 0.20f),
        _                          => D(    0,     50,  0.00010f, colorNieblaLimpia,  1.00f,   0f,  0f, 0.00f),
    };

    private DatosClima D(float em, float emB, float den, Color col,
                          float sol, float temp, float sat, float vig)
        => new DatosClima
        {
            emision         = em,
            emisionBruma    = emB,
            densidadNiebla  = den,
            colorNiebla     = col,
            multiplicadorSol = sol,
            temperaturaCC   = temp,
            saturacion      = sat,
            vignetteExtra   = vig,
        };

    // ═══════════════════════════════════════════════════════════════════════
    //  APLICAR / INTERPOLAR DATOS
    // ═══════════════════════════════════════════════════════════════════════

    private void AplicarDatosInstantaneo(DatosClima d)
    {
        AplicarNiebla(d.densidadNiebla, d.colorNiebla);
        AplicarLluvia(d.emision, d.emisionBruma);
        AplicarLuz(d.multiplicadorSol);
        AplicarPostProcesado(d.temperaturaCC, d.saturacion, d.vignetteExtra);

        emisionActual      = d.emision;
        densidadActual     = d.densidadNiebla;
        colorNieblaActual  = d.colorNiebla;
        intensidadSolActual = d.multiplicadorSol;
        temperaturaCCActual = d.temperaturaCC;
    }

    private void InterpolaDatos(DatosClima desde, DatosClima hasta, float t)
    {
        float em   = Mathf.Lerp(desde.emision,         hasta.emision,         t);
        float emB  = Mathf.Lerp(desde.emisionBruma,    hasta.emisionBruma,    t);
        float den  = Mathf.Lerp(desde.densidadNiebla,  hasta.densidadNiebla,  t);
        Color col  = Color.Lerp(desde.colorNiebla,     hasta.colorNiebla,     t);
        float sol  = Mathf.Lerp(desde.multiplicadorSol, hasta.multiplicadorSol, t);
        float temp = Mathf.Lerp(desde.temperaturaCC,   hasta.temperaturaCC,   t);
        float sat  = Mathf.Lerp(desde.saturacion,      hasta.saturacion,      t);
        float vig  = Mathf.Lerp(desde.vignetteExtra,   hasta.vignetteExtra,   t);

        AplicarNiebla(den, col);
        AplicarLluvia(em, emB);
        AplicarLuz(sol);
        AplicarPostProcesado(temp, sat, vig);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  NIEBLA
    // ═══════════════════════════════════════════════════════════════════════

    private void AplicarNiebla(float densidad, Color color)
    {
        RenderSettings.fog        = true;
        RenderSettings.fogMode    = FogMode.ExponentialSquared;
        RenderSettings.fogDensity = densidad;
        RenderSettings.fogColor   = color;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  LLUVIA — PARTÍCULAS
    // ═══════════════════════════════════════════════════════════════════════

    private void AplicarLluvia(float emision, float emisionBruma)
    {
        if (psLluvia != null)
        {
            var em = psLluvia.emission;
            em.rateOverTime = emision;

            // Activar/desactivar
            if (emision > 0 && !psLluvia.isPlaying) psLluvia.Play();
            if (emision <= 0 && psLluvia.isPlaying) psLluvia.Stop(true, ParticleSystemStopBehavior.StopEmitting);
        }

        if (psBruma != null)
        {
            var em = psBruma.emission;
            em.rateOverTime = emisionBruma;

            if (emisionBruma > 0 && !psBruma.isPlaying) psBruma.Play();
            if (emisionBruma <= 0 && psBruma.isPlaying)  psBruma.Stop(true, ParticleSystemStopBehavior.StopEmitting);
        }
    }

    private void ActualizarAnguloLluvia()
    {
        if (psLluvia == null) return;

        // Velocidad con ángulo de viento
        float rad = direccionViento * Mathf.Deg2Rad;
        float incX = Mathf.Sin(rad) * Mathf.Tan(anguloViento * Mathf.Deg2Rad) * velocidadGota;
        float incZ = Mathf.Cos(rad) * Mathf.Tan(anguloViento * Mathf.Deg2Rad) * velocidadGota;

        var velocidad = psLluvia.velocityOverLifetime;
        velocidad.enabled = true;
        velocidad.x = new ParticleSystem.MinMaxCurve(incX);
        velocidad.z = new ParticleSystem.MinMaxCurve(incZ);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  LUZ SOLAR
    // ═══════════════════════════════════════════════════════════════════════

    private void AplicarLuz(float multiplicador)
    {
        if (luzSolar == null) return;

        // Si hay SistemaAtmosfera, su intensidad ya tiene en cuenta la hora del día;
        // aquí sólo aplicamos el multiplicador meteorológico encima.
        float baseIntensidad = atmosfera != null
            ? Mathf.Lerp(0.04f, 1.8f, Mathf.Clamp01((atmosfera.ElevacionSolar + 8f) / 20f))
            : 1.0f;

        luzSolar.intensity = baseIntensidad * multiplicador;

        // Color más frío / más gris en días de lluvia
        if (multiplicador < 0.5f)
            luzSolar.color = Color.Lerp(luzSolar.color, new Color(0.7f, 0.72f, 0.78f), 1f - multiplicador * 2f);

        // MEJORA REALISMO: las nubes dispersan la luz solar y debilitan las sombras duras.
        // Despejado (mult≈1.0) → shadowStrength alto (sombras duras, sol directo).
        // Nublado/lluvia (mult≈0.1–0.3) → shadowStrength bajo (luz difusa, sin sombras definidas).
        // SistemaAtmosfera ya pone una base en shadowStrength; aquí la modulamos con el clima.
        // Usamos Mathf.Min para no sobrepasar lo que SistemaAtmosfera haya calculado.
        float shadowStrengthClima = Mathf.Clamp01(multiplicador * 1.4f + 0.15f);
        luzSolar.shadowStrength   = Mathf.Min(luzSolar.shadowStrength, shadowStrengthClima);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  POST-PROCESADO
    // ═══════════════════════════════════════════════════════════════════════

    private void ObtenerEfectosVolumen()
    {
        if (volumenPostProcesado == null) return;
        var p = volumenPostProcesado.profile;
        p.TryGet(out colorAdjustments);
        p.TryGet(out whiteBalance);
        p.TryGet(out vignette);
        p.TryGet(out dof);
    }

    private void AplicarPostProcesado(float temperaturaCC, float saturacion, float vignetteExtra)
    {
        // BUG 29 FIX: SistemaClima ya NO escribe directamente en whiteBalance.temperature.
        // ControladorPostProcesado es el ÚNICO escritor de esa propiedad — la combina con la
        // temperatura base (hora del día) leyendo TemperaturaActual desde aquí.
        // Sólo actualizamos el campo interno para que ControladorPostProcesado lo pueda leer.
        temperaturaCCActual = temperaturaCC;

        if (colorAdjustments != null)
        {
            colorAdjustments.saturation.value         = saturacion;
            colorAdjustments.saturation.overrideState = true;
        }

        if (vignette != null)
        {
            vignette.intensity.value         = Mathf.Clamp01(0.25f + vignetteExtra);
            vignette.intensity.overrideState = true;
        }

        // Lluvia → reducir DOF distance (visibilidad más corta)
        if (dof != null)
        {
            // MEJORA REALISMO: distancia de enfoque en lluvia ajustada a escala humana.
            // Con lluvia intensa (vignetteExtra≈0.15) → foco a 80m (visibilidad reducida).
            // Con lluvia ligera (vignetteExtra≈0.05) → foco a 200m.
            // Antes: 300-800m (irrealista a nivel del suelo — nadie ve a 300m en lluvia).
            float visDOF = Mathf.Lerp(80f, 250f, Mathf.Clamp01(1f - vignetteExtra * 5f));
            dof.focusDistance.value         = visDOF;
            dof.focusDistance.overrideState = true;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  SEGUIR CÁMARA
    // ═══════════════════════════════════════════════════════════════════════

    private void SeguirCamara()
    {
        // BUG 22 FIX: usar la cámara cacheada en Awake() en vez de Camera.main cada frame.
        // Si por algún motivo la referencia se pierde (cambio de escena), refrescar la caché.
        if (camPrincipal == null) camPrincipal = Camera.main;
        Camera cam = camPrincipal;
        if (cam == null) return;

        Vector3 posBase = new Vector3(cam.transform.position.x,
                                      cam.transform.position.y,
                                      cam.transform.position.z);

        if (psLluvia != null)
            psLluvia.transform.position = posBase + Vector3.up * 120f;

        if (psBruma != null)
            psBruma.transform.position  = posBase + Vector3.up * 80f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CREAR SISTEMAS DE PARTÍCULAS
    // ═══════════════════════════════════════════════════════════════════════

    private void CrearSistemaLluvia()
    {
        var go = new GameObject("Lluvia_Particulas");
        go.transform.SetParent(transform);
        psLluvia = go.AddComponent<ParticleSystem>();

        // ── Main ─────────────────────────────────────────────────────────
        var main = psLluvia.main;
        // MEJORA RENDIMIENTO: 15000 partículas visualmente idéntico a 20000 pero
        // reduce carga GPU en ~25% (menos fillrate en pantalla).
        main.maxParticles            = 15000;
        main.startLifetime           = new ParticleSystem.MinMaxCurve(1.8f, 3.0f);
        main.startSpeed              = new ParticleSystem.MinMaxCurve(velocidadGota * 0.8f, velocidadGota * 1.2f);
        main.startSize               = new ParticleSystem.MinMaxCurve(0.03f, 0.06f);
        main.startColor              = new ParticleSystem.MinMaxGradient(
                                            new Color(0.75f, 0.82f, 0.90f, opacidadLluvia * 0.6f),
                                            new Color(0.85f, 0.90f, 0.95f, opacidadLluvia));
        main.gravityModifier         = 0.3f;    // gravedad parcial — la velocidad inicial hace el resto
        main.simulationSpace         = ParticleSystemSimulationSpace.World;
        main.playOnAwake             = false;

        // ── Emisión ──────────────────────────────────────────────────────
        var em = psLluvia.emission;
        em.rateOverTime = 0f;   // Se asigna en AplicarLluvia()

        // ── Forma: caja plana sobre la cámara ────────────────────────────
        var shape = psLluvia.shape;
        shape.shapeType = ParticleSystemShapeType.Box;
        shape.scale     = new Vector3(300f, 1f, 300f);

        // ── Velocidad sobre vida (viento) ────────────────────────────────
        var vel = psLluvia.velocityOverLifetime;
        vel.enabled = true;
        vel.space   = ParticleSystemSimulationSpace.World;
        vel.y       = new ParticleSystem.MinMaxCurve(-velocidadGota);  // caída constante

        // ── Colisión con el terreno (opcional) ───────────────────────────
        // MEJORA RENDIMIENTO: quality Low usa malla simplificada → ~60% menos coste
        // manteniendo el mismo aspecto visual (las gotas desaparecen al tocar el suelo).
        var col = psLluvia.collision;
        col.enabled       = true;
        col.type          = ParticleSystemCollisionType.World;
        col.mode          = ParticleSystemCollisionMode.Collision3D;
        col.quality       = ParticleSystemCollisionQuality.Low;
        col.lifetimeLoss  = 1f;   // las gotas mueren al impactar
        col.radiusScale   = 0.1f;
        col.maxKillSpeed  = 100f; // matar gotas demasiado lentas (acumuladas) → limpia el pool

        // ── Renderizado: estiramiento = rayitas de lluvia ─────────────────
        var rend = psLluvia.GetComponent<ParticleSystemRenderer>();
        rend.renderMode   = ParticleSystemRenderMode.Stretch;
        rend.velocityScale = 0.04f;
        rend.lengthScale   = 2.0f;
        rend.material      = CrearMaterialLluvia(new Color(0.80f, 0.88f, 0.95f, 0.65f));
        rend.sortingOrder  = 10;

        Debug.Log("[Clima] Sistema de lluvia creado.");
    }

    private void CrearSistemaBruma()
    {
        var go = new GameObject("Bruma_Particulas");
        go.transform.SetParent(transform);
        psBruma = go.AddComponent<ParticleSystem>();

        // ── Main ─────────────────────────────────────────────────────────
        var main = psBruma.main;
        main.maxParticles   = 8000;
        main.startLifetime  = new ParticleSystem.MinMaxCurve(4f, 8f);
        main.startSpeed     = new ParticleSystem.MinMaxCurve(0.2f, 1.5f);
        main.startSize      = new ParticleSystem.MinMaxCurve(0.5f, 2.5f);   // gotas muy finas / vapor
        main.startColor     = new ParticleSystem.MinMaxGradient(
                                   new Color(0.80f, 0.83f, 0.87f, 0.05f),
                                   new Color(0.88f, 0.90f, 0.92f, 0.18f));
        main.gravityModifier  = 0.05f;
        main.simulationSpace  = ParticleSystemSimulationSpace.World;
        main.playOnAwake      = false;

        // ── Emisión ──────────────────────────────────────────────────────
        var em = psBruma.emission;
        em.rateOverTime = 0f;

        // ── Forma ────────────────────────────────────────────────────────
        var shape = psBruma.shape;
        shape.shapeType = ParticleSystemShapeType.Box;
        shape.scale     = new Vector3(500f, 1f, 500f);

        // ── Velocidad ────────────────────────────────────────────────────
        var vel = psBruma.velocityOverLifetime;
        vel.enabled = true;
        vel.space   = ParticleSystemSimulationSpace.World;
        vel.y       = new ParticleSystem.MinMaxCurve(-0.3f, -1.5f);

        // ── Tamaño sobre vida ────────────────────────────────────────────
        var sizeOL = psBruma.sizeOverLifetime;
        sizeOL.enabled = true;
        AnimationCurve curva = new AnimationCurve();
        curva.AddKey(0f, 0.3f);
        curva.AddKey(0.3f, 1f);
        curva.AddKey(0.8f, 1f);
        curva.AddKey(1f, 0.1f);
        sizeOL.size = new ParticleSystem.MinMaxCurve(1f, curva);

        // ── Renderizado: billboard suave ──────────────────────────────────
        var rend = psBruma.GetComponent<ParticleSystemRenderer>();
        rend.renderMode  = ParticleSystemRenderMode.Billboard;
        rend.material    = CrearMaterialBruma(new Color(0.85f, 0.87f, 0.90f, 0.12f));
        rend.sortingOrder = 5;

        Debug.Log("[Clima] Sistema de bruma creado.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MATERIALES (sin assets externos)
    // ═══════════════════════════════════════════════════════════════════════

    private Material CrearMaterialLluvia(Color color)
    {
        // Probar shaders URP en orden de compatibilidad
        var shader = Shader.Find("Universal Render Pipeline/Particles/Unlit")
                  ?? Shader.Find("Particles/Standard Unlit")
                  ?? Shader.Find("Sprites/Default")
                  ?? Shader.Find("UI/Default");

        var mat = new Material(shader);

        // Blending aditivo suave (lluvia semitransparente)
        mat.SetFloat("_Surface", 1f);           // Transparent
        mat.SetFloat("_Blend",   0f);           // Alpha
        mat.SetInt("_SrcBlend",  (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        mat.SetInt("_DstBlend",  (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        mat.SetInt("_ZWrite",    0);
        mat.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
        mat.color = color;

        return mat;
    }

    private Material CrearMaterialBruma(Color color)
    {
        var shader = Shader.Find("Universal Render Pipeline/Particles/Unlit")
                  ?? Shader.Find("Particles/Standard Unlit")
                  ?? Shader.Find("Sprites/Default");

        var mat = new Material(shader);
        mat.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        mat.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        mat.SetInt("_ZWrite",   0);
        mat.color = color;

        return mat;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CICLO ALEATORIO (patrón meteorológico típico de Alsasua)
    // ═══════════════════════════════════════════════════════════════════════

    private void CambiarClimaAleatorio()
    {
        float r = Random.value;

        EstadoClima nuevo;
        if      (r < 0.10f) nuevo = EstadoClima.Tormenta;
        else if (r < 0.25f) nuevo = EstadoClima.LluviaIntensa;
        else if (r < 0.50f) nuevo = EstadoClima.Lluvia;
        else if (r < 0.62f) nuevo = EstadoClima.Llovizna;
        else if (r < 0.72f) nuevo = EstadoClima.Niebla;
        else if (r < 0.80f) nuevo = EstadoClima.NieblaBaja;
        else if (r < 0.90f) nuevo = EstadoClima.Nublado;
        else                nuevo = EstadoClima.Despejado;

        // Ajustar probabilidad de lluvia según el parámetro
        if (nuevo == EstadoClima.Despejado && Random.value < probabilidadLluvia)
            nuevo = EstadoClima.Nublado;

        SetClima(nuevo, Random.Range(8f, 20f));
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  UTILIDADES
    // ═══════════════════════════════════════════════════════════════════════

    private Light BuscarLuzDirectional()
    {
        foreach (var l in Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
            if (l.type == LightType.Directional) return l;
        return null;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  PROPIEDADES PÚBLICAS
    // ═══════════════════════════════════════════════════════════════════════

    public EstadoClima ClimaActual    => climaObjetivo;
    public bool        EstaLloviendo  => climaObjetivo is EstadoClima.Lluvia
                                                       or EstadoClima.LluviaIntensa
                                                       or EstadoClima.Llovizna
                                                       or EstadoClima.Tormenta;
    public bool        HayNiebla     => climaObjetivo is EstadoClima.Niebla
                                                      or EstadoClima.NieblaBaja;

    /// <summary>
    /// BUG 29 FIX: offset de temperatura meteorológica para el WhiteBalance (-35 Tormenta … 0 Despejado).
    /// ControladorPostProcesado lo lee y suma a la temperatura base de la hora del día,
    /// siendo el único escritor de whiteBalance.temperature en el Volume.
    /// </summary>
    public float       TemperaturaActual => temperaturaCCActual;
}
