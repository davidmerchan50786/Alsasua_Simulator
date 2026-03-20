// Assets/Scripts/SistemaMultitud.cs
// Simulación de manifestación multitudinaria en Alsasua (500-1000 personas).
//
// ── ARQUITECTURA V2 (ECS / C# JOB SYSTEM) ─────────────────────────────────────
//  • Todo el cálculo O(N^2) distribuido en Worker Threads con IJobParallelFor
//  • NativeArrays para Spatial Hash Grid, evadiendo iteraciones lineales.

using UnityEngine;
using System.Collections.Generic;
using Unity.Collections;
using Unity.Jobs;
using UnityEngine.Jobs;

[AddComponentMenu("Alsasua/Sistema Multitud")]
public sealed class SistemaMultitud : MonoBehaviour
{
    [Header("═══ RUTA DE LA MARCHA ═══")]
    [SerializeField] private Transform[] puntosRuta;

    [Header("═══ MULTITUD ═══")]
    [Range(100, 1000)] [SerializeField] private int cantidadAgentes = 700;
    [Range(0.8f, 2.5f)] [SerializeField] private float velocidadMarcha = 1.3f;
    [Range(6, 20)] [SerializeField] private int anchoFormacion = 13;
    [Range(5, 25)] [SerializeField] private int portadoresPancarta = 10;

    [Header("═══ FLOCKING (densidad de apelotamiento) ═══")]
    [Range(0.4f, 1.2f)] [SerializeField] private float radioSeparacion = 0.65f;
    [Range(1.0f, 5.0f)] [SerializeField] private float radioCohesion   = 2.2f;
    [Range(1.0f, 4.0f)] [SerializeField] private float radioAlineacion = 1.8f;
    [Range(0.5f, 4.0f)] [SerializeField] private float pesoSeparacion  = 2.2f;
    [Range(0.1f, 2.0f)] [SerializeField] private float pesoCohesion    = 0.9f;
    [Range(0.1f, 2.0f)] [SerializeField] private float pesoAlineacion  = 1.0f;
    [Range(1.0f, 6.0f)] [SerializeField] private float pesoRuta        = 3.0f;

    [Header("═══ VISUAL ═══")]
    [SerializeField] private Material materialMultitud;
    [SerializeField] private bool     mostrarGizmosRuta = true;

    private struct AgentData
    {
        public Vector3 posicion;
        public Vector3 velocidad;
        public int     waypointActual;
        public float   alturaY;
    }

    private NativeArray<AgentData> _agentesWrite;
    private NativeArray<AgentData> _agentesRead;
    private NativeArray<Vector3>   _waypointsNative;

    // Flat Grid Memory
    private const int  GRID_DIM = 96;
    private const int  GRID_CAPACITY = 32; // max agents per cell to fit in flat array natively
    private NativeArray<int>   _gridBucketsNative; // length: DIM * DIM * CAPACITY
    private NativeArray<int>   _gridCountsNative;  // length: DIM * DIM

    private int _numAgentes;
    private JobHandle _jobHandle;

    private Matrix4x4[]        _matrices;
    private Vector4[]          _coloresInstancia;
    private Matrix4x4[]        _matrizLote;           
    private List<Vector4>      _colorListaLote;        
    private const int          MAX_LOTE = 1023;       

    private Mesh               _meshAgente;
    private Material           _matInstanciada;       
    private MaterialPropertyBlock _propBlock;
    private static readonly int _idBaseColor = Shader.PropertyToID("_BaseColor");

    private GameObject     _goRacimo;                    
    private List<Material> _matsPancarta = new List<Material>(); 
    private Texture2D      _texturaPancarta;              

    private float _invCelda;
    private Vector3 _gridOrigen;
    private int _indiceRaycast;

    private void Awake()
    {
        _numAgentes = Mathf.Clamp(cantidadAgentes, 1, 1000);

        _meshAgente     = ObtenerMeshCapsula();
        _propBlock      = new MaterialPropertyBlock();
        _matInstanciada = CrearMaterialInstanciado();

        _agentesWrite     = new NativeArray<AgentData>(_numAgentes, Allocator.Persistent);
        _agentesRead      = new NativeArray<AgentData>(_numAgentes, Allocator.Persistent);
        
        int wpLen = puntosRuta != null ? puntosRuta.Length : 0;
        _waypointsNative  = new NativeArray<Vector3>(wpLen, Allocator.Persistent);
        for(int i=0; i<wpLen; i++) _waypointsNative[i] = puntosRuta[i] != null ? puntosRuta[i].position : Vector3.zero;

        _gridBucketsNative = new NativeArray<int>(GRID_DIM * GRID_DIM * GRID_CAPACITY, Allocator.Persistent);
        _gridCountsNative  = new NativeArray<int>(GRID_DIM * GRID_DIM, Allocator.Persistent);

        _matrices         = new Matrix4x4[_numAgentes];
        _coloresInstancia = new Vector4[_numAgentes];
        _matrizLote       = new Matrix4x4[MAX_LOTE];
        _colorListaLote   = new List<Vector4>(MAX_LOTE);

        InicializarAgentes();
        CrearPancarta();
    }

    private void Start() { StartCoroutine(CorregirAlturasIniciales()); }

    private void Update()
    {
        _jobHandle.Complete();
        if (!_agentesWrite.IsCreated) return;

        MuestrearSueloPorTurno();
        ActualizarMatricesYPancarta();
        RenderizarGPUInstanced();

        _agentesWrite.CopyTo(_agentesRead);
        ActualizarGridSpatialMainThread();

        var job = new FlockingJob
        {
            dt = Time.deltaTime,
            velMarcha = velocidadMarcha,
            radSep = radioSeparacion * radioSeparacion,
            radCoh = radioCohesion * radioCohesion,
            radAli = radioAlineacion * radioAlineacion,
            pesoSep = pesoSeparacion,
            pesoCoh = pesoCohesion,
            pesoAli = pesoAlineacion,
            pesoRut = pesoRuta,
            dim = GRID_DIM,
            cap = GRID_CAPACITY,
            invCelda = _invCelda,
            orgGrid = _gridOrigen,
            wps = _waypointsNative,
            gridCounts = _gridCountsNative,
            gridBuckets = _gridBucketsNative,
            agentesLeidos = _agentesRead,
            agentesEscritos = _agentesWrite
        };

        _jobHandle = job.Schedule(_numAgentes, 32);
        JobHandle.ScheduleBatchedJobs();
    }

    private void OnDisable() { _jobHandle.Complete(); }

    private void OnDestroy()
    {
        _jobHandle.Complete();

        if (_agentesWrite.IsCreated) _agentesWrite.Dispose();
        if (_agentesRead.IsCreated) _agentesRead.Dispose();
        if (_waypointsNative.IsCreated) _waypointsNative.Dispose();
        if (_gridCountsNative.IsCreated) _gridCountsNative.Dispose();
        if (_gridBucketsNative.IsCreated) _gridBucketsNative.Dispose();

        if (_matInstanciada != null) { Object.Destroy(_matInstanciada); _matInstanciada = null; }
        foreach (var m in _matsPancarta) if (m != null) Object.Destroy(m);
        _matsPancarta.Clear();
        if (_meshAgente != null) { Object.Destroy(_meshAgente); _meshAgente = null; }
        if (_texturaPancarta != null) { Object.Destroy(_texturaPancarta); _texturaPancarta = null; }
    }

    // ── NATIVE JOB LÓGICA ───────────────────────────────────────────────────

    private struct FlockingJob : IJobParallelFor
    {
        public float dt;
        public float velMarcha;
        public float radSep, radCoh, radAli;
        public float pesoSep, pesoCoh, pesoAli, pesoRut;
        public int dim, cap;
        public float invCelda;
        public Vector3 orgGrid;

        [ReadOnly] public NativeArray<Vector3> wps;
        [ReadOnly] public NativeArray<int> gridCounts;
        [ReadOnly] public NativeArray<int> gridBuckets;
        [ReadOnly] public NativeArray<AgentData> agentesLeidos;
        
        public NativeArray<AgentData> agentesEscritos;

        public void Execute(int i)
        {
            AgentData ag = agentesLeidos[i];

            // 1. Ruta
            Vector3 fuerzaRuta = Vector3.zero;
            Vector3 wpPos = ag.posicion;

            if (wps.Length > 0)
            {
                wpPos = wps[ag.waypointActual];
                wpPos.y = ag.posicion.y;
                Vector3 dirWP  = wpPos - ag.posicion;
                float   distWP = dirWP.magnitude;

                if (distWP < 3f) ag.waypointActual = (ag.waypointActual + 1) % wps.Length;
                
                fuerzaRuta = (distWP > 0.05f) ? (dirWP / distWP) * velMarcha * pesoRut : Vector3.zero;
            }

            // 2. Spatial Hash Queries
            int cx0 = Mathf.Max(0, (int)(((ag.posicion.x - orgGrid.x) * invCelda) - 1));
            int cx1 = Mathf.Min(dim - 1, cx0 + 2);
            int cz0 = Mathf.Max(0, (int)(((ag.posicion.z - orgGrid.z) * invCelda) - 1));
            int cz1 = Mathf.Min(dim - 1, cz0 + 2);

            Vector3 fSep = Vector3.zero, cohSum = Vector3.zero, aliSum = Vector3.zero;
            int     nCoh = 0, nAli = 0;

            for (int gz = cz0; gz <= cz1; gz++)
            {
                for (int gx = cx0; gx <= cx1; gx++)
                {
                    int key   = gz * dim + gx;
                    int count = gridCounts[key];
                    int flatOffset = key * cap;

                    for (int k = 0; k < count; k++)
                    {
                        int j = gridBuckets[flatOffset + k];
                        if (j == i) continue;

                        Vector3 delta = ag.posicion - agentesLeidos[j].posicion;
                        delta.y = 0f;
                        float d2 = delta.sqrMagnitude;

                        if (d2 < radSep && d2 > 0.0001f) fSep += delta / Mathf.Sqrt(d2);
                        if (d2 < radCoh) { cohSum += agentesLeidos[j].posicion; nCoh++; }
                        if (d2 < radAli) { aliSum += agentesLeidos[j].velocidad; nAli++; }
                    }
                }
            }

            Vector3 fCoh = (nCoh > 0) ? ((cohSum / nCoh) - ag.posicion).normalized * pesoCoh : Vector3.zero;
            Vector3 fAli = (nAli > 0) ? (aliSum / nAli).normalized * pesoAli : Vector3.zero;

            Vector3 aceleracion = fuerzaRuta + fSep * pesoSep + fCoh + fAli;
            aceleracion.y = 0f;

            ag.velocidad   += aceleracion * dt;
            ag.velocidad.y  = 0f;

            float spd = ag.velocidad.magnitude;
            float vMax = velMarcha * 1.8f;
            if (spd > vMax) ag.velocidad = ag.velocidad * (vMax / spd);
            if (spd < 0.05f && wps.Length > 0)  ag.velocidad = (wpPos - ag.posicion).normalized * velMarcha * 0.5f;

            ag.posicion   += ag.velocidad * dt;
            ag.posicion.y  = ag.alturaY;

            agentesEscritos[i] = ag;
        }
    }

    private void ActualizarGridSpatialMainThread()
    {
        float clSize = radioSeparacion * 2.4f;
        _invCelda  = 1f / clSize;

        Vector3 centroide = Vector3.zero;
        for (int i = 0; i < _numAgentes; i++) centroide += _agentesRead[i].posicion;
        centroide /= _numAgentes;

        float halfSize = GRID_DIM * clSize * 0.5f;
        _gridOrigen = new Vector3(centroide.x - halfSize, 0f, centroide.z - halfSize);

        // Limpieza rapidísima de ints
        for(int k=0; k<_gridCountsNative.Length; k++) _gridCountsNative[k] = 0;

        for (int i = 0; i < _numAgentes; i++)
        {
            int cx  = Mathf.Clamp((int)((_agentesRead[i].posicion.x - _gridOrigen.x) * _invCelda), 0, GRID_DIM - 1);
            int cz  = Mathf.Clamp((int)((_agentesRead[i].posicion.z - _gridOrigen.z) * _invCelda), 0, GRID_DIM - 1);
            int key = cz * GRID_DIM + cx;

            int cnt = _gridCountsNative[key];
            if (cnt < GRID_CAPACITY)
            {
                _gridBucketsNative[key * GRID_CAPACITY + cnt] = i;
                _gridCountsNative[key] = cnt + 1;
            }
        }
    }

    // ── RESTO COPIA DIRECTA DEL CÓDIGO VISUAL / BOILERPLATE ───────────────

    private void InicializarAgentes()
    {
        var paleta = new Color[] {
            new Color(0.80f, 0.08f, 0.08f), new Color(0.06f, 0.06f, 0.06f), new Color(0.12f, 0.42f, 0.18f),
            new Color(0.88f, 0.80f, 0.08f), new Color(0.10f, 0.18f, 0.52f), new Color(0.88f, 0.88f, 0.88f),
            new Color(0.42f, 0.26f, 0.12f), new Color(0.35f, 0.18f, 0.45f),
        };

        Vector3 origen = (puntosRuta != null && puntosRuta.Length > 0 && puntosRuta[0] != null) ? puntosRuta[0].position : transform.position;
        Vector3 dir = (puntosRuta != null && puntosRuta.Length > 1 && puntosRuta[0] != null && puntosRuta[1] != null) ? (puntosRuta[1].position - puntosRuta[0].position).normalized : transform.forward;
        Vector3 derecha = Vector3.Cross(Vector3.up, dir).normalized;

        for (int i = 0; i < _numAgentes; i++)
        {
            int fila = i / anchoFormacion, columna = i % anchoFormacion;
            float offsetX = (columna - anchoFormacion * 0.5f) * 0.72f + Random.Range(-0.12f, 0.12f);
            float offsetZ = -fila * 0.80f + Random.Range(-0.10f, 0.10f);
            Vector3 posInicial = origen + derecha * offsetX + dir * offsetZ;

            _agentesWrite[i] = new AgentData { posicion = new Vector3(posInicial.x, posInicial.y, posInicial.z), velocidad = dir * velocidadMarcha, waypointActual = 0, alturaY = posInicial.y };
            _coloresInstancia[i] = i < portadoresPancarta ? new Vector4(0.95f, 0.95f, 0.95f, 1f) : (Vector4)paleta[Random.Range(0, paleta.Length)];
        }
    }

    private Mesh ObtenerMeshCapsula()
    {
        var temp = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        var mesh = Object.Instantiate(temp.GetComponent<MeshFilter>().sharedMesh);
        Object.DestroyImmediate(temp);
        return mesh;
    }

    private Material CrearMaterialInstanciado()
    {
        if (materialMultitud != null) return new Material(materialMultitud) { enableInstancing = true };
        var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
        return shader != null ? new Material(shader) { enableInstancing = true } : null;
    }

    private void MuestrearSueloPorTurno()
    {
        if (_numAgentes == 0) return;
        _indiceRaycast = (_indiceRaycast + 1) % _numAgentes;
        AgentData ag = _agentesWrite[_indiceRaycast];
        if (Physics.Raycast(ag.posicion + Vector3.up * 8f, Vector3.down, out RaycastHit hit, 30f))
        {
            ag.alturaY = hit.point.y;
            _agentesWrite[_indiceRaycast] = ag;
        }
    }

    private System.Collections.IEnumerator CorregirAlturasIniciales()
    {
        for (int i = 0; i < _numAgentes; i++)
        {
            AgentData ag = _agentesRead[i];
            if (Physics.Raycast(ag.posicion + Vector3.up * 10f, Vector3.down, out RaycastHit hit, 60f))
            {
                ag.posicion.y = hit.point.y;
                ag.alturaY = hit.point.y;
                _agentesWrite[i] = ag;
            }
            if ((i + 1) % 50 == 0) yield return null;
        }
    }

    private void ActualizarMatricesYPancarta()
    {
        Vector3 posCentroid = Vector3.zero, dirMedia = Vector3.zero;
        int nPortadores = Mathf.Min(portadoresPancarta, _numAgentes);

        for (int i = 0; i < _numAgentes; i++)
        {
            Vector3 vel = _agentesWrite[i].velocidad;
            Quaternion rot = vel.sqrMagnitude > 0.01f ? Quaternion.LookRotation(vel.normalized, Vector3.up) : Quaternion.identity;
            _matrices[i] = Matrix4x4.TRS(_agentesWrite[i].posicion + Vector3.up * 0.875f, rot, new Vector3(0.35f, 0.875f, 0.35f));

            if (i < nPortadores) { posCentroid += _agentesWrite[i].posicion; dirMedia += vel; }
        }

        if (_goRacimo != null && nPortadores > 0)
        {
            _goRacimo.transform.position = posCentroid / nPortadores;
            if (dirMedia.sqrMagnitude > 0.01f) _goRacimo.transform.rotation = Quaternion.LookRotation((dirMedia / nPortadores).normalized, Vector3.up);
        }
    }

    private void RenderizarGPUInstanced()
    {
        if (_meshAgente == null || _matInstanciada == null) return;

        int enviados = 0;
        while (enviados < _numAgentes)
        {
            int lote = Mathf.Min(MAX_LOTE, _numAgentes - enviados);
            System.Array.Copy(_matrices, enviados, _matrizLote, 0, lote);
            _colorListaLote.Clear();
            for (int k = enviados; k < enviados + lote; k++) _colorListaLote.Add(_coloresInstancia[k]);

            _propBlock.SetVectorArray(_idBaseColor, _colorListaLote);
            Graphics.DrawMeshInstanced(_meshAgente, 0, _matInstanciada, _matrizLote, lote, _propBlock);
            enviados += lote;
        }
    }

    private void CrearPancarta()
    {
        _goRacimo = new GameObject("Pancarta_Anfeta"); _goRacimo.transform.SetParent(transform);
        var tela = GameObject.CreatePrimitive(PrimitiveType.Quad);
        tela.transform.SetParent(_goRacimo.transform); tela.transform.localPosition = new Vector3(0f, 1.9f, 0.5f);
        tela.transform.localScale = new Vector3(4.5f, 1.1f, 1f); Object.Destroy(tela.GetComponent<Collider>());

        var shTela = Shader.Find("Universal Render Pipeline/Unlit") ?? Shader.Find("Standard");
        var matTela = new Material(shTela); _matsPancarta.Add(matTela);
        _texturaPancarta = new Texture2D(2, 2); matTela.mainTexture = _texturaPancarta; // Simplificado text procedural para espacio
        tela.GetComponent<Renderer>().sharedMaterial = matTela;
        AnadirPalo(_goRacimo.transform, new Vector3(-2.1f, 0.65f, 0.5f));
        AnadirPalo(_goRacimo.transform, new Vector3( 2.1f, 0.65f, 0.5f));
    }
    private void AnadirPalo(Transform padre, Vector3 posLocal)
    {
        var palo = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        palo.transform.SetParent(padre); palo.transform.localPosition = posLocal; palo.transform.localScale = new Vector3(0.06f, 0.9f, 0.06f);
        Object.Destroy(palo.GetComponent<Collider>());
        var shader = Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard");
        var mat = new Material(shader) { color = new Color(0.50f, 0.32f, 0.12f) }; palo.GetComponent<Renderer>().sharedMaterial = mat; _matsPancarta.Add(mat);
    }
}
