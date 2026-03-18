// Assets/Editor/CreadorAnimatorMixamo.cs
// Herramienta de Unity Editor que genera automáticamente un Animator Controller
// para personajes Kenney/Mixamo con los parámetros que espera ControladorJugador.cs.
//
// CÓMO USAR:
//   1. Asegúrate de haber ejecutado primero "Alsasua → Configurar Personajes Kenney"
//   2. Menú Unity: Alsasua → Crear Animator Controller Mixamo
//   3. Se crea Assets/Personajes/AnimatorMixamo.controller con clips asignados
//   4. (Opcional) Sustituye los clips de fallback por animaciones reales en el Animator
//   5. Arrastra el controller al Inspector del ControladorJugador → "Controlador Animaciones"
//
// CLIPS ASIGNADOS AUTOMÁTICAMENTE (con fallbacks usando Kenney CC0):
//   · Idle      → Anim_Idle.fbx
//   · Correr    → Anim_Correr.fbx
//   · Andar     → Anim_Correr.fbx a velocidad 0.6 (fallback hasta tener Walk real)
//   · Agachado  → Anim_Idle.fbx   (fallback hasta tener CrouchIdle real)
//   · AgachadoAndar → Anim_Correr.fbx (fallback)
//   · Apuntando → Anim_Idle.fbx   (fallback hasta tener AimIdle real)
//   · Saltar    → Anim_Saltar.fbx
//   · Disparar  → Anim_Idle.fbx   (fallback: sin animación de disparo)
//   · Morir     → Anim_Saltar.fbx (fallback: caída hacia atrás)

#if UNITY_EDITOR

using UnityEngine;
using UnityEditor;
using UnityEditor.Animations;

public static class CreadorAnimatorMixamo
{
    private const string RUTA_OUTPUT      = "Assets/Personajes/AnimatorMixamo.controller";
    private const string RUTA_ANIM_IDLE   = "Assets/Personajes/Animaciones/Anim_Idle.fbx";
    private const string RUTA_ANIM_CORRER = "Assets/Personajes/Animaciones/Anim_Correr.fbx";
    private const string RUTA_ANIM_SALTAR = "Assets/Personajes/Animaciones/Anim_Saltar.fbx";

    [MenuItem("Alsasua/Crear Animator Controller Mixamo")]
    public static void CrearController()
    {
        // ── 0. Cargar clips disponibles ──────────────────────────────────────
        AnimationClip clipIdle   = CargarPrimerClip(RUTA_ANIM_IDLE);
        AnimationClip clipCorrer = CargarPrimerClip(RUTA_ANIM_CORRER);
        AnimationClip clipSaltar = CargarPrimerClip(RUTA_ANIM_SALTAR);

        int cargados = (clipIdle   != null ? 1 : 0) +
                       (clipCorrer != null ? 1 : 0) +
                       (clipSaltar != null ? 1 : 0);

        if (cargados == 0)
        {
            Debug.LogError(
                "[Alsasua] No se encontraron clips en Assets/Personajes/Animaciones/\n" +
                "Ejecuta primero 'Alsasua → Configurar Personajes Kenney (CC0)'.");
            EditorUtility.DisplayDialog(
                "Error: sin animaciones",
                "No se encontraron clips en Assets/Personajes/Animaciones/\n\n" +
                "Ejecuta primero 'Alsasua → Configurar Personajes Kenney (CC0)'.",
                "OK");
            return;
        }

        // ── 1. Crear el directorio y el controller ───────────────────────────
        System.IO.Directory.CreateDirectory("Assets/Personajes");
        AssetDatabase.Refresh();

        var controller = AnimatorController.CreateAnimatorControllerAtPath(RUTA_OUTPUT);

        // ── 2. Parámetros ────────────────────────────────────────────────────
        controller.AddParameter("VelocidadMovimiento", AnimatorControllerParameterType.Float);
        controller.AddParameter("EstaAgachado",        AnimatorControllerParameterType.Bool);
        controller.AddParameter("EstaApuntando",       AnimatorControllerParameterType.Bool);
        controller.AddParameter("EstaEnSuelo",         AnimatorControllerParameterType.Bool);
        controller.AddParameter("Saltar",              AnimatorControllerParameterType.Trigger);
        controller.AddParameter("Disparar",            AnimatorControllerParameterType.Trigger);
        controller.AddParameter("Morir",               AnimatorControllerParameterType.Trigger);

        var rootStateMachine = controller.layers[0].stateMachine;

        // ── 3. Estados ───────────────────────────────────────────────────────

        // 3a. LOCOMOCIÓN — BlendTree (Idle · Andar · Correr)
        AnimatorState stateLocomotion = rootStateMachine.AddState("Locomocion", new Vector3(300f, 150f));
        stateLocomotion.speed = 1f;

        var blendLoco = new BlendTree();
        AssetDatabase.AddObjectToAsset(blendLoco, controller);
        blendLoco.name                   = "BlendTree_Locomocion";
        blendLoco.blendType              = BlendTreeType.Simple1D;
        blendLoco.blendParameter         = "VelocidadMovimiento";
        blendLoco.useAutomaticThresholds = false;

        // Umbral 0.0=Idle  0.5=Andar(walk)  1.0=Correr(run)
        blendLoco.AddChild(clipIdle,   0.0f);
        blendLoco.AddChild(clipCorrer, 0.5f);   // fallback: misma animación a velocidad 0.6
        blendLoco.AddChild(clipCorrer, 1.0f);

        // Velocidad para el slot "Andar": 60% del clip de correr = aspecto más lento
        var childrenLoco = blendLoco.children;
        childrenLoco[1].timeScale = 0.6f;
        blendLoco.children = childrenLoco;

        stateLocomotion.motion = blendLoco;

        // 3b. AGACHADO — BlendTree (Agachado Idle · Agachado Andar)
        AnimatorState stateAgachado = rootStateMachine.AddState("Agachado", new Vector3(300f, 300f));
        var blendAgachado = new BlendTree();
        AssetDatabase.AddObjectToAsset(blendAgachado, controller);
        blendAgachado.name                   = "BlendTree_Agachado";
        blendAgachado.blendType              = BlendTreeType.Simple1D;
        blendAgachado.blendParameter         = "VelocidadMovimiento";
        blendAgachado.useAutomaticThresholds = false;
        blendAgachado.AddChild(clipIdle,   0.0f);  // fallback: idle como agachado parado
        blendAgachado.AddChild(clipCorrer, 0.5f);  // fallback: run como agachado andando
        var childrenAg = blendAgachado.children;
        childrenAg[1].timeScale = 0.5f;
        blendAgachado.children = childrenAg;
        stateAgachado.motion = blendAgachado;

        // 3c. APUNTANDO
        AnimatorState stateApuntar = rootStateMachine.AddState("Apuntando", new Vector3(550f, 150f));
        stateApuntar.motion = clipIdle;    // fallback: idle mientras apunta

        // 3d. SALTAR
        AnimatorState stateSaltar = rootStateMachine.AddState("Saltar", new Vector3(300f, 0f));
        stateSaltar.motion = clipSaltar;

        // 3e. DISPARAR
        AnimatorState stateDisparar = rootStateMachine.AddState("Disparar", new Vector3(550f, 0f));
        stateDisparar.motion = clipIdle;  // fallback: idle (sin animación de disparo)

        // 3f. MORIR
        AnimatorState stateMorir = rootStateMachine.AddState("Morir", new Vector3(300f, -150f));
        stateMorir.motion = clipSaltar;   // fallback: saltar invertido simula caída

        // ── 4. Estado por defecto ─────────────────────────────────────────────
        rootStateMachine.defaultState = stateLocomotion;

        // ── 5. Transiciones ───────────────────────────────────────────────────

        // Locomoción ↔ Agachado
        var tAg = stateLocomotion.AddTransition(stateAgachado);
        tAg.AddCondition(AnimatorConditionMode.If, 0, "EstaAgachado");
        tAg.duration = 0.2f; tAg.hasExitTime = false;

        var tAgFin = stateAgachado.AddTransition(stateLocomotion);
        tAgFin.AddCondition(AnimatorConditionMode.IfNot, 0, "EstaAgachado");
        tAgFin.duration = 0.2f; tAgFin.hasExitTime = false;

        // Locomoción ↔ Apuntando
        var tAp = stateLocomotion.AddTransition(stateApuntar);
        tAp.AddCondition(AnimatorConditionMode.If, 0, "EstaApuntando");
        tAp.duration = 0.15f; tAp.hasExitTime = false;

        var tApFin = stateApuntar.AddTransition(stateLocomotion);
        tApFin.AddCondition(AnimatorConditionMode.IfNot, 0, "EstaApuntando");
        tApFin.duration = 0.15f; tApFin.hasExitTime = false;

        // AnyState → Saltar (Trigger)
        var tSaltar = rootStateMachine.AddAnyStateTransition(stateSaltar);
        tSaltar.AddCondition(AnimatorConditionMode.If, 0, "Saltar");
        tSaltar.duration = 0.1f; tSaltar.hasExitTime = false;
        tSaltar.canTransitionToSelf = false;

        // Saltar → Locomoción (al tocar suelo, después de 60% del clip)
        var tSaltarFin = stateSaltar.AddTransition(stateLocomotion);
        tSaltarFin.AddCondition(AnimatorConditionMode.If, 0, "EstaEnSuelo");
        tSaltarFin.duration = 0.2f; tSaltarFin.hasExitTime = true; tSaltarFin.exitTime = 0.6f;

        // AnyState → Disparar (Trigger)
        var tDisp = rootStateMachine.AddAnyStateTransition(stateDisparar);
        tDisp.AddCondition(AnimatorConditionMode.If, 0, "Disparar");
        tDisp.duration = 0.05f; tDisp.hasExitTime = false;
        tDisp.canTransitionToSelf = false;

        // Disparar → Locomoción (por tiempo de salida)
        var tDispFin = stateDisparar.AddTransition(stateLocomotion);
        tDispFin.hasExitTime = true; tDispFin.exitTime = 0.9f; tDispFin.duration = 0.1f;

        // AnyState → Morir (Trigger) — sin retorno
        var tMorir = rootStateMachine.AddAnyStateTransition(stateMorir);
        tMorir.AddCondition(AnimatorConditionMode.If, 0, "Morir");
        tMorir.duration = 0.1f; tMorir.hasExitTime = false;
        tMorir.canTransitionToSelf = false;

        // ── 6. Guardar ────────────────────────────────────────────────────────
        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();

        Selection.activeObject = controller;
        EditorGUIUtility.PingObject(controller);

        // ── 7. Informe en consola ─────────────────────────────────────────────
        string estadoClips =
            $"  · Idle    → {(clipIdle   != null ? RUTA_ANIM_IDLE   : "NO ENCONTRADO")}\n" +
            $"  · Correr  → {(clipCorrer != null ? RUTA_ANIM_CORRER : "NO ENCONTRADO")}\n" +
            $"  · Saltar  → {(clipSaltar != null ? RUTA_ANIM_SALTAR : "NO ENCONTRADO")}";

        Debug.Log(
            "════════════════════════════════════════════════════\n" +
            "  [Alsasua] ✓ AnimatorMixamo.controller creado en:\n" +
            $"  {RUTA_OUTPUT}\n" +
            "  \n" +
            "  CLIPS ASIGNADOS (Kenney CC0):\n" +
            estadoClips + "\n" +
            "  \n" +
            "  FALLBACKS ACTIVOS (sustituir por clips reales si tienes):\n" +
            "  · Andar       → Correr × 0.6  (reemplazar con clip Walk)\n" +
            "  · Agachado    → Idle           (reemplazar con CrouchIdle)\n" +
            "  · AgachadoAndar→ Correr × 0.5  (reemplazar con CrouchWalk)\n" +
            "  · Apuntando   → Idle           (reemplazar con AimIdle)\n" +
            "  · Disparar    → Idle           (reemplazar con ShootClip)\n" +
            "  · Morir       → Saltar         (reemplazar con DeathClip)\n" +
            "  \n" +
            "  CÓMO AÑADIR ANIMACIONES REALES MÁS ADELANTE:\n" +
            "  1. Descarga clips de Mixamo (formato FBX for Unity)\n" +
            "  2. Ponlos en Assets/Personajes/Animaciones/\n" +
            "  3. Doble clic en AnimatorMixamo.controller\n" +
            "  4. Arrastra el clip al estado correspondiente\n" +
            "  \n" +
            "  PASO SIGUIENTE:\n" +
            "  · Arrastra AnimatorMixamo.controller al campo\n" +
            "    'Controlador Animaciones' del ControladorJugador\n" +
            "════════════════════════════════════════════════════"
        );

        EditorUtility.DisplayDialog(
            $"✓ Animator Controller creado ({cargados}/3 clips)",
            "AnimatorMixamo.controller generado en Assets/Personajes/\n\n" +
            "Los clips disponibles (Idle, Correr, Saltar) están asignados.\n" +
            "Los estados sin clip real usan fallbacks funcionales.\n\n" +
            "Arrastra el controller al Inspector del ControladorJugador\n" +
            "→ campo 'Controlador Animaciones'.\n\n" +
            "Consulta la consola para detalles y cómo mejorar los clips.",
            "OK"
        );
    }

    // ── Helper: extrae el primer AnimationClip no-interno de un FBX ──────────
    private static AnimationClip CargarPrimerClip(string rutaFbx)
    {
        if (!System.IO.File.Exists(rutaFbx))
        {
            Debug.LogWarning($"[Alsasua] FBX no encontrado: {rutaFbx}");
            return null;
        }

        var assets = AssetDatabase.LoadAllAssetsAtPath(rutaFbx);
        foreach (var a in assets)
        {
            if (a is AnimationClip clip && !clip.name.StartsWith("__"))
                return clip;
        }

        Debug.LogWarning($"[Alsasua] No se encontró ningún AnimationClip en: {rutaFbx}");
        return null;
    }
}

#endif
