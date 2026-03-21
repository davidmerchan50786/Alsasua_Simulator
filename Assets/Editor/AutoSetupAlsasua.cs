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
        // V10: Forzamos la ejecución de AutoSetup cada vez para rastrear nuevos pacakges
        if (SessionState.GetBool("AlsasuaAjustadoV10", false)) return;
        SessionState.SetBool("AlsasuaAjustadoV10", true);

        // Auto-importar CUALQUIER unitypackage nuevo que haya descargado el usuario ahora mismo
        Debug.Log("[Alsasua V10] Escaneando si has descargado nuevos Assets de la Store para inyectarlos...");
        ImportMyAssetsTool.ImportarAssets();

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
            GameObject worldManager = new GameObject("Alsasua_V10_Ultra_Manager");
            worldManager.AddComponent<SistemaCicloDia>();
            worldManager.AddComponent<SistemaClima>();
            worldManager.AddComponent<SistemaDialogos>(); 
            var genAmb = worldManager.AddComponent<GeneradorAmbienteUrbano>(); 
            var nukeEv = worldManager.AddComponent<EventoTermonuclear>(); 
            worldManager.AddComponent<GestorAmbienteEspacial>();

            // AUTOCONEXIÓN DE ASSETS 3D DESCARGADOS (V10)
            genAmb.prefabPunk = EncontrarPrefabUnico("Punk", "Character");
            genAmb.prefabPerro = EncontrarPrefabUnico("Dog", "Hound", "Wolf");
            genAmb.prefabRata = EncontrarPrefabUnico("Rat", "Mouse");
            
            nukeEv.prefabMisil = EncontrarPrefabUnico("Missile", "Nuke", "Rocket");
            nukeEv.prefabHongo = EncontrarPrefabUnico("Mushroom", "NuclearFx", "ExplosionHuge");
            
            Debug.Log("[V10 Ultra] Buscador de AssetDatabase ha escaneado e inyectado modelos 3D a los scripts.");
            
            // Marcar la escena como modificada
            UnityEditor.SceneManagement.EditorSceneManager.MarkSceneDirty(UnityEditor.SceneManagement.EditorSceneManager.GetActiveScene());
        }
    }

    private static GameObject EncontrarPrefabUnico(params string[] palabrasClave)
    {
        foreach(string palabra in palabrasClave)
        {
            string[] guids = AssetDatabase.FindAssets("t:Prefab " + palabra);
            if (guids.Length > 0)
            {
                return AssetDatabase.LoadAssetAtPath<GameObject>(AssetDatabase.GUIDToAssetPath(guids[0]));
            }
        }
        return null;
    }
}
