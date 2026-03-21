// Assets/Editor/AutoSetupAlsasua.cs
using UnityEngine;
using UnityEditor;

[InitializeOnLoad]
public class AutoSetupAlsasua
{
    static AutoSetupAlsasua()
    {
        EditorApplication.delayCall += ReajustarAuto;
    }

    private static void ReajustarAuto()
    {
        if (SessionState.GetBool("AlsasuaAjustadoV7", false)) return;
        SessionState.SetBool("AlsasuaAjustadoV7", true);

        // Si no existen los soldados, probablemente nada está importado.
        string[] soldados = AssetDatabase.FindAssets("t:Prefab Soldier");
        if (soldados.Length == 0)
        {
            Debug.Log("[Alsasua V4] Detectada primera ejecución. Importando MyAssets AAA automáticamente...");
            ImportMyAssetsTool.ImportarAssets();
        }

        // V7 ZERO-CLICK AUTOLOAD: Instalar Sandbox Dinámico en la Escena Actual
        InyectarSistemasV7();
    }

    private static void InyectarSistemasV7()
    {
        // 1. Inyectar God Mode Camera
        Camera cam = Camera.main;
        if (cam != null && cam.GetComponent<ControladorCamaraSandbox>() == null)
        {
            // Remover componentes molestos previos si existen (como el FPS Controller viejo ligado a cámara)
            var oldRB = cam.GetComponent<Rigidbody>();
            if (oldRB != null) Object.DestroyImmediate(oldRB);

            cam.gameObject.AddComponent<ControladorCamaraSandbox>();
            Debug.Log("[V7 Sandbox] ControladorCamaraSandbox inyectado nativamente en Main Camera.");
        }

        // 2. Inyectar Gestor de Mundo Sandbox Autónomo
        if (Object.FindObjectOfType<SistemaCicloDia>() == null)
        {
            GameObject worldManager = new GameObject("Alsasua_Apocalypse_V8_Manager");
            worldManager.AddComponent<SistemaCicloDia>();
            worldManager.AddComponent<SistemaClima>();
            worldManager.AddComponent<SistemaDialogos>(); // V8 Diálogos Ramificados
            worldManager.AddComponent<GeneradorAmbienteUrbano>(); // V8 Punks, Ratas, Perros
            worldManager.AddComponent<EventoTermonuclear>(); // V8 Misil Termonuclear (Pulsa N)
            
            Debug.Log("[V8 Apocalypse] Ecosistema Climático, Lore Ácido, Ratas y Eventos Nucleares inyectados en el nivel.");
            
            // Marcar la escena como modificada
            UnityEditor.SceneManagement.EditorSceneManager.MarkSceneDirty(UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene());
        }
    }
}
