// Assets/Scripts/SintetizadorGore.cs
using UnityEngine;

[AddComponentMenu("Alsasua V12/Generador de Gore y Visceras Matemático")]
public static class SintetizadorGore
{
    public static void EsparcirSangre(Vector3 zonaImpacto, float escala)
    {
        // 1. Chorro Dinámico de Sangre (Partículas con gravedad física)
        GameObject goreSource = new GameObject("VFX_Gore_Sangre");
        goreSource.transform.position = zonaImpacto + Vector3.up * 1f;
        
        ParticleSystem ps = goreSource.AddComponent<ParticleSystem>();
        var main = ps.main;
        main.duration = 1.5f;
        main.loop = false;
        main.startLifetime = new ParticleSystem.MinMaxCurve(2f, 4f);
        main.startSpeed = new ParticleSystem.MinMaxCurve(5f * escala, 15f * escala);
        main.startSize = new ParticleSystem.MinMaxCurve(0.2f * escala, 0.6f * escala);
        main.startColor = new Color(0.6f, 0f, 0f, 0.9f); // Sangre oscura/arterial
        main.gravityModifier = 2.5f; // Cae pesado al suelo
        
        var em = ps.emission;
        em.rateOverTime = 0;
        em.SetBursts(new ParticleSystem.Burst[] { new ParticleSystem.Burst(0f, (short)(30 * escala), (short)(80 * escala)) });

        var shape = ps.shape;
        shape.shapeType = ParticleSystemShapeType.Sphere;
        shape.radius = 0.5f;

        // Opcional: Collision con el mundo para dejar charcos/manchas
        var coll = ps.collision;
        coll.enabled = true;
        coll.type = ParticleSystemCollisionType.World;
        coll.mode = ParticleSystemCollisionMode.Collision3D;
        coll.dampen = 1f; // Se pegan al suelo sin rebotar
        coll.minKillSpeed = 0f;

        // Auto-Destruir el GameObject cuando termine de emitir y las manchas desaparezcan visualmente
        var destroyEngine = goreSource.AddComponent<DestructorAutomatico>();
        destroyEngine.tiempoDeVida = 5f;

        // 2. Grito Cínico/Visceral localizado (AudioEspacial 3D V9 aprovechado)
        AudioManager.I?.Play(AudioManager.Clip.Alarma, zonaImpacto); // Placeholder de chillido
    }
}

public class DestructorAutomatico : MonoBehaviour
{
    public float tiempoDeVida = 2f;
    void Start() { Destroy(gameObject, tiempoDeVida); }
}
