// Assets/Scripts/ControladorJugador.cs
// Controlador jugador en TERCERA PERSONA — Spring Arm · Cuerpo procedural
//
// Controles:
//   WASD       – Mover           SHIFT – Correr
//   SPACE      – Saltar          C     – Agacharse
//   RMB (hold) – Apuntar (zoom)  LMB   – Disparar
//   F          – Colocar bomba   G     – Detonar última
//   ESC        – Liberar cursor

using UnityEngine;
using UnityEngine.InputSystem;
using CesiumForUnity;

[RequireComponent(typeof(CharacterController))]
public class ControladorJugador : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  MOVIMIENTO
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ MOVIMIENTO ═══")]
    [SerializeField] private float velocidadAndar    = 4.0f;
    [SerializeField] private float velocidadCorrer   = 8.5f;
    [SerializeField] private float velocidadAgachar  = 2.0f;
    [SerializeField] private float fuerzaSalto       = 5.5f;
    [SerializeField] private float gravedad          = -20f;
    [SerializeField] private float suavidadMovimiento = 9f;
    [SerializeField] private float suavidadGiro      = 14f;

    // ═══════════════════════════════════════════════════════════════════════
    //  CÁMARA TERCERA PERSONA (Spring Arm)
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ CÁMARA 3ª PERSONA ═══")]
    [SerializeField] private float distanciaBrazo   = 4.5f;    // dist. normal
    [SerializeField] private float distanciaApuntar = 2.8f;    // dist. al apuntar (RMB)
    [SerializeField] private float alturaHombro     = 1.5f;    // pivot sobre el jugador
    [SerializeField] private float sensibilidadX    = 2.8f;
    [SerializeField] private float sensibilidadY    = 2.2f;
    [SerializeField] private float limVertMin       = -25f;
    [SerializeField] private float limVertMax       =  65f;
    [SerializeField] private float suavidadCamara   = 12f;
    [SerializeField] private float fovNormal        = 68f;
    [SerializeField] private float fovApuntando     = 48f;
    [SerializeField] private float anguloVertDefault =  5f;    // 5° = casi horizontal

    // ═══════════════════════════════════════════════════════════════════════
    //  SALUD
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ SALUD ═══")]
    [SerializeField] private int vida    = 100;
    [SerializeField] private int vidaMax = 100;

    public int  Vida       => vida;
    public bool EstaMuerto => vida <= 0;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private CharacterController cc;
    private SistemaDisparo      sistemaDisparo;
    private SistemaBombas       sistemaBombas;

    // Cámara
    private Camera    camaraTP;
    private Transform pivotCam;         // sigue al jugador a alturaHombro

    private float anguloH = 0f;
    private float anguloV;              // inicializado con anguloVertDefault

    // Movimiento
    private Vector2 inputMovimiento;
    private Vector3 velHoriz;
    private Vector3 velVert;
    private float   yawJugador = 0f;

    // Estados
    private bool estaEnSuelo;
    private bool estaAgachado;
    private bool estaCorriendo;
    private bool modoApuntar;

    // Daño visual (flash pantalla)
    private float timerDano = 0f;

    // BUG 2 FIX: lista de materiales creados con new Material() para destruirlos
    // explícitamente en OnDestroy() — Unity no los libera automáticamente.
    private readonly System.Collections.Generic.List<Material> _matsCreados =
        new System.Collections.Generic.List<Material>();

    // PERF FIX: LayerMask calculado UNA sola vez en Awake() y cacheado.
    // LayerMask.GetMask() hace string lookups en cada llamada → en LateUpdate (60fps)
    // eran 60 lookups/seg innecesarios. Ahora es un simple int.
    private int _maskSpringArm;

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY LIFECYCLE
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        cc             = GetComponent<CharacterController>();
        sistemaDisparo = GetComponent<SistemaDisparo>();
        sistemaBombas  = GetComponent<SistemaBombas>();

        cc.height = 1.8f;
        cc.center = new Vector3(0f, 0.9f, 0f);

        yawJugador = transform.eulerAngles.y;
        anguloH    = yawJugador;
        anguloV    = anguloVertDefault;

        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible   = false;

        // PERF FIX: cachear la LayerMask aquí (una vez) para ActualizarCamara() en LateUpdate
        _maskSpringArm = ~LayerMask.GetMask("Player", "Ignore Raycast");
    }

    private void Start()
    {
        // Start() se ejecuta DESPUÉS de todos los Awake() → SistemaDisparo ya
        // ha cacheado la cámara-hijo antes de que la desacoplemos aquí.
        ConfigurarCamara();
        CrearCuerpoJugador();
    }

    private void Update()
    {
        if (EstaMuerto) return;
        LeerInput();
        GestionarCursor();
        MoverJugador();
        Gravedad();
        if (timerDano > 0f) timerDano -= Time.deltaTime;
    }

    private void LateUpdate()
    {
        if (!EstaMuerto && camaraTP != null) ActualizarCamara();
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CONFIGURAR CÁMARA
    // ═══════════════════════════════════════════════════════════════════════

    private void ConfigurarCamara()
    {
        // Pivot — sigue automáticamente al jugador (es hijo)
        var pivotGO = new GameObject("_PivotCam");
        pivotCam = pivotGO.transform;
        pivotCam.SetParent(transform, false);
        pivotCam.localPosition = new Vector3(0f, alturaHombro, 0f);

        // Buscar cámara entre los hijos (CamaraFPS del setup) o Camera.main
        camaraTP = GetComponentInChildren<Camera>();
        if (camaraTP == null) camaraTP = Camera.main;
        if (camaraTP == null)
        {
            var camGO = new GameObject("CamaraTP");
            camaraTP  = camGO.AddComponent<Camera>();
            if (FindFirstObjectByType<AudioListener>() == null)
                camGO.AddComponent<AudioListener>();
            Debug.LogWarning("[Jugador] Cámara no encontrada — se creó CamaraTP.");
        }

        // Desacoplar del jugador para orbitar libremente.
        // Se re-parenta bajo CesiumGeoreference para que CesiumGlobeAnchor funcione.
        var georef = Object.FindFirstObjectByType<CesiumForUnity.CesiumGeoreference>();
        camaraTP.transform.SetParent(georef != null ? georef.transform : null, worldPositionStays: true);

        // BUG 33 FIX: añadir CesiumGlobeAnchor a la cámara del jugador si no lo tiene ya.
        // Sin él, la cámara no se georreferencia correctamente y puede desviarse en latitudes altas.
        if (georef != null && camaraTP.GetComponent<CesiumForUnity.CesiumGlobeAnchor>() == null)
            camaraTP.gameObject.AddComponent<CesiumForUnity.CesiumGlobeAnchor>();
        camaraTP.nearClipPlane = 0.3f;          // FIX: consistente con ControladorPostProcesado (ratio z-buffer 1:400k)
        camaraTP.farClipPlane  = 1_000_000f;   // 1000 km — Cesium renderiza tiles lejanos
        camaraTP.fieldOfView   = fovNormal;

        // Posición inicial detrás del jugador
        Quaternion rotInit = Quaternion.Euler(anguloV, anguloH, 0f);
        camaraTP.transform.position = pivotCam.position + rotInit * Vector3.back * distanciaBrazo;
        camaraTP.transform.LookAt(pivotCam.position);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CUERPO PROCEDURAL (soldado táctico, visible desde 3ª persona)
    // ═══════════════════════════════════════════════════════════════════════

    private void CrearCuerpoJugador()
    {
        if (transform.Find("_Cuerpo") != null) return;  // ya existe

        var raiz = new GameObject("_Cuerpo");
        raiz.transform.SetParent(transform, false);

        // Paleta de colores militares
        Color negro  = new Color(0.10f, 0.10f, 0.12f);   // uniforme oscuro
        Color verde  = new Color(0.18f, 0.21f, 0.16f);   // verde militar
        Color bota   = new Color(0.08f, 0.06f, 0.05f);   // cuero negro
        Color piel   = new Color(0.72f, 0.56f, 0.42f);   // tono piel
        Color metal  = new Color(0.18f, 0.18f, 0.20f);   // acero

        // ── Piernas ─────────────────────────────────────────────────────────
        Box(raiz, "PiernaI",  new Vector3(-0.13f, 0.44f, 0f),  new Vector3(0.19f, 0.70f, 0.20f), negro);
        Box(raiz, "PiernaD",  new Vector3( 0.13f, 0.44f, 0f),  new Vector3(0.19f, 0.70f, 0.20f), negro);
        Box(raiz, "BotaI",    new Vector3(-0.13f, 0.08f, 0.04f),new Vector3(0.21f, 0.15f, 0.32f), bota);
        Box(raiz, "BotaD",    new Vector3( 0.13f, 0.08f, 0.04f),new Vector3(0.21f, 0.15f, 0.32f), bota);

        // ── Pelvis y torso ──────────────────────────────────────────────────
        Box(raiz, "Pelvis",   new Vector3(0f, 0.84f, 0f),   new Vector3(0.48f, 0.28f, 0.22f), negro);
        Box(raiz, "Torso",    new Vector3(0f, 1.22f, 0f),   new Vector3(0.52f, 0.52f, 0.24f), negro);

        // ── Chaleco / armadura táctica (encima del torso) ───────────────────
        Box(raiz, "Chaleco",  new Vector3(0f, 1.24f, 0.04f),new Vector3(0.46f, 0.46f, 0.09f), verde);
        Box(raiz, "PlacaF",   new Vector3(0f, 1.28f, 0.09f),new Vector3(0.30f, 0.22f, 0.04f), new Color(0.25f,0.28f,0.22f));

        // ── Mochila táctica ─────────────────────────────────────────────────
        Box(raiz, "Mochila",  new Vector3(0f, 1.22f, -0.19f),new Vector3(0.34f, 0.40f, 0.18f), verde);

        // ── Brazos ──────────────────────────────────────────────────────────
        Box(raiz, "BrazoI",   new Vector3(-0.34f, 1.18f, 0f),new Vector3(0.15f, 0.46f, 0.16f), negro);
        Box(raiz, "BrazoD",   new Vector3( 0.34f, 1.18f, 0f),new Vector3(0.15f, 0.46f, 0.16f), negro);
        Sphere(raiz, "ManoI", new Vector3(-0.34f, 0.93f, 0f), 0.09f, piel);
        Sphere(raiz, "ManoD", new Vector3( 0.34f, 0.93f, 0f), 0.09f, piel);

        // ── Cuello y cabeza ─────────────────────────────────────────────────
        Sphere(raiz, "Cuello", new Vector3(0f, 1.55f, 0f), 0.09f, negro);
        Sphere(raiz, "Cabeza", new Vector3(0f, 1.68f, 0f), 0.13f, piel);

        // ── Casco ───────────────────────────────────────────────────────────
        var casco = Sphere(raiz, "Casco",  new Vector3(0f, 1.76f, 0f), 0.17f, verde);
        casco.transform.localScale = new Vector3(0.36f, 0.30f, 0.36f);  // achatado

        Box(raiz, "ViseraCasco", new Vector3(0f, 1.71f, 0.14f),
            new Vector3(0.30f, 0.05f, 0.08f), new Color(0.08f, 0.08f, 0.08f));

        // ── Rifle (mano derecha, visible al jugar) ───────────────────────────
        Box(raiz, "RifleCuerpo", new Vector3( 0.35f, 1.12f, 0.25f),
            new Vector3(0.06f, 0.09f, 0.55f), metal);
        Box(raiz, "RifleCañon",  new Vector3( 0.35f, 1.14f, 0.60f),
            new Vector3(0.03f, 0.03f, 0.22f), new Color(0.06f, 0.06f, 0.06f));
        Box(raiz, "RifleCargador",new Vector3(0.35f, 1.04f, 0.28f),
            new Vector3(0.04f, 0.12f, 0.06f), metal);
    }

    // Crea un Material compatible con URP (evita el magenta por shader incorrecto)
    // BUG 1 FIX: guard contra shader==null — si URP/Lit está stripeado de la build,
    //            intentar URP/Unlit y luego Standard antes de lanzar excepción.
    // BUG 2 FIX: method de instancia (no static) para poder añadir cada material
    //            a _matsCreados y destruirlos explícitamente en OnDestroy().
    private Material MatURP(Color color)
    {
        var shader = Shader.Find("Universal Render Pipeline/Lit")
                  ?? Shader.Find("Universal Render Pipeline/Unlit")
                  ?? Shader.Find("Standard");

        if (shader == null)
        {
            Debug.LogError("[ControladorJugador] MatURP: ningún shader compatible encontrado. " +
                           "Incluye 'Universal Render Pipeline/Lit' en Always Included Shaders.");
            // Fallback de emergencia — el material saldrá magenta pero no lanzará excepción.
            shader = Shader.Find("Hidden/InternalErrorShader") ?? Shader.Find("UI/Default");
            if (shader == null) return null;
        }

        var mat = new Material(shader) { color = color };
        _matsCreados.Add(mat); // BUG 2 FIX: rastrear para destruir en OnDestroy()
        return mat;
    }

    // BUG 2 FIX: liberar los materiales del cuerpo al destruir el jugador.
    private void OnDestroy()
    {
        foreach (var m in _matsCreados)
            if (m != null) Object.Destroy(m);
        _matsCreados.Clear();
    }

    // Helpers para crear piezas del cuerpo (sin colisionador, con sombras)
    private GameObject Box(GameObject raiz, string n, Vector3 lp, Vector3 ls, Color c)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
        go.name = n;
        go.transform.SetParent(raiz.transform, false);
        go.transform.localPosition = lp;
        go.transform.localScale    = ls;
        var r = go.GetComponent<Renderer>();
        r.material              = MatURP(c);
        r.shadowCastingMode     = UnityEngine.Rendering.ShadowCastingMode.On;
        r.receiveShadows        = true;
        Object.Destroy(go.GetComponent<Collider>());
        return go;
    }

    private GameObject Sphere(GameObject raiz, string n, Vector3 lp, float radio, Color c)
    {
        var go = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        go.name = n;
        go.transform.SetParent(raiz.transform, false);
        go.transform.localPosition = lp;
        go.transform.localScale    = Vector3.one * (radio * 2f);
        var r = go.GetComponent<Renderer>();
        r.material              = MatURP(c);
        r.shadowCastingMode     = UnityEngine.Rendering.ShadowCastingMode.On;
        r.receiveShadows        = true;
        Object.Destroy(go.GetComponent<Collider>());
        return go;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INPUT
    // ═══════════════════════════════════════════════════════════════════════

    private void LeerInput()
    {
        var kb = Keyboard.current;
        var m  = Mouse.current;
        if (kb == null || m == null) return;

        // Movimiento WASD
        inputMovimiento = new Vector2(
            (kb.dKey.isPressed ? 1f : 0f) - (kb.aKey.isPressed ? 1f : 0f),
            (kb.wKey.isPressed ? 1f : 0f) - (kb.sKey.isPressed ? 1f : 0f));

        estaCorriendo = kb.leftShiftKey.isPressed;
        modoApuntar   = m.rightButton.isPressed;

        // Teclas de acción
        if (kb.spaceKey.wasPressedThisFrame && estaEnSuelo) Saltar();
        if (kb.cKey.wasPressedThisFrame)                   ToggleAgacharse();
        if (kb.escapeKey.wasPressedThisFrame)
        { Cursor.lockState = CursorLockMode.None; Cursor.visible = true; }

        // Órbita de cámara con el ratón
        // BUG FIX: mouse.delta ya da píxeles/frame — multiplicar por Time.deltaTime
        // hacía la rotación DEPENDIENTE del framerate (muy lento a 120fps, muy rápido a 30fps).
        // Solución: usar un factor fijo pequeño para convertir píxeles → grados.
        // sensibilidadX/Y = 2.8/2.2 equivalen a ~0.14/0.11 °/px — sensación natural.
        if (Cursor.lockState == CursorLockMode.Locked)
        {
            Vector2 delta = m.delta.ReadValue();
            anguloH += delta.x * sensibilidadX * 0.05f;
            anguloV -= delta.y * sensibilidadY * 0.05f;
            anguloV  = Mathf.Clamp(anguloV, limVertMin, limVertMax);
        }

        // Acciones de combate
        if (m.leftButton.isPressed  && sistemaDisparo != null) sistemaDisparo.Disparar();
        if (kb.fKey.wasPressedThisFrame && sistemaBombas != null) sistemaBombas.ColocarBomba();
        if (kb.gKey.wasPressedThisFrame && sistemaBombas != null) sistemaBombas.DetonarUltima();
    }

    private void GestionarCursor()
    {
        if (Cursor.lockState != CursorLockMode.Locked
            && Mouse.current != null
            && Mouse.current.leftButton.wasPressedThisFrame)
        {
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible   = false;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MOVIMIENTO DEL JUGADOR
    // ═══════════════════════════════════════════════════════════════════════

    private void MoverJugador()
    {
        // Dirección relativa a la cámara (horizontal)
        Vector3 forward = Quaternion.Euler(0f, anguloH, 0f) * Vector3.forward;
        Vector3 right   = Quaternion.Euler(0f, anguloH, 0f) * Vector3.right;
        Vector3 moveDir = (forward * inputMovimiento.y + right * inputMovimiento.x);
        moveDir = Vector3.ClampMagnitude(moveDir, 1f);

        float vel = estaAgachado ? velocidadAgachar
                  : estaCorriendo ? velocidadCorrer
                  : velocidadAndar;

        velHoriz = Vector3.Lerp(velHoriz, moveDir * vel, suavidadMovimiento * Time.deltaTime);
        cc.Move(velHoriz * Time.deltaTime);

        // ── Rotar jugador ───────────────────────────────────────────────────
        if (modoApuntar)
        {
            // Al apuntar: siempre encarar la dirección de cámara (anguloH)
            yawJugador = Mathf.LerpAngle(yawJugador, anguloH, suavidadGiro * 2f * Time.deltaTime);
        }
        else if (moveDir.magnitude > 0.05f)
        {
            // Al moverse sin apuntar: rotar hacia la dirección de movimiento
            float yawObj = Mathf.Atan2(moveDir.x, moveDir.z) * Mathf.Rad2Deg;
            yawJugador = Mathf.LerpAngle(yawJugador, yawObj, suavidadGiro * Time.deltaTime);
        }

        transform.rotation = Quaternion.Euler(0f, yawJugador, 0f);
    }

    private void Gravedad()
    {
        estaEnSuelo = cc.isGrounded;

        if (estaEnSuelo)
        {
            // BUG 5 FIX: asignar -2f Y salir — no sumar gravedad este frame.
            // Antes: velVert.y = -2f luego += gravedad*dt (negativo) → llegaba a ≈-2.2 cada frame
            // → CharacterController se empujaba al suelo extra → micro-saltos / "temblor" al caminar.
            if (velVert.y < 0f) velVert.y = -2f;
        }
        else
        {
            // Solo aplicar gravedad cuando estamos en el aire
            velVert.y += gravedad * Time.deltaTime;
        }

        cc.Move(velVert * Time.deltaTime);
    }

    private void Saltar() =>
        velVert.y = Mathf.Sqrt(fuerzaSalto * -2f * gravedad);

    private void ToggleAgacharse()
    {
        estaAgachado = !estaAgachado;
        float h = estaAgachado ? 0.9f : 1.8f;
        cc.height = h;
        cc.center = new Vector3(0f, h * 0.5f, 0f);
        if (pivotCam != null)
            pivotCam.localPosition = new Vector3(0f, estaAgachado ? alturaHombro * 0.55f : alturaHombro, 0f);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CÁMARA TERCERA PERSONA — SPRING ARM CON COLISIÓN
    // ═══════════════════════════════════════════════════════════════════════

    private void ActualizarCamara()
    {
        if (pivotCam == null) return;

        float dist = modoApuntar ? distanciaApuntar : distanciaBrazo;

        // Desplazamiento lateral (al apuntar, cámara ligeramente a la derecha)
        Vector3 offsetLateral = modoApuntar
            ? Quaternion.Euler(0f, anguloH, 0f) * new Vector3(0.45f, 0f, 0f)
            : Vector3.zero;

        Vector3   pivotPos  = pivotCam.position + offsetLateral;
        Quaternion rotBrazo = Quaternion.Euler(anguloV, anguloH, 0f);
        Vector3   posBuscada = pivotPos + rotBrazo * (Vector3.back * dist);

        // ── Spring Arm: detectar geometría entre pivot y cámara ─────────────
        Vector3 posFinal = posBuscada;
        if (Physics.Linecast(pivotPos, posBuscada, out RaycastHit hit, _maskSpringArm))
        {
            // Acercar la cámara al punto de colisión (+ pequeño offset)
            posFinal = hit.point + (pivotPos - posBuscada).normalized * 0.18f;
        }

        // ── Posición suavizada ───────────────────────────────────────────────
        camaraTP.transform.position = Vector3.Lerp(
            camaraTP.transform.position, posFinal, suavidadCamara * Time.deltaTime);

        // ── Orientación: la cámara mira hacia el pivot (cabeza del jugador) ──
        camaraTP.transform.LookAt(pivotPos + Vector3.up * 0.10f);

        // ── FOV dinámico: zoom suave al apuntar ──────────────────────────────
        float fovObj = modoApuntar ? fovApuntando : fovNormal;
        camaraTP.fieldOfView = Mathf.Lerp(camaraTP.fieldOfView, fovObj, 10f * Time.deltaTime);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  DAÑO Y SALUD
    // ═══════════════════════════════════════════════════════════════════════

    public void RecibirDano(int cantidad)
    {
        vida      = Mathf.Max(0, vida - cantidad);
        timerDano = 0.35f;   // activar flash rojo en pantalla
        if (vida <= 0) Morir();
    }

    public void Curar(int cantidad) =>
        vida = Mathf.Min(vidaMax, vida + cantidad);

    private void Morir()
    {
        Debug.Log("[Jugador] ¡Has muerto!");
        Cursor.lockState = CursorLockMode.None;
        Cursor.visible   = true;
        enabled          = false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HUD
    // ═══════════════════════════════════════════════════════════════════════

    private void OnGUI()
    {
        // ── Flash de daño (borde rojo al recibir impacto) ────────────────────
        if (timerDano > 0f)
        {
            GUI.color = new Color(1f, 0f, 0f, Mathf.Clamp01(timerDano * 2.5f) * 0.55f);
            GUI.DrawTexture(new Rect(0, 0, Screen.width, Screen.height), Texture2D.whiteTexture);
        }

        // ── Barra de vida ────────────────────────────────────────────────────
        float barW = 220f, barH = 18f, x = 20f, y = Screen.height - 48f;
        float ratio = (float)vida / vidaMax;

        // Fondo
        GUI.color = new Color(0f, 0f, 0f, 0.70f);
        GUI.DrawTexture(new Rect(x - 2f, y - 2f, barW + 4f, barH + 4f), Texture2D.whiteTexture);

        // Relleno (verde → rojo según vida)
        GUI.color = Color.Lerp(new Color(0.85f, 0.1f, 0.1f), new Color(0.1f, 0.80f, 0.2f), ratio);
        GUI.DrawTexture(new Rect(x, y, barW * ratio, barH), Texture2D.whiteTexture);

        // Texto
        GUI.color = Color.white;
        GUI.Label(new Rect(x + 6f, y + 1f, barW, barH), $"❤  {vida} / {vidaMax}");

        // Estado del jugador
        string estado = EstaMuerto     ? "─ MUERTO ─"
                      : estaAgachado   ? "AGACHADO"
                      : modoApuntar    ? "APUNTANDO"
                      : estaCorriendo  ? "CORRIENDO"
                      : inputMovimiento.magnitude > 0.05f ? "ANDANDO"
                      : "EN GUARDIA";
        GUI.color = new Color(1f, 1f, 0.8f, 0.9f);
        GUI.Label(new Rect(x, y - 22f, 260f, 20f), estado);

        // ── Mira central ─────────────────────────────────────────────────────
        float cx = Screen.width * 0.5f, cy = Screen.height * 0.5f;
        if (modoApuntar)
        {
            // Cruz de apuntado
            GUI.color = new Color(1f, 1f, 1f, 0.95f);
            GUI.DrawTexture(new Rect(cx - 14f, cy - 1f, 28f, 2f), Texture2D.whiteTexture);
            GUI.DrawTexture(new Rect(cx - 1f, cy - 14f, 2f, 28f), Texture2D.whiteTexture);
            // Hueco central (para que no oculte el objetivo)
            GUI.color = new Color(0f, 0f, 0f, 0f);
            GUI.DrawTexture(new Rect(cx - 3f, cy - 3f, 6f, 6f), Texture2D.whiteTexture);
        }
        else
        {
            // Punto de referencia (3ª persona)
            GUI.color = new Color(1f, 1f, 1f, 0.75f);
            GUI.DrawTexture(new Rect(cx - 2f, cy - 2f, 5f, 5f), Texture2D.whiteTexture);
        }

        // ── Panel de controles (esquina superior izquierda) ──────────────────
        GUI.color = new Color(0f, 0f, 0f, 0.50f);
        GUI.DrawTexture(new Rect(14f, 14f, 210f, 112f), Texture2D.whiteTexture);
        GUI.color = new Color(1f, 1f, 0.85f, 0.85f);
        GUI.Label(new Rect(20f, 18f, 206f, 112f),
            "WASD · Mover         SHIFT · Correr\n"  +
            "SPACE · Saltar          C · Agacharse\n" +
            "RMB · Apuntar      LMB · Disparar\n"    +
            "F · Colocar bomba   G · Detonar\n"       +
            "ESC · Cursor libre");
    }
}
