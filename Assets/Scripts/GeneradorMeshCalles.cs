// Assets/Scripts/GeneradorMeshCalles.cs
// ═══════════════════════════════════════════════════════════════════════════
//  GENERADOR DE MESH DE CALLES — ALSASUA
//
//  Convierte los datos de GeoDataCalles.CallesPrincipales en mallas 3D
//  de asfalto renderizables en tiempo real, al estilo GTA V:
//
//    · Banda de carretera con UV tiling para textura de asfalto
//    · Arcén / bordillo a cada lado (capa elevada 0.05 m)
//    · Líneas de carretera como sub-mesh separado (shader emissive)
//    · Ajuste de altura al terreno via Physics.Raycast
//    · Batching estático automático para 0 draw calls en runtime
//
//  USO:
//    Añade este componente a un GO vacío "GeneradorCalles" en la escena.
//    Pulsa Play o usa el menú Alsasua GTA → Generar Calles en Editor.
// ═══════════════════════════════════════════════════════════════════════════

using System.Collections.Generic;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif

[AddComponentMenu("Alsasua/Generador Mesh Calles")]
public sealed class GeneradorMeshCalles : MonoBehaviour
{
    // ── Inspector ────────────────────────────────────────────────────────
    [Header("Materiales")]
    [Tooltip("Material de asfalto (HDRP Lit / URP Lit). Asigna textura de asfalto con normal map.")]
    public Material materialAsfalto;
    [Tooltip("Material para arcenes y bordillos (hormigón, color gris claro).")]
    public Material materialArcen;
    [Tooltip("Material para líneas de carretera (blanco/amarillo, puede ser emissive).")]
    public Material materialLineas;

    [Header("Geometría")]
    [Tooltip("Elevación del mesh sobre el terreno (evita z-fighting).")]
    public float elevacionAsfalto = 0.05f;
    [Tooltip("Anchura del arcén a cada lado de la calzada (metros).")]
    public float anchoArcen = 0.8f;
    [Tooltip("Altura del bordillo sobre la calzada.")]
    public float altoBordillo = 0.12f;
    [Tooltip("Anchura de las líneas de carretera.")]
    public float anchoLinea = 0.12f;
    [Tooltip("Longitud de cada segmento de línea discontinua.")]
    public float longitudLineaDiscontinua = 3f;
    [Tooltip("Espacio entre segmentos discontinuos.")]
    public float espacioLineaDiscontinua = 4f;

    [Header("UV Tiling")]
    [Tooltip("Metros de carretera por repetición de textura (eje longitudinal).")]
    public float tilingLongitudinal = 6f;
    [Tooltip("Textura se repite 1x transversalmente por carril.")]
    public float tilingTransversal = 1f;

    [Header("Terreno")]
    [Tooltip("Ajustar la altura de cada vértice al terreno mediante raycast.")]
    public bool ajustarAlTerreno = true;
    [Tooltip("LayerMask para el raycast de terreno (Terrain + meshes estáticos).")]
    public LayerMask capasTerreno = ~0;

    [Header("Generación")]
    [Tooltip("Número de subdivisiones transversales por segmento (más = más suave en curvas).")]
    [Range(1, 8)]
    public int subdisTrans = 1;
    [Tooltip("Si es true, marca todos los GO de calles como Static.")]
    public bool marcarComoStatic = true;

    // ── Internos ─────────────────────────────────────────────────────────
    private Transform _contenedor;
    private const float RAYCAST_ALTO = 500f;

    // =========================================================================
    //  UNITY
    // =========================================================================

    void Start()
    {
        // Solo genera si no hay hijos (evita re-generar en recargas)
        if (transform.childCount == 0)
            GenerarTodas();
    }

    // =========================================================================
    //  API PÚBLICA
    // =========================================================================

    /// <summary>Genera todas las calles de GeoDataCalles.</summary>
    public void GenerarTodas()
    {
        LimpiarExistentes();
        _contenedor = new GameObject("Calles_Alsasua").transform;
        _contenedor.SetParent(transform);

        CrearMaterialesDefecto();

        int generadas = 0;
        foreach (var calle in GeoDataCalles.CallesPrincipales)
        {
            if (calle.Puntos == null || calle.Puntos.Length < 2) continue;
            GenerarCalle(calle);
            generadas++;
        }

        if (marcarComoStatic)
            MarcarStaticRecursivo(_contenedor.gameObject);

        Debug.Log($"[GeneradorCalles] {generadas} calles generadas en Alsasua.");
    }

    public void LimpiarExistentes()
    {
        var anterior = transform.Find("Calles_Alsasua");
        if (anterior != null)
        {
#if UNITY_EDITOR
            DestroyImmediate(anterior.gameObject);
#else
            Destroy(anterior.gameObject);
#endif
        }
    }

    // =========================================================================
    //  GENERACIÓN DE UNA CALLE
    // =========================================================================

    void GenerarCalle(GeoDataCalles.CalleData calle)
    {
        var goRaiz = new GameObject(calle.Nombre);
        goRaiz.transform.SetParent(_contenedor);

        // ── Calzada ───────────────────────────────────────────────────────
        var meshCalzada = BuildMeshBanda(calle.Puntos, calle.Ancho, elevacionAsfalto);
        CrearGoConMesh(goRaiz.transform, "Calzada", meshCalzada, materialAsfalto, false);

        // ── Arcenes ───────────────────────────────────────────────────────
        if (anchoArcen > 0f)
        {
            float anchoTotal = calle.Ancho + anchoArcen;
            var meshArcenIzq = BuildMeshBanda(calle.Puntos, anchoArcen,
                                               elevacionAsfalto + altoBordillo,
                                               -calle.Ancho * 0.5f - anchoArcen * 0.5f);
            var meshArcenDer = BuildMeshBanda(calle.Puntos, anchoArcen,
                                               elevacionAsfalto + altoBordillo,
                                               +calle.Ancho * 0.5f + anchoArcen * 0.5f);
            CrearGoConMesh(goRaiz.transform, "Arcen_Izq", meshArcenIzq, materialArcen, true);
            CrearGoConMesh(goRaiz.transform, "Arcen_Der", meshArcenDer, materialArcen, true);
        }

        // ── Líneas ────────────────────────────────────────────────────────
        bool esAutovia = calle.Tipo == GeoDataCalles.TipoCalle.Autovia;
        var meshLineas = BuildMeshLineas(calle.Puntos, calle.Ancho, esAutovia);
        if (meshLineas != null)
            CrearGoConMesh(goRaiz.transform, "Lineas", meshLineas, materialLineas, false);
    }

    // =========================================================================
    //  CONSTRUCCIÓN DE MESH — BANDA DE CARRETERA
    // =========================================================================

    /// <summary>
    /// Construye un mesh de banda plana siguiendo los waypoints.
    /// offsetLateral != 0 → desplaza la banda lateralmente (para arcenes).
    /// </summary>
    Mesh BuildMeshBanda(Vector3[] waypoints, float ancho, float elevacion, float offsetLateral = 0f)
    {
        var verts  = new List<Vector3>();
        var uvs    = new List<Vector2>();
        var tris   = new List<int>();
        var norms  = new List<Vector3>();

        float distAcum = 0f;

        for (int i = 0; i < waypoints.Length; i++)
        {
            Vector3 forward = GetForward(waypoints, i);
            Vector3 right   = Vector3.Cross(Vector3.up, forward).normalized;
            Vector3 centro  = waypoints[i] + right * offsetLateral;
            centro.y = elevacion;

            if (ajustarAlTerreno)
            {
                float altTerreno = SampleTerreno(centro);
                if (altTerreno != float.MinValue)
                    centro.y = altTerreno + elevacion;
            }

            Vector3 izq = centro - right * (ancho * 0.5f);
            Vector3 der = centro + right * (ancho * 0.5f);

            if (i > 0)
                distAcum += Vector3.Distance(waypoints[i], waypoints[i - 1]);

            float u = distAcum / tilingLongitudinal;

            verts.Add(izq);
            verts.Add(der);
            uvs.Add(new Vector2(0f, u));
            uvs.Add(new Vector2(tilingTransversal, u));
            norms.Add(Vector3.up);
            norms.Add(Vector3.up);

            if (i > 0)
            {
                int b = (i - 1) * 2;
                int t = i * 2;
                // Quad: 2 triángulos
                tris.Add(b);     tris.Add(t);     tris.Add(b + 1);
                tris.Add(t);     tris.Add(t + 1); tris.Add(b + 1);
            }
        }

        var mesh = new Mesh();
        mesh.name = "BandaCarretera";
        mesh.SetVertices(verts);
        mesh.SetUVs(0, uvs);
        mesh.SetNormals(norms);
        mesh.SetTriangles(tris, 0);
        mesh.RecalculateBounds();
        mesh.RecalculateTangents();
        return mesh;
    }

    // =========================================================================
    //  CONSTRUCCIÓN DE MESH — LÍNEAS DE CARRETERA
    // =========================================================================

    Mesh BuildMeshLineas(Vector3[] waypoints, float anchoCalzada, bool esAutovia)
    {
        var verts = new List<Vector3>();
        var uvs   = new List<Vector2>();
        var tris  = new List<int>();
        var norms = new List<Vector3>();

        float elevLinea = elevacionAsfalto + 0.005f; // justo encima del asfalto
        float distAcum  = 0f;
        bool  enSegmento = true; // línea discontinua: alterna on/off

        for (int i = 0; i < waypoints.Length - 1; i++)
        {
            Vector3 p0 = waypoints[i];
            Vector3 p1 = waypoints[i + 1];
            float   segLen = Vector3.Distance(p0, p1);
            Vector3 dir    = (p1 - p0).normalized;
            Vector3 right  = Vector3.Cross(Vector3.up, dir).normalized;

            float avanzado = 0f;
            while (avanzado < segLen)
            {
                float periodoActual = enSegmento ? longitudLineaDiscontinua : espacioLineaDiscontinua;
                float hasta = Mathf.Min(avanzado + periodoActual, segLen);

                if (enSegmento)
                {
                    // Línea central
                    Vector3 a = p0 + dir * avanzado;
                    Vector3 b = p0 + dir * hasta;
                    a.y = elevLinea; b.y = elevLinea;
                    AjustarY(ref a); AjustarY(ref b);

                    // Quad de la línea
                    int idx = verts.Count;
                    verts.Add(a - right * (anchoLinea * 0.5f));
                    verts.Add(a + right * (anchoLinea * 0.5f));
                    verts.Add(b - right * (anchoLinea * 0.5f));
                    verts.Add(b + right * (anchoLinea * 0.5f));

                    float u0 = (distAcum + avanzado) / tilingLongitudinal;
                    float u1 = (distAcum + hasta)    / tilingLongitudinal;
                    uvs.Add(new Vector2(0, u0)); uvs.Add(new Vector2(1, u0));
                    uvs.Add(new Vector2(0, u1)); uvs.Add(new Vector2(1, u1));

                    for (int n = 0; n < 4; n++) norms.Add(Vector3.up);

                    tris.Add(idx);   tris.Add(idx+2); tris.Add(idx+1);
                    tris.Add(idx+1); tris.Add(idx+2); tris.Add(idx+3);
                }

                avanzado += periodoActual;
                enSegmento = !enSegmento;
            }
            distAcum += segLen;
        }

        if (verts.Count == 0) return null;

        var mesh = new Mesh();
        mesh.name = "LineasCarretera";
        mesh.SetVertices(verts);
        mesh.SetUVs(0, uvs);
        mesh.SetNormals(norms);
        mesh.SetTriangles(tris, 0);
        mesh.RecalculateBounds();
        return mesh;
    }

    // =========================================================================
    //  HELPERS
    // =========================================================================

    void CrearGoConMesh(Transform padre, string nombre, Mesh mesh, Material mat, bool castShadow)
    {
        if (mesh == null) return;
        var go = new GameObject(nombre);
        go.transform.SetParent(padre);
        go.transform.localPosition = Vector3.zero;

        var mf = go.AddComponent<MeshFilter>();
        mf.sharedMesh = mesh;

        var mr = go.AddComponent<MeshRenderer>();
        mr.sharedMaterial = mat != null ? mat : CrearMaterialFallback(nombre);
        mr.shadowCastingMode = castShadow
            ? UnityEngine.Rendering.ShadowCastingMode.On
            : UnityEngine.Rendering.ShadowCastingMode.Off;
        mr.receiveShadows = true;

        var mc = go.AddComponent<MeshCollider>();
        mc.sharedMesh = mesh;
        mc.cookingOptions = MeshColliderCookingOptions.EnableMeshCleaning;
    }

    Vector3 GetForward(Vector3[] pts, int i)
    {
        if (i < pts.Length - 1) return (pts[i + 1] - pts[i]).normalized;
        if (i > 0)              return (pts[i] - pts[i - 1]).normalized;
        return Vector3.forward;
    }

    float SampleTerreno(Vector3 punto)
    {
        var ray = new Ray(punto + Vector3.up * RAYCAST_ALTO, Vector3.down);
        if (Physics.Raycast(ray, out var hit, RAYCAST_ALTO * 2f, capasTerreno))
            return hit.point.y;
        return float.MinValue;
    }

    void AjustarY(ref Vector3 p)
    {
        if (!ajustarAlTerreno) return;
        float y = SampleTerreno(p);
        if (y != float.MinValue) p.y = y + elevacionAsfalto + 0.005f;
    }

    void CrearMaterialesDefecto()
    {
        if (materialAsfalto == null)
        {
            materialAsfalto = new Material(Shader.Find("HDRP/Lit") ?? Shader.Find("Universal Render Pipeline/Lit") ?? Shader.Find("Standard"));
            materialAsfalto.name = "M_Asfalto_Alsasua";
            materialAsfalto.color = new Color(0.12f, 0.12f, 0.12f); // asfalto oscuro
#if UNITY_EDITOR
            // Intentar cargar textura de asfalto si existe en el proyecto
            var tex = AssetDatabase.LoadAssetAtPath<Texture2D>("Assets/Textures/Roads/asphalt_albedo.png");
            if (tex != null) materialAsfalto.mainTexture = tex;
#endif
        }

        if (materialArcen == null)
        {
            materialArcen = new Material(materialAsfalto);
            materialArcen.name = "M_Arcen_Alsasua";
            materialArcen.color = new Color(0.45f, 0.43f, 0.40f); // hormigón gris
        }

        if (materialLineas == null)
        {
            materialLineas = new Material(materialAsfalto);
            materialLineas.name = "M_Lineas_Alsasua";
            materialLineas.color = Color.white;
        }
    }

    Material CrearMaterialFallback(string nombre)
    {
        var mat = new Material(Shader.Find("HDRP/Lit") ?? Shader.Find("Standard"));
        mat.name = "M_Fallback_" + nombre;
        mat.color = Color.grey;
        return mat;
    }

    static void MarcarStaticRecursivo(GameObject go)
    {
#if UNITY_EDITOR
        foreach (var t in go.GetComponentsInChildren<Transform>(true))
        {
            GameObjectUtility.SetStaticEditorFlags(t.gameObject,
                StaticEditorFlags.ContributeGI |
                StaticEditorFlags.OccludeeStatic |
                StaticEditorFlags.BatchingStatic);
        }
#endif
    }

    // =========================================================================
    //  EDITOR TOOLS
    // =========================================================================
#if UNITY_EDITOR
    [ContextMenu("Regenerar Calles Ahora")]
    void RegenerarDesdeContextMenu() => GenerarTodas();

    [ContextMenu("Limpiar Calles")]
    void LimpiarDesdeContextMenu() => LimpiarExistentes();

    void OnDrawGizmosSelected()
    {
        // Visualizar waypoints de calles antes de generar
        foreach (var calle in GeoDataCalles.CallesPrincipales)
        {
            if (calle.Puntos == null || calle.Puntos.Length < 2) continue;

            Gizmos.color = calle.Tipo == GeoDataCalles.TipoCalle.Autovia
                ? Color.red : calle.Tipo == GeoDataCalles.TipoCalle.Primaria
                ? Color.yellow : Color.cyan;

            for (int i = 0; i < calle.Puntos.Length - 1; i++)
            {
                Gizmos.DrawLine(calle.Puntos[i] + Vector3.up * 0.5f,
                                calle.Puntos[i + 1] + Vector3.up * 0.5f);
            }

            // Label con el nombre
            Handles.Label(calle.Puntos[0] + Vector3.up * 2f, calle.Nombre,
                new GUIStyle { normal = { textColor = Color.white }, fontSize = 10 });
        }
    }
#endif
}
