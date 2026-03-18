// Assets/Scripts/SistemaBombas.cs
// Colocar bombas (F), detonar en remoto (G) o por proximidad

using UnityEngine;
using System.Collections.Generic;

public class SistemaBombas : MonoBehaviour
{
    [Header("═══ BOMBA ═══")]
    [Range(1f, 30f)]
    [SerializeField] private float radioExplosion   = 12f;
    [SerializeField] private float fuerzaExplosion  = 700f;
    [SerializeField] private int   danoExplosion    = 130;
    [SerializeField] private int   bombasDisponibles = 5;
    [Range(0f, 60f)]
    [SerializeField] private float timerAutodetonacion = 0f;  // 0 = solo remoto

    [Header("═══ DETONACIÓN POR PROXIMIDAD ═══")]
    [SerializeField] private bool  proximidadActiva = true;
    [SerializeField] private float distanciaProximidad = 4f;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private List<BombaColocada> bombas = new List<BombaColocada>();
    private Camera camara;

    // BUG FIX RENDIMIENTO: caché de enemigos + throttle para no llamar
    // FindObjectsByType cada frame (60+ veces/segundo → 5 veces/segundo)
    private EnemigoPatrulla[] enemigosCache  = new EnemigoPatrulla[0];
    private float timerProximidad            = 0f;
    private const float INTERVALO_PROXIMIDAD = 0.2f;  // refrescar cada 200 ms

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        camara = GetComponentInChildren<Camera>();
        if (camara == null) camara = Camera.main;
    }

    private void Update()
    {
        // BUG FIX RENDIMIENTO: comprobar proximidad sólo cada INTERVALO_PROXIMIDAD segundos
        // y sólo si hay bombas activas (evitar trabajo inútil cuando no hay ninguna colocada)
        if (!proximidadActiva || bombas.Count == 0) return;

        timerProximidad -= Time.deltaTime;
        if (timerProximidad > 0f) return;

        timerProximidad = INTERVALO_PROXIMIDAD;
        // Refrescar la caché de enemigos cada 200 ms (no cada frame)
        enemigosCache = Object.FindObjectsByType<EnemigoPatrulla>(FindObjectsSortMode.None);
        ComprobarProximidad();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  API PÚBLICA
    // ═══════════════════════════════════════════════════════════════════════

    public void ColocarBomba()
    {
        if (bombasDisponibles <= 0)
        {
            Debug.Log("[Bombas] Sin bombas disponibles.");
            return;
        }
        if (camara == null) { Debug.LogWarning("[Bombas] Cámara no disponible."); return; }

        // Raycast al suelo desde delante del jugador para colocar la bomba
        Vector3 posicion;
        Vector3 origenBomba = transform.position + transform.forward * 1.5f + Vector3.up * 1f;
        if (Physics.Raycast(origenBomba, Vector3.down, out RaycastHit hit, 5f))
            posicion = hit.point;
        else
            posicion = transform.position + transform.forward * 1.5f;

        var bombaGO = CrearObjetoBomba(posicion);
        var bomba   = new BombaColocada
        {
            objetoFisico = bombaGO,
            posicion     = posicion,
        };
        bombas.Add(bomba);
        bombasDisponibles--;

        Debug.Log($"[Bombas] Bomba colocada en {posicion:F1}. Quedan: {bombasDisponibles}");

        // Si tiene timer, iniciar cuenta atrás
        if (timerAutodetonacion > 0f)
            StartCoroutine(TimerBomba(bomba));
    }

    public void DetonarUltima()
    {
        if (bombas.Count == 0)
        {
            Debug.Log("[Bombas] No hay bombas colocadas.");
            return;
        }
        var ultima = bombas[bombas.Count - 1];
        Detonar(ultima);
    }

    public void DetonarTodas()
    {
        foreach (var b in new List<BombaColocada>(bombas))
            Detonar(b);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DETONACIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private void Detonar(BombaColocada bomba)
    {
        if (bomba.explotada) return;
        bomba.explotada = true;
        bombas.Remove(bomba);

        if (bomba.objetoFisico != null)
            Destroy(bomba.objetoFisico);

        SistemaExplosion.Explotar(bomba.posicion, radioExplosion, fuerzaExplosion, danoExplosion);
        Debug.Log($"[Bombas] ¡BOOM! en {bomba.posicion:F1}");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DETECCIÓN POR PROXIMIDAD
    // ═══════════════════════════════════════════════════════════════════════

    private void ComprobarProximidad()
    {
        // Usa la caché refrescada en Update() — ya NO llama FindObjectsByType aquí
        // FIX OPT: iterar con índice descendente para evitar new List<> cada 200 ms.
        // Detonar() llama bombas.Remove() — el índice descendente garantiza que los
        // elementos anteriores al índice actual no se saltan tras el Remove.
        // FIX OPT: sqrMagnitude evita Sqrt de Vector3.Distance (1 Sqrt/par bomba×enemigo).
        float distSqr = distanciaProximidad * distanciaProximidad;
        for (int i = bombas.Count - 1; i >= 0; i--)
        {
            var bomba = bombas[i];
            if (bomba.explotada) continue;
            foreach (var enemigo in enemigosCache)
            {
                if (enemigo == null) continue;
                if ((bomba.posicion - enemigo.transform.position).sqrMagnitude <= distSqr)
                {
                    Debug.Log("[Bombas] Enemigo detectado cerca. ¡DETONACIÓN POR PROXIMIDAD!");
                    Detonar(bomba);
                    break;
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  OBJETO 3D DE BOMBA
    // ═══════════════════════════════════════════════════════════════════════

    // FIX LEAK: material compartido para todas las bombas — .material (instancia) fue sustituido
    // por .sharedMaterial + MaterialPropertyBlock. Una sola instancia del material para N bombas.
    private static Material _matBombaCache;
    private static Material MatBomba()
    {
        if (_matBombaCache != null) return _matBombaCache;
        var shader = Shader.Find("Universal Render Pipeline/Lit")
                  ?? Shader.Find("Universal Render Pipeline/Unlit")
                  ?? Shader.Find("Standard");
        _matBombaCache = new Material(shader) { color = new Color(0.1f, 0.1f, 0.1f) };
        return _matBombaCache;
    }

    private GameObject CrearObjetoBomba(Vector3 posicion)
    {
        // Cuerpo principal (cilindro negro)
        var go = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        go.name = "Bomba";
        go.transform.position   = posicion + Vector3.up * 0.12f;
        go.transform.localScale = new Vector3(0.2f, 0.12f, 0.2f);

        // FIX LEAK: sharedMaterial + material cacheado estático en vez de .material (instancia nueva por bomba)
        go.GetComponent<Renderer>().sharedMaterial = MatBomba();

        // Luz parpadeante roja
        var luzGO = new GameObject("LuzBomba");
        luzGO.transform.SetParent(go.transform);
        luzGO.transform.localPosition = Vector3.up * 1.2f;
        var luz = luzGO.AddComponent<Light>();
        luz.type      = LightType.Point;
        luz.color     = Color.red;
        luz.intensity = 2f;
        luz.range     = 3f;

        // Parpadeo
        go.AddComponent<ParpadeoLuz>().luz = luz;

        return go;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  TIMER AUTOMÁTICO
    // ═══════════════════════════════════════════════════════════════════════

    private System.Collections.IEnumerator TimerBomba(BombaColocada bomba)
    {
        yield return new WaitForSeconds(timerAutodetonacion);
        if (!bomba.explotada)
            Detonar(bomba);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HUD
    // ═══════════════════════════════════════════════════════════════════════

    private void OnGUI()
    {
        float x = Screen.width - 160f;
        float y = Screen.height - 70f;

        GUI.color = new Color(0, 0, 0, 0.55f);
        GUI.DrawTexture(new Rect(x - 4, y - 4, 155, 28), Texture2D.whiteTexture);

        GUI.color = bombasDisponibles > 0 ? Color.red : new Color(0.5f, 0.5f, 0.5f);
        GUI.Label(new Rect(x, y, 150, 24),
            $"💣 Bombas: {bombasDisponibles}   [{bombas.Count} colocadas]");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CLASE AUXILIAR
    // ═══════════════════════════════════════════════════════════════════════

    private class BombaColocada
    {
        public GameObject objetoFisico;
        public Vector3    posicion;
        public bool       explotada;
    }
}

// ─── Parpadeo de la luz de la bomba ──────────────────────────────────────────
public class ParpadeoLuz : MonoBehaviour
{
    public Light luz;
    private float timer;

    private void Update()
    {
        timer += Time.deltaTime * 4f;
        if (luz != null)
            luz.enabled = Mathf.Sin(timer) > 0f;
    }
}
