// Assets/Editor/CreadorAnimatorMixamo.cs
// Herramienta de Unity Editor que genera automáticamente un Animator Controller
// para personajes Mixamo con los parámetros que espera ControladorJugador.cs.
//
// CÓMO USAR:
//   1. Importa tus FBX de Mixamo en Assets/Personajes/
//   2. Menú Unity: Alsasua → Crear Animator Controller Mixamo
//   3. Se crea Assets/Personajes/AnimatorMixamo.controller
//   4. Arrastra tus clips de animación a los estados marcados con [ASIGNAR]
//   5. Arrastra el controller al Inspector del ControladorJugador → "Controlador Animaciones"

#if UNITY_EDITOR

using UnityEngine;
using UnityEditor;
using UnityEditor.Animations;

public static class CreadorAnimatorMixamo
{
    private const string RUTA_OUTPUT = "Assets/Personajes/AnimatorMixamo.controller";

    [MenuItem("Alsasua/Crear Animator Controller Mixamo")]
    public static void CrearController()
    {
        // Crear el directorio si no existe
        System.IO.Directory.CreateDirectory("Assets/Personajes");
        AssetDatabase.Refresh();

        // Crear el controller
        var controller = AnimatorController.CreateAnimatorControllerAtPath(RUTA_OUTPUT);

        // ── Parámetros ────────────────────────────────────────────────────────
        controller.AddParameter("VelocidadMovimiento", AnimatorControllerParameterType.Float);
        controller.AddParameter("EstaAgachado",        AnimatorControllerParameterType.Bool);
        controller.AddParameter("EstaApuntando",       AnimatorControllerParameterType.Bool);
        controller.AddParameter("EstaEnSuelo",         AnimatorControllerParameterType.Bool);
        controller.AddParameter("Saltar",              AnimatorControllerParameterType.Trigger);
        controller.AddParameter("Disparar",            AnimatorControllerParameterType.Trigger);
        controller.AddParameter("Morir",               AnimatorControllerParameterType.Trigger);

        var rootStateMachine = controller.layers[0].stateMachine;

        // ── Estados ───────────────────────────────────────────────────────────

        // 1. LOCOMOCIÓN — Blend Tree (Idle · Andar · Correr)
        AnimatorState stateLocomotion = rootStateMachine.AddState("Locomocion", new Vector3(300f, 150f));
        stateLocomotion.speed = 1f;

        var blendTree = new BlendTree();
        AssetDatabase.AddObjectToAsset(blendTree, controller);
        blendTree.name       = "BlendTree_Locomocion";
        blendTree.blendType  = BlendTreeType.Simple1D;
        blendTree.blendParameter = "VelocidadMovimiento";
        blendTree.useAutomaticThresholds = false;

        // Umbral 0.0 = Idle, 0.5 = Andar, 1.0 = Correr
        blendTree.AddChild(null, 0.0f);   // [ASIGNAR] clip Anim_Idle
        blendTree.AddChild(null, 0.5f);   // [ASIGNAR] clip Anim_Andar
        blendTree.AddChild(null, 1.0f);   // [ASIGNAR] clip Anim_Correr

        stateLocomotion.motion = blendTree;

        // 2. AGACHADO — Blend Tree (Agachado Idle · Agachado Andar)
        AnimatorState stateAgachado = rootStateMachine.AddState("Agachado", new Vector3(300f, 300f));
        var blendAgachado = new BlendTree();
        AssetDatabase.AddObjectToAsset(blendAgachado, controller);
        blendAgachado.name           = "BlendTree_Agachado";
        blendAgachado.blendType      = BlendTreeType.Simple1D;
        blendAgachado.blendParameter = "VelocidadMovimiento";
        blendAgachado.useAutomaticThresholds = false;
        blendAgachado.AddChild(null, 0.0f);   // [ASIGNAR] clip Anim_Agachado
        blendAgachado.AddChild(null, 0.5f);   // [ASIGNAR] clip Anim_AgachadoAndar
        stateAgachado.motion = blendAgachado;

        // 3. APUNTANDO
        AnimatorState stateApuntar = rootStateMachine.AddState("Apuntando", new Vector3(550f, 150f));
        stateApuntar.motion = null;   // [ASIGNAR] clip Anim_Apuntar

        // 4. SALTAR
        AnimatorState stateSaltar = rootStateMachine.AddState("Saltar", new Vector3(300f, 0f));
        stateSaltar.motion = null;   // [ASIGNAR] clip Anim_Saltar

        // 5. DISPARAR
        AnimatorState stateDisparar = rootStateMachine.AddState("Disparar", new Vector3(550f, 0f));
        stateDisparar.motion = null;  // [ASIGNAR] clip Anim_Disparo (si tienes uno)

        // 6. MORIR
        AnimatorState stateMorir = rootStateMachine.AddState("Morir", new Vector3(300f, -150f));
        stateMorir.motion = null;    // [ASIGNAR] clip Anim_Morir

        // ── Estado por defecto ────────────────────────────────────────────────
        rootStateMachine.defaultState = stateLocomotion;

        // ── Transiciones ──────────────────────────────────────────────────────

        // Locomoción → Agachado
        var t = stateLocomotion.AddTransition(stateAgachado);
        t.AddCondition(AnimatorConditionMode.If, 0, "EstaAgachado");
        t.duration = 0.2f; t.hasExitTime = false;

        // Agachado → Locomoción
        var t2 = stateAgachado.AddTransition(stateLocomotion);
        t2.AddCondition(AnimatorConditionMode.IfNot, 0, "EstaAgachado");
        t2.duration = 0.2f; t2.hasExitTime = false;

        // Locomoción → Apuntando
        var t3 = stateLocomotion.AddTransition(stateApuntar);
        t3.AddCondition(AnimatorConditionMode.If, 0, "EstaApuntando");
        t3.duration = 0.15f; t3.hasExitTime = false;

        // Apuntando → Locomoción
        var t4 = stateApuntar.AddTransition(stateLocomotion);
        t4.AddCondition(AnimatorConditionMode.IfNot, 0, "EstaApuntando");
        t4.duration = 0.15f; t4.hasExitTime = false;

        // Cualquier estado → Saltar (por Trigger)
        var tSaltar = rootStateMachine.AddAnyStateTransition(stateSaltar);
        tSaltar.AddCondition(AnimatorConditionMode.If, 0, "Saltar");
        tSaltar.duration = 0.1f; tSaltar.hasExitTime = false;
        tSaltar.canTransitionToSelf = false;

        // Saltar → Locomoción (al tocar el suelo)
        var tSaltarFin = stateSaltar.AddTransition(stateLocomotion);
        tSaltarFin.AddCondition(AnimatorConditionMode.If, 0, "EstaEnSuelo");
        tSaltarFin.duration = 0.2f; tSaltarFin.hasExitTime = true; tSaltarFin.exitTime = 0.6f;

        // Cualquier estado → Disparar (por Trigger)
        var tDisparar = rootStateMachine.AddAnyStateTransition(stateDisparar);
        tDisparar.AddCondition(AnimatorConditionMode.If, 0, "Disparar");
        tDisparar.duration = 0.05f; tDisparar.hasExitTime = false;
        tDisparar.canTransitionToSelf = false;

        // Disparar → Locomoción (por tiempo de salida)
        var tDispararFin = stateDisparar.AddTransition(stateLocomotion);
        tDispararFin.hasExitTime = true; tDispararFin.exitTime = 0.9f; tDispararFin.duration = 0.1f;

        // Cualquier estado → Morir (por Trigger) — sin retorno
        var tMorir = rootStateMachine.AddAnyStateTransition(stateMorir);
        tMorir.AddCondition(AnimatorConditionMode.If, 0, "Morir");
        tMorir.duration = 0.1f; tMorir.hasExitTime = false;
        tMorir.canTransitionToSelf = false;

        // ── Guardar ───────────────────────────────────────────────────────────
        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();

        // Seleccionar el controller creado en el Project
        Selection.activeObject = controller;
        EditorGUIUtility.PingObject(controller);

        Debug.Log(
            "════════════════════════════════════════════════════\n" +
            "  [Alsasua] ✓ AnimatorMixamo.controller creado en:\n" +
            $"  {RUTA_OUTPUT}\n" +
            "  \n" +
            "  PASOS SIGUIENTES:\n" +
            "  1. Abre el Animator Controller (doble clic)\n" +
            "  2. Asigna los clips de Mixamo a cada estado [ASIGNAR]:\n" +
            "     · Locomocion > BlendTree: Idle (0.0) · Andar (0.5) · Correr (1.0)\n" +
            "     · Agachado   > BlendTree: Agachado (0.0) · AgachadoAndar (0.5)\n" +
            "     · Apuntando  → Anim_Apuntar\n" +
            "     · Saltar     → Anim_Saltar\n" +
            "     · Disparar   → Anim_Disparo (opcional)\n" +
            "     · Morir      → Anim_Morir\n" +
            "  3. Arrastra el controller al Inspector del ControladorJugador\n" +
            "════════════════════════════════════════════════════"
        );

        EditorUtility.DisplayDialog(
            "Animator Controller creado",
            "AnimatorMixamo.controller generado en Assets/Personajes/\n\n" +
            "Abre el Animator Controller y arrastra tus clips de Mixamo " +
            "a los estados marcados con [ASIGNAR].\n\n" +
            "Consulta la consola para ver los pasos detallados.",
            "OK"
        );
    }
}

#endif
