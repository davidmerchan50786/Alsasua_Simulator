// Assets/Editor/AutoSetupAlsasua.cs
using UnityEngine;
using UnityEditor;

[InitializeOnLoad]
public class AutoSetupAlsasua
{
    static AutoSetupAlsasua() { EditorApplication.delayCall += ReajustarAuto; }

    private static void ReajustarAuto()
    {
        if (SessionState.GetBool("AlsasuaAjustadoV10", false)) return;
        SessionState.SetBool("AlsasuaAjustadoV10", true);

        Debug.Log("[Alsasua V10] Escaneando si has descargado nuevos Assets de la Store para inyectarlos...");
        ImportMyAssetsTool.ImportarAssets();
        InyectarSistemasV7();
    }

    private static void InyectarSistemasV7()
    {
        Camera cam = Camera.main;
        ControladorCamaraSandbox camParams = null;
        
        if (cam != null && cam.GetComponent<ControladorCamaraSandbox>() == null)
        {
            var oldRB = cam.GetComponent<Rigidbody>();
            if (oldRB != null) Object.DestroyImmediate(oldRB);

            camParams = cam.gameObject.AddComponent<ControladorCamaraSandbox>();
            Debug.Log("[V7 Sandbox] ControladorCamaraSandbox inyectado nativamente en Main Camera.");
        } 
        else if (cam != null) 
        {
            camParams = cam.GetComponent<ControladorCamaraSandbox>();
        }

        if (Object.FindFirstObjectByType<SistemaCicloDia>() == null)
        {
            GameObject worldManager = new GameObject("Alsasua_V16_Arquitectura_Limpia");
            worldManager.AddComponent<SistemaCicloDia>();
            worldManager.AddComponent<SistemaClima>();
            worldManager.AddComponent<SistemaDialogos>(); 
            var genAmb = worldManager.AddComponent<GeneradorAmbienteUrbano>(); 
            
            if (camParams != null) {
                camParams.transform.localPosition = new Vector3(0, 0, -15);
                camParams.gameObject.AddComponent<SistemaBalistico>(); 
            }

            worldManager.AddComponent<UnityEngine.InputSystem.PlayerInput>(); 
            var nukeEv = worldManager.AddComponent<EventoTermonuclear>(); 
            worldManager.AddComponent<GestorAmbienteEspacial>();

            // V13
            worldManager.AddComponent<ControladorDisturbios>();
            worldManager.AddComponent<EscuadraAntiDisturbios>();
            
            // V14
            worldManager.AddComponent<MegaManifestacion>();

            // V16 Atmosférico
            worldManager.AddComponent<ControladorClimatico>();

            genAmb.prefabPunk = EncontrarPrefabUnico("Punk", "Character");
            genAmb.prefabPerro = EncontrarPrefabUnico("Dog", "Hound", "Wolf");
            genAmb.prefabRata = EncontrarPrefabUnico("Rat", "Mouse");
            
            nukeEv.prefabMisil = EncontrarPrefabUnico("Missile", "Nuke", "Rocket");
            nukeEv.prefabHongo = EncontrarPrefabUnico("Mushroom", "NuclearFx", "ExplosionHuge");
            
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
