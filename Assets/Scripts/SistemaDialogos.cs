// Assets/Scripts/SistemaDialogos.cs
using UnityEngine;
using System.Collections.Generic;

[AddComponentMenu("Alsasua/Interactive Lore NPCs")]
public class SistemaDialogos : MonoBehaviour
{
    private class NPCBizarro
    {
        public GameObject modelo;
        public string nombre;
        public string historiaMisteriosa;
    }

    private List<NPCBizarro> catalogoNPCs = new List<NPCBizarro>();
    private bool mostrarUI = false;
    private string textoEnPantalla = "";
    private float tiempoTexto = 0f;

    private void Start()
    {
        GenerarNPCsHistoricos();
    }

    private void GenerarNPCsHistoricos()
    {
        string[] loresBizarros = new string[]
        {
            "¿Sabías que bajo estos robles aseguran que volaban las Sorginak (brujas) en 1560? A las 03:00 AM, si pones Visión Térmica, dicen que las sombras dejan rastro.",
            "Aquí explotó la tanqueta en la tercera guerra Carlista. Si cavas hondo, todavía saltan las esquirlas de plomo en el simulador.",
            "Llevo en esta rotonda 40 años viendo pasar lecheras. El puente del río Alzania trae rumores metálicos... ten cuidado.",
            "Han documentado OVNIs sobre la Sierra de Aralar, justo detrás de las vías del tren viejo. Las farolas rojas no son un glitch del juego.",
            "Operación Ogro. Dicen que si detonas un coche patrulla desde la cámara dron, vuela exactamente 50 metros replicando el evento real."
        };

        string[] nombres = { 
            "Anciano del Akelarre", 
            "Fantasma Carlista", 
            "Testigo de la Benemérita", 
            "Ufólogo de Aralar", 
            "Mecánico Sospechoso" 
        };

        for (int i = 0; i < 5; i++)
        {
            // NPC placeholder cinemático. 
            GameObject npc = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            npc.name = "NPC_Historia_" + nombres[i];
            
            // Random scatter inicial (caerán automáticamente a la malla urbana gracias al script ancla)
            Vector3 spawnCielo = new Vector3(Random.Range(-300f, 300f), 800f, Random.Range(-300f, 300f));
            npc.transform.position = spawnCielo;
            
            Renderer rend = npc.GetComponent<Renderer>();
            rend.material.color = new Color(0.3f, 0.3f, 0.1f); // Tinte apagado histórico

            // Script autograbitatorio (hace que aterricen en la topografía irregular sin NavMesh)
            npc.AddComponent<AnclaTopograficaNPC>();

            catalogoNPCs.Add(new NPCBizarro { 
                modelo = npc, 
                nombre = nombres[i], 
                historiaMisteriosa = loresBizarros[i] 
            });
        }
    }

    private void Update()
    {
        if (Camera.main == null) return;

        bool cercaDeAlguien = false;
        foreach (var npc in catalogoNPCs)
        {
            float dist = Vector3.Distance(Camera.main.transform.position, npc.modelo.transform.position);
            
            if (dist < 40f) // Radio de interacción ampliado para la cámara dron
            {
                cercaDeAlguien = true;
                
                if (Time.time > tiempoTexto)
                {
                    mostrarUI = true;
                    textoEnPantalla = $"[Pulsa E] Escuchar los murmullos de: {npc.nombre}";
                }

                if (Input.GetKeyDown(KeyCode.E))
                {
                    textoEnPantalla = $"--- {npc.nombre.ToUpper()} ---\n\n\"{npc.historiaMisteriosa}\"";
                    tiempoTexto = Time.time + 8f; // Mantiene fijado el diálogo 8 segundos
                }
                break;
            }
        }

        if (!cercaDeAlguien && Time.time > tiempoTexto) 
        {
            mostrarUI = false;
        }
    }

    private void OnGUI()
    {
        if (mostrarUI)
        {
            GUI.color = new Color(1f, 1f, 1f, 0.9f);
            GUIStyle stPanel = new GUIStyle(GUI.skin.box);
            
            GUIStyle stTexto = new GUIStyle(GUI.skin.label);
            stTexto.fontSize = 22;
            stTexto.fontStyle = FontStyle.Bold;
            stTexto.alignment = TextAnchor.MiddleCenter;
            stTexto.wordWrap = true;
            stTexto.normal.textColor = Color.yellow;
            
            // UI tipo RPG / Novela visual fija abajo
            Rect panelRect = new Rect(Screen.width * 0.1f, Screen.height - 180, Screen.width * 0.8f, 150);
            GUI.Box(panelRect, "", stPanel);
            
            Rect textRect = new Rect(Screen.width * 0.15f, Screen.height - 170, Screen.width * 0.7f, 130);
            GUI.Label(textRect, textoEnPantalla, stTexto);
        }
    }
}

// Clase autoejecutable para anclar NPs a la malla física de calles/terreno V5
public class AnclaTopograficaNPC : MonoBehaviour
{
    private bool anclado = false;
    private void Update()
    {
        if (anclado) return;
        
        // Raycast infinito hacia abajo para pinchar los edificios o calle
        if (Physics.Raycast(transform.position + Vector3.up * 50f, Vector3.down, out RaycastHit hit, 5000f))
        {
            transform.position = hit.point + Vector3.up * 1f;
            anclado = true;
            // Destruimos el script para liberar O(1) la RAM, ya cumplió su ciclo.
            Destroy(this);
        }
    }
}
