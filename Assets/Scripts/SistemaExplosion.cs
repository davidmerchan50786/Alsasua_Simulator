// Assets/Scripts/SistemaExplosion.cs
// Explosión con físicas, daño en radio, fuego y humo

using UnityEngine;
using System.Collections.Generic;

public class SistemaExplosion : MonoBehaviour
{
    [Header("═══ EXPLOSIÓN ═══")]
    [SerializeField] public float radio        = 15f;
    [SerializeField] public float fuerzaFisica = 800f;
    [SerializeField] public int   danoMaximo   = 150;
    [SerializeField] public float duracionFuego = 6f;

    [Header("═══ FX CINEMÁTICOS (MYASSETS) ═══")]
    [Tooltip("Asigna aquí el Prefab del Asset Store 'Cinematic Explosions FREE'. Ignorar para mantener FX procedurales.")]
    [SerializeField] public GameObject prefabExplosionCinematica;

    // FIX GC: HashSets pre-alloc estáticos para eliminar 5 allocaciones heap por explosión.
    // Son estáticos seguros porque AplicarFisicasYDano() es síncrono —
    // Unity no ejecuta dos explosiones en paralelo dentro del mismo frame.
    private static readonly HashSet<Rigidbody>          _rbYaDanados         = new HashSet<Rigidbody>();
    private static readonly HashSet<EnemigoPatrulla>    _enemigosYaDanados   = new HashSet<EnemigoPatrulla>();
    private static readonly HashSet<ControladorJugador> _jugadoresYaDanados  = new HashSet<ControladorJugador>();
    private static readonly HashSet<VehiculoNPC>        _vehiculosYaDanados  = new HashSet<VehiculoNPC>();
    private static readonly HashSet<BarricadaFuego>     _barricadasYaDanadas = new HashSet<BarricadaFuego>();
    private static readonly HashSet<SistemaReaccionVital> _seresVivosYaProcesados = new HashSet<SistemaReaccionVital>();

    /// <summary>
    /// Crea una explosión en la posición indicada.
    /// Llamar estáticamente: SistemaExplosion.Explotar(pos, radio, fuerza, dano)
    /// </summary>
    public static void Explotar(Vector3 posicion, float radio = 12f,
                                 float fuerza = 600f, int dano = 120)
    {
        var go = new GameObject("Explosion");
        go.transform.position = posicion;
        var se = go.AddComponent<SistemaExplosion>();
        se.radio        = radio;
        se.fuerzaFisica = fuerza;
        se.danoMaximo   = dano;
        se.Detonar();
    }

    public void Detonar()
    {
#if UNITY_EDITOR
        // V4 AUTO-ASSIGN: Buscar dinámicamente el prefab cinemático de explosión
        if (prefabExplosionCinematica == null)
        {
            string[] guids = UnityEditor.AssetDatabase.FindAssets("t:Prefab CinematicExplosion");
            if (guids.Length > 0)
            {
                prefabExplosionCinematica = UnityEditor.AssetDatabase.LoadAssetAtPath<GameObject>(UnityEditor.AssetDatabase.GUIDToAssetPath(guids[0]));
            }
            else
            {
                // Fallback secondary name search if "CinematicExplosion" isn't exactly matched
                string[] guids2 = UnityEditor.AssetDatabase.FindAssets("t:Prefab Explosion");
                foreach (var g in guids2)
                {
                    string path = UnityEditor.AssetDatabase.GUIDToAssetPath(g);
                    if (path.Contains("Cinematic") || path.Contains("Mirza"))
                    {
                        prefabExplosionCinematica = UnityEditor.AssetDatabase.LoadAssetAtPath<GameObject>(path);
                        break;
                    }
                }
            }
        }
#endif

        // FIX: las explosiones no tenían audio. AudioManager.Clip.Explosion está definido
        // pero nunca se llamaba desde aquí → explosiones completamente silenciosas.
        AudioManager.I?.Play(AudioManager.Clip.Explosion, transform.position);

        AplicarFisicasYDano();

        // V15 CLEAN ARCHITECTURE: Delegar renders a su módulo estricto
        if (prefabExplosionCinematica != null)
        {
            var fx = Instantiate(prefabExplosionCinematica, transform.position, Quaternion.identity);
            Destroy(fx, duracionFuego + 3f);
        }
        else
        {
            var vfxEngine = gameObject.AddComponent<EfectosVisualesExplosion>();
            vfxEngine.GenerarVFX(radio, duracionFuego);
        }

        // V15 CLEAN ARCHITECTURE: Delegar cámara a su módulo estricto
        GestorTraumaCamara.EvaluarTrauma(transform.position, radio);

        Destroy(gameObject, duracionFuego + 2f);
    }

    // ─── Físicas y daño ──────────────────────────────────────────────────

    private static readonly Collider[] hitBuffer = new Collider[200];

    private void AplicarFisicasYDano()
    {
        // V3 FIX: Zero-GC OverlapSphereNonAlloc
        // V12: Ampliar radio detección para generar pánico lejano
        int numHits = Physics.OverlapSphereNonAlloc(transform.position, radio * 3f, hitBuffer);

        // FIX GC: limpiar los HashSets estáticos en lugar de crear nuevos.
        _rbYaDanados.Clear();
        _enemigosYaDanados.Clear();
        _jugadoresYaDanados.Clear();
        _vehiculosYaDanados.Clear();
        _barricadasYaDanadas.Clear();
        _seresVivosYaProcesados.Clear();

        for (int i = 0; i < numHits; i++)
        {
            var col = hitBuffer[i];
            
            // Fuerza de explosión a Rigidbodies (una sola vez por objeto físico)
            var rb = col.GetComponent<Rigidbody>();
            if (rb != null && _rbYaDanados.Add(rb))
            {
                // V7 EXTREME CAR-BOMB PHYSICS (Operación Ogro Effect)
                float forceMult = 1f;
                float upwardMod = 1f; 

                string n = rb.gameObject.name.ToLower();
                if (n.Contains("vehiculo") || n.Contains("car") || n.Contains("police"))
                {
                    // Fuerzas hiperrealistas para hacer volar un coche blindado decenas de metros girando
                    forceMult = 18f; 
                    upwardMod = 60f; 
                    
                    // Remueve bloqueos de rotación para volar descontroladamente
                    rb.constraints = RigidbodyConstraints.None; 
                    rb.mass = 1200f; // Estandariza la masa para que la campana de Gauss no falle
                }

                rb.AddExplosionForce(fuerzaFisica * forceMult, transform.position, radio, upwardMod, ForceMode.Impulse);
            }

            // Daño basado en la distancia al centro del objeto raíz
            float dist   = Vector3.Distance(transform.position, col.transform.position);
            float factor = 1f - Mathf.Clamp01(dist / radio);
            int   dano   = Mathf.RoundToInt(danoMaximo * factor);

            var enemigo  = col.GetComponentInParent<EnemigoPatrulla>();
            if (enemigo != null && _enemigosYaDanados.Add(enemigo))
                enemigo.RecibirDano(dano);

            var jugador  = col.GetComponentInParent<ControladorJugador>();
            if (jugador != null && _jugadoresYaDanados.Add(jugador))
                jugador.RecibirDano(dano);

            var vehiculo = col.GetComponentInParent<VehiculoNPC>();
            if (vehiculo != null && _vehiculosYaDanados.Add(vehiculo))
            {
                vehiculo.RecibirDano(dano);
                InstanciarFuegoPersistente(vehiculo.transform);
                ConvertirEnCarroceriaQuemada(vehiculo.gameObject);
            }

            var barricada = col.GetComponentInParent<BarricadaFuego>();
            if (barricada != null && dist <= radio && _barricadasYaDanadas.Add(barricada))
            {
                barricada.RecibirDano(dano);
                InstanciarFuegoPersistente(barricada.transform);
            }

            // V13: Cristalería Urbana (Las ventanas explotan por la onda térmica y de choque)
            var cristales = col.GetComponentInParent<CristalDestructible>();
            if (cristales != null) cristales.RecibirOndaExpansiva(dist, radio * 2f); // El cristal es más frágil, se rompe desde más lejos

            // V12: Biomecánica de Supervivencia, Gore y Carbonización
            var serVivo = col.GetComponentInParent<SistemaReaccionVital>();
            if (serVivo != null && _seresVivosYaProcesados.Add(serVivo))
            {
                if (dist <= radio * 0.4f)
                {
                    // Zona Cero: Muerte instantánea, Ragdoll sangriento extremo
                    SintetizadorGore.EsparcirSangre(serVivo.transform.position, 1.5f);
                    AplicarLanzamientoRagdoll(serVivo.gameObject);
                }
                else if (dist <= radio)
                {
                    // Zona Media: Se prende fuego y corre en llamas hasta ser ceniza
                    serVivo.PrenderFuegoVivo();
                }
                else
                {
                    // Zona Exterior: Oye la bomba y entra en Panico huyendo
                    serVivo.DetectarPeligro(transform.position);
                }
            }
        }
    }

    // ─── V9 Físicas Avanzadas y Persistencia ──────────────────────────

    private void ConvertirEnCarroceriaQuemada(GameObject coche)
    {
        // 1. Quitar IA y scripts para que sea un trozo de metal inerte
        Destroy(coche.GetComponent<VehiculoNPC>());
        
        // 2. Mesh Swapping / Tintado Negro Carbón
        Renderer[] renderers = coche.GetComponentsInChildren<Renderer>();
        foreach(Renderer r in renderers)
        {
            r.material.color = new Color(0.05f, 0.05f, 0.05f); // Negro calcinado
            r.material.SetFloat("_Glossiness", 0f); // Sin reflejos
            r.material.SetFloat("_Metallic", 0.9f); // Metal oxidado
            
            // Si es un cristal (luna), destruirlo para simular que ha estallado
            if (r.gameObject.name.ToLower().Contains("glass") || r.gameObject.name.ToLower().Contains("window"))
            {
                Destroy(r.gameObject);
            }
        }

        // 3. Añadir peso muerto para bloquear la calle
        Rigidbody rb = coche.GetComponent<Rigidbody>();
        if (rb != null)
        {
            rb.mass = 3000f; 
            rb.drag = 2f;
        }
        
        coche.name = "Vehiculo_Calcinado_Obstaculo";
    }

    private void AplicarLanzamientoRagdoll(GameObject entidad)
    {
        // Si es un humano (Punks, Policías, Soldados) o Animal (V8)
        string n = entidad.name.ToLower();
        if (n.Contains("enemigo") || n.Contains("soldier") || n.Contains("punk") || n.Contains("npc"))
        {
            // Matar animador rígido
            Animator anim = entidad.GetComponentInParent<Animator>();
            if (anim != null) anim.enabled = false;

            // Asegurar que tiene Rigidbody para salir volando libremente
            Rigidbody rb = entidad.GetComponentInParent<Rigidbody>();
            if (rb == null) rb = entidad.transform.parent ? entidad.transform.parent.gameObject.AddComponent<Rigidbody>() : entidad.AddComponent<Rigidbody>();
            
            rb.isKinematic = false;
            rb.useGravity = true;
            rb.mass = 80f; // Peso humano estándar
            rb.collisionDetectionMode = CollisionDetectionMode.Continuous;

            // Desbloquear rotación para que dé vueltas de campana en el aire como un muñeco de trapo
            rb.constraints = RigidbodyConstraints.None;

            // Recalcular fuerza de Ragdoll
            rb.AddExplosionForce(fuerzaFisica * 2f, transform.position, radio, 5f, ForceMode.Impulse);
        }
    }

    private void InstanciarFuegoPersistente(Transform victima)
    {
        // V8 APOCALYPSE: Fuego perpetuo en los restos que quema durante horas
        GameObject fuegoPersistente = new GameObject("Fuego_Persistente_V8");
        fuegoPersistente.transform.SetParent(victima);
        fuegoPersistente.transform.localPosition = Vector3.up * 1f;

        ParticleSystem ps = fuegoPersistente.AddComponent<ParticleSystem>();
        var main = ps.main;
        main.duration = 800f; // Fuego infinito/largo plazo
        main.loop = true;
        main.startLifetime = new ParticleSystem.MinMaxCurve(1f, 2.5f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(2f, 4f);
        main.startSize = new ParticleSystem.MinMaxCurve(1f, 3f);
        main.startColor = new ParticleSystem.MinMaxGradient(new Color(1f, 0.4f, 0f, 0.9f), new Color(1f, 0.1f, 0f, 0.6f));
        main.gravityModifier = -0.3f;
        
        var em = ps.emission; em.rateOverTime = 40f;
        var shape = ps.shape; shape.shapeType = ParticleSystemShapeType.Sphere; shape.radius = 2f;
        
        ps.Play();
    }
}
