// Assets/Scripts/SistemaPersonajes.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Gestiona todos los tipos de personaje del simulador de Alsasua.
//
//  · GuardiaCivil, PoliciaForal, Manifestantes, Civiles.
//
//  FIX V2 (ESCALABILIDAD EXTREMA - DATA ORIENTED DESIGN):
//  · Migrado de arrays gestionados a NativeArray.
//  · Lógica de movimiento migrada a IJobParallelForTransform (Worker Threads).
//  · Eliminado el SincronizarGOs() mono-hilo.
//  · Rendimiento masivo sin bloqueos en el hilo principal.
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;
using System.Collections.Generic;
using Unity.Collections;
using UnityEngine.Jobs;
using Unity.Jobs;
using AlsasuaSimulator.Scripts;

public sealed class SistemaPersonajes : MonoBehaviour
{
    // ───────────────────────────────────────────────────────────────────────
    //  TIPOS
    // ───────────────────────────────────────────────────────────────────────
    public enum TipoPersonaje : byte
    {
        GuardiaCivil,
        PoliciaForal,
        ManifestantePalestino,
        ManifestanteJurrutu,
        PortadorIkurriña,
        PortadorNavarra,
        CivilianMale,
        CivilianFemale,
    }

    public enum EstadoPersonaje : byte { Caminando, Parado, Patrullando }

    public struct PersonajeData
    {
        public Vector3          posicion;
        public Vector3          velocidad;
        public float            alturaY;
        public TipoPersonaje    tipo;
        public EstadoPersonaje  estado;
        public int              waypointActual;
        public float            tiempoParado;
        public float            varianteTono;   
        public Vector3          destinoCivil;   
        
        // PRNG (Pseudo-Random Number Generator) determinista para usar en el Worker Thread (sin UnityEngine.Random)
        public uint rndState;
        public float NextFloat()
        {
            rndState ^= rndState << 13;
            rndState ^= rndState >> 17;
            rndState ^= rndState << 5;
            return (rndState & 0xFFFFFF) / 16777216f; // Rango 0.0 - 1.0
        }
        public float RndRange(float min, float max) => min + NextFloat() * (max - min);
        public Vector2 RndInsideCircle()
        {
            float theta = NextFloat() * 2f * Mathf.PI;
            float r     = Mathf.Sqrt(NextFloat());
            return new Vector2(r * Mathf.Cos(theta), r * Mathf.Sin(theta));
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  INSPECTOR
    // ───────────────────────────────────────────────────────────────────────
    [Header("═══ CANTIDADES ═══")]
    [SerializeField] private int numGuardiaCivil         = 8;
    [SerializeField] private int numPoliciaForal         = 6;
    [SerializeField] private int numManifestantesPales   = 40;
    [SerializeField] private int numManifestantesJurrutu = 30;
    [SerializeField] private int numPortadoresIkurriña   = 8;
    [SerializeField] private int numPortadoresNavarra    = 4;
    [SerializeField] private int numCiviles              = 50;

    [Header("═══ RUTAS Y ÁREAS ═══")]
    [SerializeField] private Transform[] rutaGuardiaCivil;
    [SerializeField] private Transform[] rutaPoliciaForal;
    [SerializeField] private Bounds areaCiviles = new Bounds(Vector3.zero, new Vector3(200f, 10f, 200f));
    [SerializeField] private Vector3 posicionManifestantes = new Vector3(-50f, 0f, 0f);

    [Header("═══ VELOCIDADES ═══")]
    [SerializeField] private float velocidadGC      = 1.2f;
    [SerializeField] private float velocidadPF      = 1.4f;
    [SerializeField] private float velocidadManif   = 1.1f;
    [SerializeField] private float velocidadCivil   = 0.95f;

    // ───────────────────────────────────────────────────────────────────────
    //  ESTADO INTERNO (DATA-ORIENTED)
    // ───────────────────────────────────────────────────────────────────────
    private NativeArray<PersonajeData> _personajesNative;
    private TransformAccessArray       _transformAccess;
    private NativeArray<Vector3>       _rutaGCNative;
    private NativeArray<Vector3>       _rutaPFNative;
    private JobHandle                  _movimientoJobHandle;
    
    private GameObject[] _goPersonajes; // Mantener referencias puras si es necesario liberar memoria
    private int _totalInstancias;

    // Rendering procedural (sin assets)
    private Material[] _matsBase;   
    private readonly List<Material>  _matsCreados   = new List<Material>();
    private readonly List<Texture2D> _texsCreadas   = new List<Texture2D>();
    private readonly List<Mesh>      _meshesCreados = new List<Mesh>();

    private Texture2D _texIkurriña;
    private Texture2D _texNavarra;
    private Texture2D _texKeffiyeh;
    private Mesh _meshCuerpo;     
    private Mesh _meshBandera;    

    private int _idxRaycast = 0;

    private static readonly int _idBaseColor   = Shader.PropertyToID("_BaseColor");
    private static readonly int _idBaseMap     = Shader.PropertyToID("_BaseMap");

    // ───────────────────────────────────────────────────────────────────────
    //  UNITY LIFECYCLE
    // ───────────────────────────────────────────────────────────────────────
    private void Awake()
    {
        CrearTexturas();
        CrearMeshes();
        CrearMaterialesBase();
        
        // Convertir rutas al inicio a NativeArrays antes del spawn
        PrepararRutasNativas();
        SpawnTodos();
    }

    private void Update()
    {
        // 1. Finalizar el trabajo multihilo del frame anterior (obligatorio antes de leer/escribir nativos)
        _movimientoJobHandle.Complete();

        if (_totalInstancias == 0 || !_personajesNative.IsCreated) return;

        // 2. Ejecutar tareas seguras en el hilo principal (Raycast, muy pesado en Jobs si no se usa Command)
        _idxRaycast = (_idxRaycast + 1) % _totalInstancias;
        SampleTerreno(_idxRaycast);

        // 3. Programar (Schedule) el Job masivo de simulación cinemática
        var job = new PersonajesCaminarJob
        {
            dt = Time.deltaTime,
            velGC = velocidadGC,
            velPF = velocidadPF,
            velManif = velocidadManif,
            velCivil = velocidadCivil,
            posManifestantes = posicionManifestantes,
            centroCiviles = areaCiviles.center,
            extentsCiviles = areaCiviles.extents,
            rutaGC = _rutaGCNative,
            rutaPF = _rutaPFNative,
            personajes = _personajesNative
        };

        // Schedule reparte la carga en todos los núcleos lógicos del procesador
        _movimientoJobHandle = job.Schedule(_transformAccess);
        
        // Empacar la ejecución inmediata para reducir latencia frame a frame
        JobHandle.ScheduleBatchedJobs();
    }

    private void LateUpdate()
    {
        // Forzar la finalización en LateUpdate garantiza que para cuando Unity renderice las cámaras,
        // los transforms modificados en el Job System ya hayan sido aplicados.
        _movimientoJobHandle.Complete();
    }

    private void OnEnable()
    {
        GameEventBus.OnAlertaCambio += OnAlertaCambiada;
    }

    private void OnDisable()
    {
        GameEventBus.OnAlertaCambio -= OnAlertaCambiada;
        _movimientoJobHandle.Complete(); // Seguridad antes de apagar
    }

    private void OnDestroy()
    {
        // 1. Detener el job en curso pase lo que pase para evitar Data Races (Access Violation)
        _movimientoJobHandle.Complete();

        // 2. Liberar de memoria no-administrada los buffers nativos previniendo "Memory Leaks" gravísimos
        if (_personajesNative.IsCreated) _personajesNative.Dispose();
        if (_transformAccess.isCreated)  _transformAccess.Dispose();
        if (_rutaGCNative.IsCreated)     _rutaGCNative.Dispose();
        if (_rutaPFNative.IsCreated)     _rutaPFNative.Dispose();

        // 3. Liberar texturas y mallas URP
        foreach (var m    in _matsCreados)   if (m    != null) Object.Destroy(m);
        foreach (var t    in _texsCreadas)   if (t    != null) Object.Destroy(t);
        foreach (var mesh in _meshesCreados) if (mesh != null) Object.Destroy(mesh);
        _matsCreados.Clear();
        _texsCreadas.Clear();
        _meshesCreados.Clear();
    }

    // ───────────────────────────────────────────────────────────────────────
    //  JOB MULTI-THREADING LÓGICA (ECS BASICO)
    // ───────────────────────────────────────────────────────────────────────
    // Este struct se envía a un Worker Thread aislado. No puede tocar GameObjects ni Unity API.
    // Solo matemática pura.
    
    // [BurstCompile] (Atributo potencial para máximo rendimiento, Omitido para mantener portabilidad sin requerir paquete externo Burst)
    private struct PersonajesCaminarJob : IJobParallelForTransform
    {
        public float dt;
        public float velGC;
        public float velPF;
        public float velManif;
        public float velCivil;
        
        public Vector3 posManifestantes;
        public Vector3 centroCiviles;
        public Vector3 extentsCiviles;

        [ReadOnly] public NativeArray<Vector3> rutaGC;
        [ReadOnly] public NativeArray<Vector3> rutaPF;
        
        public NativeArray<PersonajeData> personajes;

        public void Execute(int i, TransformAccess transform)
        {
            var p = personajes[i];

            // 1. Lógica de Máquina de Estados
            switch (p.estado)
            {
                case EstadoPersonaje.Patrullando: p = TickPatrulla(p); break;
                case EstadoPersonaje.Caminando:   p = TickCaminar(p);  break;
                case EstadoPersonaje.Parado:
                    p.tiempoParado -= dt;
                    if (p.tiempoParado <= 0f) p.estado = EstadoPersonaje.Caminando;
                    p.velocidad = Vector3.zero;
                    break;
            }

            // 2. Aplicar traslación a Transform mediante struct Access (Sincronizado atómicamente por Unity)
            transform.position = new Vector3(p.posicion.x, p.alturaY, p.posicion.z);
            
            // 3. Aplicar rotación (LookAt hacia donde andan)
            if (p.velocidad.sqrMagnitude > 0.0025f)
            {
                Vector3 flatH = new Vector3(p.velocidad.x, 0f, p.velocidad.z);
                transform.rotation = Quaternion.LookRotation(flatH, Vector3.up);
            }

            // Guarda el valor modificado hacia la memoria principal compartida
            personajes[i] = p;
        }

        private PersonajeData TickPatrulla(PersonajeData p)
        {
            NativeArray<Vector3> ruta = (p.tipo == TipoPersonaje.GuardiaCivil) ? rutaGC : rutaPF;
            
            if (!ruta.IsCreated || ruta.Length == 0) return TickCaminar(p); // Fallback

            int wp = p.waypointActual % ruta.Length;
            Vector3 destino = ruta[wp];
            Vector3 dir = destino - p.posicion; 
            dir.y = 0f;
            float dist = dir.magnitude;

            if (dist < 1.8f)
            {
                p.waypointActual = (p.waypointActual + 1) % ruta.Length;
                if (p.RndRange(0f, 1f) < 0.12f)
                {
                    p.estado       = EstadoPersonaje.Parado;
                    p.tiempoParado = p.RndRange(2f, 7f);
                    p.velocidad    = Vector3.zero;
                    return p;
                }
            }

            float speed = (p.tipo == TipoPersonaje.GuardiaCivil) ? velGC : velPF;
            p.velocidad = dir.normalized * speed;
            p.posicion += p.velocidad * dt;
            return p;
        }

        private PersonajeData TickCaminar(PersonajeData p)
        {
            if (p.tipo == TipoPersonaje.GuardiaCivil || p.tipo == TipoPersonaje.PoliciaForal)
            { 
                p.estado = EstadoPersonaje.Patrullando; 
                return p; 
            }

            if (p.tipo == TipoPersonaje.ManifestantePalestino ||
                p.tipo == TipoPersonaje.ManifestanteJurrutu   ||
                p.tipo == TipoPersonaje.PortadorIkurriña      ||
                p.tipo == TipoPersonaje.PortadorNavarra)
            {
                Vector3 dir = Vector3.right;
                float speed = velManif + p.RndRange(-0.1f, 0.1f);
                p.velocidad = dir * speed;
                p.posicion += p.velocidad * dt;

                if (p.posicion.x > posManifestantes.x + 200f)
                    p.posicion.x = posManifestantes.x - 5f;
                return p;
            }

            // Civiles
            Vector3 diff = p.destinoCivil - p.posicion; diff.y = 0f;
            if (diff.magnitude < 2f)
            {
                Vector2 rnd = p.RndInsideCircle() * extentsCiviles.x * 0.7f;
                p.destinoCivil = centroCiviles + new Vector3(rnd.x, 0f, rnd.y);

                if (p.RndRange(0f, 1f) < 0.30f)
                {
                    p.estado       = EstadoPersonaje.Parado;
                    p.tiempoParado = p.RndRange(1f, 9f);
                    p.velocidad    = Vector3.zero;
                    return p;
                }
            }

            p.velocidad = diff.normalized * velCivil;
            p.posicion += p.velocidad * dt;

            // Clamping the civil area manually (Mathf.Clamp works in Burst/Jobs)
            float cx = Mathf.Clamp(p.posicion.x, centroCiviles.x - extentsCiviles.x, centroCiviles.x + extentsCiviles.x);
            float cz = Mathf.Clamp(p.posicion.z, centroCiviles.z - extentsCiviles.z, centroCiviles.z + extentsCiviles.z);
            if (cx != p.posicion.x || cz != p.posicion.z)
            {
                p.posicion.x = cx;
                p.posicion.z = cz;
                p.destinoCivil = centroCiviles;
            }

            return p;
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  SPAWN Y RUTAS (MEMORIA PRINCIPAL)
    // ───────────────────────────────────────────────────────────────────────

    private void PrepararRutasNativas()
    {
        if (rutaGuardiaCivil != null && rutaGuardiaCivil.Length > 0)
        {
            _rutaGCNative = new NativeArray<Vector3>(rutaGuardiaCivil.Length, Allocator.Persistent);
            for (int i = 0; i < rutaGuardiaCivil.Length; i++) _rutaGCNative[i] = rutaGuardiaCivil[i].position;
        }
        else
            _rutaGCNative = new NativeArray<Vector3>(0, Allocator.Persistent);

        if (rutaPoliciaForal != null && rutaPoliciaForal.Length > 0)
        {
            _rutaPFNative = new NativeArray<Vector3>(rutaPoliciaForal.Length, Allocator.Persistent);
            for (int i = 0; i < rutaPoliciaForal.Length; i++) _rutaPFNative[i] = rutaPoliciaForal[i].position;
        }
        else
            _rutaPFNative = new NativeArray<Vector3>(0, Allocator.Persistent);
    }

    private void SpawnTodos()
    {
        _totalInstancias = numGuardiaCivil + numPoliciaForal
                         + numManifestantesPales + numManifestantesJurrutu
                         + numPortadoresIkurriña + numPortadoresNavarra
                         + numCiviles;

        if (_totalInstancias == 0) return;

        // Allocator.Persistent: Esta memoria sobrevive múltiples frames y debe ser destruida manualmente
        _personajesNative = new NativeArray<PersonajeData>(_totalInstancias, Allocator.Persistent);
        _transformAccess  = new TransformAccessArray(_totalInstancias);
        _goPersonajes     = new GameObject[_totalInstancias];

        int idx = 0;
        
        Vector3 origenGC = _rutaGCNative.Length > 0 ? _rutaGCNative[0] : posicionManifestantes + new Vector3(100f, 0f, 0f);
        idx = Spawn(idx, numGuardiaCivil, TipoPersonaje.GuardiaCivil, origenGC, 6f, EstadoPersonaje.Patrullando);

        Vector3 origenPF = _rutaPFNative.Length > 0 ? _rutaPFNative[0] : posicionManifestantes + new Vector3(120f, 0f, 0f);
        idx = Spawn(idx, numPoliciaForal, TipoPersonaje.PoliciaForal, origenPF, 6f, EstadoPersonaje.Patrullando);

        idx = Spawn(idx, numManifestantesPales,   TipoPersonaje.ManifestantePalestino, posicionManifestantes + new Vector3(20f, 0f,  10f), 14f, EstadoPersonaje.Caminando);
        idx = Spawn(idx, numManifestantesJurrutu, TipoPersonaje.ManifestanteJurrutu,   posicionManifestantes + new Vector3(30f, 0f, -10f), 12f, EstadoPersonaje.Caminando);
        idx = Spawn(idx, numPortadoresIkurriña,   TipoPersonaje.PortadorIkurriña,      posicionManifestantes + new Vector3(5f,  0f,   8f),  8f, EstadoPersonaje.Caminando);
        idx = Spawn(idx, numPortadoresNavarra,    TipoPersonaje.PortadorNavarra,       posicionManifestantes + new Vector3(5f,  0f,  -8f),  6f, EstadoPersonaje.Caminando);
        
        int civilMale   = numCiviles / 2;
        int civilFemale = numCiviles - civilMale;
        idx = Spawn(idx, civilMale,   TipoPersonaje.CivilianMale,   areaCiviles.center, areaCiviles.extents.x * 0.8f, EstadoPersonaje.Caminando);
        idx = Spawn(idx, civilFemale, TipoPersonaje.CivilianFemale, areaCiviles.center, areaCiviles.extents.x * 0.8f, EstadoPersonaje.Caminando);
    }

    private int Spawn(int baseIdx, int count, TipoPersonaje tipo, Vector3 centro, float radio, EstadoPersonaje estadoInicial)
    {
        for (int i = 0; i < count; i++)
        {
            Vector2 o = Random.insideUnitCircle * radio;
            Vector3 pos = centro + new Vector3(o.x, 0f, o.y);
            float variante = Random.value;

            // Semilla determinista vital > 0 para evitar entropía estática
            uint seed = (uint)(baseIdx + i + 1000) * 1103515245;

            _personajesNative[baseIdx + i] = new PersonajeData
            {
                posicion       = pos,
                velocidad      = Vector3.zero,
                alturaY        = pos.y,
                tipo           = tipo,
                estado         = estadoInicial,
                waypointActual = Random.Range(0, 100),
                tiempoParado   = 0f,
                varianteTono   = variante,
                destinoCivil   = pos,
                rndState       = seed
            };

            var go = CrearGO(pos, tipo, baseIdx + i, variante);
            _goPersonajes[baseIdx + i] = go;
            
            // Inyectamos el root para la manipulación multi-hilo en la jerarquía Transform
            _transformAccess.Add(go.transform);
        }
        return baseIdx + count;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  RESTO DE LA LOGICA MAIN THREAD (SIN CAMBIOS, PROCEDURAL GENERATION)
    // ───────────────────────────────────────────────────────────────────────

    private void SampleTerreno(int idx)
    {
        var p = _personajesNative[idx];
        Ray ray = new Ray(new Vector3(p.posicion.x, p.alturaY + 8f, p.posicion.z), Vector3.down);
        if (Physics.Raycast(ray, out RaycastHit hit, 25f, ~0))
        {
            p.alturaY = Mathf.Lerp(p.alturaY, hit.point.y, 0.25f);
            _personajesNative[idx] = p; // Re-asignar a NativeArray (solo main thread es seguro aquí entre complete y schedule)
        }
    }

    private GameObject CrearGO(Vector3 pos, TipoPersonaje tipo, int idx, float variante)
    {
        var go = new GameObject($"Personaje_{tipo}_{idx}");
        go.transform.position = pos;
        go.transform.SetParent(transform);
        go.layer = LayerMask.NameToLayer("Default");

        var mf = go.AddComponent<MeshFilter>();
        mf.sharedMesh = _meshCuerpo;
        var mr = go.AddComponent<MeshRenderer>();
        mr.sharedMaterial = _matsBase[(int)tipo];

        var mpb = new MaterialPropertyBlock();
        mr.GetPropertyBlock(mpb);
        Color bc = ColorBase(tipo);
        float dt = variante * 0.18f - 0.09f;
        mpb.SetColor(_idBaseColor, new Color(
            Mathf.Clamp01(bc.r + dt),
            Mathf.Clamp01(bc.g + dt),
            Mathf.Clamp01(bc.b + dt), 1f));
        mr.SetPropertyBlock(mpb);

        switch (tipo)
        {
            case TipoPersonaje.GuardiaCivil:          AñadirTricornio(go);              break;
            case TipoPersonaje.PoliciaForal:          AñadirCasco(go);                  break;
            case TipoPersonaje.ManifestantePalestino: AñadirKeffiyeh(go);               break;
            case TipoPersonaje.ManifestanteJurrutu:   AñadirBoina(go);                  break;
            case TipoPersonaje.PortadorIkurriña:      AñadirBandera(go, _texIkurriña);  break;
            case TipoPersonaje.PortadorNavarra:       AñadirBandera(go, _texNavarra);   break;
        }

        return go;
    }

    private void AñadirBandera(GameObject portador, Texture2D tex)
    {
        var mastil = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        mastil.name = "Mastil";
        mastil.transform.SetParent(portador.transform);
        mastil.transform.localPosition = new Vector3(0.3f, 1.1f, 0f);
        mastil.transform.localScale    = new Vector3(0.04f, 1.3f, 0.04f);
        mastil.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.55f, 0.42f, 0.18f));
        Object.Destroy(mastil.GetComponent<Collider>());

        var tela = new GameObject("Bandera");
        tela.transform.SetParent(portador.transform);
        tela.transform.localPosition = new Vector3(0.3f, 2.22f, 0f);
        tela.transform.localScale    = Vector3.one;
        var mfT = tela.AddComponent<MeshFilter>();
        mfT.sharedMesh = _meshBandera;
        var mrT = tela.AddComponent<MeshRenderer>();
        var matB = MatURP(Color.white);
        if (tex != null) matB.SetTexture(_idBaseMap, tex);
        mrT.sharedMaterial = matB;
    }

    private void AñadirKeffiyeh(GameObject portador)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.name = "Keffiyeh";
        go.transform.SetParent(portador.transform);
        go.transform.localPosition = new Vector3(0f, 1.74f, 0f);
        go.transform.localScale    = new Vector3(0.33f, 0.22f, 0.40f);
        var mat = MatURP(Color.white);
        if (_texKeffiyeh != null) mat.SetTexture(_idBaseMap, _texKeffiyeh);
        go.GetComponent<Renderer>().sharedMaterial = mat;
        Object.Destroy(go.GetComponent<Collider>());
    }

    private void AñadirBoina(GameObject portador)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        go.name = "Boina";
        go.transform.SetParent(portador.transform);
        go.transform.localPosition = new Vector3(0.05f, 1.76f, 0f);
        go.transform.localScale    = new Vector3(0.30f, 0.05f, 0.28f);
        go.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.04f, 0.04f, 0.04f));
        Object.Destroy(go.GetComponent<Collider>());
    }

    private void AñadirTricornio(GameObject portador)
    {
        var ala = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        ala.name = "TricornioAla";
        ala.transform.SetParent(portador.transform);
        ala.transform.localPosition = new Vector3(0f, 1.75f, 0f);
        ala.transform.localScale    = new Vector3(0.36f, 0.04f, 0.36f);
        ala.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.06f, 0.06f, 0.06f));
        Object.Destroy(ala.GetComponent<Collider>());

        var copa = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        copa.name = "TricornioCopa";
        copa.transform.SetParent(portador.transform);
        copa.transform.localPosition = new Vector3(0f, 1.84f, 0f);
        copa.transform.localScale    = new Vector3(0.22f, 0.09f, 0.22f);
        copa.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.06f, 0.06f, 0.06f));
        Object.Destroy(copa.GetComponent<Collider>());
    }

    private void AñadirCasco(GameObject portador)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.name = "CascoPoliciaForal";
        go.transform.SetParent(portador.transform);
        go.transform.localPosition = new Vector3(0f, 1.79f, 0f);
        go.transform.localScale    = new Vector3(0.32f, 0.27f, 0.32f);
        go.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.06f, 0.10f, 0.22f));
        Object.Destroy(go.GetComponent<Collider>());

        var franja = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        franja.name = "FranjaHivis";
        franja.transform.SetParent(portador.transform);
        franja.transform.localPosition = new Vector3(0f, 1.40f, 0f);
        franja.transform.localScale    = new Vector3(0.30f, 0.025f, 0.30f);
        franja.GetComponent<Renderer>().sharedMaterial = MatURP(new Color(0.95f, 0.85f, 0.0f));
        Object.Destroy(franja.GetComponent<Collider>());
    }

    private void CrearTexturas()
    {
        _texIkurriña = GenIkurriña();   _texsCreadas.Add(_texIkurriña);
        _texNavarra  = GenNavarra();    _texsCreadas.Add(_texNavarra);
        _texKeffiyeh = GenKeffiyeh();   _texsCreadas.Add(_texKeffiyeh);
    }

    private static Texture2D GenIkurriña()
    {
        const int W = 256, H = 160;
        var tex = new Texture2D(W, H, TextureFormat.RGBA32, false);
        var px   = new Color32[W * H];
        Color32 verde  = new Color32(0,   130,  60,  255);
        Color32 blanco = new Color32(255, 255, 255,  255);
        Color32 rojo   = new Color32(210,  30,  30,  255);
        for (int i = 0; i < px.Length; i++) px[i] = verde;
        for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            float fx = (float)x / W, fy = (float)y / H;
            if (Mathf.Abs(fy - fx * (float)H / W)               < 0.09f * H ||
                Mathf.Abs(fy - (1f - fx) * (float)H / W)        < 0.09f * H)
                px[y * W + x] = blanco;
        }
        int bandaV = (int)(W * 0.05f), bandaH = (int)(H * 0.08f);
        for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            if (Mathf.Abs(x - W / 2) < bandaV || Mathf.Abs(y - H / 2) < bandaH) px[y * W + x] = rojo;
        tex.SetPixels32(px); tex.Apply(false, false);
        return tex;
    }

    private static Texture2D GenNavarra()
    {
        const int W = 256, H = 160;
        var tex = new Texture2D(W, H, TextureFormat.RGBA32, false);
        var px   = new Color32[W * H];
        Color32 rojoN  = new Color32(175, 15,  30,  255);
        Color32 dorado = new Color32(210, 165, 30,  255);
        for (int i = 0; i < px.Length; i++) px[i] = rojoN;
        int paso = 20, radio = 5;
        for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            int rx = x - W / 2, ry = y - H / 2;
            if (Mathf.Abs(rx) > W / 3 || Mathf.Abs(ry) > H / 3) continue;
            int gx = ((rx + ry) / 2 + 200) % paso, gy = ((rx - ry) / 2 + 200) % paso;
            int cx = paso / 2, cy = paso / 2;
            float dd = Mathf.Sqrt((gx - cx) * (gx - cx) + (gy - cy) * (gy - cy));
            if (dd < radio || (dd > radio + 1 && dd < radio + 3)) px[y * W + x] = dorado;
        }
        tex.SetPixels32(px); tex.Apply(false, false);
        return tex;
    }

    private static Texture2D GenKeffiyeh()
    {
        const int W = 64, H = 64;
        var tex = new Texture2D(W, H, TextureFormat.RGBA32, false) { filterMode = FilterMode.Point, wrapMode = TextureWrapMode.Repeat };
        var px         = new Color32[W * H];
        Color32 blanco = new Color32(240, 235, 228, 255);
        Color32 negro  = new Color32( 22,  22,  22, 255);
        Color32 rojo2  = new Color32(190,  30,  30, 255);
        const int cuad = 8;
        for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
        {
            bool bX = (x / cuad) % 2 == 0, bY = (y / cuad) % 2 == 0;
            if (bX == bY) px[y * W + x] = blanco;
            else if ((x + y) % 3 == 0) px[y * W + x] = rojo2;
            else px[y * W + x] = negro;
        }
        tex.SetPixels32(px); tex.Apply(false, false);
        return tex;
    }

    private void CrearMeshes()
    {
        _meshCuerpo  = BuildCapsule(0.22f, 1.80f, 10, 6);
        _meshBandera = BuildBanderaQuad(0.60f, 1.20f);
        _meshesCreados.Add(_meshCuerpo);
        _meshesCreados.Add(_meshBandera);
    }

    private static Mesh BuildCapsule(float r, float h, int seg, int rings)
    {
        var mesh  = new Mesh { name = "PersonajeCapsule" };
        var verts = new List<Vector3>(); var norms = new List<Vector3>();
        var uvs   = new List<Vector2>(); var tris  = new List<int>();
        float halfH = h / 2f - r;
        for (int ri = 0; ri <= rings; ri++)
        {
            float t = (float)ri / rings, y = Mathf.Lerp(-halfH, halfH, t);
            for (int si = 0; si <= seg; si++)
            {
                float a = si * Mathf.PI * 2f / seg, x = Mathf.Cos(a) * r, z = Mathf.Sin(a) * r;
                verts.Add(new Vector3(x, y, z)); norms.Add(new Vector3(x, 0f, z).normalized);
                uvs.Add(new Vector2((float)si / seg, t * 0.6f));
            }
        }
        for (int ri = 0; ri < rings; ri++)
        for (int si = 0; si < seg;   si++)
        {
            int b = ri * (seg + 1) + si;
            tris.Add(b); tris.Add(b + seg + 1); tris.Add(b + 1);
            tris.Add(b + 1); tris.Add(b + seg + 1); tris.Add(b + seg + 2);
        }
        AddHemi(verts, norms, uvs, tris,  halfH, r, seg, 5, false);
        AddHemi(verts, norms, uvs, tris, -halfH, r, seg, 5, true);
        mesh.SetVertices(verts); mesh.SetNormals(norms); mesh.SetUVs(0, uvs);
        mesh.SetTriangles(tris, 0); mesh.RecalculateBounds();
        return mesh;
    }

    private static void AddHemi(List<Vector3> v, List<Vector3> n, List<Vector2> uv, List<int> t, float baseY, float r, int seg, int rings, bool flip)
    {
        int bv = v.Count; float signY = flip ? -1f : 1f;
        for (int ri = 0; ri <= rings; ri++)
        {
            float phi = (float)ri / rings * (Mathf.PI / 2f), py  = signY * Mathf.Sin(phi) * r + baseY, pr  = Mathf.Cos(phi) * r;
            for (int si = 0; si <= seg; si++)
            {
                float a = si * Mathf.PI * 2f / seg, x = Mathf.Cos(a) * pr, z = Mathf.Sin(a) * pr;
                v.Add(new Vector3(x, py, z)); n.Add(new Vector3(x, signY * Mathf.Sin(phi), z).normalized);
                uv.Add(new Vector2((float)si / seg, flip ? 0f : 1f));
            }
        }
        for (int ri = 0; ri < rings; ri++)
        for (int si = 0; si < seg;   si++)
        {
            int b = bv + ri * (seg + 1) + si;
            if (flip) { t.Add(b); t.Add(b + 1); t.Add(b + seg + 1); t.Add(b + 1); t.Add(b + seg + 2); t.Add(b + seg + 1); }
            else      { t.Add(b); t.Add(b + seg + 1); t.Add(b + 1); t.Add(b + 1); t.Add(b + seg + 1); t.Add(b + seg + 2); }
        }
    }

    private static Mesh BuildBanderaQuad(float W, float H)
    {
        var mesh = new Mesh { name = "BanderaQuad" };
        mesh.vertices = new[] { new Vector3(0, 0, 0), new Vector3(W, 0, 0), new Vector3(W, H, 0), new Vector3(0, H, 0), new Vector3(W, 0, 0), new Vector3(0, 0, 0), new Vector3(0, H, 0), new Vector3(W, H, 0) };
        mesh.uv = new[] { new Vector2(0,0), new Vector2(1,0), new Vector2(1,1), new Vector2(0,1), new Vector2(1,0), new Vector2(0,0), new Vector2(0,1), new Vector2(1,1) };
        mesh.triangles = new[] { 0,1,2, 0,2,3, 4,5,6, 4,6,7 };
        mesh.RecalculateNormals(); mesh.RecalculateBounds();
        return mesh;
    }

    private void CrearMaterialesBase()
    {
        var tipos = (TipoPersonaje[])System.Enum.GetValues(typeof(TipoPersonaje));
        _matsBase = new Material[tipos.Length];
        for (int i = 0; i < tipos.Length; i++) _matsBase[i] = MatURP(ColorBase(tipos[i]));
    }

    private static Color ColorBase(TipoPersonaje t) => t switch
    {
        TipoPersonaje.GuardiaCivil          => new Color(0.50f, 0.53f, 0.28f),
        TipoPersonaje.PoliciaForal          => new Color(0.10f, 0.14f, 0.34f),
        TipoPersonaje.ManifestantePalestino => new Color(0.18f, 0.18f, 0.20f),
        TipoPersonaje.ManifestanteJurrutu   => new Color(0.06f, 0.06f, 0.06f),
        TipoPersonaje.PortadorIkurriña      => new Color(0.12f, 0.12f, 0.14f),
        TipoPersonaje.PortadorNavarra       => new Color(0.12f, 0.12f, 0.14f),
        TipoPersonaje.CivilianMale          => new Color(0.52f, 0.46f, 0.40f),
        TipoPersonaje.CivilianFemale        => new Color(0.60f, 0.42f, 0.48f),
        _                                   => Color.gray,
    };

    private Material MatURP(Color col)
    {
        var sh = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Universal Render Pipeline/Unlit") ?? Shader.Find("Standard") ?? Shader.Find("Hidden/InternalErrorShader");
        var mat = new Material(sh) { color = col };
        _matsCreados.Add(mat);
        return mat;
    }

    // ───────────────────────────────────────────────────────────────────────
    //  API PÚBLICA
    // ───────────────────────────────────────────────────────────────────────
    public Vector3[] ObtenerPosiciones(TipoPersonaje tipo)
    {
        _movimientoJobHandle.Complete(); // Asegurar datos recientes
        if (!_personajesNative.IsCreated) return System.Array.Empty<Vector3>();
        var lista = new List<Vector3>(_totalInstancias / 4);
        for (int i = 0; i < _totalInstancias; i++)
            if (_personajesNative[i].tipo == tipo)
                lista.Add(new Vector3(_personajesNative[i].posicion.x, _personajesNative[i].alturaY, _personajesNative[i].posicion.z));
        return lista.ToArray();
    }

    public void AsignarRutas(Transform[] rutaGC, Transform[] rutaPF)
    {
        _movimientoJobHandle.Complete();
        
        rutaGuardiaCivil = rutaGC;
        rutaPoliciaForal = rutaPF;

        if (_rutaGCNative.IsCreated) _rutaGCNative.Dispose();
        if (_rutaPFNative.IsCreated) _rutaPFNative.Dispose();
        
        PrepararRutasNativas();

        if (!_personajesNative.IsCreated) return;
        for (int i = 0; i < _totalInstancias; i++)
        {
            var p = _personajesNative[i];
            if (p.tipo == TipoPersonaje.GuardiaCivil || p.tipo == TipoPersonaje.PoliciaForal)
            {
                p.waypointActual = 0;
                _personajesNative[i] = p;
            }
        }
    }

    private void OnAlertaCambiada(bool activo)
    {
        if (!activo) return;
        _movimientoJobHandle.Complete();

        if (!_personajesNative.IsCreated) return;
        for (int i = 0; i < _totalInstancias; i++)
        {
            var p = _personajesNative[i];
            if (p.tipo != TipoPersonaje.GuardiaCivil && p.tipo != TipoPersonaje.PoliciaForal) continue;
            p.estado       = EstadoPersonaje.Patrullando;
            p.tiempoParado = 0f;
            _personajesNative[i] = p;
        }
    }

    public int TotalPersonajes => _totalInstancias;
}
