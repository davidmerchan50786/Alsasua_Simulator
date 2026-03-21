// Assets/Scripts/SistemaClima.cs
using UnityEngine;
using System.Collections;

[AddComponentMenu("Alsasua/Simulador Climatico PBR")]
public class SistemaClima : MonoBehaviour
{
    public enum EstadoClima { Despejado, Lluvia, Nieve, TormentaOscura }
    public EstadoClima climaActual = EstadoClima.Despejado;

    private ParticleSystem particulasLluvia;
    private ParticleSystem particulasNieve;
    
    [Tooltip("Cota Z absoluta (altura nivel del mar Cesium) para inyectar el río Arakil proceduralmente.")]
    public float nivelDelRioY = 520f; 

    private Material matAguaActivo; // V23 FIX: Prevenir fuga de RAM

    private void Start()
    {
        CrearVenaFluvial();
        // V23 FIX: Atmósfera y Clima ya gestionados por ControladorClimatico (V16) y SistemaAtmosfera.
        // Se desactiva el duplicado para prevenir peleas en el Thread de RenderSettings por el control de la niebla.
        // InicializarAtmósfera();
        // StartCoroutine(CicloClimaticoProcedural());
    }

    private void OnDestroy()
    {
        // V23 FIX: Sellar memory leak severo de generacion de material procedimental
        if (matAguaActivo != null) Destroy(matAguaActivo);
    }

    private void CrearVenaFluvial()
    {
        // Gemelo Digital - Plano hídrico para simular el Río Alzania/Arakil
        GameObject rio = GameObject.CreatePrimitive(PrimitiveType.Plane);
        rio.name = "Río_Arakil_PBR";
        rio.transform.position = new Vector3(0, nivelDelRioY, 0);
        rio.transform.localScale = new Vector3(2000f, 1f, 2000f);
        
        Destroy(rio.GetComponent<Collider>()); // El agua no debe bloquear balas ni tráfico
        
        Renderer rend = rio.GetComponent<Renderer>();
        rend.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;

        // Inversión Shader Transparente al Vuelo
        Shader shaderStandard = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
        Material matAgua = new Material(shaderStandard);
        matAgua.color = new Color(0.05f, 0.25f, 0.35f, 0.75f); // Azul pantano / río sucio
        matAgua.SetFloat("_Smoothness", 0.98f);
        matAgua.SetFloat("_Glossiness", 0.98f);
        matAgua.SetFloat("_Metallic", 0.15f);
        
        // Setup PBR Transparente Estándar
        matAgua.SetFloat("_Mode", 3f); 
        matAgua.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
        matAgua.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
        matAgua.SetInt("_ZWrite", 0);
        matAgua.DisableKeyword("_ALPHATEST_ON");
        matAgua.EnableKeyword("_ALPHABLEND_ON");
        matAgua.DisableKeyword("_ALPHAPREMULTIPLY_ON");
        matAgua.renderQueue = 3000;
        
        matAguaActivo = matAgua;
        rend.sharedMaterial = matAgua;
    }

    private void InicializarAtmósfera()
    {
        var tl = new GameObject("FX_Lluvia_Masiva").transform;
        tl.SetParent(Camera.main != null ? Camera.main.transform : transform);
        tl.localPosition = new Vector3(0, 40f, 50f); 
        
        particulasLluvia = tl.gameObject.AddComponent<ParticleSystem>();
        var ml = particulasLluvia.main;
        ml.startLifetime = 3f; ml.startSpeed = 80f; ml.startSize = 0.15f; 
        ml.maxParticles = 8000; ml.gravityModifier = 3f;
        ml.startColor = new Color(0.8f, 0.8f, 0.9f, 0.4f);
        
        var el = particulasLluvia.emission; el.rateOverTime = 0f;
        var sl = particulasLluvia.shape; sl.shapeType = ParticleSystemShapeType.Box; sl.scale = new Vector3(150, 1, 150);
        
        tl.gameObject.AddComponent<ParticleSystemRenderer>().lengthScale = 12f; // Gotas estiradas por motion blur

        var tn = new GameObject("FX_Nieve_Nevisca").transform;
        tn.SetParent(Camera.main != null ? Camera.main.transform : transform);
        tn.localPosition = new Vector3(0, 30f, 40f);
        
        particulasNieve = tn.gameObject.AddComponent<ParticleSystem>();
        var mn = particulasNieve.main;
        mn.startLifetime = 8f; mn.startSpeed = 6f; mn.startSize = 0.45f; 
        mn.maxParticles = 5000; mn.gravityModifier = 0.15f;
        mn.startColor = new Color(0.9f, 0.95f, 1f, 0.8f);
        
        var en = particulasNieve.emission; en.rateOverTime = 0f;
        var sn = particulasNieve.shape; sn.shapeType = ParticleSystemShapeType.Box; sn.scale = new Vector3(100, 1, 100);
    }

    private IEnumerator CicloClimaticoProcedural()
    {
        RenderSettings.fog = true;
        RenderSettings.fogMode = FogMode.ExponentialSquared;

        while (true)
        {
            yield return new WaitForSeconds(180f); // Cambia el clima cada 3 min de simulación
            
            float rnd = Random.value;
            if (rnd < 0.4f) SetClima(EstadoClima.Despejado);
            else if (rnd < 0.7f) SetClima(EstadoClima.Lluvia);
            else if (rnd < 0.85f) SetClima(EstadoClima.TormentaOscura);
            else SetClima(EstadoClima.Nieve);
        }
    }

    private void SetClima(EstadoClima estado)
    {
        climaActual = estado;
        var el = particulasLluvia.emission;
        var en = particulasNieve.emission;

        switch (estado)
        {
            case EstadoClima.Despejado:
                el.rateOverTime = 0; en.rateOverTime = 0;
                RenderSettings.fogDensity = 0.001f;
                RenderSettings.fogColor = new Color(0.7f, 0.8f, 0.9f); // Cielo azul polvo
                break;
            case EstadoClima.Lluvia:
                el.rateOverTime = 3000; en.rateOverTime = 0;
                RenderSettings.fogDensity = 0.015f;
                RenderSettings.fogColor = new Color(0.4f, 0.45f, 0.5f);
                break;
            case EstadoClima.Nieve:
                el.rateOverTime = 0; en.rateOverTime = 1500;
                RenderSettings.fogDensity = 0.035f;
                RenderSettings.fogColor = new Color(0.85f, 0.9f, 0.95f);
                break;
            case EstadoClima.TormentaOscura:
                el.rateOverTime = 6000; en.rateOverTime = 0;
                RenderSettings.fogDensity = 0.05f; // Visibilidad Nula
                RenderSettings.fogColor = new Color(0.12f, 0.14f, 0.16f);
                break;
        }
    }
}
