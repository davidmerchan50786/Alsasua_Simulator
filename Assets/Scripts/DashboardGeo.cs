// Assets/Scripts/DashboardGeo.cs
// Dashboard táctico geoespacial — replica look & funciones de mi-dashboard-geo (Cesium.js)
//
// ESTÉTICA: panel monoespacio verde oscuro idéntico al "Spy Dashboard" web
// FUNCIONES:
//   · LOCALIZADOR_GPE : botones FlyTo a Pamplona/Vitoria/Donosti/Bilbao/Altsasu
//   · ÓPTICAS_AVANZADAS: NVG (visión nocturna verde) y FLIR Térmica (azul→naranja)
//   · Atajos de teclado: 1–5 ubicaciones | F1 Normal | F2 NVG | F3 FLIR
//
// CONFIGURACIÓN:
//   1. Añadir este script a un GameObject vacío en la escena (p.ej. "GestorDashboard")
//   2. El script busca Volume, CamaraDron y CesiumGlobeAnchor automáticamente
//   3. Si Main Camera no tiene CesiumGlobeAnchor, se añade en runtime
//
// COMPATIBILIDAD: Unity 6 · URP 17 · Cesium for Unity 2.x
// ─────────────────────────────────────────────────────────────────────────────

using System.Collections;
using Unity.Mathematics;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;
using CesiumForUnity;

public class DashboardGeo : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  INSPECTOR
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ REFERENCIAS (opcionales — se buscan automáticamente) ═══")]
    [Tooltip("Volume URP global. Si es null se busca en escena.")]
    [SerializeField] private Volume volumenGlobal;

    [Tooltip("ControladorJugador (modo a pie). Se pausa durante el salto entre ciudades.")]
    [SerializeField] private ControladorJugador controladorJugador;

    [Tooltip("CamaraDron (modo vuelo libre). Alternativo a ControladorJugador.")]
    [SerializeField] private CamaraDron camaraDron;

    [Header("═══ VUELO ═══")]
    [Range(1f, 8f)]
    [Tooltip("Duración en segundos del vuelo suavizado a cada destino.")]
    [SerializeField] private float duracionVuelo = 3f;

    // ═══════════════════════════════════════════════════════════════════════
    //  OBJETIVOS GEOGRÁFICOS  (idénticos a mi-dashboard-geo/src/main.ts)
    // ═══════════════════════════════════════════════════════════════════════

    private struct Objetivo
    {
        public string nombre;
        public double lon, lat;
        // alt    : altura sobre el elipsoide WGS84 para el modo cámara libre (vista aérea)
        // altPie : altura de spawn del jugador (terreno estimado + 150 m de margen de caída)
        //          El jugador spawna aquí arriba y la gravedad lo lleva al suelo.
        public double alt, altPie;
        public float  heading, pitch;
    }

    private static readonly Objetivo[] Objetivos =
    {
        //                                                alt   altPie   hdg  pitch
        new Objetivo { nombre="PAMPLONA", lon=-1.6432, lat=42.8125, alt= 280, altPie= 565, heading=  0, pitch=-25 },
        new Objetivo { nombre="VITORIA",  lon=-2.6727, lat=42.8465, alt= 280, altPie= 675, heading= 45, pitch=-20 },
        new Objetivo { nombre="DONOSTI",  lon=-1.9812, lat=43.3209, alt= 300, altPie= 160, heading=180, pitch=-15 },
        new Objetivo { nombre="BILBAO",   lon=-2.9350, lat=43.2630, alt= 280, altPie= 170, heading= 90, pitch=-20 },
        new Objetivo { nombre="ALTSASU",  lon=-2.1685, lat=42.8953, alt= 180, altPie= 680, heading=  0, pitch=-12 },
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  MODOS DE VISIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private enum ModoVision { Normal, NVG, Termica }
    private ModoVision modoActual = ModoVision.Normal;

    // Efectos URP Volume
    private ColorAdjustments          colorAdj;
    private Bloom                      bloom;
    private Vignette                   vignette;
    private ShadowsMidtonesHighlights  smh;

    // Backup de valores originales (guardados tras ControladorPostProcesado.Start())
    private float   savedSat, savedContrast, savedExposure;
    private Color   savedColorFilter;
    private float   savedBloomThreshold, savedBloomIntensity, savedBloomScatter;
    private float   savedVignetteIntensity, savedVignetteSmoothness;
    private Vector4 savedSmhShadows, savedSmhMidtones, savedSmhHighlights;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO
    // ═══════════════════════════════════════════════════════════════════════

    private CesiumGlobeAnchor anchorCamara;
    private bool              enVuelo  = false;
    private string            destino  = "";

    // ═══════════════════════════════════════════════════════════════════════
    //  GUI — ESTILOS  (idéntico al panel CSS de mi-dashboard-geo)
    //  background: rgba(0,15,0,0.9)  border: #0f0  font: monospace  color: #0f0
    // ═══════════════════════════════════════════════════════════════════════

    private GUIStyle styleBorderL, styleBorderR, styleBorderT, styleBorderB;
    private GUIStyle styleHeader;
    private GUIStyle styleHint;
    private GUIStyle styleBtnBase;
    private GUIStyle styleBtnNVGActivo;
    private GUIStyle styleBtnFLIRActivo;
    private bool guiInit = false;

    // Textures reutilizables (creadas una sola vez)
    private Texture2D texPanelBG;
    private Texture2D texBtnBase;
    private Texture2D texBtnHover;
    private Texture2D texBtnNVG;
    private Texture2D texBtnFLIR;
    private Texture2D texBtnNVGHover;
    private Texture2D texBtnFLIRHover;

    // Paleta (mismo hex que el CSS)
    private static readonly Color CssVerde    = new Color(0.000f, 1.000f, 0.000f, 1f); // #0f0
    private static readonly Color CssNaranja  = new Color(1.000f, 0.533f, 0.000f, 1f); // #f80
    private static readonly Color CssPanelBG  = new Color(0.000f, 0.059f, 0.000f, 0.9f); // rgba(0,15,0,0.9)
    private static readonly Color CssBtnBG    = new Color(0.000f, 0.067f, 0.000f, 1f); // #001100
    private static readonly Color CssNVGBG    = new Color(0.000f, 0.200f, 0.000f, 1f);
    private static readonly Color CssFLIRBG   = new Color(0.200f, 0.100f, 0.000f, 1f);

    // Layout del panel  (mismo tamaño que el web: 230px)
    private const float PanelX = 15f;
    private const float PanelY = 15f;
    private const float PanelW = 245f;
    private const float BtnH   = 26f;
    private const float Gap    = 5f;
    private const float PadX   = 14f;
    private const float PadY   = 14f;

    // ═══════════════════════════════════════════════════════════════════════
    //  INICIALIZACIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private void Start()
    {
        // Buscar Volume global
        if (volumenGlobal == null)
            volumenGlobal = Object.FindFirstObjectByType<Volume>();

        // FIX: NO obtener efectos del Volume profile aquí — ControladorPostProcesado.Start()
        // (order 0, mismo frame) todavía no ha llamado p.Add<T>() en este punto si
        // DashboardGeo también tiene order 0 y corre antes. Los efectos se obtienen en
        // GuardarValoresOriginales() que se invoca con Invoke(0f), garantizando que
        // todos los Start() hayan terminado antes de hacer TryGet.

        // Buscar controladores si no están asignados
        if (controladorJugador == null)
            controladorJugador = Object.FindFirstObjectByType<ControladorJugador>();
        if (camaraDron == null)
            camaraDron = Object.FindFirstObjectByType<CamaraDron>();

        // El CesiumGlobeAnchor del jugador/cámara lo añade ConfiguradorAlsasua en Start().
        // Lo buscamos con un pequeño retraso para que todos los Start() hayan terminado.
        // Invoke(0f) = final del primer frame, después de todos los Start().
        Invoke(nameof(GuardarValoresOriginales), 0f);
        Invoke(nameof(IrAPamplonaInicio), 0.15f);
    }

    private void IrAPamplonaInicio()
    {
        BuscarAnchor();
        IrA(0);
    }

    // Busca el CesiumGlobeAnchor adecuado para el fly-to:
    //   · Modo jugador a pie → anchor del ControladorJugador (mueve al jugador)
    //   · Modo cámara libre  → anchor de la Main Camera
    private void BuscarAnchor()
    {
        if (anchorCamara != null) return;

        if (controladorJugador != null)
        {
            anchorCamara = controladorJugador.GetComponent<CesiumGlobeAnchor>();
            if (anchorCamara != null)
            {
                Debug.Log("[DashboardGeo] Anchor del jugador encontrado.");
                return;
            }
        }

        if (Camera.main != null)
        {
            anchorCamara = Camera.main.GetComponent<CesiumGlobeAnchor>();
            if (anchorCamara == null)
                anchorCamara = Camera.main.GetComponentInParent<CesiumGlobeAnchor>();
            if (anchorCamara == null)
            {
                anchorCamara = Camera.main.gameObject.AddComponent<CesiumGlobeAnchor>();
                Debug.Log("[DashboardGeo] CesiumGlobeAnchor añadido a Main Camera.");
            }
        }
    }

    private void GuardarValoresOriginales()
    {
        // FIX: obtener efectos aquí — ControladorPostProcesado.Start() ya ha corrido
        // y añadido los efectos al profile mediante p.Add<T>().
        // Invoke(nameof(GuardarValoresOriginales), 0f) garantiza ejecución post-Start.
        if (volumenGlobal != null && volumenGlobal.profile != null)
        {
            var p = volumenGlobal.profile;
            p.TryGet(out colorAdj);
            p.TryGet(out bloom);
            p.TryGet(out vignette);
            p.TryGet(out smh);
        }

        if (colorAdj != null)
        {
            savedSat         = colorAdj.saturation.value;
            savedContrast    = colorAdj.contrast.value;
            savedExposure    = colorAdj.postExposure.value;
            savedColorFilter = colorAdj.colorFilter.value;
        }
        if (bloom != null)
        {
            savedBloomThreshold = bloom.threshold.value;
            savedBloomIntensity = bloom.intensity.value;
            savedBloomScatter   = bloom.scatter.value;
        }
        if (vignette != null)
        {
            savedVignetteIntensity  = vignette.intensity.value;
            savedVignetteSmoothness = vignette.smoothness.value;
        }
        if (smh != null)
        {
            savedSmhShadows    = smh.shadows.value;
            savedSmhMidtones   = smh.midtones.value;
            savedSmhHighlights = smh.highlights.value;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INPUT — atajos de teclado
    // ═══════════════════════════════════════════════════════════════════════

    private void Update()
    {
        var kb = Keyboard.current;
        if (kb == null) return;

        // Ubicaciones: teclas 1–5
        if (kb.digit1Key.wasPressedThisFrame) IrA(0);
        if (kb.digit2Key.wasPressedThisFrame) IrA(1);
        if (kb.digit3Key.wasPressedThisFrame) IrA(2);
        if (kb.digit4Key.wasPressedThisFrame) IrA(3);
        if (kb.digit5Key.wasPressedThisFrame) IrA(4);

        // Visión: F1 Normal · F2 NVG · F3 FLIR
        if (kb.f1Key.wasPressedThisFrame) SetVision(ModoVision.Normal);
        if (kb.f2Key.wasPressedThisFrame) SetVision(ModoVision.NVG);
        if (kb.f3Key.wasPressedThisFrame) SetVision(ModoVision.Termica);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  GUI TÁCTICA — replica panel CSS de mi-dashboard-geo
    // ═══════════════════════════════════════════════════════════════════════

    private void OnGUI()
    {
        if (!guiInit) InicializarEstilos();

        float innerW = PanelW - PadX * 2f;

        // ── Calcular altura total del panel ─────────────────────────────────
        float panelH = PadY
            + 18f + 4f               // header LOCALIZADOR_GPE + espacio
            + 1f  + 6f               // línea separadora
            + (BtnH + Gap) * 2f      // fila Pamplona/Vitoria + fila Donosti/Bilbao
            + BtnH + Gap             // fila Altsasu full
            + 14f                    // espacio
            + 18f + 4f               // header ÓPTICAS_AVANZADAS
            + 1f  + 6f               // línea separadora
            + BtnH + Gap             // botón Luz Visible
            + BtnH + Gap             // fila NVG + FLIR
            + 14f                    // espacio
            + (enVuelo ? 22f : 0f)   // estado vuelo
            + 38f                    // hint
            + PadY;

        Rect panelRect = new Rect(PanelX, PanelY, PanelW, panelH);

        // ── Fondo oscuro ────────────────────────────────────────────────────
        GUI.DrawTexture(panelRect, texPanelBG);

        // ── Borde verde (1px) sobre el fondo ────────────────────────────────
        var borderColor = modoActual == ModoVision.Termica ? CssNaranja : CssVerde;
        DibujarBorde(panelRect, borderColor, 1f);

        // ── Contenido ───────────────────────────────────────────────────────
        float y = PanelY + PadY;
        float x = PanelX + PadX;

        // -- LOCALIZADOR_GPE --
        GUI.Label(new Rect(x, y, innerW, 18f), ">> LOCALIZADOR_GPE", styleHeader);
        y += 22f;
        DibujarLinea(x, y, innerW);
        y += 7f;

        float btnW2 = (innerW - Gap) / 2f;

        // Pamplona | Vitoria
        if (DibujarBoton(new Rect(x,              y, btnW2, BtnH), "PAMPLONA")) IrA(0);
        if (DibujarBoton(new Rect(x + btnW2 + Gap, y, btnW2, BtnH), "VITORIA"))  IrA(1);
        y += BtnH + Gap;

        // Donosti | Bilbao
        if (DibujarBoton(new Rect(x,              y, btnW2, BtnH), "DONOSTI")) IrA(2);
        if (DibujarBoton(new Rect(x + btnW2 + Gap, y, btnW2, BtnH), "BILBAO"))  IrA(3);
        y += BtnH + Gap;

        // Altsasu — ancho completo
        if (DibujarBoton(new Rect(x, y, innerW, BtnH), "ALTSASU  [MODO SAT]")) IrA(4);
        y += BtnH + 14f;

        // -- ÓPTICAS_AVANZADAS --
        GUI.Label(new Rect(x, y, innerW, 18f), ">> ÓPTICAS_AVANZADAS", styleHeader);
        y += 22f;
        DibujarLinea(x, y, innerW);
        y += 7f;

        // Luz visible — ancho completo
        if (DibujarBoton(new Rect(x, y, innerW, BtnH), "LUZ VISIBLE  (RESETEAR)")) SetVision(ModoVision.Normal);
        y += BtnH + Gap;

        // NVG | FLIR
        bool nvgAct  = modoActual == ModoVision.NVG;
        bool flirAct = modoActual == ModoVision.Termica;

        if (GUI.Button(new Rect(x,              y, btnW2, BtnH), "NOCTURNA (NVG)",  nvgAct  ? styleBtnNVGActivo  : styleBtnBase)) SetVision(ModoVision.NVG);
        if (GUI.Button(new Rect(x + btnW2 + Gap, y, btnW2, BtnH), "TÉRMICA (FLIR)", flirAct ? styleBtnFLIRActivo : styleBtnBase)) SetVision(ModoVision.Termica);
        y += BtnH + 14f;

        // Estado de vuelo
        if (enVuelo)
        {
            GUI.Label(new Rect(x, y, innerW, 18f), $">> TRAZANDO RUTA... {destino}", styleHeader);
            y += 22f;
        }

        // Hint inferior
        GUI.Label(new Rect(x, y, innerW, 38f),
            "[MANTÉN BOTÓN DERECHO PARA ÓRBITA 360°]\n[1-5 ubicaciones | F1 normal | F2 NVG | F3 FLIR]",
            styleHint);
    }

    // ── Helpers GUI ─────────────────────────────────────────────────────────

    private bool DibujarBoton(Rect r, string label)
        => GUI.Button(r, label, styleBtnBase);

    private void DibujarLinea(float x, float y, float w)
    {
        var prev = GUI.color;
        GUI.color = new Color(0f, 0.5f, 0f, 0.55f);
        GUI.DrawTexture(new Rect(x, y, w, 1f), Texture2D.whiteTexture);
        GUI.color = prev;
    }

    private void DibujarBorde(Rect r, Color color, float grosor)
    {
        var prev = GUI.color;
        GUI.color = color;
        GUI.DrawTexture(new Rect(r.x,                 r.y,                  r.width,  grosor), Texture2D.whiteTexture); // top
        GUI.DrawTexture(new Rect(r.x,                 r.y + r.height - grosor, r.width, grosor), Texture2D.whiteTexture); // bottom
        GUI.DrawTexture(new Rect(r.x,                 r.y,                  grosor,   r.height), Texture2D.whiteTexture); // left
        GUI.DrawTexture(new Rect(r.x + r.width - grosor, r.y,               grosor,   r.height), Texture2D.whiteTexture); // right
        GUI.color = prev;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INICIALIZACIÓN DE ESTILOS  (solo dentro de OnGUI)
    // ═══════════════════════════════════════════════════════════════════════

    private void InicializarEstilos()
    {
        texPanelBG       = MakeTex(CssPanelBG);
        texBtnBase       = MakeTex(CssBtnBG);
        texBtnHover      = MakeTex(CssVerde);
        texBtnNVG        = MakeTex(CssNVGBG);
        texBtnFLIR       = MakeTex(CssFLIRBG);
        texBtnNVGHover   = MakeTex(new Color(0f, 0.8f, 0f));
        texBtnFLIRHover  = MakeTex(CssNaranja);

        // Cabeceras ">> SECCION"
        styleHeader = new GUIStyle(GUI.skin.label)
        {
            fontSize  = 11,
            fontStyle = FontStyle.Bold,
        };
        styleHeader.normal.textColor = CssVerde;

        // Hint inferior
        styleHint = new GUIStyle(GUI.skin.label)
        {
            fontSize  = 9,
            wordWrap  = true,
        };
        styleHint.normal.textColor = new Color(0f, 1f, 0f, 0.65f);

        // Botón base (verde sobre negro)
        styleBtnBase = CrearEstiloBtn(
            CssVerde,   texBtnBase,
            Color.black, texBtnHover);

        // Botón NVG activo (verde más intenso)
        styleBtnNVGActivo = CrearEstiloBtn(
            CssVerde,   texBtnNVG,
            Color.black, texBtnNVGHover);

        // Botón FLIR activo (naranja)
        styleBtnFLIRActivo = CrearEstiloBtn(
            CssNaranja, texBtnFLIR,
            Color.black, texBtnFLIRHover);

        guiInit = true;
    }

    private static GUIStyle CrearEstiloBtn(
        Color textoNormal, Texture2D bgNormal,
        Color textoHover,  Texture2D bgHover)
    {
        var s = new GUIStyle(GUI.skin.button)
        {
            fontSize  = 10,
            alignment = TextAnchor.MiddleCenter,
            padding   = new RectOffset(4, 4, 5, 5),
        };
        s.normal.textColor   = textoNormal;
        s.normal.background  = bgNormal;
        s.hover.textColor    = textoHover;
        s.hover.background   = bgHover;
        s.active.textColor   = textoHover;
        s.active.background  = bgHover;
        s.focused.textColor  = textoNormal;
        s.focused.background = bgNormal;
        return s;
    }

    private static Texture2D MakeTex(Color color)
    {
        var t = new Texture2D(1, 1);
        t.SetPixel(0, 0, color);
        t.Apply();
        return t;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  VUELO SUAVIZADO A OBJETIVO  (equivale a viewer.camera.flyTo con duration:3)
    // ═══════════════════════════════════════════════════════════════════════

    private void IrA(int idx)
    {
        StopAllCoroutines();
        StartCoroutine(VolarA(idx));
    }

    private IEnumerator VolarA(int idx)
    {
        var obj = Objetivos[idx];

        // Buscar anchor si aún no está disponible (puede que ConfiguradorAlsasua
        // lo haya añadido después de que DashboardGeo.Start() corriera)
        BuscarAnchor();

        if (anchorCamara == null)
        {
            Debug.LogWarning("[DashboardGeo] Sin CesiumGlobeAnchor — teleport cancelado.");
            yield break;
        }

        enVuelo = true;
        destino = obj.nombre;

        // ── Elegir altura según el modo activo ──────────────────────────────
        // · Jugador a pie → altPie (spawn sobre el terreno; la gravedad lo baja)
        // · Cámara libre  → alt   (punto de vista aéreo del web dashboard)
        bool modoAPie  = controladorJugador != null && controladorJugador.isActiveAndEnabled;
        double altDestino = modoAPie ? obj.altPie : obj.alt;

        // ── Pausar control manual durante el teleport ────────────────────────
        if (controladorJugador != null) controladorJugador.enabled = false;
        if (camaraDron         != null) camaraDron.enabled         = false;

        // ── Coordenadas de origen y destino ──────────────────────────────────
        double3 startLLH  = anchorCamara.longitudeLatitudeHeight;
        double3 targetLLH = new double3(obj.lon, obj.lat, altDestino);

        Quaternion startRot = Camera.main != null
            ? Camera.main.transform.rotation
            : Quaternion.identity;
        // En modo a pie la cámara la controla ControladorJugador → no la rotamos
        Quaternion targetRot = modoAPie
            ? Quaternion.Euler(0f, obj.heading, 0f)
            : Quaternion.Euler(obj.pitch, obj.heading, 0f);

        // ── Interpolación suavizada (SmoothStep = ease-in/out como Cesium.js) ─
        float elapsed = 0f;
        while (elapsed < duracionVuelo)
        {
            double t = Mathf.SmoothStep(0f, 1f, elapsed / duracionVuelo);
            anchorCamara.longitudeLatitudeHeight = new double3(
                startLLH.x + (targetLLH.x - startLLH.x) * t,
                startLLH.y + (targetLLH.y - startLLH.y) * t,
                startLLH.z + (targetLLH.z - startLLH.z) * t
            );
            if (Camera.main != null && !modoAPie)
                Camera.main.transform.rotation = Quaternion.Slerp(startRot, targetRot, (float)t);
            elapsed += Time.deltaTime;
            yield return null;
        }

        // ── Valores finales exactos ──────────────────────────────────────────
        anchorCamara.longitudeLatitudeHeight = targetLLH;
        if (Camera.main != null && !modoAPie)
            Camera.main.transform.rotation = targetRot;

        // V3 FIX: Protección contra caída en streaming de terreno (Async Topography Validation)
        // Evita que el jugador caiga al infinito si los tiles de Cesium aún no se han descargado.
        if (modoAPie && controladorJugador != null)
        {
            float maxEspera = 10f; // Timeout de seguridad
            float t = 0f;
            // Raycast hacia abajo buscando terreno real (ignorar Triggers)
            // Mientras no encuentre suelo, el jugador permanecerá levitando congelado sin gravedad.
            while (!Physics.Raycast(controladorJugador.transform.position, Vector3.down, (float)obj.altPie * 1.5f, ~0, QueryTriggerInteraction.Ignore) && t < maxEspera)
            {
                t += Time.deltaTime;
                yield return null;
            }
        }

        enVuelo = false;
        destino = "";

        // ── Reactivar control manual ─────────────────────────────────────────
        if (controladorJugador != null) controladorJugador.enabled = true;
        if (camaraDron         != null) camaraDron.enabled         = true;

        Debug.Log($"[DashboardGeo] ✓ En destino: {obj.nombre}  " +
                  $"({obj.lat:F4}°N, {obj.lon:F4}°E, {altDestino:F0}m)" +
                  $"  [{(modoAPie ? "modo pie — cayendo al suelo" : "modo aéreo")}]");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MODOS DE VISIÓN  (equivale a window.setVision en el web dashboard)
    //
    //  Los shaders GLSL del web se replican mediante overrides del URP Volume:
    //
    //  NVG:     lum = dot(color, vec3(0.299,0.587,0.114)); out = (0, lum*2, 0, 1)
    //    →  saturation=-100 (escala grises) + colorFilter=verde + contraste alto
    //
    //  Térmica: gray → mix(blue(0,0.1,0.5), orange(1,0.6,0.1), gray)
    //    →  saturation=-100 + SMH: sombras=azul frío, luces=naranja calor
    // ═══════════════════════════════════════════════════════════════════════

    private void SetVision(ModoVision modo)
    {
        modoActual = modo;

        if (colorAdj == null)
        {
            Debug.LogWarning("[DashboardGeo] Volume sin ColorAdjustments — verifica que " +
                             "ControladorPostProcesado haya configurado el Volume profile.");
            return;
        }

        switch (modo)
        {
            case ModoVision.Normal:  AplicarNormal();  break;
            case ModoVision.NVG:     AplicarNVG();     break;
            case ModoVision.Termica: AplicarTermica(); break;
        }

        Debug.Log($"[DashboardGeo] Modo visión → {modo}");
    }

    // ── Normal — restaura preset de ControladorPostProcesado ────────────────

    private void AplicarNormal()
    {
        if (colorAdj != null)
        {
            Set(colorAdj.saturation,   savedSat);
            Set(colorAdj.contrast,     savedContrast);
            Set(colorAdj.postExposure, savedExposure);
            Set(colorAdj.colorFilter,  savedColorFilter);
        }
        if (bloom != null)
        {
            Set(bloom.threshold,  savedBloomThreshold);
            Set(bloom.intensity,  savedBloomIntensity);
            Set(bloom.scatter,    savedBloomScatter);
        }
        if (vignette != null)
        {
            Set(vignette.intensity,  savedVignetteIntensity);
            Set(vignette.smoothness, savedVignetteSmoothness);
        }
        if (smh != null)
        {
            smh.shadows.value         = savedSmhShadows;
            smh.shadows.overrideState = true;
            smh.midtones.value        = savedSmhMidtones;
            smh.midtones.overrideState = true;
            smh.highlights.value      = savedSmhHighlights;
            smh.highlights.overrideState = true;
        }
    }

    // ── NVG — Visión Nocturna Verde ─────────────────────────────────────────
    // Shader web: out = vec4(0.0, lum * 2.0, 0.0, 1.0)
    // URP Volume:
    //   · saturation = -100  → escala de grises (calcula lum)
    //   · postExposure = 1.1 → simula el "lum * 2.0" del shader
    //   · colorFilter = verde puro → tiñe el buffer en canal verde
    //   · contraste alto, bloom y viñeta para efecto de tubo NVG

    private void AplicarNVG()
    {
        if (colorAdj != null)
        {
            Set(colorAdj.saturation,   -100f);
            Set(colorAdj.contrast,      58f);
            Set(colorAdj.postExposure,  1.1f);
            Set(colorAdj.colorFilter,   new Color(0f, 1f, 0f));
        }
        if (bloom != null)
        {
            Set(bloom.threshold,  0.55f);
            Set(bloom.intensity,  0.75f);
            Set(bloom.scatter,    0.5f);
        }
        if (vignette != null)
        {
            Set(vignette.intensity,  0.42f);
            Set(vignette.smoothness, 0.6f);
        }
        // Restaurar SMH a neutral para no interferir
        if (smh != null)
        {
            smh.shadows.value         = new Vector4(1f, 1f, 1f, 0f);
            smh.shadows.overrideState = true;
            smh.midtones.value        = new Vector4(1f, 1f, 1f, 0f);
            smh.midtones.overrideState = true;
            smh.highlights.value      = new Vector4(1f, 1f, 1f, 0f);
            smh.highlights.overrideState = true;
        }
    }

    // ── FLIR Térmica ────────────────────────────────────────────────────────
    // Shader web: gray → mix(blue(0,0.1,0.5), orange(1,0.6,0.1), gray)
    // URP Volume:
    //   · saturation = -100  → escala de grises
    //   · SMH Shadows → azul frío (zonas oscuras/frías)
    //   · SMH Midtones → violeta transición
    //   · SMH Highlights → naranja/rojo calor (zonas brillantes/calientes)
    //   El resultado aproxima el gradiente azul→naranja del shader GLSL

    private void AplicarTermica()
    {
        if (colorAdj != null)
        {
            Set(colorAdj.saturation,   -100f);
            Set(colorAdj.contrast,      28f);
            Set(colorAdj.postExposure,  0f);
            Set(colorAdj.colorFilter,   Color.white);
        }
        if (smh != null)
        {
            // blue(0, 0.1, 0.5) = zonas frías/oscuras
            smh.shadows.value         = new Vector4(0f, 0.12f, 0.62f, 0f);
            smh.shadows.overrideState = true;
            // transición violeta
            smh.midtones.value        = new Vector4(0.55f, 0.18f, 0.38f, 0f);
            smh.midtones.overrideState = true;
            // orange(1, 0.6, 0.1) = zonas calientes/brillantes
            smh.highlights.value      = new Vector4(1f, 0.60f, 0.10f, 0f);
            smh.highlights.overrideState = true;
        }
        if (bloom != null)
        {
            Set(bloom.threshold,  0.80f);
            Set(bloom.intensity,  0.20f);
            Set(bloom.scatter,    0.65f);
        }
        if (vignette != null)
        {
            Set(vignette.intensity,  0.30f);
            Set(vignette.smoothness, 0.45f);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HELPERS  (mismo patrón que ControladorPostProcesado)
    // ═══════════════════════════════════════════════════════════════════════

    private static void Set(FloatParameter p, float v) { p.value = v; p.overrideState = true; }
    private static void Set(ColorParameter p, Color v) { p.value = v; p.overrideState = true; }

    // ═══════════════════════════════════════════════════════════════════════
    //  CLEANUP
    // ═══════════════════════════════════════════════════════════════════════

    private void OnDestroy()
    {
        // Liberar texturas creadas en runtime para evitar leaks
        foreach (var tex in new[] {
            texPanelBG, texBtnBase, texBtnHover,
            texBtnNVG, texBtnFLIR, texBtnNVGHover, texBtnFLIRHover })
        {
            if (tex != null) Destroy(tex);
        }
    }
}
