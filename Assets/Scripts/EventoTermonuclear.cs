// Assets/Scripts/EventoTermonuclear.cs
using UnityEngine;
using UnityEngine.UI;

[AddComponentMenu("Alsasua V8/Misil Termonuclear (Game Over)")]
public class EventoTermonuclear : MonoBehaviour
{
    [Header("Modelos 3D (Se autoasignan desde AutoSetup)")]
    public GameObject prefabMisil;
    public GameObject prefabHongo;

    private bool detonado = false;
    private float tiempoDestruccion = 0f;
    private GameObject misilObjeto;
    
    private void Update()
    {
        if (!detonado && Input.GetKeyDown(KeyCode.N))
        {
            IniciarApocalipsis();
        }

        if (detonado)
        {
            // Temblor de cámara masivo previo al impacto
            Camera.main.transform.position += new Vector3(Random.Range(-2f, 2f), Random.Range(-2f, 2f), Random.Range(-2f, 2f));

            if (misilObjeto != null)
            {
                misilObjeto.transform.Translate(Vector3.down * 400f * Time.deltaTime, Space.World);
                if (misilObjeto.transform.position.y <= 520f) // Nivel del suelo aproximado
                {
                    ExplotarMundo();
                }
            }
        }
    }

    private void IniciarApocalipsis()
    {
        detonado = true;
        
        // Sonido de sirena o caída (Placeholder usando explosiones de Unity)
        AudioManager.I?.Play(AudioManager.Clip.Explosion, Camera.main.transform.position);

        // Crear modelo de misil ICBM bajando desde la estratosfera
        if (prefabMisil != null)
        {
            misilObjeto = Instantiate(prefabMisil);
        }
        else
        {
            misilObjeto = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            misilObjeto.GetComponent<Renderer>().material.color = Color.black;
        }
        misilObjeto.name = "Misil_Termonuclear_V8";
        misilObjeto.transform.position = Camera.main.transform.position + Camera.main.transform.forward * 500f + Vector3.up * 3000f;
        misilObjeto.transform.localScale = new Vector3(10f, 50f, 10f);
    }

    private void ExplotarMundo()
    {
        Destroy(misilObjeto);
        
        // Efecto Ceguera y Onda Térmica (Manipulando Post-Procesado global y Skybox)
        RenderSettings.ambientLight = Color.red;
        RenderSettings.fogState = true;
        RenderSettings.fogColor = new Color(1f, 0.2f, 0.1f);
        RenderSettings.fogDensity = 0.05f;
        
        // Instanciar Hongo Nuclear (Partículas Múltiples)
        GenerarHongoNuclear(Camera.main.transform.position + Camera.main.transform.forward * 500f);

        // Gritos y estruendo
        for(int i=0; i<10; i++) AudioManager.I?.Play(AudioManager.Clip.Explosion, Camera.main.transform.position);

        // Disparar Game Over Text
        MostrarPantallaMuerte();

        // Congelar el tiempo para simular la muerte del motor
        Time.timeScale = 0.1f; // Slow motion infernal
        Destroy(this); // Detener el temblor
    }

    private void GenerarHongoNuclear(Vector3 centro)
    {
        if (prefabHongo != null)
        {
            Instantiate(prefabHongo, centro, Quaternion.identity);
            return;
        }

        // V11: SÍNTESIS PROCEDURAL DEL HONGO NUCLEAR Y ONDA EXPANSIVA (SIN ASSETS)
        GameObject raizNuke = new GameObject("Hongo_Nuclear_Procedural");
        raizNuke.transform.position = centro;

        // 1. Flash Cegador Hiper-Intenso
        GameObject flashObj = new GameObject("Ceguera_Atomica");
        flashObj.transform.SetParent(raizNuke.transform);
        flashObj.transform.localPosition = Vector3.zero;
        Light flashluz = flashObj.AddComponent<Light>();
        flashluz.type = LightType.Point;
        flashluz.color = new Color(1f, 0.9f, 0.8f);
        flashluz.range = 8000f;
        flashluz.intensity = 1000f; // Blanco absoluto
        
        // 2. Columna de Humo (Cilindro creciendo al cielo)
        GameObject columna = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        columna.transform.SetParent(raizNuke.transform);
        columna.transform.localPosition = Vector3.up * 200f;
        columna.transform.localScale = new Vector3(80f, 200f, 80f);
        columna.GetComponent<Renderer>().material.color = new Color(0.2f, 0.1f, 0.05f); // Tono fuego oscuro ceniza
        
        // 3. Sombrero del Hongo (Esfera Gorda arriba)
        GameObject sombrero = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        sombrero.transform.SetParent(raizNuke.transform);
        sombrero.transform.localPosition = Vector3.up * 500f;
        sombrero.transform.localScale = new Vector3(300f, 150f, 300f);
        sombrero.GetComponent<Renderer>().material.color = new Color(1f, 0.4f, 0f); // Naranja radiactivo

        // 4. Anillo Expansivo de Presión Magnética (Disco aplastado expandiéndose)
        GameObject ondaExpansiva = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        ondaExpansiva.transform.SetParent(raizNuke.transform);
        ondaExpansiva.transform.localPosition = Vector3.up * 10f;
        ondaExpansiva.transform.localScale = new Vector3(10f, 1f, 10f); // Aplastado
        ondaExpansiva.GetComponent<Renderer>().material.color = new Color(1f, 1f, 1f, 0.8f); // Blanco translúcido
        
        // Aislar físicas
        Destroy(columna.GetComponent<Collider>());
        Destroy(sombrero.GetComponent<Collider>());
        Destroy(ondaExpansiva.GetComponent<Collider>());

        // 5. Animador Matemático de la Nuke
        var animador = raizNuke.AddComponent<MecanicaHongoNuclear>();
        animador.flash = flashluz;
        animador.columna = columna.transform;
        animador.sombrero = sombrero.transform;
        animador.ondaExpansiva = ondaExpansiva.transform;

        // 6. Inyección de Partículas Complementarias Subyacentes
        ParticleSystem ps = raizNuke.AddComponent<ParticleSystem>();
        var main = ps.main;
        main.duration = 60f; main.startLifetime = 15f; main.startSpeed = 150f; main.startSize = 300f; 
        main.startColor = new Color(0.1f, 0.1f, 0.1f, 0.8f); // Ceniza cubriendo Alsasua
        main.maxParticles = 5000;
        var em = ps.emission; em.rateOverTime = 1200;
        var shape = ps.shape; shape.shapeType = ParticleSystemShapeType.Hemisphere; shape.radius = 400f;
    }

    private void MostrarPantallaMuerte()
    {
        // Crear Canvas 100% por código
        GameObject canvasGO = new GameObject("Canvas_GameOver_Nuclear");
        Canvas canvas = canvasGO.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        canvas.sortingOrder = 999;
        canvasGO.AddComponent<CanvasScaler>();
        canvasGO.AddComponent<GraphicRaycaster>();

        // Fondo Negro
        GameObject fondo = new GameObject("FondoNegro");
        fondo.transform.SetParent(canvasGO.transform, false);
        Image bg = fondo.AddComponent<Image>();
        bg.color = new Color(0, 0, 0, 0.85f);
        bg.rectTransform.anchorMin = Vector2.zero; bg.rectTransform.anchorMax = Vector2.one;
        bg.rectTransform.sizeDelta = Vector2.zero;

        // Texto Rojo Sangre
        GameObject textoGO = new GameObject("TextoDesolacion");
        textoGO.transform.SetParent(canvasGO.transform, false);
        Text txt = textoGO.AddComponent<Text>();
        
        // Try assigning a default font
        txt.font = Resources.GetBuiltinResource<Font>("Arial.ttf");
        if(txt.font == null) txt.font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");

        txt.text = "ALSASUA HA SIDO DESTRUIDA.\nSOSPECHAD DE LOS QUE HABLAN BIEN DE ALSASUA.";
        txt.fontSize = 50;
        txt.color = Color.red;
        txt.alignment = TextAnchor.MiddleCenter;
        txt.fontStyle = FontStyle.Bold;

        txt.rectTransform.anchorMin = Vector2.zero; txt.rectTransform.anchorMax = Vector2.one;
        txt.rectTransform.sizeDelta = Vector2.zero;
    }
}

public class MecanicaHongoNuclear : MonoBehaviour
{
    public Light flash;
    public Transform columna;
    public Transform sombrero;
    public Transform ondaExpansiva;
    private float tiempoNacimiento;

    void Start()
    {
        tiempoNacimiento = Time.time;
    }

    void Update()
    {
        float t = Time.time - tiempoNacimiento;

        // Flash se apaga drásticamente
        if (flash != null) flash.intensity = Mathf.Lerp(1000f, 0f, t / 4f);

        // La onda expansiva supersónica arrasa el nivel horizontalmente
        if (ondaExpansiva != null) ondaExpansiva.localScale += new Vector3(800f, 0f, 800f) * Time.deltaTime;

        // El hongo asciende y se ensancha
        if (sombrero != null)
        {
            sombrero.position += Vector3.up * 60f * Time.deltaTime;
            sombrero.localScale += new Vector3(40f, 10f, 40f) * Time.deltaTime;
        }

        // La columna crece con el hongo
        if (columna != null)
        {
            columna.localScale += new Vector3(20f, 30f, 20f) * Time.deltaTime;
            columna.position += Vector3.up * 30f * Time.deltaTime;
        }
    }
}
