// Assets/Scripts/FabricaSintetica.cs
using UnityEngine;

/// <summary>
/// Fábrica de Arte Sintético Procedural.
/// Patrón: Factory Method / Abstraction (V15 CLEAN ARCHITECTURE).
/// Aisla toda la carga poligonal y de materiales del motor de IA.
/// </summary>
public static class FabricaSintetica
{
    public static GameObject EnsamblarPunkBase(Material matCuerpo, Material matPiel, Material matCresta, bool optimizadoParaBoids)
    {
        GameObject punk = new GameObject("Estructura_Sintetica_Punk");
        
        // Tórax
        GameObject cuerpo = GameObject.CreatePrimitive(PrimitiveType.Capsule);
        cuerpo.transform.SetParent(punk.transform);
        cuerpo.transform.localPosition = Vector3.up * 1f;
        cuerpo.GetComponent<Renderer>().sharedMaterial = matCuerpo;
        Object.Destroy(cuerpo.GetComponent<Collider>());

        // Cráneo
        GameObject cabeza = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        cabeza.transform.SetParent(punk.transform);
        cabeza.transform.localPosition = Vector3.up * 2.2f;
        cabeza.transform.localScale = Vector3.one * 0.8f;
        cabeza.GetComponent<Renderer>().sharedMaterial = matPiel;
        Object.Destroy(cabeza.GetComponent<Collider>());

        // Pelo
        if (optimizadoParaBoids)
        {
            GameObject pelo = GameObject.CreatePrimitive(PrimitiveType.Cube);
            pelo.transform.SetParent(cabeza.transform);
            pelo.transform.localPosition = new Vector3(0f, 0.45f, 0f);
            pelo.transform.localScale = new Vector3(0.1f, 0.4f, 0.6f);
            pelo.GetComponent<Renderer>().sharedMaterial = matCresta;
            Object.Destroy(pelo.GetComponent<Collider>());
        }
        else
        {
            for(int i = -3; i <= 3; i++)
            {
                GameObject pelo = GameObject.CreatePrimitive(PrimitiveType.Cube);
                pelo.transform.SetParent(cabeza.transform);
                pelo.transform.localPosition = new Vector3(0f, 0.45f, i * 0.12f);
                pelo.transform.localScale = new Vector3(0.1f, 0.5f - Mathf.Abs(i)*0.05f, 0.2f);
                pelo.transform.localRotation = Quaternion.Euler(i*12f, 0, 0);
                pelo.GetComponent<Renderer>().sharedMaterial = matCresta;
                Object.Destroy(pelo.GetComponent<Collider>());
            }
        }

        return punk;
    }

    public static GameObject InstanciarManchaGore(Vector3 pos, float tamaño, Color color)
    {
        GameObject charco = GameObject.CreatePrimitive(PrimitiveType.Quad);
        charco.transform.position = pos;
        charco.transform.rotation = Quaternion.Euler(90, 0, Random.Range(0, 360));
        charco.transform.localScale = Vector3.one * tamaño;
        
        Material mat = new Material(Shader.Find("Universal Render Pipeline/Unlit") ?? Shader.Find("Unlit/Color"));
        mat.color = color;
        charco.GetComponent<Renderer>().sharedMaterial = mat;
        Object.Destroy(charco.GetComponent<Collider>());
        
        return charco;
    }

    public static GameObject InstanciarJeringuillaFisica(Vector3 pos)
    {
        GameObject jeringa = new GameObject("Props_Jeringuilla_Fisica");
        jeringa.transform.position = pos;
        
        var cilindro = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        cilindro.transform.SetParent(jeringa.transform);
        cilindro.transform.localPosition = Vector3.zero;
        cilindro.transform.localScale = new Vector3(0.01f, 0.08f, 0.01f);
        cilindro.transform.localRotation = Quaternion.Euler(90, 45, 0);
        
        Material mat = new Material(Shader.Find("Standard"));
        mat.color = new Color(0.8f, 0.9f, 0.9f, 0.5f);
        mat.SetFloat("_Mode", 3); // Traslúcido Standard
        cilindro.GetComponent<Renderer>().sharedMaterial = mat;
        Object.Destroy(cilindro.GetComponent<Collider>());

        return jeringa;
    }
}
