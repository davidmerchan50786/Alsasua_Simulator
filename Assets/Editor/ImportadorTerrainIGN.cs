using UnityEngine;
using UnityEditor;
using System.IO;
using System;
using CesiumForUnity;
using Unity.Mathematics;

/// <summary>
/// Herramienta del Editor para importar datos MDT del IGN (Instituto Geográfico Nacional)
/// y crear un Unity Terrain estático correctamente dimensionado y posicionado sobre Alsasua.
///
/// FLUJO COMPLETO:
///   1. Descargar MDT05 desde CNIG: https://centrodedescargas.cnig.es/CentroDescargas/
///   2. (Opcional) Descargar PNOA Ortofoto para la misma hoja
///   3. Convertir el .tif a RAW 16-bit con los comandos GDAL de la pestaña "Comandos GDAL"
///   4. Configurar esta ventana (pestaña "Importar") y pulsar "Crear Terrain"
///
/// RESULTADO:
///   - Unity Terrain asset en Assets/Terrains/Altsasu/
///   - GameObject posicionado geográficamente con CesiumGlobeAnchor
///   - Textura de ortofoto PNOA aplicada como TerrainLayer
///   - GestorCapasTerrain para alternar entre este terrain y los tiles Cesium en runtime
/// </summary>
public class ImportadorTerrainIGN : EditorWindow
{
    // ── Pestañas ──────────────────────────────────────────────────────────────
    private enum Pestana { Importar, Guia, GDAL }
    private Pestana pestanaActual = Pestana.Importar;

    // ── Heightmap ─────────────────────────────────────────────────────────────
    private string rutaHeightmap        = "";
    private bool   heightmapLittleEndian = true;    // GDAL genera LE (Windows) por defecto
    private int    heightmapResolucion   = 1025;    // debe ser 2^n + 1 para Unity Terrain

    // ── Ortofoto ──────────────────────────────────────────────────────────────
    private string rutaOrtofoto      = "";
    private bool   crearCapaOrtofoto = true;

    // ── Dimensiones del terreno (metros) ─────────────────────────────────────
    // Bbox de ~4×4 km centrado en Alsasua — ajustar según el área real descargada
    private float anchoTerrain = 4600f;   // metros E-O
    private float largoTerrain = 5100f;   // metros N-S
    private float alturaMin    = 510f;    // m WGS84 elipsoidal (fondo del valle de Sakana)
    private float alturaMax    = 1095f;   // m WGS84 elipsoidal (Monte Aratz ~ 1027 m + margen)
    private float AlturaRango => alturaMax - alturaMin;

    // ── Posición geográfica: esquina SO (SW) del heightmap ────────────────────
    // SW del bbox 4.6 km × 5.1 km centrado en Alsasua (42.9037°N, -2.1668°E):
    //   lat_SW = 42.9037 - 5100/(111320×2) ≈ 42.881
    //   lon_SW = -2.1668 - 4600/(81500×2)  ≈ -2.209  (cos 42.9° ≈ 0.732)
    private double geoLat  = 42.881;
    private double geoLon  = -2.209;

    // Ondulación del geoide EGM96 en Alsasua: ~+44 m
    // Las altitudes IGN son ortométricas (sobre el mar). Para Cesium (WGS84 elipsoidal)
    // hay que sumar la ondulación.  El script suma este valor automáticamente.
    private float geoidUndulation = 44f;

    // ── Opciones ──────────────────────────────────────────────────────────────
    private bool   posicionarConCesium = true;
    private bool   anadirGestorCapas   = true;
    private string nombreTerrain       = "Terrain_Altsasu_IGN";

    // ── UI state ──────────────────────────────────────────────────────────────
    private Vector2 scrollGuia;
    private Vector2 scrollImportar;

    // ─────────────────────────────────────────────────────────────────────────
    [MenuItem("Alsasua/Importar Terrain IGN %#t")]   // Ctrl+Shift+T
    public static void MostrarVentana()
    {
        var w = GetWindow<ImportadorTerrainIGN>("Terrain IGN");
        w.minSize = new Vector2(460, 580);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  GUI PRINCIPAL
    // ─────────────────────────────────────────────────────────────────────────
    private void OnGUI()
    {
        EditorGUILayout.Space(6);
        pestanaActual = (Pestana)GUILayout.Toolbar(
            (int)pestanaActual,
            new[] { "  Importar  ", "  Guía QGIS/IGN  ", "  Comandos GDAL  " });
        EditorGUILayout.Space(6);

        switch (pestanaActual)
        {
            case Pestana.Importar: DibujarPestanaImportar(); break;
            case Pestana.Guia:    DibujarPestanaGuia();     break;
            case Pestana.GDAL:    DibujarPestanaGDAL();     break;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  PESTAÑA: IMPORTAR
    // ─────────────────────────────────────────────────────────────────────────
    private void DibujarPestanaImportar()
    {
        scrollImportar = EditorGUILayout.BeginScrollView(scrollImportar);

        // ── Heightmap ─────────────────────────────────────────────────────────
        SeccionTitulo("1. Heightmap RAW (MDT del IGN convertido con GDAL)");

        EditorGUILayout.BeginHorizontal();
        rutaHeightmap = EditorGUILayout.TextField("Archivo .raw / .r16", rutaHeightmap);
        if (GUILayout.Button("…", GUILayout.Width(28)))
            rutaHeightmap = EditorUtility.OpenFilePanel("Seleccionar heightmap", "", "raw,r16,bin");
        EditorGUILayout.EndHorizontal();

        heightmapResolucion = EditorGUILayout.IntPopup(
            "Resolución (píxeles)", heightmapResolucion,
            new[] { "129×129", "257×257", "513×513", "1025×1025", "2049×2049", "4097×4097" },
            new[] { 129, 257, 513, 1025, 2049, 4097 });
        heightmapLittleEndian = EditorGUILayout.Toggle(
            "Little-endian (GDAL/Windows)", heightmapLittleEndian);

        if (!string.IsNullOrEmpty(rutaHeightmap) && File.Exists(rutaHeightmap))
        {
            long esperados = (long)heightmapResolucion * heightmapResolucion * 2;
            long reales    = new FileInfo(rutaHeightmap).Length;
            if (reales < esperados)
                EditorGUILayout.HelpBox(
                    $"El archivo tiene {reales:N0} bytes, pero {heightmapResolucion}×{heightmapResolucion} " +
                    $"16-bit requiere {esperados:N0}. Verifica la resolución o el archivo.", MessageType.Warning);
            else
                EditorGUILayout.HelpBox($"✓ Archivo válido ({reales:N0} bytes).", MessageType.None);
        }

        EditorGUILayout.Space(8);

        // ── Ortofoto ──────────────────────────────────────────────────────────
        SeccionTitulo("2. Ortofoto PNOA (opcional — textura del terreno)");

        crearCapaOrtofoto = EditorGUILayout.Toggle("Aplicar ortofoto como textura", crearCapaOrtofoto);
        if (crearCapaOrtofoto)
        {
            EditorGUILayout.BeginHorizontal();
            rutaOrtofoto = EditorGUILayout.TextField("Ortofoto .jpg / .png / .tif", rutaOrtofoto);
            if (GUILayout.Button("…", GUILayout.Width(28)))
                rutaOrtofoto = EditorUtility.OpenFilePanel("Seleccionar ortofoto PNOA", "", "jpg,jpeg,png,tif");
            EditorGUILayout.EndHorizontal();
        }

        EditorGUILayout.Space(8);

        // ── Dimensiones ───────────────────────────────────────────────────────
        SeccionTitulo("3. Dimensiones del terreno (metros)");

        anchoTerrain = EditorGUILayout.FloatField("Ancho E-O  (m)", anchoTerrain);
        largoTerrain = EditorGUILayout.FloatField("Largo N-S  (m)", largoTerrain);

        EditorGUILayout.Space(2);
        alturaMin = EditorGUILayout.FloatField("Altitud mínima ortométrica (m MSL)", alturaMin);
        alturaMax = EditorGUILayout.FloatField("Altitud máxima ortométrica (m MSL)", alturaMax);
        geoidUndulation = EditorGUILayout.FloatField(
            "Ondulación geoide EGM96 (+m)", geoidUndulation);
        EditorGUILayout.HelpBox(
            $"Rango de elevación: {AlturaRango:F0} m  |  " +
            $"Alturas WGS84 elipsoidales: {alturaMin + geoidUndulation:F0} – {alturaMax + geoidUndulation:F0} m",
            MessageType.None);

        if (alturaMin >= alturaMax)
            EditorGUILayout.HelpBox("Altitud mínima ≥ máxima.", MessageType.Error);

        EditorGUILayout.Space(8);

        // ── Posicionamiento ───────────────────────────────────────────────────
        SeccionTitulo("4. Posicionamiento geográfico (esquina SW del heightmap)");

        geoLat = EditorGUILayout.DoubleField("Latitud SW  (°N)", geoLat);
        geoLon = EditorGUILayout.DoubleField("Longitud SW (°E)", geoLon);
        posicionarConCesium = EditorGUILayout.Toggle(
            "Posicionar con CesiumGlobeAnchor", posicionarConCesium);
        anadirGestorCapas = EditorGUILayout.Toggle(
            "Añadir GestorCapasTerrain", anadirGestorCapas);
        EditorGUILayout.HelpBox(
            "CesiumGlobeAnchor coloca el corner SW del terrain en las coordenadas " +
            "geográficas indicadas. El terrain se extiende hacia el Este (+X) y el Norte (+Z).",
            MessageType.Info);

        EditorGUILayout.Space(8);

        // ── Nombre ────────────────────────────────────────────────────────────
        nombreTerrain = EditorGUILayout.TextField("Nombre del asset", nombreTerrain);

        EditorGUILayout.Space(10);

        // ── Botón principal ───────────────────────────────────────────────────
        bool archivosOk = !string.IsNullOrEmpty(rutaHeightmap) && File.Exists(rutaHeightmap)
                          && alturaMin < alturaMax && anchoTerrain > 0 && largoTerrain > 0;

        using (new EditorGUI.DisabledGroupScope(!archivosOk))
        {
            if (GUILayout.Button("▶  Crear Unity Terrain", GUILayout.Height(38)))
                CrearTerrain();
        }

        if (!archivosOk)
            EditorGUILayout.HelpBox(
                "Selecciona un heightmap .raw válido y comprueba las dimensiones.", MessageType.Info);

        EditorGUILayout.EndScrollView();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  PESTAÑA: GUÍA QGIS / IGN
    // ─────────────────────────────────────────────────────────────────────────
    private void DibujarPestanaGuia()
    {
        scrollGuia = EditorGUILayout.BeginScrollView(scrollGuia);

        SeccionTitulo("1. Descargar MDT05 del CNIG (gratuito)");
        Caja(
            "URL: https://centrodedescargas.cnig.es/CentroDescargas/\n" +
            "→ Modelos Digitales del Terreno → MDT05 (resolución 5 m/px)\n" +
            "→ Hoja MTN25 que cubre Alsasua: 114-3  (en algunos portales: 0114-3)\n" +
            "→ Descarga un .zip con un .tif GeoTiff en ETRS89 UTM Zone 30N\n\n" +
            "Alternativa mayor resolución: MDT02 (2 m/px) o MDT01 (1 m/px)\n" +
            "— útil para detalle de calles y edificios.");

        SeccionTitulo("2. Descargar PNOA Ortofoto (opcional)");
        Caja(
            "URL: https://centrodedescargas.cnig.es/CentroDescargas/\n" +
            "→ Fotogrametría y Lidar → PNOA Máxima Actualidad\n" +
            "→ Hoja 114-3 → archivo:  PNOA_MA_OF_ETRS89_HU30_h50_0114.tif\n" +
            "Resolución: 25–50 cm/px. Cubre exactamente la misma área que el MDT.");

        SeccionTitulo("3. Obtener el rango de altitudes (necesario para GDAL)");
        Caja(
            "En QGIS:\n" +
            "a) Abre el .tif del MDT05.\n" +
            "b) Raster → Herramientas de análisis → Estadísticas de ráster.\n" +
            "c) Anota los valores MÍNIMO y MÁXIMO.\n" +
            "   Ejemplo para Alsasua: mín ≈ 518 m, máx ≈ 1040 m.\n" +
            "d) Introduce esos valores en los campos 'Altitud mínima/máxima' de\n" +
            "   la pestaña Importar. Añade 10-20 m de margen para evitar clipping.");

        SeccionTitulo("4. Convertir a RAW 16-bit para Unity");
        Caja(
            "Usa los comandos GDAL de la pestaña siguiente. En resumen:\n\n" +
            "  gdal_translate  → recorta al bbox de Alsasua\n" +
            "  gdal_translate  → escala alturas a 0-65535 y redimensiona a 1025×1025\n\n" +
            "El resultado es altsasu_terrain.raw (RAW 16-bit little-endian).\n" +
            "Cópialo a cualquier carpeta de tu sistema de ficheros — no hace falta\n" +
            "que esté dentro del proyecto Unity.");

        SeccionTitulo("5. Importar con esta herramienta");
        Caja(
            "a) Heightmap → selecciona altsasu_terrain.raw\n" +
            "b) Ortofoto   → selecciona altsasu_ortofoto.jpg (opcional)\n" +
            "c) Altitud mínima / máxima → los valores del paso 3\n" +
            "d) Ancho / Largo → tamaño del bbox en metros\n" +
            "   bbox 4.6 km E-O × 5.1 km N-S → ancho=4600, largo=5100\n" +
            "e) Pulsar 'Crear Unity Terrain'\n\n" +
            "El script crea el asset en Assets/Terrains/Altsasu/ y posiciona el\n" +
            "terrain con CesiumGlobeAnchor en las coordenadas del corner SW.");

        SeccionTitulo("6. Instalación de GDAL");
        Caja(
            "Windows : osgeo4w.net → instalar OSGeo4W → marcar gdal-bin\n" +
            "          Lanzar desde 'OSGeo4W Shell'\n" +
            "macOS   : brew install gdal\n" +
            "Linux   : sudo apt install gdal-bin\n" +
            "QGIS    : incluye GDAL — disponible desde Procesado → Consola Python");

        EditorGUILayout.EndScrollView();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  PESTAÑA: COMANDOS GDAL
    // ─────────────────────────────────────────────────────────────────────────
    private void DibujarPestanaGDAL()
    {
        EditorGUILayout.HelpBox(
            "Los comandos usan los valores actuales de la pestaña Importar. " +
            "Cámbialo allí y recarga esta pestaña para actualizarlos.",
            MessageType.Info);
        EditorGUILayout.Space(4);

        // ── Paso 1: Recortar ──────────────────────────────────────────────────
        // El bbox NE se calcula sumando ancho y largo desde la SW
        double latNE = geoLat + largoTerrain / 111320.0;
        double lonNE = geoLon + anchoTerrain / (111320.0 * Math.Cos(geoLat * Math.PI / 180.0));

        string cmd1 =
            $"gdal_translate \\\n" +
            $"  -projwin {geoLon:F5} {latNE:F5} {lonNE:F5} {geoLat:F5} \\\n" +
            $"  -projwin_srs EPSG:4326 \\\n" +
            $"  MDT05_ETRS89_HU30_0114-3.tif \\\n" +
            $"  altsasu_mdt_clipped.tif";

        SeccionTitulo("Paso 1 — Recortar el MDT a la región de Alsasua");
        EditorGUILayout.LabelField(
            $"Bbox: SW ({geoLat:F4}°N, {geoLon:F4}°E) → NE ({latNE:F4}°N, {lonNE:F4}°E)",
            EditorStyles.miniLabel);
        EditorGUILayout.TextArea(cmd1, GUILayout.Height(90));
        if (GUILayout.Button("Copiar", GUILayout.Width(70)))
            EditorGUIUtility.systemCopyBuffer = cmd1;

        EditorGUILayout.Space(8);

        // ── Paso 2: Convertir a RAW 16-bit ───────────────────────────────────
        string cmd2 =
            $"gdal_translate \\\n" +
            $"  -ot UInt16 \\\n" +
            $"  -scale {alturaMin:F0} {alturaMax:F0} 0 65535 \\\n" +
            $"  -outsize {heightmapResolucion} {heightmapResolucion} \\\n" +
            $"  -of ENVI \\\n" +
            $"  altsasu_mdt_clipped.tif \\\n" +
            $"  altsasu_terrain.raw\n\n" +
            $"# El archivo .hdr generado por ENVI se puede borrar; solo se usa el .raw";

        SeccionTitulo("Paso 2 — Convertir a RAW 16-bit para Unity");
        EditorGUILayout.HelpBox(
            $"El -scale mapea el rango [{alturaMin:F0} m, {alturaMax:F0} m] a [0, 65535].\n" +
            $"Si cambias los valores de altitud en la pestaña Importar, regenera este comando.",
            MessageType.Info);
        EditorGUILayout.TextArea(cmd2, GUILayout.Height(110));
        if (GUILayout.Button("Copiar", GUILayout.Width(70)))
            EditorGUIUtility.systemCopyBuffer = cmd2;

        EditorGUILayout.Space(8);

        // ── Paso 3: Ortofoto ──────────────────────────────────────────────────
        string cmd3 =
            $"# Recortar PNOA al mismo bbox\n" +
            $"gdal_translate \\\n" +
            $"  -projwin {geoLon:F5} {latNE:F5} {lonNE:F5} {geoLat:F5} \\\n" +
            $"  -projwin_srs EPSG:4326 \\\n" +
            $"  PNOA_MA_OF_ETRS89_HU30_h50_0114.tif \\\n" +
            $"  altsasu_ortofoto_clipped.tif\n\n" +
            $"# Convertir a JPEG para ahorrar tamaño (Unity admite .tif pero es pesado)\n" +
            $"gdal_translate \\\n" +
            $"  -of JPEG -co QUALITY=90 \\\n" +
            $"  altsasu_ortofoto_clipped.tif \\\n" +
            $"  altsasu_ortofoto.jpg";

        SeccionTitulo("Paso 3 — Recortar y convertir la ortofoto PNOA (opcional)");
        EditorGUILayout.TextArea(cmd3, GUILayout.Height(130));
        if (GUILayout.Button("Copiar", GUILayout.Width(70)))
            EditorGUIUtility.systemCopyBuffer = cmd3;

        EditorGUILayout.Space(8);

        // ── Nota: QGIS Python ─────────────────────────────────────────────────
        EditorGUILayout.HelpBox(
            "Si prefieres QGIS: Procesado → Consola Python → escribe:\n" +
            "  import subprocess\n" +
            "  subprocess.run(['gdal_translate', ...])\n" +
            "O usa los menús: Raster → Extracción → Recortar ráster por extensión\n" +
            "y luego Raster → Conversión → Traducir (convertir formato).",
            MessageType.None);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  CREACIÓN DEL TERRAIN
    // ─────────────────────────────────────────────────────────────────────────
    private void CrearTerrain()
    {
        if (!ValidarEntradas()) return;

        int undoGroup = Undo.GetCurrentGroup();
        Undo.SetCurrentGroupName("Crear Terrain IGN Altsasu");

        try
        {
            EditorUtility.DisplayProgressBar("Creando Terrain…", "Leyendo heightmap…", 0.1f);

            // ── 1. Leer heightmap RAW ─────────────────────────────────────────
            float[,] alturas = LeerHeightmapRAW(rutaHeightmap, heightmapResolucion, heightmapLittleEndian);
            if (alturas == null) { EditorUtility.ClearProgressBar(); return; }

            // ── 2. Crear TerrainData ──────────────────────────────────────────
            EditorUtility.DisplayProgressBar("Creando Terrain…", "Creando TerrainData…", 0.3f);

            var data = new TerrainData();
            data.heightmapResolution = heightmapResolucion;
            // El tamaño Y de TerrainData es el RANGO de elevación en metros.
            // Unity mapea height=0.0 → alturaMin, height=1.0 → alturaMax.
            data.size = new Vector3(anchoTerrain, AlturaRango, largoTerrain);
            data.SetHeights(0, 0, alturas);

            // ── 3. Guardar assets ─────────────────────────────────────────────
            EditorUtility.DisplayProgressBar("Creando Terrain…", "Guardando assets…", 0.5f);

            const string CARPETA_RAIZ   = "Assets/Terrains";
            const string CARPETA_ALSASUA = "Assets/Terrains/Altsasu";
            if (!AssetDatabase.IsValidFolder(CARPETA_RAIZ))
                AssetDatabase.CreateFolder("Assets", "Terrains");
            if (!AssetDatabase.IsValidFolder(CARPETA_ALSASUA))
                AssetDatabase.CreateFolder(CARPETA_RAIZ, "Altsasu");

            string rutaData = $"{CARPETA_ALSASUA}/{nombreTerrain}_Data.asset";
            AssetDatabase.CreateAsset(data, rutaData);

            // ── 4. TerrainLayer con ortofoto ──────────────────────────────────
            if (crearCapaOrtofoto && !string.IsNullOrEmpty(rutaOrtofoto) && File.Exists(rutaOrtofoto))
            {
                EditorUtility.DisplayProgressBar("Creando Terrain…", "Importando ortofoto…", 0.65f);

                string ext      = Path.GetExtension(rutaOrtofoto);
                string destName = $"Ortofoto_Altsasu{ext}";
                string destPath = $"{CARPETA_ALSASUA}/{destName}";
                string destAbs  = Path.Combine(Application.dataPath, destPath.Substring("Assets/".Length));

                File.Copy(rutaOrtofoto, destAbs, overwrite: true);
                AssetDatabase.ImportAsset(destPath);

                // Forzar tipo "Default" (textura color, no normal map)
                if (AssetImporter.GetAtPath(destPath) is TextureImporter ti)
                {
                    ti.textureType     = TextureImporterType.Default;
                    ti.wrapMode        = TextureWrapMode.Clamp;
                    ti.maxTextureSize  = 8192;
                    ti.textureCompression = TextureImporterCompression.Compressed;
                    ti.SaveAndReimport();
                }

                var tex = AssetDatabase.LoadAssetAtPath<Texture2D>(destPath);
                if (tex != null)
                {
                    var layer = new TerrainLayer();
                    layer.diffuseTexture = tex;
                    // tileSize igual al terrain → la ortofoto cubre el área completa una vez
                    layer.tileSize       = new Vector2(anchoTerrain, largoTerrain);
                    layer.tileOffset     = Vector2.zero;

                    string rutaLayer = $"{CARPETA_ALSASUA}/Layer_Ortofoto.terrainlayer";
                    AssetDatabase.CreateAsset(layer, rutaLayer);
                    data.terrainLayers = new[] { layer };
                }
            }

            // ── 5. Crear GameObject Terrain ───────────────────────────────────
            EditorUtility.DisplayProgressBar("Creando Terrain…", "Creando GameObject…", 0.80f);

            var terrainGO = Terrain.CreateTerrainGameObject(data);
            terrainGO.name = nombreTerrain;
            Undo.RegisterCreatedObjectUndo(terrainGO, "Crear Terrain");

            var terrain  = terrainGO.GetComponent<Terrain>();
            terrain.castShadows   = true;
            terrain.drawInstanced = true;

            // TerrainCollider explícito
            var col = terrainGO.GetComponent<TerrainCollider>();
            if (col == null) col = terrainGO.AddComponent<TerrainCollider>();
            col.terrainData = data;

            // ── 6. Posicionamiento geográfico ─────────────────────────────────
            EditorUtility.DisplayProgressBar("Creando Terrain…", "Posicionando…", 0.90f);

            if (posicionarConCesium)
            {
                // Parentar bajo CesiumGeoreference si existe
                var geo = FindObjectOfType<CesiumGeoreference>();
                if (geo != null)
                    terrainGO.transform.SetParent(geo.transform, worldPositionStays: false);

                // CesiumGlobeAnchor en la esquina SW
                // La altura que pasa a Cesium debe ser WGS84 elipsoidal
                double altSW_WGS84 = alturaMin + geoidUndulation;

                var anchor = terrainGO.AddComponent<CesiumGlobeAnchor>();
                // longitudeLatitudeHeight → double3(lon, lat, height_elipsoidal)
                anchor.longitudeLatitudeHeight = new double3(geoLon, geoLat, altSW_WGS84);

                Debug.Log($"[ImportadorTerrainIGN] CesiumGlobeAnchor → " +
                          $"lon={geoLon:F4}°, lat={geoLat:F4}°, " +
                          $"h={altSW_WGS84:F1} m (WGS84 elipsoidal)");
            }
            else
            {
                // Sin Cesium: colocar en Unity world space; Y=0 coincide con alturaMin
                terrainGO.transform.position = Vector3.zero;
            }

            // ── 7. GestorCapasTerrain ─────────────────────────────────────────
            if (anadirGestorCapas)
                terrainGO.AddComponent<GestorCapasTerrain>();

            // ── 8. Finalizar ──────────────────────────────────────────────────
            Undo.CollapseUndoOperations(undoGroup);
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            EditorUtility.ClearProgressBar();

            Selection.activeGameObject = terrainGO;
            SceneView.FrameLastActiveSceneView();

            Debug.Log(
                $"[ImportadorTerrainIGN] ✓ '{nombreTerrain}' creado.\n" +
                $"  Tamaño: {anchoTerrain}×{largoTerrain} m | " +
                $"Elevación: {alturaMin}–{alturaMax} m | " +
                $"Resolución: {heightmapResolucion}×{heightmapResolucion}\n" +
                $"  Asset: {rutaData}");

            EditorUtility.DisplayDialog(
                "✓ Terrain creado",
                $"'{nombreTerrain}' generado correctamente.\n\n" +
                $"Tamaño : {anchoTerrain} × {largoTerrain} m\n" +
                $"Elevación: {alturaMin} – {alturaMax} m MSL\n" +
                $"Resolución: {heightmapResolucion}×{heightmapResolucion} px\n" +
                $"Asset: {rutaData}\n\n" +
                "Usa el componente GestorCapasTerrain para alternar entre este terrain\n" +
                "y los tiles Cesium en tiempo de ejecución.",
                "OK");
        }
        catch (Exception e)
        {
            EditorUtility.ClearProgressBar();
            EditorUtility.DisplayDialog("Error al crear Terrain", e.Message, "OK");
            Debug.LogError("[ImportadorTerrainIGN] " + e);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  LECTURA DE HEIGHTMAP RAW 16-BIT
    // ─────────────────────────────────────────────────────────────────────────
    /// <summary>
    /// Lee un archivo RAW de 16 bits (unsigned) generado por GDAL con formato ENVI.
    /// Unity espera valores normalizados [0, 1] donde 0 = alturaMin y 1 = alturaMax.
    /// GDAL escribe en fila mayor (row-major, top→bottom), Unity indexa [z, x].
    /// </summary>
    private static float[,] LeerHeightmapRAW(string ruta, int res, bool littleEndian)
    {
        byte[] bytes;
        try   { bytes = File.ReadAllBytes(ruta); }
        catch (Exception e)
        {
            EditorUtility.DisplayDialog("Error al leer heightmap", e.Message, "OK");
            return null;
        }

        long bytesNecesarios = (long)res * res * 2;
        if (bytes.Length < bytesNecesarios)
        {
            EditorUtility.DisplayDialog(
                "Heightmap demasiado pequeño",
                $"El archivo tiene {bytes.Length:N0} bytes, pero {res}×{res} 16-bit " +
                $"requiere {bytesNecesarios:N0}.\n\n" +
                "Comprueba que -outsize en el comando GDAL coincide con la resolución seleccionada.",
                "OK");
            return null;
        }

        var alturas = new float[res, res];
        int i = 0;
        // GDAL escribe top→bottom (fila 0 = norte). Unity espera [z, x] con z=0 en sur.
        // Invertir el eje Z para que el norte quede al norte en Unity (Z+).
        for (int fila = res - 1; fila >= 0; fila--)
        {
            for (int col = 0; col < res; col++)
            {
                ushort raw = littleEndian
                    ? (ushort)(bytes[i] | (bytes[i + 1] << 8))
                    : (ushort)((bytes[i] << 8) | bytes[i + 1]);
                alturas[fila, col] = raw / 65535f;
                i += 2;
            }
        }
        return alturas;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  VALIDACIÓN
    // ─────────────────────────────────────────────────────────────────────────
    private bool ValidarEntradas()
    {
        if (string.IsNullOrEmpty(rutaHeightmap) || !File.Exists(rutaHeightmap))
        {
            EditorUtility.DisplayDialog("Falta heightmap",
                "Selecciona un archivo .raw válido en la pestaña Importar.", "OK");
            return false;
        }
        if (alturaMin >= alturaMax)
        {
            EditorUtility.DisplayDialog("Rango de alturas inválido",
                "La altitud mínima debe ser estrictamente menor que la máxima.", "OK");
            return false;
        }
        if (anchoTerrain <= 0 || largoTerrain <= 0)
        {
            EditorUtility.DisplayDialog("Dimensiones inválidas",
                "El ancho y largo del terreno deben ser mayores que 0.", "OK");
            return false;
        }
        if (string.IsNullOrWhiteSpace(nombreTerrain))
        {
            EditorUtility.DisplayDialog("Nombre inválido",
                "Introduce un nombre para el asset del terrain.", "OK");
            return false;
        }
        return true;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  HELPERS DE GUI
    // ─────────────────────────────────────────────────────────────────────────
    private static void SeccionTitulo(string texto)
    {
        EditorGUILayout.Space(4);
        EditorGUILayout.LabelField(texto, EditorStyles.boldLabel);
    }

    private static void Caja(string texto)
    {
        EditorGUILayout.LabelField(texto, EditorStyles.helpBox);
        EditorGUILayout.Space(2);
    }
}
