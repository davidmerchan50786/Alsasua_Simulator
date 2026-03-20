// Assets/Scripts/AudioManager.cs
// Gestor de audio centralizado con pool de AudioSources.
//
// ── USO ───────────────────────────────────────────────────────────────────
//  AudioManager.I.Play(AudioManager.Clip.Disparo, transform.position);
//  AudioManager.I.Play(AudioManager.Clip.Impacto, hit.point);
//  AudioManager.I.Play(AudioManager.Clip.Recarga);            // sin posición → 2D
//
// ── ARQUITECTURA ──────────────────────────────────────────────────────────
//  · Singleton persistente (DontDestroyOnLoad).
//  · Pool de POOL_SIZE AudioSources pre-creados en Awake() → cero AddComponent en runtime.
//  · Cada PlayOneShot toma el AudioSource libre más antiguo y lo reutiliza.
//  · Volumen por categoría (efectos, pasos, ambiente, motores).
//  · Clips asignables desde el Inspector (arrastrar archivos .wav/.ogg).
//    Sin clip asignado → se ignora silenciosamente (no lanza excepciones).
//
// ── SETUP ─────────────────────────────────────────────────────────────────
//  1. Añadir un GameObject "AudioManager" a la escena.
//  2. Adjuntar este componente.
//  3. Arrastrar clips en el Inspector (disparo, impacto, sangre, chispa,
//     recarga, pasos, motorCoche).
//  4. Llamar desde SistemaDisparo, EnemigoPatrulla, SistemaTrafico, etc.

using UnityEngine;
using System.Collections.Generic;

[AddComponentMenu("Alsasua/Audio Manager")]
public sealed class AudioManager : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  SINGLETON
    // ═══════════════════════════════════════════════════════════════════════

    public static AudioManager I { get; private set; }

    // ═══════════════════════════════════════════════════════════════════════
    //  ENUM DE CLIPS
    // ═══════════════════════════════════════════════════════════════════════

    public enum Clip
    {
        Disparo,        // flash del arma
        ImpactoSuelo,   // bala en pared/suelo
        ImpactoSangre,  // bala en enemigo
        ImpactoMetal,   // bala en vehículo
        Recarga,        // clic de recarga
        PasoNormal,     // paso andando
        PasoCorrer,     // paso corriendo
        MotorCoche,     // ruido motor (loop)
        Explosion,      // explosión bomba
        Silbato,        // policía alerta
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INSPECTOR
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ CLIPS DE AUDIO ═══")]
    [Tooltip("Arrastrar clips WAV/OGG para cada efecto. Dejar vacío = silencioso.")]
    [SerializeField] private AudioClip clipDisparo;
    [SerializeField] private AudioClip clipImpactoSuelo;
    [SerializeField] private AudioClip clipImpactoSangre;
    [SerializeField] private AudioClip clipImpactoMetal;
    [SerializeField] private AudioClip clipRecarga;
    [SerializeField] private AudioClip clipPasoNormal;
    [SerializeField] private AudioClip clipPasoCorrer;
    [SerializeField] private AudioClip clipMotorCoche;
    [SerializeField] private AudioClip clipExplosion;
    [SerializeField] private AudioClip clipSilbato;

    [Header("═══ VOLÚMENES ═══")]
    [Range(0f, 1f)] [SerializeField] private float volEfectos  = 0.80f;
    [Range(0f, 1f)] [SerializeField] private float volPasos    = 0.50f;
    [Range(0f, 1f)] [SerializeField] private float volAmbiente = 0.35f;
    [Range(0f, 1f)] [SerializeField] private float volMotores  = 0.25f;

    [Header("═══ POOL ═══")]
    [Range(8, 32)]
    [SerializeField] private int poolSize = 16;

    // ═══════════════════════════════════════════════════════════════════════
    //  POOL INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private AudioSource[] _pool;
    private int           _poolCursor;   // índice rotatorio (round-robin)

    // Cache del mapa Clip→AudioClip para no usar switch en runtime
    private Dictionary<Clip, AudioClip> _clipMap;

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY LIFECYCLE
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        // Singleton: destruir duplicados
        if (I != null && I != this) { Destroy(gameObject); return; }
        I = this;
        DontDestroyOnLoad(gameObject);

        CrearPool();
        ConstruirMapaClips();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INICIALIZACIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private void CrearPool()
    {
        _pool = new AudioSource[poolSize];
        for (int i = 0; i < poolSize; i++)
        {
            var go = new GameObject($"AudioSource_{i:D2}");
            go.transform.SetParent(transform);
            var src = go.AddComponent<AudioSource>();
            src.playOnAwake   = false;
            src.spatialBlend  = 1f;   // 3D por defecto (se puede cambiar por clip)
            src.rolloffMode   = AudioRolloffMode.Linear;
            src.minDistance   = 5f;
            src.maxDistance   = 80f;
            _pool[i] = src;
        }
    }

    private void ConstruirMapaClips()
    {
        _clipMap = new Dictionary<Clip, AudioClip>(16)
        {
            { Clip.Disparo,       clipDisparo       },
            { Clip.ImpactoSuelo,  clipImpactoSuelo  },
            { Clip.ImpactoSangre, clipImpactoSangre },
            { Clip.ImpactoMetal,  clipImpactoMetal  },
            { Clip.Recarga,       clipRecarga       },
            { Clip.PasoNormal,    clipPasoNormal    },
            { Clip.PasoCorrer,    clipPasoCorrer    },
            { Clip.MotorCoche,    clipMotorCoche    },
            { Clip.Explosion,     clipExplosion     },
            { Clip.Silbato,       clipSilbato       },
        };
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  API PÚBLICA
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Reproduce un clip en posición 3D (spatialBlend = 1).
    /// Si el clip no está asignado en el Inspector, no hace nada.
    /// </summary>
    public void Play(Clip tipo, Vector3 posicion)
    {
        if (!_clipMap.TryGetValue(tipo, out var clip) || clip == null) return;

        var src = SiguienteSource();
        src.transform.position = posicion;
        src.spatialBlend       = 1f;
        src.volume             = VolParaTipo(tipo);
        src.PlayOneShot(clip);
    }

    /// <summary>
    /// Reproduce un clip sin posición (2D — UI, recarga, etc.).
    /// </summary>
    public void Play(Clip tipo)
    {
        if (!_clipMap.TryGetValue(tipo, out var clip) || clip == null) return;

        var src = SiguienteSource();
        src.transform.localPosition = Vector3.zero;
        src.spatialBlend            = 0f;  // 2D
        src.volume                  = VolParaTipo(tipo);
        src.PlayOneShot(clip);
    }

    /// <summary>
    /// Asigna un AudioClip a un AudioSource externo en modo loop (motores, etc.).
    /// Llama con clip=null para detener el loop.
    /// </summary>
    public void PlayLoop(Clip tipo, AudioSource source)
    {
        if (source == null) return;
        if (!_clipMap.TryGetValue(tipo, out var clip)) return;

        if (clip == null)
        {
            source.Stop();
            return;
        }

        if (source.clip == clip && source.isPlaying) return;  // ya está sonando → no reiniciar

        source.clip   = clip;
        source.loop   = true;
        source.volume = VolParaTipo(tipo);
        source.Play();
    }

    /// <summary>Devuelve el tamaño del pool asignado (Usado en Testing).</summary>
    public int TamañoPool => _pool?.Length ?? 0;

    // ═══════════════════════════════════════════════════════════════════════
    //  INTERNOS
    // ═══════════════════════════════════════════════════════════════════════

    // Round-robin sobre el pool → si el slot está ocupado, lo roba (one-shot breve)
    private AudioSource SiguienteSource()
    {
        var src = _pool[_poolCursor];
        _poolCursor = (_poolCursor + 1) % poolSize;
        return src;
    }

    private float VolParaTipo(Clip tipo)
    {
        switch (tipo)
        {
            case Clip.PasoNormal:
            case Clip.PasoCorrer:
                return volPasos;
            case Clip.MotorCoche:
                return volMotores;
            case Clip.Silbato:
                return volAmbiente;
            default:
                return volEfectos;
        }
    }

#if UNITY_EDITOR
    private void OnDrawGizmos()
    {
        // Mostrar los AudioSources activos como esferas en la vista de escena
        if (_pool == null) return;
        Gizmos.color = new Color(0f, 1f, 0.8f, 0.4f);
        foreach (var src in _pool)
            if (src != null && src.isPlaying)
                Gizmos.DrawWireSphere(src.transform.position, 1.2f);
    }
#endif
}
