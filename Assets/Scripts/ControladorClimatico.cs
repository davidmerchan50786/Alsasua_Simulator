// Assets/Scripts/ControladorClimatico.cs
using UnityEngine;

/// <summary>
/// V16: Estación Atmosférica y Clima Realista.
/// Responsabilidad Única: Orquestar la inmersión visual de lluvia, niebla y relámpagos sin bloquear la CPU.
/// </summary>
[AddComponentMenu("Alsasua V16/Estación Climática Dinámica")]
public class ControladorClimatico : MonoBehaviour
{
    private ParticleSystem lluviaPS;
    private AudioSource sonidoLluvia;
    private Light luzSolar;

    [Header("Ajustes Atmosféricos")]
    [Range(0f, 1f)] public float intensidadTormenta = 0f;
    private float targetIntensidad = 0.8f; // Por defecto buscará arrancar una tormenta fuerte
    
    // Parámetros de Truenos y Luz
    private AudioSource sonidoTrueno;
    private float tiempoProximoTrueno = 12f;
    private float tiempoEsperaSonidoTrueno = -1f;
    private float intensidadBaseLuz = 1.2f;

    // Parámetros de Niebla
    private float densidadNieblaDespejado = 0.001f;
    private float densidadNieblaTormenta = 0.015f;
    private Color colorNieblaTormenta = new Color(0.15f, 0.18f, 0.22f);

    private void Start()
    {
        ConstruirCieloYNiebla();
        ConstruirMotorLluviaProcedural();
        ConstruirAudioAtmosferico();
        
        // V16 Búsqueda Dinámica de la Luz Principal del Mundo
        var luces = Object.FindObjectsByType<Light>(FindObjectsSortMode.None);
        foreach(var l in luces) {
            if (l.type == LightType.Directional) {
                luzSolar = l;
                break;
            }
        }
    }

    private void ConstruirCieloYNiebla()
    {
        RenderSettings.fog = true;
        RenderSettings.fogMode = FogMode.ExponentialSquared;
        RenderSettings.fogDensity = densidadNieblaDespejado;
    }

    private void ConstruirMotorLluviaProcedural()
    {
        GameObject goLluvia = new GameObject("VFX_Lluvia_Masiva");
        goLluvia.transform.SetParent(this.transform);
        goLluvia.transform.localPosition = new Vector3(0, 50f, 0); // Emitir desde las nubes

        lluviaPS = goLluvia.AddComponent<ParticleSystem>();
        
        // Core Limits
        var main = lluviaPS.main;
        main.duration = 5f;
        main.loop = true;
        main.startLifetime = 3f;
        main.startSpeed = 60f; // Gravedad terrestre extrema acelerando gotas
        main.startSize = 0.15f;
        main.startColor = new Color(0.6f, 0.7f, 0.8f, 0.5f); // Translúcido azulado grisáceo
        main.gravityModifier = 1.5f; // Caen muy rápido
        main.maxParticles = 5000;
        main.simulationSpace = ParticleSystemSimulationSpace.World; // La lluvia pertenece al mundo, no al emisor

        // Emission volume
        var em = lluviaPS.emission;
        em.rateOverTime = 0f; // Se controla dinaámicamente en Update()

        // Shape (Huge Box over the city)
        var shape = lluviaPS.shape;
        shape.shapeType = ParticleSystemShapeType.Box;
        shape.scale = new Vector3(400f, 1f, 400f); // Cubre una manzana entera

        // Collision logic (Splashes against the asphalt)
        var collision = lluviaPS.collision;
        collision.enabled = true;
        collision.type = ParticleSystemCollisionType.World;
        collision.mode = ParticleSystemCollisionMode.Collision3D;
        collision.bounce = 0.1f;
        collision.lifetimeLoss = 1f; // Mueren al impactar
        collision.quality = ParticleSystemCollisionQuality.Medium;

        // Renderer (Stretched Billboard imitates rain trails physically)
        var rend = lluviaPS.GetComponent<ParticleSystemRenderer>();
        rend.renderMode = ParticleSystemRenderMode.Stretch;
        rend.cameraVelocityScale = 0f;
        rend.velocityScale = 0.05f; // Se estiran según lo rápido que caen
        rend.lengthScale = 2f;
        
        Material matLluvia = new Material(Shader.Find("Hidden/Internal-Flare")); // Default ligero
        rend.sharedMaterial = matLluvia;

        lluviaPS.Play();
    }

    private void ConstruirAudioAtmosferico()
    {
        sonidoLluvia = gameObject.AddComponent<AudioSource>();
        sonidoLluvia.loop = true;
        sonidoLluvia.volume = 0f;
        sonidoLluvia.spatialBlend = 0f; // 2D ambiental estéreo
        
        sonidoTrueno = gameObject.AddComponent<AudioSource>();
        sonidoTrueno.playOnAwake = false;
    }

    private void Update()
    {
        // 1. Interpolación de Intensidad Orgánica (Transición de clima suave a tormenta)
        intensidadTormenta = Mathf.Lerp(intensidadTormenta, targetIntensidad, Time.deltaTime * 0.1f);

        // 2. Aplicar Densidad de Niebla Lúgubre
        RenderSettings.fogDensity = Mathf.Lerp(densidadNieblaDespejado, densidadNieblaTormenta, intensidadTormenta);
        RenderSettings.fogColor = Color.Lerp(RenderSettings.ambientSkyColor, colorNieblaTormenta, intensidadTormenta);

        // 3. Ciclo Noche/Día y Lluvia
        if (lluviaPS != null)
        {
            var em = lluviaPS.emission;
            em.rateOverTime = 3000f * intensidadTormenta;

            if (intensidadTormenta > 0.5f) {
                var f = lluviaPS.forceOverLifetime;
                f.enabled = true;
                f.x = new ParticleSystem.MinMaxCurve(Mathf.Sin(Time.time * 0.5f) * 15f); // Viento procedural
            }
        }

        if (luzSolar != null)
        {
            // V16: Ciclo Noche/Día Acelerado (Rotación). ~10 minutos reales = día completo. (0.6 grados por segundo cruzado)
            luzSolar.transform.Rotate(Vector3.right * Time.deltaTime * 0.6f);

            // Recuperación suave del brillo solar base vs Tormenta
            intensidadBaseLuz = Mathf.Lerp(intensidadBaseLuz, Mathf.Lerp(1.2f, 0.3f, intensidadTormenta), Time.deltaTime * 2f);
            
            // Si no estamos en un pico de relámpago, establecemos la intensidad base
            if (luzSolar.intensity <= intensidadBaseLuz + 0.1f) 
            {
                luzSolar.intensity = intensidadBaseLuz;
            }
            else 
            {
                // Atenuar el pico cegador del relámpago drásticamente
                luzSolar.intensity = Mathf.Lerp(luzSolar.intensity, intensidadBaseLuz, Time.deltaTime * 15f);
            }
            
            luzSolar.color = Color.Lerp(Color.white, new Color(0.4f, 0.5f, 0.6f), intensidadTormenta);
        }

        // 4. Mecánica de Truenos Retrasados (Velocidad del Sonido)
        if (intensidadTormenta > 0.7f)
        {
            tiempoProximoTrueno -= Time.deltaTime;
            if (tiempoProximoTrueno <= 0)
            {
                DesatarRelampagoLuminico();
                tiempoProximoTrueno = Random.Range(15f, 40f);
            }
        }

        if (tiempoEsperaSonidoTrueno > 0)
        {
            tiempoEsperaSonidoTrueno -= Time.deltaTime;
            if (tiempoEsperaSonidoTrueno <= 0)
            {
                SintetizadorAudioProcedural.PlayTrueno(sonidoTrueno);
            }
        }
    }

    private void DesatarRelampagoLuminico()
    {
        if (luzSolar != null)
        {
            luzSolar.intensity = 8f; // Flash cegador de un frame
            
            // Físicas Acústicas: La velocidad del sonido es ~343 m/s. 
            // Fingimos una tormenta cayendo a 1,5 - 4 kilómetros de la ciudad.
            float distanciaFicticia = Random.Range(500f, 3500f);
            tiempoEsperaSonidoTrueno = distanciaFicticia / 343f; // Delay realista

            Debug.Log($"[V16 Clima] Relámpago detectado a {distanciaFicticia} metros. Trueno llegará en {tiempoEsperaSonidoTrueno:F1}s.");
        }
    }

    /// <summary>
    /// API Pública para forzar el clima (Ej: Activar en cierto punto de la historia)
    /// </summary>
    public void DesatarTormenta() => targetIntensidad = 1f;
    public void DespejarCielos() => targetIntensidad = 0f;
}
