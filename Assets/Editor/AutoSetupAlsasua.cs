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
        if (SessionState.GetBool("AlsasuaAjustado", false)) return;
        SessionState.SetBool("AlsasuaAjustado", true);

        // Si no existen los soldados, probablemente nada está importado.
        string[] soldados = AssetDatabase.FindAssets("t:Prefab Soldier");
        if (soldados.Length == 0)
        {
            Debug.Log("[Alsasua V4] Detectada primera ejecución. Importando MyAssets AAA automáticamente...");
            ImportMyAssetsTool.ImportarAssets();
        }
    }
}
