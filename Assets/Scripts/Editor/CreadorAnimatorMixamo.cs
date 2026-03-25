// Assets/Editor/CreadorAnimatorMixamo.cs
// Herramienta de Unity Editor que genera automáticamente un Animator Controller
// para personajes Kenney (CC0) + Mixamo con los parámetros que espera ControladorJugador.cs.
//
// CÓMO USAR:
//   1. Asegúrate de haber ejecutado primero "Alsasua → Configurar Personajes Kenney"
//   2. Menú Unity: Alsasua → Crear Animator Controller Mixamo
//   3. Se crea Assets/Personajes/AnimatorMixamo.controller con TODOS los clips asignados
//   4. Arrastra el controller al Inspector del ControladorJugador → "Controlador Animaciones"
//
// CLIPS ASIGNADOS AUTOMÁTICAMENTE:
//   · Idle           → Anim_Idle.fbx          (Kenney CC0)
//   · Andar          → Anim_Andar.fbx          (Mixamo: Walking, In Place)
//   · Correr         → Anim_Correr.fbx         (Kenney CC0)
//   · Agachado Idle  → Anim_Agachado.fbx       (Mixamo: Crouching Idle)
//   · Agachado Andar → Anim_AgachadoAndar.fbx  (Mixamo: Walk Crouching Forward, In Place)
//   · Apuntando      → Anim_Apuntar.fbx        (Mixamo: Rifle Aiming Idle)
//   · Saltar         → Anim_Saltar.fbx         (Kenney CC0)
//   · Disparar       → Anim_Disparar.fbx       (Mixamo: Shooting)
//   · Morir          → Anim_Morir.fbx          (Mixamo: Dying)

#if UNITY_EDITOR

using System.Linq;
using UnityEngine;
using UnityEditor;
using UnityEditor.Animations;

public static class CreadorAnimatorMixamo
{
    private const string RUTA_OUTPUT           = "Assets/Personajes/AnimatorMixamo.controller";
    private const string RUTA_ANIM_IDLE        = "Assets/Personajes/Animaciones/Anim_Idle.fbx";
    private const string RUTA_ANIM_ANDAR       = "Assets/Personajes/Animaciones/Anim_Andar.fbx";
    private const string RUTA_ANIM_CORRER      = "Assets/Personajes/Animaciones/Anim_Correr.fbx";
    private const string RUTA_ANIM_AGACHADO    = "Assets/Personajes/Animaciones/Anim_Agachado.fbx";
    private const string RUTA_ANIM_AGACH_ANDAR = "Assets/Personajes/Animaciones/Anim_AgachadoAndar.fbx";
    private const string RUTA_ANIM_APUNTAR     = "Assets/Personajes/Animaciones/Anim_Apuntar.fbx";
    private const string RUTA_ANIM_SALTAR      = "Assets/Personajes/Animaciones/Anim_Saltar.fbx";
    private const string RUTA_ANIM_DISPARAR    = "Assets/Personajes/Animaciones/Anim_Disparar.fbx";
    private const string RUTA_ANIM_MORIR       = "Assets/Personajes/Animaciones/Anim_Morir.fbx";

    [MenuItem("Alsasua/Crear Animator Controller Mixamo")]
    public static void CrearController() => CrearController(silencioso: false);

    public static void CrearController(bool silencioso)
    {
        // ── 0. Cargar todos los clips ─────────────────────────────────────────
        AnimationClip clipIdle       = CargarPrimerClip(RUTA_ANIM_IDLE);
        AnimationClip clipAndar      = CargarPrimerClip(RUTA_ANIM_ANDAR);
        AnimationClip clipCorrer     = CargarPrimerClip(RUTA_ANIM_CORRER);
        AnimationClip clipAgachado   = CargarPrimerClip(RUTA_ANIM_AGACHADO);
        AnimationClip clipAgachAndar = CargarPrimerClip(RUTA_ANIM_AGACH_ANDAR);
        AnimationClip clipApuntar    = CargarPrimerClip(RUTA_ANIM_APUNTAR);
        AnimationClip clipSaltar     = CargarPrimerClip(RUTA_ANIM_SALTAR);
        AnimationClip clipDisparar   = CargarPrimerClip(RUTA_ANIM_DISPARAR);
        AnimationClip clipMorir      = CargarPrimerClip(RUTA_ANIM_MORIR);

        // Fallbacks: usar clips Kenney si los Mixamo no están disponibles aún
        if (clipAndar      == null) clipAndar      = clipCorrer;
        if (clipAgachado   == null) clipAgachado   = clipIdle;
        if (clipAgachAndar == null) clipAgachAndar = clipCorrer;
        if (clipApuntar    == null) clipApuntar    = clipIdle;
        if (clipDisparar   == null) clipDisparar   = clipIdle;
        if (clipMorir      == null) clipMorir      = clipSaltar;

        int cargados = new[] { clipIdle, clipAndar, clipCorrer, clipAgachado,
                               clipAgachAndar, clipApuntar, clipSaltar, clipDisparar, clipMorir }
                       .Count(c => c != null);

        if (cargados == 0)
        {
            Debug.LogError(
                "[Alsasua] No se encontraron clips en Assets/Personajes/Animaciones/\n" +
                "Ejecuta primero 'Alsasua → Configurar Personajes Kenney (CC0)'.");
            if (!silencioso) EditorUtility.DisplayDialog(
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
        blendLoco.AddChild(clipAndar,  0.5f);
        blendLoco.AddChild(clipCorrer, 1.0f);

        stateLocomotion.motion = blendLoco;

        // 3b. AGACHADO — BlendTree (Agachado Idle · Agachado Andar)
        AnimatorState stateAgachado = rootStateMachine.AddState("Agachado", new Vector3(300f, 300f));
        var blendAgachado = new BlendTree();
        AssetDatabase.AddObjectToAsset(blendAgachado, controller);
        blendAgachado.name                   = "BlendTree_Agachado";
        blendAgachado.blendType              = BlendTreeType.Simple1D;
        blendAgachado.blendParameter         = "VelocidadMovimiento";
        blendAgachado.useAutomaticThresholds = false;
        blendAgachado.AddChild(clipAgachado,   0.0f);
        blendAgachado.AddChild(clipAgachAndar, 0.5f);
        stateAgachado.motion = blendAgachado;

        // 3c. APUNTANDO
        AnimatorState stateApuntar = rootStateMachine.AddState("Apuntando", new Vector3(550f, 150f));
        stateApuntar.motion = clipApuntar;

        // 3d. SALTAR
        AnimatorState stateSaltar = rootStateMachine.AddState("Saltar", new Vector3(300f, 0f));
        stateSaltar.motion = clipSaltar;

        // 3e. DISPARAR
        AnimatorState stateDisparar = rootStateMachine.AddState("Disparar", new Vector3(550f, 0f));
        stateDisparar.motion = clipDisparar;

        // 3f. MORIR
        AnimatorState stateMorir = rootStateMachine.AddState("Morir", new Vector3(300f, -150f));
        stateMorir.motion = clipMorir;

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
        string C(AnimationClip c, string ruta) => c != null ? ruta : "⚠ NO ENCONTRADO";
        Debug.Log(
            "════════════════════════════════════════════════════\n" +
            "  [Alsasua] ✓ AnimatorMixamo.controller creado en:\n" +
            $"  {RUTA_OUTPUT}\n" +
            "  \n" +
            "  CLIPS ASIGNADOS:\n" +
            $"  · Idle           → {C(clipIdle,       RUTA_ANIM_IDLE)}\n" +
            $"  · Andar          → {C(clipAndar,      RUTA_ANIM_ANDAR)}\n" +
            $"  · Correr         → {C(clipCorrer,     RUTA_ANIM_CORRER)}\n" +
            $"  · Agachado Idle  → {C(clipAgachado,   RUTA_ANIM_AGACHADO)}\n" +
            $"  · Agachado Andar → {C(clipAgachAndar, RUTA_ANIM_AGACH_ANDAR)}\n" +
            $"  · Apuntando      → {C(clipApuntar,    RUTA_ANIM_APUNTAR)}\n" +
            $"  · Saltar         → {C(clipSaltar,     RUTA_ANIM_SALTAR)}\n" +
            $"  · Disparar       → {C(clipDisparar,   RUTA_ANIM_DISPARAR)}\n" +
            $"  · Morir          → {C(clipMorir,      RUTA_ANIM_MORIR)}\n" +
            "  \n" +
            "  PASO SIGUIENTE:\n" +
            "  · Arrastra AnimatorMixamo.controller al campo\n" +
            "    'Controlador Animaciones' del ControladorJugador\n" +
            "════════════════════════════════════════════════════"
        );

        if (!silencioso) EditorUtility.DisplayDialog(
            $"✓ Animator Controller creado ({cargados}/9 clips)",
            $"AnimatorMixamo.controller generado con {cargados}/9 clips asignados.\n\n" +
            "Arrastra el controller al Inspector del ControladorJugador\n" +
            "→ campo 'Controlador Animaciones'.\n\n" +
            "Consulta la consola para ver el detalle de cada clip.",
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
