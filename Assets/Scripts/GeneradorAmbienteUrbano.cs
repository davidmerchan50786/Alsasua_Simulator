// Assets/Scripts/GeneradorAmbienteUrbano.cs
using UnityEngine;
using System.Collections.Generic;

[AddComponentMenu("Alsasua V8/Generador de Ambiente Callejero")]
public class GeneradorAmbienteUrbano : MonoBehaviour
{
    private void Start()
    {
        GenerarPunksYFauna();
    }

    private void GenerarPunksYFauna()
    {
        // Generar Punks (Izquierda Abertzale Aesthetic Placeholder) con Humo (Consumo)
        for(int i = 0; i < 15; i++)
        {
            GameObject punk = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            punk.name = "NPC_Punk_Abertzale_" + i;
            punk.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            punk.GetComponent<Renderer>().material.color = new Color(0.2f, 0f, 0f); // Rojinegro oscuro
            
            // Simulación de Humo (Porro/Sustancias)
            GameObject humo = new GameObject("Humo_Sustancia");
            humo.transform.SetParent(punk.transform);
            humo.transform.localPosition = new Vector3(0, 0.8f, 0.5f);
            ParticleSystem ps = humo.AddComponent<ParticleSystem>();
            var main = ps.main;
            main.startLifetime = 2f; main.startSpeed = 1f; main.startSize = 0.2f; main.startColor = new Color(0.8f, 0.8f, 0.8f, 0.5f);
            var shape = ps.shape; shape.shapeType = ParticleSystemShapeType.Cone; shape.angle = 10f;

            punk.AddComponent<GravedadCalles>();
        }

        // Generar Perros Callejeros
        for(int i = 0; i < 20; i++)
        {
            GameObject perro = GameObject.CreatePrimitive(PrimitiveType.Cube); // Cube placeholder
            perro.name = "Perro_Vagabundo_" + i;
            perro.transform.localScale = new Vector3(0.4f, 0.6f, 0.8f);
            perro.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            perro.GetComponent<Renderer>().material.color = new Color(0.4f, 0.3f, 0.2f); // Marrón sucio
            perro.AddComponent<GravedadCalles>();
            perro.AddComponent<MovimientoErratico>().velocidad = 3f;
        }

        // Generar Ratas de Alcantarilla
        for(int i = 0; i < 40; i++)
        {
            GameObject rata = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            rata.name = "Rata_Alcantarilla_" + i;
            rata.transform.localScale = new Vector3(0.2f, 0.2f, 0.4f);
            rata.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            rata.GetComponent<Renderer>().material.color = Color.black;
            rata.AddComponent<GravedadCalles>();
            rata.AddComponent<MovimientoErratico>().velocidad = 7f; // Corren rápido
        }
    }
}

public class GravedadCalles : MonoBehaviour
{
    private void Update()
    {
        if (Physics.Raycast(transform.position + Vector3.up * 50f, Vector3.down, out RaycastHit hit, 2000f))
        {
            transform.position = hit.point + Vector3.up * (transform.localScale.y / 2f);
            Destroy(this); // Anclado permanente
        }
    }
}

public class MovimientoErratico : MonoBehaviour
{
    public float velocidad = 5f;
    private Vector3 direccion;
    private float tiempoCambio = 0f;

    private void Update()
    {
        if (Time.time > tiempoCambio)
        {
            direccion = new Vector3(Random.Range(-1f, 1f), 0, Random.Range(-1f, 1f)).normalized;
            tiempoCambio = Time.time + Random.Range(1f, 4f);
        }
        
        transform.Translate(direccion * velocidad * Time.deltaTime, Space.World);
        if (direccion != Vector3.zero)
            transform.rotation = Quaternion.Slerp(transform.rotation, Quaternion.LookRotation(direccion), 10f * Time.deltaTime);
    }
}
