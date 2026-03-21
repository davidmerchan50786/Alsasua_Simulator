// Assets/Scripts/EventoTermonuclear.cs
using UnityEngine;
using UnityEngine.UI;

[AddComponentMenu("Alsasua V8/Misil Termonuclear (Game Over)")]
public class EventoTermonuclear : MonoBehaviour
{
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
        misilObjeto = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        misilObjeto.name = "Misil_Termonuclear_V8";
        misilObjeto.transform.position = Camera.main.transform.position + Camera.main.transform.forward * 500f + Vector3.up * 3000f;
        misilObjeto.transform.localScale = new Vector3(10f, 50f, 10f);
        misilObjeto.GetComponent<Renderer>().material.color = Color.black;
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
        GameObject hongo = new GameObject("Hongo_Nuclear");
        hongo.transform.position = centro;
        ParticleSystem ps = hongo.AddComponent<ParticleSystem>();
        
        var main = ps.main;
        main.duration = 60f; main.startLifetime = 10f; main.startSpeed = 80f; main.startSize = 200f; 
        main.startColor = new Color(1f, 0.3f, 0f, 0.8f);
        main.maxParticles = 5000;
        
        var em = ps.emission; em.rateOverTime = 800;
        var shape = ps.shape; shape.shapeType = ParticleSystemShapeType.Hemisphere; shape.radius = 300f;
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
