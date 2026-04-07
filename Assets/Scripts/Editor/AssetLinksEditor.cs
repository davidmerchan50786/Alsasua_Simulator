// Assets/Scripts/Editor/AssetLinksEditor.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Panel de descarga de assets externos para el simulador de Alsasua.
//  Menú Unity: Alsasua → Descargar Assets Externos
//  Muestra una ventana con todos los links directos a cada asset.
// ═══════════════════════════════════════════════════════════════════════════

#if UNITY_EDITOR
using UnityEngine;
using UnityEditor;

public sealed class AssetLinksEditor : EditorWindow
{
    private Vector2 _scroll;

    [MenuItem("Alsasua/Descargar Assets Externos")]
    private static void MostrarVentana()
    {
        var w = GetWindow<AssetLinksEditor>("Assets Alsasua");
        w.minSize = new Vector2(680, 580);
        w.Show();
    }

    [MenuItem("Alsasua/Log URLs de todos los assets")]
    private static void LogAllURLs()
    {
        Debug.Log(
"╔═══════════════════════════════════════════════════════════════════╗\n" +
"║         ASSETS EXTERNOS — ALSASUA SIMULATOR                       ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  1. GUARDIA CIVIL — Personaje                                      ║\n" +
"║     Guardia Civil España (Sketchfab, CC-BY):                       ║\n" +
"║     https://sketchfab.com/3d-models/guardia-civil-espana-         ║\n" +
"║     c91cd770ea194157aa2bbc8cfd64e8a0                               ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  2. GUARDIA CIVIL — Coche patrulla Toyota Land Cruiser             ║\n" +
"║     Toyota Land Cruiser Guardia Civil (Sketchfab, CC-BY):          ║\n" +
"║     https://sketchfab.com/3d-models/toyota-land-cruiser-          ║\n" +
"║     guardia-civil-3af00e417f78410096f681f007d830e5                 ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  3. POLICÍA FORAL — Personaje (sin modelo dedicado)                ║\n" +
"║     Usar personaje genérico + textura azul marino/verde boina.     ║\n" +
"║     Policeman tag en Sketchfab:                                     ║\n" +
"║     https://sketchfab.com/tags/policeman                            ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  4. KEFFIYEH / PAÑUELO PALESTINO (Sketchfab, CC-BY):               ║\n" +
"║     https://sketchfab.com/3d-models/palestinian-scarf-            ║\n" +
"║     9778da8a04c14091ae3b8388973bbc1f                               ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  5. IKURRIÑA — base mesh bandera animada (Sketchfab, CC-BY):        ║\n" +
"║     https://sketchfab.com/3d-models/animated-flag-               ║\n" +
"║     3aa23ffa68cb4cbba1acfe983f8f4b4c                               ║\n" +
"║     Textura SVG ikurriña (Wikimedia, PD):                           ║\n" +
"║     https://commons.wikimedia.org/wiki/File:Flag_of_the_          ║\n" +
"║     Basque_Country.svg                                              ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  6. BANDERA NAVARRA — misma base + textura SVG (Wikimedia, PD):     ║\n" +
"║     https://commons.wikimedia.org/wiki/File:Bandera_de_Navarra.svg ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  7. CIVILES — City People FREE Samples (Unity Asset Store):          ║\n" +
"║     https://assetstore.unity.com/packages/3d/characters/           ║\n" +
"║     city-people-free-samples-260446                                 ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  8. SEAT IBIZA (CGTrader, gratis):                                  ║\n" +
"║     https://www.cgtrader.com/free-3d-models/car/car/               ║\n" +
"║     seat-ibiza-2-1999-2002                                          ║\n" +
"╠═══════════════════════════════════════════════════════════════════╣\n" +
"║  9. BOSQUE — Environment Pack Free Forest Sample (Asset Store):     ║\n" +
"║     https://assetstore.unity.com/packages/3d/environments/         ║\n" +
"║     environment-pack-free-forest-sample-168396                      ║\n" +
"╚═══════════════════════════════════════════════════════════════════╝\n" +
"\nPara cada asset descargado desde Sketchfab:\n" +
"  1. Descarga como GLB/FBX\n" +
"  2. Importa en Unity: arrastra a Assets/Personajes/[TipoPersonaje]/\n" +
"  3. Asigna el mesh en Inspector de SistemaPersonajes o SistemaTrafico\n" +
"  4. Ejecuta menú Alsasua → Configurar Assets Kenney"
        );
    }

    private void OnGUI()
    {
        EditorGUILayout.Space(8);
        var titleStyle = new GUIStyle(EditorStyles.boldLabel)
        {
            fontSize   = 15,
            alignment  = TextAnchor.MiddleCenter,
        };
        EditorGUILayout.LabelField("ALSASUA SIMULATOR — Assets Externos", titleStyle,
                                   GUILayout.Height(24));
        EditorGUILayout.Space(4);
        EditorGUILayout.HelpBox(
            "Descarga cada asset desde los enlaces de abajo, impórtalos en Unity " +
            "y asígnalos en los campos Inspector de SistemaPersonajes / SistemaTrafico / SistemaVegetacion.\n\n" +
            "CC-BY = requiere atribuir al autor  |  CC0 / PD = dominio público",
            MessageType.Info);
        EditorGUILayout.Space(6);

        _scroll = EditorGUILayout.BeginScrollView(_scroll);

        // ── GUARDIA CIVIL ────────────────────────────────────────────────
        Seccion("GUARDIA CIVIL — Personaje",
            "Guardia Civil España", "Ernesto.Sanabria.Gonzalez",
            "CC-BY 4.0", "FBX/GLB",
            "https://sketchfab.com/3d-models/guardia-civil-espana-c91cd770ea194157aa2bbc8cfd64e8a0",
            "Uniforme Benemérita completo con tricornio. Importar en\n" +
            "Assets/Personajes/GuardiaCivil/ y asignar en SistemaPersonajes → Mesh GC.");

        // ── GC COCHE ─────────────────────────────────────────────────────
        Seccion("GUARDIA CIVIL — Coche patrulla",
            "Toyota Land Cruiser Guardia Civil", "CB1IFA",
            "CC-BY 4.0", "GLB/GLTF",
            "https://sketchfab.com/3d-models/toyota-land-cruiser-guardia-civil-3af00e417f78410096f681f007d830e5",
            "Livery oficial GC. 715k tris (reducir en Blender si es necesario).\n" +
            "Importar en Assets/Vehiculos/ y asignar en SistemaTrafico → Prefab Patrulla GC.");

        // ── POLICÍA FORAL ─────────────────────────────────────────────────
        Seccion("POLICÍA FORAL DE NAVARRA — Personaje",
            "(Sin modelo dedicado — usar genérico + textura)", "—",
            "—", "—",
            "https://sketchfab.com/tags/policeman",
            "Busca 'police officer' en Sketchfab, descarga cualquier personaje\n" +
            "de policía con licencia libre y aplica textura azul marino oscuro\n" +
            "con franja amarilla hi-vis (textura procedural incluida en SistemaPersonajes).");

        // ── KEFFIYEH ──────────────────────────────────────────────────────
        Seccion("KEFFIYEH / PAÑUELO PALESTINO (accesorio)",
            "Palestinian Scarf", "Fraxion Fx",
            "CC-BY 4.0", "FBX + PBR 4K",
            "https://sketchfab.com/3d-models/palestinian-scarf-9778da8a04c14091ae3b8388973bbc1f",
            "Accesorio de cabeza/cuello. Importar en Assets/Personajes/Accesorios/\n" +
            "y asignar en SistemaPersonajes → Mesh Keffiyeh.\n" +
            "Alternativa: https://sketchfab.com/3d-models/palestinian-kofiya-scarf-70f1156d78354300a412c4b8ca30d6c2b15baca0");

        // ── IKURRIÑA ──────────────────────────────────────────────────────
        Seccion("IKURRIÑA — Bandera vasca (mesh animado + textura)",
            "Animated Flag (base mesh)", "ManySince910",
            "CC-BY", "GLB",
            "https://sketchfab.com/3d-models/animated-flag-3aa23ffa68cb4cbba1acfe983f8f4b4c",
            "Descarga el mesh de bandera animada y aplica la textura SVG de la\n" +
            "ikurriña (incluida proceduralmente en SistemaPersonajes ya).\n" +
            "SVG fuente: https://commons.wikimedia.org/wiki/File:Flag_of_the_Basque_Country.svg\n\n" +
            "ALTERNATIVA (paquete de banderas animadas, CC-BY):\n" +
            "https://sketchfab.com/3d-models/free-animated-3d-flags-ac8c876ac1a94179b2e827454c1ecc97");

        // ── NAVARRA ────────────────────────────────────────────────────────
        Seccion("BANDERA DE NAVARRA — Textura SVG (dominio público)",
            "Bandera de Navarra SVG", "Wikimedia Commons",
            "Dominio público", "SVG → PNG",
            "https://commons.wikimedia.org/wiki/File:Bandera_de_Navarra.svg",
            "Descarga el SVG, convierte a PNG 512×320 px y asigna como textura\n" +
            "al mesh de bandera genérico (misma base que ikurriña).\n" +
            "NOTA: SistemaPersonajes ya genera una textura Navarra procedural\n" +
            "como fallback; esta textura real mejora el realismo.");

        // ── CIVILES ────────────────────────────────────────────────────────
        Seccion("CIVILES — Personajes peatones (Unity Asset Store)",
            "City People FREE Samples", "Denys Almaral",
            "Unity Asset Store EULA (gratis)", ".unitypackage",
            "https://assetstore.unity.com/packages/3d/characters/city-people-free-samples-260446",
            "8 personajes humanoides con 22+ variantes de textura.\n" +
            "Instalar via Package Manager y asignar meshes en SistemaPersonajes.\n\n" +
            "TAMBIÉN: PolyPeople Series - City People [Free]\n" +
            "https://assetstore.unity.com/packages/3d/characters/polypeople-series-city-people-free-325204");

        // ── SEAT IBIZA ─────────────────────────────────────────────────────
        Seccion("SEAT IBIZA — Coche civil español",
            "Seat Ibiza 2 (1999–2002)", "CGTrader contributor",
            "CGTrader Royalty Free (gratis con cuenta)", "FBX / OBJ",
            "https://www.cgtrader.com/free-3d-models/car/car/seat-ibiza-2-1999-2002",
            "Período correcto para Alsasua. Importar en Assets/Vehiculos/ y\n" +
            "asignar en SistemaTrafico → Prefab Coche Civil.\n\n" +
            "SKETCHFAB 1996 Seat Ibiza (alternativa):\n" +
            "https://sketchfab.com/3d-models/1996-seat-ibiza-free-download-aca74285d9874785bff308c7c288f445");

        // ── BOSQUE ─────────────────────────────────────────────────────────
        Seccion("BOSQUE — Environment Pack Free Forest Sample",
            "Environment Pack: Free Forest Sample", "MentalUnity",
            "Unity Asset Store EULA (gratis)", ".unitypackage",
            "https://assetstore.unity.com/packages/3d/environments/environment-pack-free-forest-sample-168396",
            "Árboles y vegetación de bosque boreal. Compatible con URP.\n" +
            "Instalar via Package Manager.\n" +
            "Para usar con SistemaVegetacion: asigna los Prefabs de árbol\n" +
            "en los campos Mesh Pino / Mesh Roble del Inspector.");

        EditorGUILayout.EndScrollView();
        EditorGUILayout.Space(6);

        if (GUILayout.Button("Abrir menú Cesium (tilesets fotorrealistas)", GUILayout.Height(32)))
            EditorApplication.ExecuteMenuItem("Cesium/Connect to Cesium Ion");

        if (GUILayout.Button("Copiar todas las URLs al portapapeles", GUILayout.Height(28)))
        {
            GUIUtility.systemCopyBuffer =
                "1. Guardia Civil España:  https://sketchfab.com/3d-models/guardia-civil-espana-c91cd770ea194157aa2bbc8cfd64e8a0\n" +
                "2. Toyota Land Cruiser GC: https://sketchfab.com/3d-models/toyota-land-cruiser-guardia-civil-3af00e417f78410096f681f007d830e5\n" +
                "3. Policeman tag:          https://sketchfab.com/tags/policeman\n" +
                "4. Keffiyeh:               https://sketchfab.com/3d-models/palestinian-scarf-9778da8a04c14091ae3b8388973bbc1f\n" +
                "5. Animated Flag:          https://sketchfab.com/3d-models/animated-flag-3aa23ffa68cb4cbba1acfe983f8f4b4c\n" +
                "6. Ikurriña SVG:           https://commons.wikimedia.org/wiki/File:Flag_of_the_Basque_Country.svg\n" +
                "7. Bandera Navarra SVG:    https://commons.wikimedia.org/wiki/File:Bandera_de_Navarra.svg\n" +
                "8. City People FREE:       https://assetstore.unity.com/packages/3d/characters/city-people-free-samples-260446\n" +
                "9. Seat Ibiza:             https://www.cgtrader.com/free-3d-models/car/car/seat-ibiza-2-1999-2002\n" +
                "10. Forest Sample:         https://assetstore.unity.com/packages/3d/environments/environment-pack-free-forest-sample-168396";
            Debug.Log("[Alsasua] URLs copiadas al portapapeles.");
        }

        EditorGUILayout.Space(4);
    }

    // ───────────────────────────────────────────────────────────────────────
    //  HELPER GUI
    // ───────────────────────────────────────────────────────────────────────
    private void Seccion(string titulo, string nombre, string autor,
                         string licencia, string formato, string url, string notas)
    {
        var headerStyle = new GUIStyle(EditorStyles.boldLabel)
        {
            normal = { textColor = new Color(0.9f, 0.75f, 0.2f) },
        };

        EditorGUILayout.LabelField(titulo, headerStyle);

        EditorGUILayout.BeginVertical(EditorStyles.helpBox);
        EditorGUILayout.LabelField($"Nombre: {nombre}", EditorStyles.wordWrappedLabel);
        EditorGUILayout.LabelField($"Autor: {autor}  |  Licencia: {licencia}  |  Formato: {formato}",
                                   EditorStyles.miniLabel);

        EditorGUILayout.BeginHorizontal();
        EditorGUILayout.SelectableLabel(url, EditorStyles.textField, GUILayout.Height(18));
        if (GUILayout.Button("Abrir", GUILayout.Width(54), GUILayout.Height(18)))
            Application.OpenURL(url);
        EditorGUILayout.EndHorizontal();

        EditorGUILayout.LabelField(notas, EditorStyles.wordWrappedMiniLabel);
        EditorGUILayout.EndVertical();

        EditorGUILayout.Space(6);
    }
}
#endif
