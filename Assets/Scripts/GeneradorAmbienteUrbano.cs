// Assets/Scripts/GeneradorAmbienteUrbano.cs
using UnityEngine;
using System.Collections.Generic;

[AddComponentMenu("Alsasua V8/Generador de Ambiente Callejero")]
public class GeneradorAmbienteUrbano : MonoBehaviour
{
    [Header("Modelos 3D (Se autoasignan desde AutoSetup)")]
    public GameObject prefabPunk;
    public GameObject prefabPerro;
    public GameObject prefabRata;

    private void Start()
    {
        GenerarPunksYFauna();
    }

    private void GenerarPunksYFauna()
    {
        // Generar Punks (Procedural Mohawk Assembly) con Humo (Consumo)
        for(int i = 0; i < 15; i++)
        {
            GameObject punk = prefabPunk != null ? Instantiate(prefabPunk) : EnsamblarPunkProcedural();
            punk.name = "NPC_Punk_Procedural_" + i;
            punk.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            
            // Simulación de Humo (Sustancias)
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
            GameObject perro = prefabPerro != null ? Instantiate(prefabPerro) : GameObject.CreatePrimitive(PrimitiveType.Cube);
            perro.name = "Perro_Vagabundo_" + i;
            perro.transform.localScale = new Vector3(0.4f, 0.6f, 0.8f);
            perro.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            perro.GetComponent<Renderer>().material.color = new Color(0.4f, 0.3f, 0.2f); // Marrón sucio
            perro.AddComponent<GravedadCalles>();
            
            // V9: Inteligencia Artificial NavMesh en lugar de movimiento matemático
            var nav = perro.AddComponent<UnityEngine.AI.NavMeshAgent>();
            nav.speed = 3f;
            nav.radius = 0.3f;
            perro.AddComponent<NavegacionAnimalIA>();
        }

        // Generar Ratas de Alcantarilla
        for(int i = 0; i < 40; i++)
        {
            GameObject rata = prefabRata != null ? Instantiate(prefabRata) : GameObject.CreatePrimitive(PrimitiveType.Sphere);
            rata.name = "Rata_Alcantarilla_" + i;
            rata.transform.localScale = new Vector3(0.2f, 0.2f, 0.4f);
            rata.transform.position = new Vector3(Random.Range(-200f, 200f), 550f, Random.Range(-200f, 200f));
            rata.GetComponent<Renderer>().material.color = Color.black;
            rata.AddComponent<GravedadCalles>();
            
            var nav = rata.AddComponent<UnityEngine.AI.NavMeshAgent>();
            nav.speed = 7f; // Corren rápido
            nav.radius = 0.1f;
            rata.AddComponent<NavegacionAnimalIA>();
        }
    }

    private GameObject EnsamblarPunkProcedural()
    {
        GameObject punk = new GameObject("Estructura_Punk");
        
        // Cuerpo de Cuero Negro
        GameObject cuerpo = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        cuerpo.transform.SetParent(punk.transform);
        cuerpo.transform.localPosition = Vector3.up * 1f;
        cuerpo.GetComponent<Renderer>().material.color = new Color(0.1f, 0.1f, 0.1f);
        Destroy(cuerpo.GetComponent<Collider>()); // Quitamos colisionador interno

        // Cabeza
        GameObject cabeza = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        cabeza.transform.SetParent(punk.transform);
        cabeza.transform.localPosition = Vector3.up * 2.2f;
        cabeza.transform.localScale = Vector3.one * 0.8f;
        cabeza.GetComponent<Renderer>().material.color = new Color(0.9f, 0.8f, 0.7f);
        Destroy(cabeza.GetComponent<Collider>());

        // Cresta Punk (Mohawk Fucsia Intenso)
        for(int i = -3; i <= 3; i++)
        {
            GameObject pelo = GameObject.CreatePrimitive(PrimitiveType.Cube);
            pelo.transform.SetParent(cabeza.transform);
            pelo.transform.localPosition = new Vector3(0f, 0.45f, i * 0.12f);
            pelo.transform.localScale = new Vector3(0.1f, 0.5f - Mathf.Abs(i)*0.05f, 0.2f);
            pelo.transform.localRotation = Quaternion.Euler(i*12f, 0, 0);
            pelo.GetComponent<Renderer>().material.color = Color.magenta;
            Destroy(pelo.GetComponent<Collider>());
        }

        // Colisionador Maestro
        var col = punk.AddComponent<CapsuleCollider>();
        col.center = Vector3.up * 1f;
        col.height = 2f;
        
        return punk;
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

public class NavegacionAnimalIA : MonoBehaviour
{
    private UnityEngine.AI.NavMeshAgent agente;
    private float tiempoSiguienteRuta = 0f;

    private void Start()
    {
        agente = GetComponent<UnityEngine.AI.NavMeshAgent>();
    }

    private void Update()
    {
        if (agente == null || !agente.isOnNavMesh) return;

        if (Time.time > tiempoSiguienteRuta || agente.remainingDistance < 1f)
        {
            // Buscar un punto al azar en 40 metros
            Vector3 puntoAleatorio = transform.position + new Vector3(Random.Range(-40f, 40f), 0, Random.Range(-40f, 40f));
            if (UnityEngine.AI.NavMesh.SamplePosition(puntoAleatorio, out UnityEngine.AI.NavMeshHit hit, 40f, UnityEngine.AI.NavMesh.AllAreas))
            {
                agente.SetDestination(hit.position);
                tiempoSiguienteRuta = Time.time + Random.Range(5f, 15f);
            }
        }
    }
}
