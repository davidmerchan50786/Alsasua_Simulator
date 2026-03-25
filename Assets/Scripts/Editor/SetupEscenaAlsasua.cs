// Assets/Editor/SetupEscenaAlsasua.cs
// Menú Unity:  Alsasua ▶  ⚙ Configurar Escena Completa
//              Alsasua ▶  🎮 Configurar Gameplay
//              Alsasua ▶  🔍 Diagnosticar Escena
//
// Crea todos los GameObjects que falten en la escena.
// Operación NO destructiva: sólo añade lo que no existe.
// Úsalo tanto en modo edición como después de borrar objetos accidentalmente.

using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using UnityEngine.EventSystems;
using UnityEngine.SceneManagement;
using System.Collections.Generic;
using CesiumForUnity;

public static class SetupEscenaAlsasua
{
    const double LAT        = 42.8169;
    const double LON        = -1.6432;
    const double ALT_GEO   = 450.0;   // altura geográfica de Pamplona — Plaza del Castillo (m s.n.m.)
    const float  CAM_ALTURA = 1500f;   // altura inicial de la cámara dron sobre el origen

    // Posiciones locales (metros desde el origen = centro de Pamplona)
    // Y=0  → nivel de calle (altura geográfica 450 m)
    // Y=1  → 1 metro sobre el suelo (pies del personaje)

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: CONFIGURAR ESCENA COMPLETA (infraestructura + gameplay)
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Pamplona/⚙ Configurar Escena Completa", priority = 0)]
    public static void ConfigurarEscena()
    {
        Debug.Log("══════════════════════════════════════════════════");
        Debug.Log("[Setup Pamplona] Iniciando configuración de escena");
        Debug.Log("══════════════════════════════════════════════════");

        var geo = AsegurarGeoreference();
        AsegurarCreditSystem(geo);
        AsegurarLuz();
        AsegurarEventSystem();
        AsegurarCamara();
        AsegurarVolumenPostProcesado();
        AsegurarManager();
        AsegurarGestorTexturas();
        AsegurarAtmosfera();
        AsegurarClima();
        AsegurarJugador();
        AsegurarEnemigos();
        AsegurarVehiculos();
        AsegurarBarricadas();

        EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
        AssetDatabase.SaveAssets();

        Debug.Log("══════════════════════════════════════════════════");
        Debug.Log("[Setup Pamplona] ✓ Completo — guarda la escena con Ctrl+S y dale Play");
        Debug.Log("[Setup Pamplona]   Para jugar en modo FPS: desactiva 'Main Camera'");
        Debug.Log("[Setup Pamplona]   y activa 'CamaraFPS' (hija de Jugador) en el Inspector");
        Debug.Log("══════════════════════════════════════════════════");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: CONFIGURAR SÓLO GAMEPLAY (jugador, enemigos, vehículos, barricadas)
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Pamplona/🎮 Configurar Gameplay", priority = 1)]
    public static void ConfigurarGameplay()
    {
        Debug.Log("[Setup Pamplona] ── Configurando sistemas de gameplay ──");
        AsegurarJugador();
        AsegurarEnemigos();
        AsegurarVehiculos();
        AsegurarBarricadas();
        EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
        Debug.Log("[Setup Pamplona] ✓ Gameplay configurado. Guarda la escena.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: DIAGNOSTICAR ESCENA
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Pamplona/🔍 Diagnosticar Escena", priority = 10)]
    public static void DiagnosticarEscena()
    {
        Debug.Log("── [Diagnóstico Alsasua] ──────────────────────────");

        var geo = Object.FindFirstObjectByType<CesiumGeoreference>();
        Debug.Log(geo != null
            ? $"  ✓ CesiumGeoreference: lat={geo.latitude:F4} lon={geo.longitude:F4} h={geo.height:F0}"
            : "  ✗ CesiumGeoreference: FALTA");

        var credits = Object.FindFirstObjectByType<CesiumCreditSystem>();
        Debug.Log(credits != null ? "  ✓ CesiumCreditSystem" : "  ✗ CesiumCreditSystem: FALTA");

        var cam = Camera.main;
        if (cam != null)
        {
            bool tieneOriginShiftCam = cam.GetComponent<CesiumOriginShift>() != null;
            bool tieneDron           = cam.GetComponent<CamaraDron>() != null;
            Debug.Log($"  ✓ Main Camera pos={cam.transform.position}  depth={cam.depth}  Far={cam.farClipPlane:F0} m");
            if (tieneOriginShiftCam)
                Debug.LogWarning("  ⚠ CesiumOriginShift está en Main Camera (Y=1500) — BUG: " +
                    "Cesium cargará tiles aéreos. Muévelo al Jugador y re-ejecuta Configurar Escena Completa.");
            else
                Debug.Log("  ✓ Main Camera NO tiene CesiumOriginShift (correcto: está en Jugador)");
            Debug.Log(tieneDron ? "  ✓ CamaraDron en cámara" : "  ✗ CamaraDron: FALTA en cámara");
            // Advertir si Far Clipping Plane es insuficiente para Cesium
            if (cam.farClipPlane < 100_000f)
                Debug.LogWarning($"  ⚠ Main Camera Far={cam.farClipPlane:F0} m — MUY PEQUEÑO para Cesium (mínimo 100.000, recomendado 1.000.000). " +
                    "Pantalla negra probable. Re-ejecuta 'Configurar Escena Completa'.");
        }
        else
            Debug.Log("  ✗ Main Camera: FALTA");

        // Verificar CesiumOriginShift en el Jugador (donde debe estar)
        var jugadorDiag = Object.FindFirstObjectByType<ControladorJugador>();
        if (jugadorDiag != null)
        {
            bool tieneOriginShiftJugador = jugadorDiag.GetComponent<CesiumOriginShift>() != null;
            Debug.Log(tieneOriginShiftJugador
                ? "  ✓ CesiumOriginShift en Jugador (correcto: tiles a nivel de suelo)"
                : "  ✗ CesiumOriginShift: FALTA en Jugador — re-ejecuta '⚙ Configurar Escena Completa'");
        }

        var vol = Object.FindFirstObjectByType<Volume>();
        Debug.Log(vol != null
            ? $"  ✓ Volume post-procesado (perfil: {(vol.sharedProfile != null ? vol.sharedProfile.name : "NINGUNO")})"
            : "  ✗ Volume de post-procesado: FALTA");

        var config = Object.FindFirstObjectByType<ConfiguradorAlsasua>();
        Debug.Log(config != null ? $"  ✓ ConfiguradorAlsasua en '{config.gameObject.name}'" : "  ✗ ConfiguradorAlsasua: FALTA");

        var gest = Object.FindFirstObjectByType<GestionTilesets>();
        Debug.Log(gest != null ? "  ✓ GestionTilesets" : "  ✗ GestionTilesets: FALTA");

        var pp = Object.FindFirstObjectByType<ControladorPostProcesado>();
        Debug.Log(pp != null ? "  ✓ ControladorPostProcesado" : "  ✗ ControladorPostProcesado: FALTA");

        var tex = Object.FindFirstObjectByType<TexturizadorEdificiosReales>();
        Debug.Log(tex != null ? "  ✓ TexturizadorEdificiosReales" : "  ✗ TexturizadorEdificiosReales: FALTA");

        var tilesets = Object.FindObjectsByType<Cesium3DTileset>(FindObjectsSortMode.None);
        Debug.Log($"  Tilesets en escena: {tilesets.Length}");
        foreach (var t in tilesets)
            Debug.Log($"    · {t.gameObject.name}  activo={t.gameObject.activeSelf}");

        Debug.Log("  ── Gameplay ──────────────────────────────────────");

        var jugador = Object.FindFirstObjectByType<ControladorJugador>();
        Debug.Log(jugador != null
            ? $"  ✓ Jugador (ControladorJugador) en pos={jugador.transform.position}"
            : "  ✗ Jugador: FALTA — usa Alsasua > 🎮 Configurar Gameplay");

        var suelo = GameObject.Find("SueloBase");
        Debug.Log(suelo != null ? "  ✓ SueloBase (colisionador invisible)" : "  ✗ SueloBase: FALTA");

        var enemigos = Object.FindObjectsByType<EnemigoPatrulla>(FindObjectsSortMode.None);
        Debug.Log($"  {(enemigos.Length > 0 ? "✓" : "✗")} Enemigos en escena: {enemigos.Length}");

        var vehiculos = Object.FindObjectsByType<VehiculoNPC>(FindObjectsSortMode.None);
        Debug.Log($"  {(vehiculos.Length > 0 ? "✓" : "✗")} Vehículos NPC en escena: {vehiculos.Length}");

        var barricadas = Object.FindObjectsByType<BarricadaFuego>(FindObjectsSortMode.None);
        Debug.Log($"  {(barricadas.Length > 0 ? "✓" : "✗")} Barricadas de fuego: {barricadas.Length}");

        Debug.Log("───────────────────────────────────────────────────");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  1. CesiumGeoreference — origen geográfico fijado en Alsasua
    // ═══════════════════════════════════════════════════════════════════════
    static CesiumGeoreference AsegurarGeoreference()
    {
        var geo = Object.FindFirstObjectByType<CesiumGeoreference>();
        if (geo == null)
        {
            var go = new GameObject("CesiumGeoreference");
            Undo.RegisterCreatedObjectUndo(go, "Crear CesiumGeoreference");
            geo = go.AddComponent<CesiumGeoreference>();
            Debug.Log("[Setup] ✓ CesiumGeoreference CREADO");
        }
        else
            Debug.Log($"[Setup]   CesiumGeoreference ya existe en '{geo.gameObject.name}'");

        geo.latitude  = LAT;
        geo.longitude = LON;
        geo.height    = ALT_GEO;
        EditorUtility.SetDirty(geo);
        return geo;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  2. CesiumCreditSystem — obligatorio para que Cesium Ion muestre créditos
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarCreditSystem(CesiumGeoreference geo)
    {
        if (Object.FindFirstObjectByType<CesiumCreditSystem>() != null)
        { Debug.Log("[Setup]   CesiumCreditSystem ya existe"); return; }

        var go = new GameObject("CesiumCreditSystem");
        go.transform.SetParent(geo.transform, false);
        Undo.RegisterCreatedObjectUndo(go, "Crear CesiumCreditSystem");
        go.AddComponent<CesiumCreditSystem>();
        Debug.Log("[Setup] ✓ CesiumCreditSystem CREADO (bajo CesiumGeoreference)");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  3. Directional Light — iluminación solar básica
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarLuz()
    {
        foreach (var l in Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
            if (l.type == LightType.Directional)
            { Debug.Log("[Setup]   Directional Light ya existe"); return; }

        var go = new GameObject("Directional Light");
        Undo.RegisterCreatedObjectUndo(go, "Crear Directional Light");
        go.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
        var luz = go.AddComponent<Light>();
        luz.type      = LightType.Directional;
        luz.intensity = 1f;
        luz.shadows   = LightShadows.Soft;
        Debug.Log("[Setup] ✓ Directional Light CREADO");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  4. EventSystem — necesario para Input y UI
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarEventSystem()
    {
        if (Object.FindFirstObjectByType<EventSystem>() != null)
        { Debug.Log("[Setup]   EventSystem ya existe"); return; }

        var go = new GameObject("EventSystem");
        Undo.RegisterCreatedObjectUndo(go, "Crear EventSystem");
        go.AddComponent<EventSystem>();
        go.AddComponent<UnityEngine.InputSystem.UI.InputSystemUIInputModule>();
        Debug.Log("[Setup] ✓ EventSystem CREADO (InputSystemUIInputModule)");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  5. Main Camera (dron) — CamaraDron únicamente, SIN CesiumOriginShift
    //     NOTA: CesiumOriginShift va en el Jugador (a nivel de suelo) para que
    //     Cesium cargue tiles de alta resolución desde la perspectiva del jugador.
    //     Si lo ponemos en la cámara dron a Y=1500, Cesium cargaría tiles de baja
    //     resolución optimizados para vista aérea → terreno negro desde el suelo.
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarCamara()
    {
        Camera cam = Camera.main;
        // FIX PANTALLA NEGRA: la dron camera NO debe tener tag MainCamera.
        // Camera.main = primera cámara con tag 'MainCamera' → si es la dron (Y=1500) el Game view
        // muestra el cielo negro desde 1500m. CamaraFPS (hija del Jugador, Y=1.8m) es la correcta.
        // Por eso buscamos también por nombre por si el tag ya fue cambiado.
        GameObject go = cam?.gameObject ?? GameObject.Find("Main Camera");
        bool camaraExistia = go != null;

        if (!camaraExistia)
        {
            go = new GameObject("Main Camera");
            Undo.RegisterCreatedObjectUndo(go, "Crear Main Camera");
            cam = go.AddComponent<Camera>();
            Debug.Log("[Setup] ✓ Main Camera CREADO (sin AudioListener — lo gestiona CamaraFPS)");
        }
        else
        {
            if (cam == null) cam = go.GetComponent<Camera>();
            if (cam == null) cam = go.AddComponent<Camera>();
            Debug.Log($"[Setup]   Main Camera ya existe en pos={go.transform.position}");

            // Quitar CesiumOriginShift si lo tiene (bug heredado)
            var cos = go.GetComponent<CesiumOriginShift>();
            if (cos != null)
            {
                Object.DestroyImmediate(cos);
                Debug.Log("[Setup] ✓ CesiumOriginShift eliminado de Main Camera (ahora va en Jugador)");
            }
        }

        // FIX: la dron camera usa tag 'Untagged' — CamaraFPS tendrá el tag 'MainCamera'.
        go.tag = "Untagged";

        // Eliminar componentes Cesium de cámara que entran en conflicto con ControladorJugador
        var cflyTo = go.GetComponent<CesiumFlyToController>();
        if (cflyTo != null) { Object.DestroyImmediate(cflyTo); Debug.Log("[Setup] ✓ CesiumFlyToController eliminado de Main Camera"); }
        var cCamCtrl = go.GetComponent<CesiumCameraController>();
        if (cCamCtrl != null) { Object.DestroyImmediate(cCamCtrl); Debug.Log("[Setup] ✓ CesiumCameraController eliminado de Main Camera"); }

        cam.nearClipPlane = 0.3f;
        cam.farClipPlane  = 1_000_000f;
        cam.fieldOfView   = 60f;
        cam.depth         = -10f;  // renderiza por detrás de CamaraFPS (depth 0)

        if (go.GetComponent<CamaraDron>() == null)
        {
            Undo.AddComponent<CamaraDron>(go);
            Debug.Log("[Setup] ✓ CamaraDron añadido a la cámara");
        }

        go.transform.position = new Vector3(0f, CAM_ALTURA, 0f);
        go.transform.rotation = Quaternion.Euler(60f, 0f, 0f);

        // Limpiar también DynamicCamera si existe en escena
        var dynCam = GameObject.Find("DynamicCamera");
        if (dynCam != null)
        {
            var df = dynCam.GetComponent<CesiumFlyToController>();
            if (df != null) { Object.DestroyImmediate(df); Debug.Log("[Setup] ✓ CesiumFlyToController eliminado de DynamicCamera"); }
            var dc = dynCam.GetComponent<CesiumCameraController>();
            if (dc != null) { Object.DestroyImmediate(dc); Debug.Log("[Setup] ✓ CesiumCameraController eliminado de DynamicCamera"); }
            if (dynCam.CompareTag("MainCamera")) dynCam.tag = "Untagged";
        }

        EditorUtility.SetDirty(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  6. Volume de post-procesado (global, perfil URP)
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarVolumenPostProcesado()
    {
        Volume vol = Object.FindFirstObjectByType<Volume>();
        GameObject go;

        if (vol == null)
        {
            go = new GameObject("VolumenPostProcesado");
            Undo.RegisterCreatedObjectUndo(go, "Crear VolumenPostProcesado");
            vol = go.AddComponent<Volume>();
            Debug.Log("[Setup] ✓ VolumenPostProcesado CREADO");
        }
        else
        {
            go = vol.gameObject;
            Debug.Log($"[Setup]   Volume ya existe en '{go.name}'");
        }

        vol.isGlobal = true;
        vol.priority = 0f;

        const string ruta = "Assets/Settings/AlsasuaPostProfile.asset";
        System.IO.Directory.CreateDirectory(Application.dataPath + "/Settings");

        var profile = AssetDatabase.LoadAssetAtPath<VolumeProfile>(ruta);
        if (profile == null)
        {
            profile = ScriptableObject.CreateInstance<VolumeProfile>();
            AssetDatabase.CreateAsset(profile, ruta);
            Debug.Log("[Setup] ✓ VolumeProfile CREADO: " + ruta);
        }

        if (!profile.Has<Tonemapping>())
        {
            var tm = profile.Add<Tonemapping>(false);
            tm.mode.value = TonemappingMode.ACES; tm.mode.overrideState = true;
        }
        if (!profile.Has<ColorAdjustments>())
        {
            var ca = profile.Add<ColorAdjustments>(false);
            ca.contrast.value = 12f;   ca.contrast.overrideState   = true;
            ca.saturation.value = -10f; ca.saturation.overrideState = true;
            ca.colorFilter.value = Color.white; ca.colorFilter.overrideState = true;
        }
        if (!profile.Has<WhiteBalance>())
        {
            var wb = profile.Add<WhiteBalance>(false);
            wb.temperature.value = -10f; wb.temperature.overrideState = true;
        }
        if (!profile.Has<ShadowsMidtonesHighlights>())
        {
            var smh = profile.Add<ShadowsMidtonesHighlights>(false);
            smh.shadows.value = new Vector4(0.97f, 0.97f, 1.03f, 0f);
            smh.shadows.overrideState = true;
        }
        if (!profile.Has<Bloom>())
        {
            var bl = profile.Add<Bloom>(false);
            bl.threshold.value = 0.9f; bl.threshold.overrideState = true;
            bl.intensity.value = 0.25f; bl.intensity.overrideState = true;
            bl.scatter.value   = 0.65f; bl.scatter.overrideState   = true;
        }
        if (!profile.Has<Vignette>())
        {
            var vig = profile.Add<Vignette>(false);
            vig.intensity.value  = 0.25f; vig.intensity.overrideState  = true;
            vig.smoothness.value = 0.45f; vig.smoothness.overrideState = true;
            vig.rounded.value    = true;  vig.rounded.overrideState    = true;
        }
        if (!profile.Has<DepthOfField>())
        {
            var dof = profile.Add<DepthOfField>(false);
            dof.mode.value          = DepthOfFieldMode.Bokeh; dof.mode.overrideState          = true;
            dof.focusDistance.value = 800f; dof.focusDistance.overrideState = true;
            dof.aperture.value      = 2.5f; dof.aperture.overrideState      = true;
            dof.focalLength.value   = 50f;  dof.focalLength.overrideState   = true;
        }
        if (!profile.Has<MotionBlur>())
        {
            var mb = profile.Add<MotionBlur>(false);
            mb.mode.value      = MotionBlurMode.CameraAndObjects; mb.mode.overrideState      = true;
            mb.intensity.value = 0.15f; mb.intensity.overrideState = true;
            mb.clamp.value     = 0.05f; mb.clamp.overrideState     = true;
        }
        if (!profile.Has<FilmGrain>())
        {
            var fg = profile.Add<FilmGrain>(false);
            fg.type.value      = FilmGrainLookup.Thin1; fg.type.overrideState      = true;
            fg.intensity.value = 0.06f; fg.intensity.overrideState = true;
            fg.response.value  = 0.8f;  fg.response.overrideState  = true;
        }
        if (!profile.Has<ChromaticAberration>())
        {
            var ca2 = profile.Add<ChromaticAberration>(false);
            ca2.intensity.value = 0.04f; ca2.intensity.overrideState = true;
        }

        EditorUtility.SetDirty(profile);
        AssetDatabase.SaveAssets();
        vol.sharedProfile = profile;
        EditorUtility.SetDirty(vol);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  7. ManagerAlsasua — ConfiguradorAlsasua + GestionTilesets + PostProcesado
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarManager()
    {
        var config = Object.FindFirstObjectByType<ConfiguradorAlsasua>();
        GameObject go;

        if (config == null)
        {
            go = new GameObject("ManagerAlsasua");
            Undo.RegisterCreatedObjectUndo(go, "Crear ManagerAlsasua");
            go.AddComponent<ConfiguradorAlsasua>();
            Debug.Log("[Setup] ✓ ManagerAlsasua CREADO con ConfiguradorAlsasua");
        }
        else
        {
            go = config.gameObject;
            Debug.Log($"[Setup]   ConfiguradorAlsasua ya existe en '{go.name}'");
        }

        if (go.GetComponent<GestionTilesets>() == null)
        {
            // BUG FIX: usar Undo.AddComponent para que Ctrl+Z pueda deshacer la adición del componente.
            Undo.AddComponent<GestionTilesets>(go);
            Debug.Log("[Setup] ✓ GestionTilesets añadido");
        }
        // ControladorPostProcesado requiere Camera — se añade a la cámara principal, no al Manager
        Camera camPP = Camera.main;
        if (camPP != null && camPP.GetComponent<ControladorPostProcesado>() == null)
        {
            // BUG FIX: usar Undo.AddComponent para que Ctrl+Z pueda deshacer la adición del componente.
            Undo.AddComponent<ControladorPostProcesado>(camPP.gameObject);
            Debug.Log("[Setup] ✓ ControladorPostProcesado añadido a la Main Camera");
            EditorUtility.SetDirty(camPP.gameObject);
        }
        else if (camPP == null)
            Debug.LogWarning("[Setup] ✗ ControladorPostProcesado NO añadido: no hay Main Camera todavía. Ejecuta 'Configurar Escena Completa' de nuevo.");
        EditorUtility.SetDirty(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  8. GestorTexturas — TexturizadorEdificiosReales
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarGestorTexturas()
    {
        if (Object.FindFirstObjectByType<TexturizadorEdificiosReales>() != null)
        { Debug.Log("[Setup]   GestorTexturas ya existe"); return; }

        var go = new GameObject("GestorTexturas");
        Undo.RegisterCreatedObjectUndo(go, "Crear GestorTexturas");
        go.AddComponent<TexturizadorEdificiosReales>();
        Debug.Log("[Setup] ✓ GestorTexturas CREADO con TexturizadorEdificiosReales");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  9. SistemaAtmosfera — sol real + hora del día + niebla
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarAtmosfera()
    {
        if (Object.FindFirstObjectByType<SistemaAtmosfera>() != null)
        { Debug.Log("[Setup]   SistemaAtmosfera ya existe"); return; }

        var config = Object.FindFirstObjectByType<ConfiguradorAlsasua>();
        GameObject go = config != null ? config.gameObject : new GameObject("ManagerAlsasua");
        if (config == null) Undo.RegisterCreatedObjectUndo(go, "Crear ManagerAlsasua para Atmósfera");

        // BUG FIX: usar Undo.AddComponent para que Ctrl+Z pueda deshacer la adición del componente.
        // Antes usaba go.AddComponent<SistemaAtmosfera>() sin Undo, inconsistente con AsegurarClima().
        Undo.AddComponent<SistemaAtmosfera>(go);
        Debug.Log("[Setup] ✓ SistemaAtmosfera CREADO (sol astronómico + niebla dinámica)");
        EditorUtility.SetDirty(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  10. SistemaClima — lluvia, niebla, tormenta
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarClima()
    {
        if (Object.FindFirstObjectByType<SistemaClima>() != null)
        { Debug.Log("[Setup]   SistemaClima ya existe"); return; }

        var config = Object.FindFirstObjectByType<ConfiguradorAlsasua>();
        GameObject go;
        if (config != null)
        {
            go = config.gameObject;
        }
        else
        {
            go = new GameObject("ManagerAlsasua");
            // BUG 34/35 FIX: registrar el nuevo GO en el sistema de Undo para que
            // Ctrl+Z pueda deshacer la creación. Antes se omitía este registro cuando
            // config==null, dejando el GO huérfano del historial de Undo.
            Undo.RegisterCreatedObjectUndo(go, "Crear ManagerAlsasua para Clima");
        }

        Undo.AddComponent<SistemaClima>(go);
        Debug.Log("[Setup] ✓ SistemaClima CREADO (lluvia·niebla·tormenta — clima atlántico Alsasua)");
        EditorUtility.SetDirty(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  11. Jugador FPS — CharacterController + ControladorJugador + armas
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarJugador()
    {
        if (Object.FindFirstObjectByType<ControladorJugador>() != null)
        { Debug.Log("[Setup]   Jugador ya existe"); return; }

        // ── Suelo base invisible ─────────────────────────────────────────────
        // Colisionador plano a Y=0 para que personajes y coches no caigan al vacío
        // mientras los tiles de Cesium están cargando o si no tienen physic meshes.
        if (GameObject.Find("SueloBase") == null)
        {
            var suelo = GameObject.CreatePrimitive(PrimitiveType.Plane);
            suelo.name = "SueloBase";
            Undo.RegisterCreatedObjectUndo(suelo, "Crear SueloBase");
            suelo.transform.position   = Vector3.zero;
            suelo.transform.localScale = new Vector3(200f, 1f, 200f); // 2000 × 2000 m
            // Invisible: borrar el renderer pero conservar el MeshCollider
            var mr = suelo.GetComponent<MeshRenderer>();
            if (mr != null) Object.DestroyImmediate(mr);
            Debug.Log("[Setup] ✓ SueloBase CREADO (colisionador invisible 2 km × 2 km a Y=0)");
        }
        else
            Debug.Log("[Setup]   SueloBase ya existe");

        // ── Jugador ──────────────────────────────────────────────────────────
        var jugadorGO = new GameObject("Jugador");
        jugadorGO.tag   = "Player";
        jugadorGO.layer = 2;   // Capa 2 = "Ignore Raycast" → los disparos
                                // del propio jugador no se bloquean en su cuerpo
        Undo.RegisterCreatedObjectUndo(jugadorGO, "Crear Jugador");

        // FIX CESIUM: CesiumOriginShift requiere estar dentro de la jerarquía de CesiumGeoreference.
        // Sin este parenting, Cesium imprime el warning "not nested inside a CesiumGeoreference"
        // y la funcionalidad de CesiumGlobeAnchor/OriginShift no funciona.
        var geoPadre = Object.FindFirstObjectByType<CesiumGeoreference>();
        if (geoPadre != null)
        {
            jugadorGO.transform.SetParent(geoPadre.transform, worldPositionStays: false);
            Debug.Log("[Setup]   Jugador anidado bajo CesiumGeoreference (requerido por CesiumOriginShift)");
        }

        // Posición: cerca de la plaza principal, a 1 m sobre el suelo
        jugadorGO.transform.position = new Vector3(0f, 1f, -5f);

        // CharacterController (cápsula de colisión del jugador)
        var cc = jugadorGO.AddComponent<CharacterController>();
        cc.height = 1.8f;
        cc.radius = 0.3f;
        cc.center = new Vector3(0f, 0.9f, 0f);

        // Scripts de gameplay
        jugadorGO.AddComponent<ControladorJugador>();
        jugadorGO.AddComponent<SistemaDisparo>();
        jugadorGO.AddComponent<SistemaBombas>();

        // CesiumOriginShift en el Jugador (Y=1 = nivel de suelo) para que Cesium
        // cargue tiles de ALTA RESOLUCIÓN desde la perspectiva a ras de suelo.
        // NUNCA ponerlo en la Main Camera (Y=1500) o los tiles serán aéreos/borrosos.
        jugadorGO.AddComponent<CesiumOriginShift>();
        Debug.Log("[Setup] ✓ CesiumOriginShift añadido al Jugador (tiles a nivel de suelo)");

        // ── Cámara FPS (hija del jugador, a la altura de los ojos) ───────────
        var camaraGO = new GameObject("CamaraFPS");
        Undo.RegisterCreatedObjectUndo(camaraGO, "Crear CamaraFPS");
        camaraGO.transform.SetParent(jugadorGO.transform, false);
        camaraGO.transform.localPosition = new Vector3(0f, 0.8f, 0f); // ojo a 1.7 m del suelo

        // FIX PANTALLA NEGRA: CamaraFPS debe tener tag 'MainCamera' para que
        // Camera.main resuelva a la cámara del jugador (Y=1.8m), no a la dron (Y=1500m).
        camaraGO.tag = "MainCamera";

        var cam = camaraGO.AddComponent<Camera>();
        cam.nearClipPlane = 0.1f;
        cam.farClipPlane  = 1_000_000f;
        cam.fieldOfView   = 70f;
        cam.depth         = 0f;   // depth 0 = mayor prioridad que Main Camera (depth=-10)

        // FIX PANTALLA NEGRA: clearFlags=Skybox → fondo azul/cielo en vez de negro sólido.
        // Unity crea cámaras con SolidColor (negro) por defecto cuando no se especifica.
        cam.clearFlags    = CameraClearFlags.Skybox;
        cam.backgroundColor = Color.black;

        // Habilitar post-procesado (Bloom, Tonemapping, etc.) en esta cámara.
        var camData = cam.GetUniversalAdditionalCameraData();
        if (camData != null)
        {
            camData.renderPostProcessing = true;
            camData.antialiasing         = AntialiasingMode.SubpixelMorphologicalAntiAliasing;
            camData.antialiasingQuality  = AntialiasingQuality.High;
        }

        // ControladorPostProcesado gestiona Tonemapping, Bloom, DOF, etc. en runtime.
        // Requiere estar en el mismo GO que la Camera ([RequireComponent(typeof(Camera))]).
        if (camaraGO.GetComponent<ControladorPostProcesado>() == null)
            camaraGO.AddComponent<ControladorPostProcesado>();

        camaraGO.AddComponent<AudioListener>();

        EditorUtility.SetDirty(jugadorGO);
        Debug.Log("[Setup] ✓ Jugador CREADO en (0, 1, -5) con:");
        Debug.Log("[Setup]     CharacterController · ControladorJugador · SistemaDisparo · SistemaBombas");
        Debug.Log("[Setup]     Hijo: CamaraFPS (tag=MainCamera · depth=0 · far=1.000.000 · FOV=70°)");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  12. Enemigos — EnemigoPatrulla con rutas de waypoints
    //      3 patrullas en zonas clave de Alsasua (plaza, calle mayor, estación)
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarEnemigos()
    {
        if (Object.FindFirstObjectByType<EnemigoPatrulla>() != null)
        { Debug.Log("[Setup]   Enemigos ya existen"); return; }

        var grupo = new GameObject("GrupoEnemigos");
        Undo.RegisterCreatedObjectUndo(grupo, "Crear GrupoEnemigos");

        // ── Patrulla 1: Plaza San Juan (cuadrado ~20 m) ──────────────────────
        CrearEnemigo("EnemigoPatrulla_Plaza", grupo.transform,
            posInicial: new Vector3(0f, 1f, 0f),
            posWaypoints: new Vector3[]
            {
                new Vector3(  0f, 1f,   0f),
                new Vector3( 18f, 1f,   0f),
                new Vector3( 18f, 1f,  18f),
                new Vector3(  0f, 1f,  18f),
            });

        // ── Patrulla 2: Calle Mayor (recorrido lineal ~40 m) ─────────────────
        CrearEnemigo("EnemigoPatrulla_CalleM", grupo.transform,
            posInicial: new Vector3(-40f, 1f, 5f),
            posWaypoints: new Vector3[]
            {
                new Vector3(-40f, 1f,  5f),
                new Vector3(-20f, 1f,  5f),
                new Vector3(  0f, 1f,  5f),
                new Vector3(-20f, 1f,  5f),
            });

        // ── Patrulla 3: Zona estación (triángulo ~15 m) ──────────────────────
        CrearEnemigo("EnemigoPatrulla_Estacion", grupo.transform,
            posInicial: new Vector3(-10f, 1f, -30f),
            posWaypoints: new Vector3[]
            {
                new Vector3(-10f, 1f, -30f),
                new Vector3( 10f, 1f, -30f),
                new Vector3(  0f, 1f, -44f),
            });

        EditorUtility.SetDirty(grupo);
        Debug.Log("[Setup] ✓ GrupoEnemigos CREADO: 3 patrullas (plaza, calle mayor, estación)");
        Debug.Log("[Setup]   Los cuerpos 3D de los enemigos se generan al darle Play");
    }

    // Helper: crea un EnemigoPatrulla con sus waypoints
    static void CrearEnemigo(string nombre, Transform padre, Vector3 posInicial, Vector3[] posWaypoints)
    {
        var enemigoGO = new GameObject(nombre);
        Undo.RegisterCreatedObjectUndo(enemigoGO, "Crear " + nombre);
        enemigoGO.transform.SetParent(padre, true);
        enemigoGO.transform.position = posInicial;

        var enemigo = enemigoGO.AddComponent<EnemigoPatrulla>();

        // Crear waypoints como GameObjects vacíos (hijos del grupo para mantener orden)
        var wpList = new List<Transform>();
        for (int i = 0; i < posWaypoints.Length; i++)
        {
            var wpGO = new GameObject($"{nombre}_WP{i}");
            Undo.RegisterCreatedObjectUndo(wpGO, $"Crear WP {nombre} {i}");
            wpGO.transform.SetParent(padre, true);
            wpGO.transform.position = posWaypoints[i];
            wpList.Add(wpGO.transform);
        }

        // Asignar waypoints al componente usando SerializedObject (serialización correcta)
        // El campo en EnemigoPatrulla se llama "waypointsPatrulla" (Transform[])
        var so     = new SerializedObject(enemigo);
        var wpProp = so.FindProperty("waypointsPatrulla");
        wpProp.arraySize = wpList.Count;
        for (int i = 0; i < wpList.Count; i++)
            wpProp.GetArrayElementAtIndex(i).objectReferenceValue = wpList[i];
        so.ApplyModifiedProperties();

        EditorUtility.SetDirty(enemigoGO);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  13. Vehículos NPC — coches que siguen rutas por las calles de Alsasua
    //      2 coches: ruta este-oeste (Calle Mayor) y ruta norte-sur (travesía)
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarVehiculos()
    {
        if (Object.FindFirstObjectByType<VehiculoNPC>() != null)
        { Debug.Log("[Setup]   Vehículos NPC ya existen"); return; }

        var grupo = new GameObject("GrupoVehiculos");
        Undo.RegisterCreatedObjectUndo(grupo, "Crear GrupoVehiculos");

        // ── Coche 1: Calle Mayor, sentido E→O (ciclo en 6 waypoints) ─────────
        CrearVehiculo("CocheNPC_CalleM", grupo.transform,
            posInicial: new Vector3(-70f, 1f, -15f),
            posWaypoints: new Vector3[]
            {
                new Vector3(-70f, 1f, -15f),
                new Vector3(-30f, 1f, -15f),
                new Vector3(  0f, 1f, -15f),
                new Vector3( 55f, 1f, -15f),
                new Vector3( 55f, 1f, -22f),  // giro
                new Vector3(-70f, 1f, -22f),  // vuelta al inicio por carril contrario
            });

        // ── Coche 2: Travesía Norte-Sur ───────────────────────────────────────
        CrearVehiculo("CocheNPC_NS", grupo.transform,
            posInicial: new Vector3(28f, 1f, -65f),
            posWaypoints: new Vector3[]
            {
                new Vector3(28f, 1f, -65f),
                new Vector3(28f, 1f, -20f),
                new Vector3(28f, 1f,   0f),
                new Vector3(28f, 1f,  35f),
                new Vector3(35f, 1f,  35f),  // giro
                new Vector3(35f, 1f, -65f),  // vuelta
            });

        EditorUtility.SetDirty(grupo);
        Debug.Log("[Setup] ✓ GrupoVehiculos CREADO: 2 coches NPC con rutas (Calle Mayor + travesía N-S)");
    }

    // Helper: crea un VehiculoNPC con carrocería básica y waypoints
    static void CrearVehiculo(string nombre, Transform padre, Vector3 posInicial, Vector3[] posWaypoints)
    {
        // ── GameObject raíz ──────────────────────────────────────────────────
        var go = new GameObject(nombre);
        Undo.RegisterCreatedObjectUndo(go, "Crear " + nombre);
        go.transform.SetParent(padre, true);
        go.transform.position = posInicial;

        // ── Carrocería (visual, sin colisionador propio) ─────────────────────
        var cuerpo = GameObject.CreatePrimitive(PrimitiveType.Cube);
        cuerpo.name = "Carroceria";
        cuerpo.transform.SetParent(go.transform, false);
        cuerpo.transform.localPosition = new Vector3(0f, 0.45f, 0f);
        cuerpo.transform.localScale    = new Vector3(1.8f, 0.8f, 4.2f);
        var colCuerpo = cuerpo.GetComponent<Collider>();
        if (colCuerpo != null) Object.DestroyImmediate(colCuerpo);

        // ── Techo ────────────────────────────────────────────────────────────
        var techo = GameObject.CreatePrimitive(PrimitiveType.Cube);
        techo.name = "Techo";
        techo.transform.SetParent(go.transform, false);
        techo.transform.localPosition = new Vector3(0f, 1.05f, -0.2f);
        techo.transform.localScale    = new Vector3(1.6f, 0.55f, 2.4f);
        var colTecho = techo.GetComponent<Collider>();
        if (colTecho != null) Object.DestroyImmediate(colTecho);

        // ── Colisionador principal del coche ─────────────────────────────────
        var boxCol = go.AddComponent<BoxCollider>();
        boxCol.center = new Vector3(0f, 0.45f, 0f);
        boxCol.size   = new Vector3(1.8f, 1.4f, 4.2f);

        // ── Rigidbody (requerido por VehiculoNPC) ────────────────────────────
        var rb = go.AddComponent<Rigidbody>();
        rb.constraints = RigidbodyConstraints.FreezeRotationX | RigidbodyConstraints.FreezeRotationZ;
        rb.linearDamping        = 1.5f;

        // ── Script NPC ───────────────────────────────────────────────────────
        var npc = go.AddComponent<VehiculoNPC>();

        // ── Waypoints ────────────────────────────────────────────────────────
        var wpList = new List<Transform>();
        for (int i = 0; i < posWaypoints.Length; i++)
        {
            var wpGO = new GameObject($"{nombre}_WP{i}");
            Undo.RegisterCreatedObjectUndo(wpGO, $"Crear WP {nombre} {i}");
            wpGO.transform.SetParent(padre, true);
            wpGO.transform.position = posWaypoints[i];
            wpList.Add(wpGO.transform);
        }

        // Asignar waypoints via SerializedObject
        var so     = new SerializedObject(npc);
        var wpProp = so.FindProperty("waypoints");
        wpProp.arraySize = wpList.Count;
        for (int i = 0; i < wpList.Count; i++)
            wpProp.GetArrayElementAtIndex(i).objectReferenceValue = wpList[i];
        so.ApplyModifiedProperties();

        EditorUtility.SetDirty(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  14. Barricadas con fuego — obstáculos en intersecciones clave
    //      3 barricadas bloqueando entradas a la plaza principal
    // ═══════════════════════════════════════════════════════════════════════
    static void AsegurarBarricadas()
    {
        if (Object.FindFirstObjectByType<BarricadaFuego>() != null)
        { Debug.Log("[Setup]   Barricadas ya existen"); return; }

        var grupo = new GameObject("GrupoBarricadas");
        Undo.RegisterCreatedObjectUndo(grupo, "Crear GrupoBarricadas");

        // (posición_local, rotaciónY) — bloquean accesos a la plaza
        var datos = new (Vector3 pos, float rotY)[]
        {
            (new Vector3( 22f, 0f,  0f),   0f),   // acceso este de la plaza
            (new Vector3(-22f, 0f,  5f),   0f),   // acceso oeste de la plaza
            (new Vector3(  0f, 0f, 24f),  90f),   // acceso norte (calle transversal)
        };

        foreach (var (pos, rotY) in datos)
        {
            var barricadaGO = new GameObject("Barricada");
            Undo.RegisterCreatedObjectUndo(barricadaGO, "Crear Barricada");
            barricadaGO.transform.SetParent(grupo.transform, true);
            barricadaGO.transform.position = pos;
            barricadaGO.transform.rotation = Quaternion.Euler(0f, rotY, 0f);
            barricadaGO.AddComponent<BarricadaFuego>();
            EditorUtility.SetDirty(barricadaGO);
        }

        EditorUtility.SetDirty(grupo);
        Debug.Log("[Setup] ✓ GrupoBarricadas CREADO: 3 barricadas en accesos a la plaza");
        Debug.Log("[Setup]   Las estructuras y el fuego se generan al darle Play");
    }
}
