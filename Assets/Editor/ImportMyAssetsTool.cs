// Assets/Editor/ImportMyAssetsTool.cs
using UnityEngine;
using UnityEditor;

public class ImportMyAssetsTool : EditorWindow
{
    [MenuItem("Alsasua/Importar MyAssets (Gráficos AAA)")]
    public static void ImportarAssets()
    {
        string[] packages = {
            @"C:\Users\coperenea\AppData\Roaming\Unity\Asset Store-5.x\Polygon Blacksmith\3D ModelsCharacters\Low Poly Soldiers Demo.unitypackage",
            @"C:\Users\coperenea\AppData\Roaming\Unity\Asset Store-5.x\SICS Games\3D ModelsVehiclesLand\Police Car Helicopter.unitypackage",
            @"C:\Users\coperenea\AppData\Roaming\Unity\Asset Store-5.x\Mirza Beig\Particle SystemsFire\Cinematic Explosions FREE.unitypackage"
        };
        
        int count = 0;
        foreach (var pkg in packages)
        {
            if (System.IO.File.Exists(pkg))
            {
                Debug.Log($"[Alsasua V4] Desempaquetando e instalando automáticamente: {pkg}");
                AssetDatabase.ImportPackage(pkg, false); // interactive = false -> instalación silenciosa rápida
                count++;
            }
            else
            {
                Debug.LogWarning($"[Alsasua V4] No se localizó el paquete: {pkg}");
            }
        }
        
        if (count > 0)
        {
            Debug.Log($"[Alsasua V4] ¡Éxito! Se han inyectado {count} paquetes visuales AAA en el Proyecto.");
        }
        else
        {
            Debug.LogError("[Alsasua V4] Error crítico: No se encontraron los paquetes en la caché local de AppData.");
        }
    }
}
