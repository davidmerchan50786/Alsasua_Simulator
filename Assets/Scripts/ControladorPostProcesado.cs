using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

/// <summary>
/// Controlador de Post-Procesado para Alsasua Simulator.
/// Aplica efectos visuales URP que hacen que las fachadas y el entorno
/// se vean más fotorrealistas, simulando la atmósfera del País Vasco.
///
/// INSTRUCCIONES:
/// 1. Añade este script a la cámara principal
/// 2. Añade un componente "Volume" (Global) a un GameObject en la escena
/// 3. Asigna el Volume al campo "volumenGlobal" del Inspector
/// 4. El script ajusta automáticamente los efectos según la hora del día
/// </summary>
[RequireComponent(typeof(Camera))]
public class ControladorPostProcesado : MonoBehaviour
{
    // ============================================================
    //  REFERENCIAS
    // ============================================================
    [Header("═══ VOLUME URP ═══")]
    [Tooltip("GameObject con el componente Volume Global de la escena")]
    [SerializeField] private Volume volumenGlobal;

    // ============================================================
    //  CONFIGURACIÓN VISUAL
    // ============================================================
    [Header("═══ WHITE BALANCE ═══")]
    [Tooltip("Temperatura de color — positivo = cálido/amarillo, negativo = frío/azulado (clima norteño)")]
    [Range(-100f, 100f)]
    [SerializeField] private float temperaturaColor = -15f;   // Ligeramente frío

    [Tooltip("Tinte (Tint) del balance de blancos")]
    [Range(-100f, 100f)]
    [SerializeField] private float tintColor = 0f;

    [Header("═══ COLOR GRADING ═══")]
    [Tooltip("Saturación del color — 100% = colores naturales")]
    [Range(0f, 200f)]
    [SerializeField] private float saturacion = 85f;

    [Tooltip("Contraste de la imagen")]
    [Range(-100f, 100f)]
    [SerializeField] private float contraste = 10f;

    [Header("═══ NIEBLA ATMOSFÉRICA ═══")]
    [Tooltip("Activar niebla volumétrica (característica del clima de Alsasua)")]
    [SerializeField] private bool activarNiebla = true;

    [Tooltip("Color de la niebla atmosférica")]
    [SerializeField] private Color colorNiebla = new Color(0.75f, 0.8f, 0.85f, 1f);

    [Tooltip("Densidad de la niebla")]
    [Range(0f, 0.001f)]
    [SerializeField] private float densidadNiebla = 0.00015f;

    [Tooltip("Distancia a la que empieza la niebla (metros)")]
    [SerializeField] private float inicioNiebla = 2000f;

    [Tooltip("Distancia máxima de la niebla (metros)")]
    [SerializeField] private float finNiebla = 25000f;

    [Header("═══ ANTI-ALIASING ═══")]
    [Tooltip("Modo de anti-aliasing para suavizar los bordes de las fachadas")]
    [SerializeField] private AntialiasingMode modoAntialiasing = AntialiasingMode.SubpixelMorphologicalAntiAliasing;

    [Header("═══ CICLO HORARIO ═══")]
    [Tooltip("Simular cambios de luz según hora del día")]
    [SerializeField] private bool simularHoraDia = false;

    [Range(0f, 24f)]
    [Tooltip("Hora actual (0-24)")]
    [SerializeField] private float horaActual = 12f;

    // Referencias a efectos de post-procesado
    private ColorAdjustments colorAdjustments;
    private WhiteBalance      whiteBalance;
    private Bloom             bloom;
    private Camera            camaraComponent;
    private UniversalAdditionalCameraData cameraData;

    // ============================================================
    //  INICIALIZACIÓN
    // ============================================================

    private void Awake()
    {
        camaraComponent = GetComponent<Camera>();
        cameraData      = camaraComponent.GetUniversalAdditionalCameraData();
    }

    private void Start()
    {
        ConfigurarAntiAliasing();
        ConfigurarNiebla();

        if (volumenGlobal != null)
        {
            ObtenerEfectosDelVolumen();
            AplicarWhiteBalance();
            AplicarColorGrading();
            AplicarBloom();
        }
        else
        {
            Debug.LogWarning("[PostProcesado] No se asignó un Volume Global. " +
                             "Crea un GameObject con componente 'Volume' (modo Global) y asígnalo al script.");
        }
    }

    private void Update()
    {
        if (simularHoraDia)
        {
            // 1 segundo real = 6 minutos en juego (ciclo de 4 minutos reales)
            horaActual = (horaActual + Time.deltaTime * 0.1f) % 24f;
            ActualizarLuzSegunHora();
        }
    }

    // ============================================================
    //  CONFIGURACIÓN DE EFECTOS
    // ============================================================

    private void ConfigurarAntiAliasing()
    {
        if (cameraData != null)
        {
            cameraData.antialiasing        = modoAntialiasing;
            cameraData.antialiasingQuality = AntialiasingQuality.High;
            Debug.Log($"[PostProcesado] Anti-aliasing: {modoAntialiasing}");
        }
    }

    private void ConfigurarNiebla()
    {
        if (!activarNiebla)
        {
            RenderSettings.fog = false;
            return;
        }

        RenderSettings.fog              = true;
        RenderSettings.fogMode          = FogMode.Linear;
        RenderSettings.fogColor         = colorNiebla;
        RenderSettings.fogStartDistance = inicioNiebla;
        RenderSettings.fogEndDistance   = finNiebla;
        RenderSettings.fogDensity       = densidadNiebla;

        Debug.Log($"[PostProcesado] Niebla atmosférica configurada ({inicioNiebla}m - {finNiebla}m).");
    }

    private void ObtenerEfectosDelVolumen()
    {
        volumenGlobal.profile.TryGet(out colorAdjustments);
        volumenGlobal.profile.TryGet(out whiteBalance);
        volumenGlobal.profile.TryGet(out bloom);

        if (colorAdjustments == null)
        {
            colorAdjustments = volumenGlobal.profile.Add<ColorAdjustments>(true);
            Debug.Log("[PostProcesado] ColorAdjustments añadido al perfil.");
        }

        if (whiteBalance == null)
        {
            whiteBalance = volumenGlobal.profile.Add<WhiteBalance>(true);
            Debug.Log("[PostProcesado] WhiteBalance añadido al perfil.");
        }

        if (bloom == null)
        {
            bloom = volumenGlobal.profile.Add<Bloom>(true);
            Debug.Log("[PostProcesado] Bloom añadido al perfil.");
        }
    }

    /// <summary>
    /// Aplica temperatura de color usando el efecto WhiteBalance de URP.
    /// WhiteBalance.temperature: -100 (frío/azul) a +100 (cálido/amarillo).
    /// </summary>
    private void AplicarWhiteBalance()
    {
        if (whiteBalance == null) return;

        whiteBalance.active = true;

        whiteBalance.temperature.value         = temperaturaColor;
        whiteBalance.temperature.overrideState = true;

        whiteBalance.tint.value         = tintColor;
        whiteBalance.tint.overrideState = true;

        Debug.Log($"[PostProcesado] WhiteBalance aplicado (temp: {temperaturaColor}, tint: {tintColor}).");
    }

    private void AplicarColorGrading()
    {
        if (colorAdjustments == null) return;

        colorAdjustments.active = true;

        // colorFilter = Color.white significa sin tinte adicional
        colorAdjustments.colorFilter.value         = Color.white;
        colorAdjustments.colorFilter.overrideState = true;

        // Saturación: en URP va de -100 a +100; el inspector muestra 0-200 para facilidad
        colorAdjustments.saturation.value         = saturacion - 100f;
        colorAdjustments.saturation.overrideState = true;

        colorAdjustments.contrast.value         = contraste;
        colorAdjustments.contrast.overrideState = true;

        Debug.Log("[PostProcesado] Color grading aplicado.");
    }

    private void AplicarBloom()
    {
        if (bloom == null) return;

        bloom.active = true;
        bloom.threshold.value = 0.9f;   // Solo objetos muy brillantes (ventanas iluminadas)
        bloom.intensity.value = 0.3f;   // Sutil
        bloom.scatter.value   = 0.7f;

        bloom.threshold.overrideState = true;
        bloom.intensity.overrideState = true;
        bloom.scatter.overrideState   = true;

        Debug.Log("[PostProcesado] Bloom configurado.");
    }

    // ============================================================
    //  CICLO DÍA / NOCHE
    // ============================================================

    private void ActualizarLuzSegunHora()
    {
        if (whiteBalance == null) return;

        // Temperatura según hora:
        // Amanecer (6-9h): cálido naranja → blanco
        // Día (9-17h): neutro a ligeramente frío
        // Atardecer (17-21h): cálido → frío
        // Noche (21-6h): azul frío

        float tempHora;

        if (horaActual >= 6f && horaActual < 9f)
        {
            float t = (horaActual - 6f) / 3f;
            tempHora = Mathf.Lerp(50f, 10f, t);         // Naranja → blanco cálido
        }
        else if (horaActual >= 9f && horaActual < 17f)
        {
            float t = (horaActual - 9f) / 8f;
            tempHora = Mathf.Lerp(10f, -15f, t);        // Blanco → ligeramente frío
        }
        else if (horaActual >= 17f && horaActual < 21f)
        {
            float t = (horaActual - 17f) / 4f;
            tempHora = Mathf.Lerp(-15f, -50f, t);       // Atardecer → noche fría
        }
        else
        {
            tempHora = -50f;                              // Noche: azul frío
        }

        whiteBalance.temperature.value = tempHora;
    }

    // ============================================================
    //  INSPECTOR HELPERS
    // ============================================================

    [ContextMenu("Aplicar configuración actual")]
    public void AplicarConfiguracion()
    {
        ConfigurarNiebla();
        if (volumenGlobal != null)
        {
            ObtenerEfectosDelVolumen();
            AplicarWhiteBalance();
            AplicarColorGrading();
            AplicarBloom();
        }
        Debug.Log("[PostProcesado] Configuración aplicada.");
    }

    [ContextMenu("Preset: Día nublado (típico Alsasua)")]
    private void PresetDiaNublado()
    {
        temperaturaColor = -20f;
        tintColor        = 5f;
        saturacion       = 75f;
        contraste        = 8f;
        activarNiebla    = true;
        densidadNiebla   = 0.0002f;
        inicioNiebla     = 1500f;
        finNiebla        = 15000f;
        colorNiebla      = new Color(0.7f, 0.75f, 0.8f);
        AplicarConfiguracion();
        Debug.Log("[PostProcesado] Preset 'Día nublado' aplicado.");
    }

    [ContextMenu("Preset: Día soleado (verano)")]
    private void PresetDiaSoleado()
    {
        temperaturaColor = 15f;
        tintColor        = 0f;
        saturacion       = 95f;
        contraste        = 15f;
        activarNiebla    = true;
        densidadNiebla   = 0.00008f;
        inicioNiebla     = 5000f;
        finNiebla        = 40000f;
        colorNiebla      = new Color(0.85f, 0.9f, 0.95f);
        AplicarConfiguracion();
        Debug.Log("[PostProcesado] Preset 'Día soleado' aplicado.");
    }

    [ContextMenu("Preset: Amanecer")]
    private void PresetAmanecer()
    {
        temperaturaColor = 50f;
        tintColor        = 10f;
        saturacion       = 90f;
        contraste        = 12f;
        activarNiebla    = true;
        densidadNiebla   = 0.0003f;
        inicioNiebla     = 500f;
        finNiebla        = 8000f;
        colorNiebla      = new Color(0.95f, 0.75f, 0.6f);
        AplicarConfiguracion();
        Debug.Log("[PostProcesado] Preset 'Amanecer' aplicado.");
    }
}
