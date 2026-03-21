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
        GenerarCallejonesOscuros(); // V14: Lore crudo de decadencia urbana
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
            perro.AddComponent<SistemaReaccionVital>(); // V12
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
            rata.AddComponent<SistemaReaccionVital>(); // V12
        }
    }

    private void GenerarCallejonesOscuros()
    {
        for (int i=0; i < 15; i++)
        {
            // Encontrar punto aleatorio para el callejón decadente
            Vector3 pos = new Vector3(Random.Range(-180f, 180f), 550f, Random.Range(-180f, 180f));
            if (Physics.Raycast(pos, Vector3.down, out RaycastHit hit, 1000f))
            {
                pos = hit.point + Vector3.up * 0.1f;
            }

            // Cuerpo del NPC Desplomado (Adicción/Decaimiento)
            GameObject adicto = EnsamblarPunkProcedural();
            adicto.name = "NPC_Decadente_Desplomado";
            adicto.transform.position = pos + Vector3.up * 0.5f;
            
            // Inmovilizamos y lo tumbamos en el suelo
            Destroy(adicto.GetComponent<SistemaReaccionVital>());
            Destroy(adicto.GetComponent<GravedadCalles>());
            adicto.transform.localRotation = Quaternion.Euler(0, 0, 90f); // Postura fetal colapsada

            // Parafernalia (Jeringuilla física y cucharas)
            GameObject jeringa = new GameObject("Props_Jeringuilla");
            jeringa.transform.position = pos + new Vector3(0.5f, 0, 0.5f);
            var cilindro = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            cilindro.transform.SetParent(jeringa.transform);
            cilindro.transform.localPosition = Vector3.zero;
            cilindro.transform.localScale = new Vector3(0.01f, 0.08f, 0.01f);
            cilindro.transform.localRotation = Quaternion.Euler(90, 45, 0);
            cilindro.GetComponent<Renderer>().material.color = new Color(0.8f, 0.9f, 0.9f, 0.5f); // Plástico translúcido
            Destroy(cilindro.GetComponent<Collider>());
            
            // Sangre seca y mugre
            GameObject charco = GameObject.CreatePrimitive(PrimitiveType.Quad);
            charco.name = "Mancha_SangreSeca_Callejon";
            charco.transform.position = pos + Vector3.up * 0.02f;
            charco.transform.rotation = Quaternion.Euler(90, 0, Random.Range(0, 360));
            charco.transform.localScale = Vector3.one * Random.Range(0.8f, 1.5f);
            charco.GetComponent<Renderer>().material.color = new Color(0.2f, 0.02f, 0.02f); 
            Destroy(charco.GetComponent<Collider>());

            // Ratas atraídas al cuerpo
            for(int r=0; r<2; r++)
            {
                GameObject rata = prefabRata != null ? Instantiate(prefabRata) : GameObject.CreatePrimitive(PrimitiveType.Sphere);
                rata.transform.position = pos + new Vector3(Random.Range(-1.5f, 1.5f), 0.5f, Random.Range(-1.5f, 1.5f));
                rata.transform.localScale = new Vector3(0.2f, 0.2f, 0.4f);
                rata.AddComponent<GravedadCalles>();
                rata.GetComponent<Renderer>().material.color = Color.black;
            }
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
        
        punk.AddComponent<SistemaReaccionVital>(); // V12
        punk.AddComponent<GravedadCalles>();

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
