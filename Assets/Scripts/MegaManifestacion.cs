// Assets/Scripts/MegaManifestacion.cs
using UnityEngine;
using UnityEngine.AI;
using System.Collections.Generic;

/// <summary>
/// Motor de Macro-Manifestaciones Optimizado (V14).
/// Arquitectura basada en Patrón Flocking Simplificado (Líder-Seguidor).
/// Utiliza Instanciación en Tarjeta Gráfica (GPU Instancing) para evadir los límites de Draw-Calls de Unity.
/// </summary>
[AddComponentMenu("Alsasua V14/Motor de Macro-Manifestaciones (1000 NPCs)")]
public class MegaManifestacion : MonoBehaviour
{
    private const int TOTAL_MANIFESTANTES = 1000;
    private const int NUM_LIDERES = 20;

    // Listas internas cacheadas para prevenir el Garbage Collection (GC Allocation = 0)
    private readonly List<Transform> lideres = new List<Transform>();
    private readonly Transform[] manifestantes = new Transform[TOTAL_MANIFESTANTES - NUM_LIDERES];
    private readonly float[] offsetsX = new float[TOTAL_MANIFESTANTES - NUM_LIDERES];
    private readonly float[] offsetsZ = new float[TOTAL_MANIFESTANTES - NUM_LIDERES];
    private readonly int[] liderAsignado = new int[TOTAL_MANIFESTANTES - NUM_LIDERES];

    private Material matCueroNegro;
    private Material matCrestaRosa;
    private Material matPiel;

    void Start()
    {
        // 1. Optimización GPU (GPU Instancing para 1000 modelos)
        matCueroNegro = new Material(Shader.Find("Standard"));
        matCueroNegro.color = new Color(0.1f, 0.1f, 0.1f);
        matCueroNegro.enableInstancing = true;

        matPiel = new Material(Shader.Find("Standard"));
        matPiel.color = new Color(0.9f, 0.8f, 0.7f);
        matPiel.enableInstancing = true;

        matCrestaRosa = new Material(Shader.Find("Standard"));
        matCrestaRosa.color = Color.magenta;
        matCrestaRosa.enableInstancing = true;

        GenerarManifestacion();
    }

    /// <summary>
    /// Genera la manifestación estructurada.
    /// Crea un subconjunto de Líderes (NavMesh) y un Enjambre masivo (Followers Matemáticos).
    /// </summary>
    private void GenerarManifestacion()
    {
        Vector3 epicentroInicial = transform.position;

        // Proyección Topográfica: Buscar colisión contra el asfalto empleando Raycast O(1)
        if (Physics.Raycast(epicentroInicial + Vector3.up * 500f, Vector3.down, out RaycastHit hitFloor, 1000f))
        {
            epicentroInicial = hitFloor.point;
        }

        // Crear Líderes (NavMeshAgents Reales)
        for (int i = 0; i < NUM_LIDERES; i++)
        {
            GameObject lider = EnsamblarClonPunk(true);
            lider.name = "Lider_Manifestacion_" + i;
            Vector3 spawn = epicentroIncial + new Vector3(Random.Range(-30f, 30f), 0, Random.Range(-30f, 30f));
            lider.transform.position = spawn;

            var nav = lider.AddComponent<NavMeshAgent>();
            nav.speed = 1.5f; // Marcha lenta
            nav.radius = 0.5f;
            lider.AddComponent<LiderRutaIA>();
            lideres.Add(lider.transform);

            // 50% probabilidad de llevar pancarta generada proceduralmente
            if (Random.value > 0.5f) CrearPancarta(lider.transform);
        }

        // Crear el Enjambre Seguidor
        // Coste Computacional: Zero-CPU para Pathfinding, se limitarán a interpelaciones Lerp espaciales
        for (int i = 0; i < manifestantes.Length; i++)
        {
            GameObject clon = EnsamblarClonPunk(false);
            clon.name = "Manifestante_" + i;
            clon.transform.position = epicentroInicial + new Vector3(Random.Range(-50f, 50f), 0, Random.Range(-50f, 50f));
            manifestantes[i] = clon.transform;

            liderAsignado[i] = Random.Range(0, NUM_LIDERES);
            offsetsX[i] = Random.Range(-10f, 10f);
            offsetsZ[i] = Random.Range(-20f, -2f); // Siempre detrás del líder

            // 15% probabilidad de llevar bandera
            if (Random.value > 0.85f) CrearBandera(clon.transform);
        }

        Debug.Log("[V14 Macro-Turba] 1000 Punks generados en manifestación.");
        
        // Audio ambiental de turba gritando
        AudioSource audioTurba = gameObject.AddComponent<AudioSource>();
        audioTurba.spatialBlend = 1f;
        audioTurba.minDistance = 50f;
        audioTurba.maxDistance = 800f;
        audioTurba.loop = true;
        // Simulador procedural de ruido de masa
        SintetizadorAudioProcedural.PlayEstaticaTurba(audioTurba); 
    }

    private GameObject EnsamblarClonPunk(bool esLider)
    {
        GameObject punk = new GameObject("Cuerpo_Manifestante");
        
        GameObject cuerpo = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        cuerpo.transform.SetParent(punk.transform);
        cuerpo.transform.localPosition = Vector3.up * 1f;
        cuerpo.GetComponent<Renderer>().sharedMaterial = matCueroNegro;
        Destroy(cuerpo.GetComponent<Collider>()); // Colisiones desactivadas en seguidores para max FPS

        GameObject cabeza = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        cabeza.transform.SetParent(punk.transform);
        cabeza.transform.localPosition = Vector3.up * 2.2f;
        cabeza.transform.localScale = Vector3.one * 0.8f;
        cabeza.GetComponent<Renderer>().sharedMaterial = matPiel;
        Destroy(cabeza.GetComponent<Collider>());

        // Cresta Punk Low-Poly (solo 1 cubo central para ahorrar vértices x1000)
        GameObject pelo = GameObject.CreatePrimitive(PrimitiveType.Cube);
        pelo.transform.SetParent(cabeza.transform);
        pelo.transform.localPosition = new Vector3(0f, 0.45f, 0f);
        pelo.transform.localScale = new Vector3(0.1f, 0.4f, 0.6f);
        pelo.GetComponent<Renderer>().sharedMaterial = matCrestaRosa;
        Destroy(pelo.GetComponent<Collider>());

        if (esLider)
        {
            var col = punk.AddComponent<CapsuleCollider>();
            col.center = Vector3.up * 1f;
            col.height = 2f;
            punk.AddComponent<SistemaReaccionVital>(); // Solo los líderes reaccionan al gore por optimización masiva RAM
        }
        
        return punk;
    }

    private void CrearBandera(Transform portador)
    {
        GameObject bandera = new GameObject("Bandera_Abertzale");
        bandera.transform.SetParent(portador);
        bandera.transform.localPosition = new Vector3(0.6f, 2.5f, 0f);

        // Mastil
        var palo = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        palo.transform.SetParent(bandera.transform);
        palo.transform.localPosition = Vector3.zero;
        palo.transform.localScale = new Vector3(0.05f, 1.5f, 0.05f);
        palo.GetComponent<Renderer>().sharedMaterial = matCueroNegro;
        Destroy(palo.GetComponent<Collider>());

        // Tela rotando al viento proceduralmente
        var tela = GameObject.CreatePrimitive(PrimitiveType.Quad);
        tela.transform.SetParent(bandera.transform);
        tela.transform.localPosition = new Vector3(0.6f, 1.2f, 0f);
        tela.transform.localScale = new Vector3(1.2f, 0.8f, 1f);
        
        Material matTela = new Material(Shader.Find("Unlit/Color"));
        // 50% Roja (Comunista/Sindical), 50% Rojinegra (Anarquista) o Ikurriña abstracta (Verde/Roja)
        float r = Random.value;
        if(r < 0.3f) matTela.color = Color.red;
        else if(r < 0.6f) matTela.color = new Color(0.1f, 0.5f, 0.1f); // Verde
        else matTela.color = new Color(0.2f, 0f, 0f); // Rojinegra oscura
        
        tela.GetComponent<Renderer>().sharedMaterial = matTela;
        tela.AddComponent<OsciladorViento>();
        Destroy(tela.GetComponent<Collider>());
    }

    private void CrearPancarta(Transform portador)
    {
        GameObject pancarta = GameObject.CreatePrimitive(PrimitiveType.Cube);
        pancarta.name = "Pancarta_Tex";
        pancarta.transform.SetParent(portador);
        pancarta.transform.localPosition = new Vector3(0f, 3f, 0.3f);
        pancarta.transform.localScale = new Vector3(3f, 1f, 0.1f);
        Destroy(pancarta.GetComponent<Collider>());

        Material lona = new Material(Shader.Find("Standard"));
        lona.color = Color.white;
        pancarta.GetComponent<Renderer>().sharedMaterial = lona;

        // Texto
        GameObject goTexto = new GameObject("Texto_Pancarta");
        goTexto.transform.SetParent(pancarta.transform);
        goTexto.transform.localPosition = new Vector3(-1.3f, 0.2f, -0.06f);
        goTexto.transform.localScale = new Vector3(0.1f, 0.1f, 0.1f);
        TextMesh tm = goTexto.AddComponent<TextMesh>();
        tm.text = Random.value > 0.5f ? "ALTSASUKOAK\nASKE!" : "GORA\nBORROKA!";
        tm.fontSize = 24;
        tm.color = Color.black;
        tm.fontStyle = FontStyle.Bold;
    }

    /// <summary>
    /// Update Funcional.
    /// Complejidad Asintótica O(N): Escala linealmente mediante 1000 multiplicadores de vectores.
    /// Mitiga severamente la degradación de FPS que causaría el A* en 1000 entidades simultáneas.
    /// </summary>
    void Update()
    {
        for (int i = 0; i < manifestantes.Length; i++)
        {
            if (manifestantes[i] == null) continue;
            
            Transform lider = lideres[liderAsignado[i]];
            if (lider == null) continue;

            // Offset matricial: sitúa a los seguidores rígidamente detrás o a los lados del líder
            Vector3 objetivo = lider.position + lider.right * offsetsX[i] + lider.forward * offsetsZ[i];
            
            // Interpolación Suavizada (Lerp): Rompe la rigidez matricial simulando a personas empujándose orgánicamente
            manifestantes[i].position = Vector3.Lerp(manifestantes[i].position, objetivo, Time.deltaTime * 1.5f);
            
            // Vector de Rotación Continua
            Vector3 dir = (objetivo - manifestantes[i].position).normalized;
            if (dir != Vector3.zero)
            {
                manifestantes[i].rotation = Quaternion.Slerp(manifestantes[i].rotation, Quaternion.LookRotation(dir), Time.deltaTime * 5f);
            }
        }
    }
}

public class LiderRutaIA : MonoBehaviour
{
    private NavMeshAgent agente;
    void Start()
    {
        agente = GetComponent<NavMeshAgent>();
        AsignarNuevaRuta();
    }
    void Update()
    {
        if(agente.isOnNavMesh && agente.remainingDistance < 2f) AsignarNuevaRuta();
    }
    void AsignarNuevaRuta()
    {
        if (NavMesh.SamplePosition(transform.position + new Vector3(Random.Range(-100f, 100f), 0, Random.Range(-100f, 100f)), out NavMeshHit hit, 100f, NavMesh.AllAreas))
        {
            agente.SetDestination(hit.position);
        }
    }
}

public class OsciladorViento : MonoBehaviour
{
    private float offset;
    void Start() { offset = Random.Range(0f, 100f); }
    void Update()
    {
        // Rotación local rápida para simular tela y viento
        transform.localRotation = Quaternion.Euler(0, Mathf.Sin(Time.time * 8f + offset) * 15f, 0);
    }
}
