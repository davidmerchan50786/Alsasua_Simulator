// Assets/Scripts/SistemaTrafico.cs
// Simulación de tráfico rodado en autovías y calles de Alsasua.
//
// ── ARQUITECTURA (DATA-ORIENTED / JOBS) ───────────────────────────────────────
//  • Migrado a IJobParallelFor usando Double Buffering (Lectura/Escritura).
//  • Waypoints aplanados en NativeArrays para acceso desde Worker Threads.
//  • Complejidad O(N^2) distribuida en múltiples núcleos = rendimiento masivo.
//
// ── SETUP EN EDITOR ───────────────────────────────────────────────────────────
//  1. Crear un GameObject "SistemaTrafico" y añadir este componente.
//  2. Asignar carriles (waypoints).

using UnityEngine;
using System.Collections.Generic;
using Unity.Collections;
using Unity.Jobs;
using UnityEngine.Jobs;

[AddComponentMenu("Alsasua/Sistema Trafico")]
public sealed class SistemaTrafico : MonoBehaviour
{
    private enum EstadoVehiculo : byte
    {
        Circulando,   
        Frenando,     
        Parado,       
    }

    [System.Serializable]
    public sealed class Carril
    {
        public Transform[] waypoints;
        [Range(20f, 140f)] public float velocidadMaxKmh = 80f;
        public bool esAutovia = false;
    }

    private struct VehiculoData
    {
        public Vector3        posicion;
        public Vector3        velocidad;
        public int            carrilIdx;
        public int            waypointActual;
        public EstadoVehiculo estado;
        public float          alturaY;
        public float          velocidadMax;       
        public Color          colorCarroceria;
    }

    private struct RangoCarril
    {
        public int inicio;
        public int longitud;
    }

    [Header("═══ CARRILES ═══")]
    [SerializeField] private Carril[] carriles;

    [Header("═══ VEHÍCULOS ═══")]
    [Range(5, 80)] [SerializeField] private int totalVehiculos = 40;
    [Range(20f, 50f)] [SerializeField] private float distanciaSeguridad = 12f;
    [Range(0.3f, 1.0f)] [SerializeField] private float variacionVelocidad = 0.35f;

    [Header("═══ VISUAL (pool) ═══")]
    [Range(5, 30)] [SerializeField] private int maxObjetosVisuales = 18;
    [Range(30f, 200f)] [SerializeField] private float radioPoolVisual = 80f;
    [SerializeField] private bool mostrarGizmos = true;

    [Header("═══ GRÁFICOS (MYASSETS) ═══")]
    [Tooltip("Asignar el Prefab del coche (SICS Police Car). Si se omite, usa cubos renderizados instanciados.")]
    [SerializeField] private GameObject prefabVehiculo;

    // Estado Interno Nativos
    private int            _numVehiculos;
    private NativeArray<VehiculoData> _vehiculosWrite;
    private NativeArray<VehiculoData> _vehiculosRead;
    private NativeArray<Vector3>      _waypointsNative;
    private NativeArray<RangoCarril>  _rangosCarril;
    private JobHandle _jobHandle;

    // Visual
    private GameObject[]   _poolGO;
    private Renderer[]     _poolRenderers;       
    private bool[]         _poolEnUso;
    private int[]          _vehiculoAGO;         
    private int[]          _goAVehiculo;         

    private Mesh           _meshCoche;
    private Material       _matVehiculo;
    private Matrix4x4[]    _matrices;
    private Vector4[]      _colores;
    private Matrix4x4[]    _matrizLote;
    private List<Vector4>  _colorListaLote;    
    private const int      MAX_LOTE = 1023;
    private MaterialPropertyBlock _propBlock;
    private static readonly int _idBaseColor = Shader.PropertyToID("_BaseColor");
    private List<Material> _matsCreados = new List<Material>();

    private Transform _jugadorTr;
    private int   _indiceRaycast;

    private static readonly Color[] PALETA_COCHES = new Color[]
    {
        new Color(0.90f, 0.90f, 0.90f),  
        new Color(0.15f, 0.15f, 0.15f),  
        new Color(0.55f, 0.55f, 0.60f),  
        new Color(0.08f, 0.18f, 0.45f),  
        new Color(0.65f, 0.08f, 0.08f),  
        new Color(0.42f, 0.35f, 0.28f),  
        new Color(0.08f, 0.28f, 0.10f),  
    };

    private void Awake()
    {
        if (carriles == null || carriles.Length == 0)
        {
            Debug.LogWarning("[SistemaTrafico] Sin carriles definidos.");
            enabled = false; return;
        }

#if UNITY_EDITOR
        // V4 AUTO-ASSIGN: Autodescubrir vehículo policial inyectado por MyAssets
        if (prefabVehiculo == null)
        {
            string[] guids = UnityEditor.AssetDatabase.FindAssets("t:Prefab Police");
            if (guids.Length > 0)
                prefabVehiculo = UnityEditor.AssetDatabase.LoadAssetAtPath<GameObject>(UnityEditor.AssetDatabase.GUIDToAssetPath(guids[0]));
        }
#endif

        _numVehiculos = Mathf.Clamp(totalVehiculos, 1, 200);
        _propBlock    = new MaterialPropertyBlock();

        _meshCoche  = CrearMeshCoche();
        _matVehiculo = CrearMaterialVehiculo();

        PreallocarEstructuras();
        CrearPoolVisual();
        CachearJugador();
    }

    private void Update()
    {
        // 1. Completion of previous frame's job
        _jobHandle.Complete();

        if (!_vehiculosWrite.IsCreated) return;

        // 2. Main thread tasks (Raycasts)
        MuestrearSuelo();

        // 3. Update Sync to Visuals (Reading from _vehiculosWrite)
        SincronizarPoolVisual();
        RenderizarVehiculosLejanos();

        // 4. Double buffer copy for next logic execution
        _vehiculosWrite.CopyTo(_vehiculosRead);

        // 5. Schedule Next Frame's Job
        var job = new TraficoUpdateJob
        {
            dt = Time.deltaTime,
            distanciaSeguridad = distanciaSeguridad,
            vehiculosLeidos = _vehiculosRead,
            vehiculosEscritos = _vehiculosWrite,
            waypoints = _waypointsNative,
            carrilesInfo = _rangosCarril
        };

        _jobHandle = job.Schedule(_numVehiculos, 8); // chunk size of 8
        JobHandle.ScheduleBatchedJobs();
    }

    private void OnDisable()
    {
        // Ensure no outstanding jobs
        _jobHandle.Complete();
    }

    private void OnDestroy()
    {
        _jobHandle.Complete();
        if (_vehiculosWrite.IsCreated) _vehiculosWrite.Dispose();
        if (_vehiculosRead.IsCreated) _vehiculosRead.Dispose();
        if (_waypointsNative.IsCreated) _waypointsNative.Dispose();
        if (_rangosCarril.IsCreated) _rangosCarril.Dispose();

        foreach (var m in _matsCreados) if (m != null) Object.Destroy(m);
        _matsCreados.Clear();
        if (_meshCoche != null) Object.Destroy(_meshCoche);
    }

    // ── JOB ───────────────────────────────────────────────────────────────────

    // Computes traffic physics and flocking distance using O(N^2) safely parallelized
    private struct TraficoUpdateJob : IJobParallelFor
    {
        public float dt;
        public float distanciaSeguridad;
        
        [ReadOnly] public NativeArray<Vector3> waypoints;
        [ReadOnly] public NativeArray<RangoCarril> carrilesInfo;
        [ReadOnly] public NativeArray<VehiculoData> vehiculosLeidos;
        
        // Write destination
        public NativeArray<VehiculoData> vehiculosEscritos;

        public void Execute(int i)
        {
            VehiculoData v = vehiculosLeidos[i];
            if (v.estado == EstadoVehiculo.Parado) 
            {
                vehiculosEscritos[i] = v;
                return;
            }

            RangoCarril carril = carrilesInfo[v.carrilIdx];
            if (carril.longitud < 2) 
            {
                vehiculosEscritos[i] = v;
                return;
            }

            // 1. Target Waypoint
            int targetIdx = carril.inicio + v.waypointActual;
            Vector3 target = waypoints[targetIdx];
            target.y = v.posicion.y;

            Vector3 dirTarget = target - v.posicion;
            float   distTarget = dirTarget.magnitude;

            if (distTarget < 5f)
            {
                v.waypointActual = (v.waypointActual + 1) % carril.longitud;
            }

            // 2. Collision checking / Braking
            float distanciaAlFrente = float.MaxValue;
            if (v.velocidad.sqrMagnitude >= 0.01f)
            {
                Vector3 velNorm = v.velocidad.normalized;
                for (int j = 0; j < vehiculosLeidos.Length; j++)
                {
                    if (j == i) continue;
                    VehiculoData otro = vehiculosLeidos[j];
                    if (otro.carrilIdx != v.carrilIdx) continue;

                    Vector3 diff = otro.posicion - v.posicion;
                    diff.y = 0f;
                    float dist = diff.magnitude;

                    if (dist >= distanciaAlFrente) continue;

                    float dot = Vector3.Dot(diff / dist, velNorm);
                    if (dot > 0.7f)
                    {
                        distanciaAlFrente = dist;
                        if (distanciaAlFrente <= distanciaSeguridad * 0.6f) break;
                    }
                }
            }

            if (distanciaAlFrente < distanciaSeguridad * 0.6f) v.estado = EstadoVehiculo.Frenando;
            else if (distanciaAlFrente > distanciaSeguridad) v.estado = EstadoVehiculo.Circulando;

            // 3. Apply velocity & ease-in
            float velObjetivo = (v.estado == EstadoVehiculo.Frenando)
                ? Mathf.Max(0f, v.velocidadMax * (distanciaAlFrente / distanciaSeguridad - 0.4f))
                : v.velocidadMax;

            Vector3 dirMovimiento = (distTarget > 0.1f) ? (dirTarget / distTarget) : v.velocidad.normalized;
            Vector3 velObjetivoVec = dirMovimiento * velObjetivo;

            float velActual  = v.velocidad.magnitude;
            float factorAcel = (velObjetivo > velActual) 
                ? Mathf.Lerp(0.5f, 2.5f, Mathf.Clamp01(velActual / Mathf.Max(v.velocidadMax * 0.4f, 0.1f))) 
                : 6.0f;
            
            v.velocidad   = Vector3.MoveTowards(v.velocidad, velObjetivoVec, factorAcel * dt);
            v.posicion   += v.velocidad * dt;
            v.posicion.y  = v.alturaY;

            vehiculosEscritos[i] = v;
        }
    }

    private Mesh CrearMeshCoche()
    {
        if (prefabVehiculo != null)
        {
            var filter = prefabVehiculo.GetComponentInChildren<MeshFilter>();
            if (filter != null && filter.sharedMesh != null) return filter.sharedMesh;
        }
        var temp = GameObject.CreatePrimitive(PrimitiveType.Cube);
        var mesh = Object.Instantiate(temp.GetComponent<MeshFilter>().sharedMesh);
        Object.DestroyImmediate(temp);
        return mesh;
    }

    private Material CrearMaterialVehiculo()
    {
        if (prefabVehiculo != null)
        {
            var r = prefabVehiculo.GetComponentInChildren<Renderer>();
            if (r != null && r.sharedMaterial != null)
            {
                var matPrefab = new Material(r.sharedMaterial);
                matPrefab.enableInstancing = true;
                _matsCreados.Add(matPrefab);
                return matPrefab;
            }
        }
        var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
        var mat = new Material(shader) { enableInstancing = true };
        _matsCreados.Add(mat);
        return mat;
    }

    private void PreallocarEstructuras()
    {
        _vehiculosWrite = new NativeArray<VehiculoData>(_numVehiculos, Allocator.Persistent);
        _vehiculosRead  = new NativeArray<VehiculoData>(_numVehiculos, Allocator.Persistent);

        // Aplanar waypoints
        int totalWps = 0;
        foreach(var c in carriles) if (c?.waypoints != null) totalWps += c.waypoints.Length;

        _waypointsNative = new NativeArray<Vector3>(totalWps, Allocator.Persistent);
        _rangosCarril    = new NativeArray<RangoCarril>(carriles.Length, Allocator.Persistent);

        int offset = 0;
        for (int c = 0; c < carriles.Length; ++c)
        {
            var wps = carriles[c]?.waypoints;
            if (wps != null)
            {
                for(int w=0; w<wps.Length; ++w) _waypointsNative[offset+w] = wps[w] != null ? wps[w].position : Vector3.zero;
                _rangosCarril[c] = new RangoCarril { inicio = offset, longitud = wps.Length };
                offset += wps.Length;
            }
            else
            {
                _rangosCarril[c] = new RangoCarril { inicio = 0, longitud = 0 };
            }
        }

        // Initialize vehicles
        for (int i = 0; i < _numVehiculos; i++)
        {
            int carrilIdx = i % carriles.Length;
            RangoCarril rc = _rangosCarril[carrilIdx];

            if (rc.longitud < 2)
            {
                _vehiculosWrite[i] = new VehiculoData { estado = EstadoVehiculo.Parado, carrilIdx = carrilIdx };
                continue;
            }

            int wpIdx = (i / carriles.Length) % (rc.longitud - 1);
            float t   = (float)((i / carriles.Length) % 4) / 4f;

            Vector3 w0 = _waypointsNative[rc.inicio + wpIdx];
            Vector3 w1 = _waypointsNative[rc.inicio + ((wpIdx + 1) % rc.longitud)];
            Vector3 posBase = Vector3.Lerp(w0, w1, t);

            float velMaxMs = (carriles[carrilIdx].velocidadMaxKmh / 3.6f) * (1f + Random.Range(-variacionVelocidad, variacionVelocidad));
            Vector3 dir = (w1 - posBase).normalized;

            _vehiculosWrite[i] = new VehiculoData
            {
                posicion       = posBase,
                velocidad      = dir * velMaxMs,
                carrilIdx      = carrilIdx,
                waypointActual = wpIdx,
                estado         = EstadoVehiculo.Circulando,
                alturaY        = posBase.y,
                velocidadMax   = velMaxMs,
                colorCarroceria = PALETA_COCHES[Random.Range(0, PALETA_COCHES.Length)],
            };
        }

        _matrices       = new Matrix4x4[_numVehiculos];
        _colores        = new Vector4[_numVehiculos];
        _matrizLote     = new Matrix4x4[MAX_LOTE];
        _colorListaLote = new List<Vector4>(MAX_LOTE);  
        _vehiculoAGO    = new int[_numVehiculos];
        for (int i = 0; i < _numVehiculos; i++) _vehiculoAGO[i] = -1;
    }

    private void CrearPoolVisual()
    {
        _poolGO        = new GameObject[maxObjetosVisuales];
        _poolRenderers = new Renderer[maxObjetosVisuales];   
        _poolEnUso     = new bool[maxObjetosVisuales];
        _goAVehiculo   = new int[maxObjetosVisuales];
        for (int i = 0; i < maxObjetosVisuales; i++) _goAVehiculo[i] = -1;

        for (int i = 0; i < maxObjetosVisuales; i++)
        {
            GameObject go;
            if (prefabVehiculo != null)
            {
                go = Instantiate(prefabVehiculo);
            }
            else
            {
                go = GameObject.CreatePrimitive(PrimitiveType.Cube);
                go.transform.localScale = new Vector3(1.8f, 0.75f, 4.2f);
                var col = go.GetComponent<Collider>();
                if (col != null) Object.Destroy(col);
            }
            go.name = $"Vehiculo_Pool_{i:D2}";
            go.transform.SetParent(transform);
            go.SetActive(false);

            var rend = go.GetComponentInChildren<Renderer>();
            if (rend != null && prefabVehiculo == null)
            {
                rend.sharedMaterial = _matVehiculo;
            }
            
            _poolRenderers[i]   = rend;
            _poolGO[i]          = go;
        }
    }

    private void CachearJugador()
    {
        var ctrl = Object.FindFirstObjectByType<ControladorJugador>();
        if (ctrl != null) _jugadorTr = ctrl.transform;
    }

    private void MuestrearSuelo()
    {
        if (_numVehiculos == 0) return;
        _indiceRaycast = (_indiceRaycast + 1) % _numVehiculos;
        
        VehiculoData v = _vehiculosWrite[_indiceRaycast];
        if (Physics.Raycast(v.posicion + Vector3.up * 8f, Vector3.down, out RaycastHit hit, 30f))
        {
            v.alturaY = hit.point.y;
            _vehiculosWrite[_indiceRaycast] = v;
        }
    }

    private void SincronizarPoolVisual()
    {
        Vector3 posJugador = _jugadorTr != null ? _jugadorTr.position : Vector3.zero;
        float   radio2     = radioPoolVisual * radioPoolVisual;

        for (int p = 0; p < maxObjetosVisuales; p++)
        {
            if (!_poolEnUso[p]) continue;
            int vi = _goAVehiculo[p];
            if (vi < 0 || vi >= _numVehiculos) { LiberarSlotPool(p); continue; }

            float d2 = (_vehiculosWrite[vi].posicion - posJugador).sqrMagnitude;
            if (d2 > radio2 * 1.2f) LiberarSlotPool(p);
        }

        for (int i = 0; i < _numVehiculos; i++)
        {
            if (_vehiculoAGO[i] >= 0) continue; 
            float d2 = (_vehiculosWrite[i].posicion - posJugador).sqrMagnitude;
            if (d2 > radio2) continue;

            int slotLibre = EncontrarSlotLibre();
            if (slotLibre < 0) break;
            AsignarSlotPool(slotLibre, i);
        }

        for (int p = 0; p < maxObjetosVisuales; p++)
        {
            if (!_poolEnUso[p]) continue;
            int vi = _goAVehiculo[p];
            if (vi < 0) continue;

            VehiculoData v = _vehiculosWrite[vi];
            var go = _poolGO[p];
            go.transform.position = v.posicion + Vector3.up * 0.375f;
            if (v.velocidad.sqrMagnitude > 0.01f)
            {
                Quaternion rotDestino = Quaternion.LookRotation(v.velocidad.normalized, Vector3.up);
                go.transform.rotation = Quaternion.Slerp(go.transform.rotation, rotDestino, Time.deltaTime * 10f);
            }

            var rend = _poolRenderers[p];
            if (rend != null)
            {
                _propBlock.SetColor(_idBaseColor, v.colorCarroceria);
                rend.SetPropertyBlock(_propBlock);
            }
        }
    }

    private int EncontrarSlotLibre()
    {
        for (int p = 0; p < maxObjetosVisuales; p++) if (!_poolEnUso[p]) return p;
        return -1;
    }

    private void AsignarSlotPool(int slot, int vehiculoIdx)
    {
        _poolEnUso[slot]           = true;
        _goAVehiculo[slot]         = vehiculoIdx;
        _vehiculoAGO[vehiculoIdx]  = slot;
        _poolGO[slot].SetActive(true);
    }

    private void LiberarSlotPool(int slot)
    {
        int vi = _goAVehiculo[slot];
        if (vi >= 0 && vi < _numVehiculos) _vehiculoAGO[vi] = -1;
        _goAVehiculo[slot] = -1;
        _poolEnUso[slot]   = false;
        _poolGO[slot].SetActive(false);
    }

    private void RenderizarVehiculosLejanos()
    {
        if (_meshCoche == null || _matVehiculo == null) return;

        int n = 0;
        for (int i = 0; i < _numVehiculos; i++)
        {
            if (_vehiculoAGO[i] >= 0) continue; 
            
            VehiculoData v = _vehiculosWrite[i];
            Vector3 vel = v.velocidad;
            Quaternion rot = vel.sqrMagnitude > 0.01f
                ? Quaternion.LookRotation(vel.normalized, Vector3.up)
                : Quaternion.identity;

            _matrices[n] = Matrix4x4.TRS(v.posicion + Vector3.up * 0.375f, rot, new Vector3(1.8f, 0.75f, 4.2f));
            _colores[n]  = (Vector4)v.colorCarroceria;
            n++;
        }

        if (n == 0) return;

        int enviados = 0;
        while (enviados < n)
        {
            int lote = Mathf.Min(MAX_LOTE, n - enviados);
            System.Array.Copy(_matrices, enviados, _matrizLote, 0, lote);

            _colorListaLote.Clear();
            for (int k = enviados; k < enviados + lote; k++) _colorListaLote.Add(_colores[k]);

            _propBlock.SetVectorArray(_idBaseColor, _colorListaLote);
            Graphics.DrawMeshInstanced(_meshCoche, 0, _matVehiculo, _matrizLote, lote, _propBlock);
            enviados += lote;
        }
    }

    private void OnDrawGizmos()
    {
        if (!mostrarGizmos || carriles == null) return;
        Color[] gizmoColores = { Color.green, Color.cyan, Color.yellow, Color.magenta };
        for (int c = 0; c < carriles.Length; c++)
        {
            Gizmos.color = gizmoColores[c % gizmoColores.Length];
            var wps = carriles[c]?.waypoints;
            if (wps == null) continue;
            for (int i = 0; i < wps.Length - 1; i++)
                if (wps[i] != null && wps[i + 1] != null) Gizmos.DrawLine(wps[i].position, wps[i + 1].position);
        }
    }
}
