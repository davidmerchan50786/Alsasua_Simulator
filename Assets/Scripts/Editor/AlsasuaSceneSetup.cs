// Assets/Scripts/Editor/AlsasuaSceneSetup.cs
// Menú Unity:  Alsasua ▶  🗺 Configurar GestorEscena
//              Alsasua ▶  📍 Crear Puntos de Spawn
//              Alsasua ▶  🌲 Configurar Vegetación (GeoData)
//              Alsasua ▶  📋 Informe Geográfico
//
// Wizard de configuración de la escena de Alsasua que conecta todos los sistemas
// con los datos geográficos reales de GeoDataAlsasua.
// Complementa SetupEscenaAlsasua.cs (infraestructura Cesium) añadiendo:
//   · GestorEscena con todos los sistemas de simulación
//   · Puntos de spawn visibles en la escena
//   · Validación de coordenadas GPS vs. Cesium
//   · Informe de coherencia geográfica

using UnityEngine;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine.SceneManagement;

public static class AlsasuaSceneSetup
{
    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: CONFIGURAR GESTOR DE ESCENA
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Alsasua/🗺 Configurar GestorEscena", priority = 5)]
    public static void ConfigurarGestorEscena()
    {
        Debug.Log("── [AlsasuaSceneSetup] Configurando GestorEscena ──────────────");

        var gestor = AsegurarGestorEscena();

        if (!Application.isPlaying)
        {
            EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
        }

        Debug.Log($"── [AlsasuaSceneSetup] ✓ GestorEscena listo en '{gestor.gameObject.name}'");
        Debug.Log("──  Sistemas activos: Multitud, Personajes, Tráfico, Tren, Vegetación,");
        Debug.Log("──  Barricadas, Farolas, Edificios — coordenadas reales de Alsasua.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: CREAR PUNTOS DE SPAWN
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Alsasua/📍 Crear Puntos de Spawn", priority = 6)]
    public static void CrearPuntosSpawn()
    {
        Debug.Log("── [AlsasuaSceneSetup] Creando puntos de spawn ────────────────");

        var padre = GameObject.Find("SpawnPoints");
        if (padre == null)
        {
            padre = new GameObject("SpawnPoints");
            Debug.Log("  ✓ Creado contenedor 'SpawnPoints'");
        }

        CrearSpawn(padre.transform, "Spawn_Plaza_Principal",
            GeoDataAlsasua.SpawnPrincipal,
            "Herriko Plaza / Plaza de los Fueros — spawn principal del jugador.");

        CrearSpawn(padre.transform, "Spawn_Estacion_Tren",
            GeoDataAlsasua.SpawnEstacion,
            "Estación de tren Alsasua (Adif/Renfe) — acceso sur.");

        CrearSpawn(padre.transform, "Spawn_Poligono_Industrial",
            GeoDataAlsasua.SpawnIndustrial,
            "Polígono Industrial Ondarria — zona este.");

        CrearSpawn(padre.transform, "Spawn_Mirador_Norte",
            GeoDataAlsasua.SpawnMirador,
            "Ladera norte — vistas al Valle de Burunda.");

        if (!Application.isPlaying)
            EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());

        Debug.Log("── [AlsasuaSceneSetup] ✓ 4 puntos de spawn creados/actualizados.");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: CONFIGURAR VEGETACIÓN DESDE GEODATA
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Alsasua/🌲 Configurar Vegetación (GeoData)", priority = 7)]
    public static void ConfigurarVegetacion()
    {
        Debug.Log("── [AlsasuaSceneSetup] Configurando vegetación desde GeoDataAlsasua ─");

        var veg = Object.FindFirstObjectByType<SistemaVegetacion>();
        if (veg == null)
        {
            Debug.LogWarning("  ✗ SistemaVegetacion no encontrado en escena.");
            Debug.LogWarning("    Ejecuta primero 'Alsasua > ⚙ Configurar Escena Completa'.");
            return;
        }

        // Mostrar info de las zonas que GeoDataAlsasua usa
        Debug.Log($"  ✓ SistemaVegetacion encontrado en '{veg.gameObject.name}'");
        Debug.Log($"  Zonas de bosque reales (GeoDataAlsasua.ZonasBosque):");
        for (int i = 0; i < GeoDataAlsasua.ZonasBosque.Length; i++)
        {
            var z = GeoDataAlsasua.ZonasBosque[i];
            Debug.Log($"    [{i}] {z.Nombre,-25} centro={z.Centro}  r={z.Radio:F0}m  pinos={z.FraccionPinos:P0}");
        }
        Debug.Log("  Los valores por defecto del Inspector ya usan estas zonas.");
        Debug.Log("  Para regenerar árboles: dale Play y los GenerarArboles() usará GeoData.");

        if (!Application.isPlaying)
            EditorSceneManager.MarkSceneDirty(SceneManager.GetActiveScene());
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: VALIDAR HEIGHTMAP RAW
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Alsasua/⛰ Validar Heightmap RAW", priority = 10)]
    public static void ValidarHeightmapRAW()
    {
        Debug.Log("── [AlsasuaSceneSetup] Validando heightmap RAW ────────────────");

        // Rutas esperadas (relativas a Application.dataPath = Assets/)
        string[] candidatos = new[]
        {
            System.IO.Path.Combine(Application.dataPath, "Terrain", "alsasua_heightmap.raw"),
            System.IO.Path.Combine(Application.dataPath, "..", "Assets", "Terrain", "alsasua_heightmap.raw"),
        };

        string rutaEncontrada = null;
        foreach (var c in candidatos)
        {
            if (System.IO.File.Exists(c)) { rutaEncontrada = c; break; }
        }

        if (rutaEncontrada == null)
        {
            Debug.LogWarning("  ✗ Heightmap RAW no encontrado.");
            Debug.LogWarning("    Ejecuta: python DescargadorIDENA.py --analitico --res 513 --radio 2048");
            Debug.LogWarning("    (O con internet: python DescargadorIDENA.py --res 513 --radio 2048)");
            Debug.LogWarning("    El archivo debe quedar en: Assets/Terrain/alsasua_heightmap.raw");
            EditorUtility.DisplayDialog("Heightmap RAW",
                "Archivo Assets/Terrain/alsasua_heightmap.raw no encontrado.\n\n" +
                "Para generarlo:\n" +
                "  python DescargadorIDENA.py --analitico --res 513 --radio 2048\n\n" +
                "O con datos reales del LiDAR IDENA:\n" +
                "  python DescargadorIDENA.py --res 513 --radio 2048", "OK");
            return;
        }

        // Analizar el archivo
        byte[] bytes = System.IO.File.ReadAllBytes(rutaEncontrada);
        long   byteCount = bytes.Length;
        int    rawRes    = Mathf.RoundToInt(Mathf.Sqrt(byteCount / 2f));
        bool   cuadrado  = (rawRes * rawRes * 2 == byteCount);

        Debug.Log($"  ✓ Heightmap RAW encontrado en:\n    {rutaEncontrada}");
        Debug.Log($"  Tamaño: {byteCount / 1024} KB ({byteCount} bytes)");
        Debug.Log($"  Resolución detectada: {rawRes}×{rawRes} muestras de 16 bits " +
                  $"({(cuadrado ? "✓ correcto" : "⚠ no cuadrado")})");

        // Leer estadísticas de altura
        ushort minVal = 65535, maxVal = 0;
        long   suma   = 0;
        int    n      = byteCount / 2;
        for (int i = 0; i < n; i++)
        {
            ushort v = (ushort)(bytes[i * 2] | (bytes[i * 2 + 1] << 8));
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
            suma += v;
        }
        float promedio = (float)suma / n;
        // Para alturaMaxima=600m, el heightmap se generó con altMax=alturaMaxima.
        // DescargadorIDENA usa 600m como altMax por defecto.
        const float ALT_MAX_DEFECTO = 600f;
        float minAlt = minVal / 65535f * ALT_MAX_DEFECTO;
        float maxAlt = maxVal / 65535f * ALT_MAX_DEFECTO;
        float medAlt = promedio / 65535f * ALT_MAX_DEFECTO;

        Debug.Log($"  Alturas (base altMax={ALT_MAX_DEFECTO}m):");
        Debug.Log($"    Min: {minAlt:F1} m  |  Max: {maxAlt:F1} m  |  Media: {medAlt:F1} m");

        // Verificar coherencia con SistemaTerrenoAlsasua en escena
        var terreno = Object.FindFirstObjectByType<SistemaTerrenoAlsasua>();
        if (terreno != null)
            Debug.Log($"  SistemaTerrenoAlsasua en escena — 'Cargar Desde RAW' = {terreno.HeightmapDesdeRAW}.");
        else
            Debug.Log("  SistemaTerrenoAlsasua no instanciado (normal si no está en Play).");

        string mensaje =
            $"✓ Heightmap RAW válido\n\n" +
            $"Resolución: {rawRes}×{rawRes}\n" +
            $"Tamaño: {byteCount / 1024} KB\n\n" +
            $"Alturas (altMax={ALT_MAX_DEFECTO}m):\n" +
            $"  Min: {minAlt:F1} m\n" +
            $"  Max: {maxAlt:F1} m\n" +
            $"  Media: {medAlt:F1} m\n\n" +
            "SistemaTerrenoAlsasua lo cargará automáticamente\n" +
            "si 'Cargar Desde Raw' está activado en el Inspector.";
        EditorUtility.DisplayDialog("Heightmap RAW — Alsasua", mensaje, "OK");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  MENÚ: INFORME GEOGRÁFICO
    // ═══════════════════════════════════════════════════════════════════════

    [MenuItem("Alsasua/📋 Informe Geográfico", priority = 20)]
    public static void InformeGeografico()
    {
        Debug.Log("══════════════════════════════════════════════════════════════");
        Debug.Log("[GeoDataAlsasua] INFORME GEOGRÁFICO — Alsasua / Altsasu");
        Debug.Log("══════════════════════════════════════════════════════════════");

        Debug.Log($"  Centro: lat={GeoDataAlsasua.LAT_CENTRO:F4}° N, " +
                  $"lon={GeoDataAlsasua.LON_CENTRO:F4}° W, " +
                  $"alt={GeoDataAlsasua.ALT_CENTRO:F0} m s.n.m.");
        Debug.Log($"  Escala: 1 grado lat ≈ {GeoDataAlsasua.M_POR_GRADO_LAT:N0} m, " +
                  $"1 grado lon ≈ {GeoDataAlsasua.M_POR_GRADO_LON:N0} m");

        Debug.Log("\n  ── Puntos de referencia urbanos ──");
        Debug.Log($"    Herriko Plaza         : {GeoDataAlsasua.HerrikoPlaza}");
        Debug.Log($"    Ayuntamiento          : {GeoDataAlsasua.Ayuntamiento}");
        Debug.Log($"    Cuartel Guardia Civil : {GeoDataAlsasua.CuartelGuardiaCivil}");
        Debug.Log($"    Comisaría Pol. Foral  : {GeoDataAlsasua.ComisariaPolForal}");
        Debug.Log($"    Estación de Tren      : {GeoDataAlsasua.EstacionTren}");
        Debug.Log($"    Polígono Isasia       : {GeoDataAlsasua.PoligonoIsasia}");
        Debug.Log($"    Polígono Ondarria     : {GeoDataAlsasua.PoligonoOndarria}");

        Debug.Log("\n  ── Montes y cimas ──");
        Debug.Log($"    Monte Artia           : {GeoDataAlsasua.MonteArtia}");
        Debug.Log($"    Peña Blanca (Askiz)   : {GeoDataAlsasua.PenaBlanca}");
        Debug.Log($"    Collado Burunda       : {GeoDataAlsasua.ColladoBurunda}");

        Debug.Log("\n  ── Zonas de bosque ──");
        foreach (var z in GeoDataAlsasua.ZonasBosque)
            Debug.Log($"    {z.Nombre,-25} centro={z.Centro}  r={z.Radio:F0}m  pinos={z.FraccionPinos:P0}");

        Debug.Log("\n  ── Rutas de tráfico ──");
        Debug.Log($"    N1_Norte       : {GeoDataAlsasua.N1_Norte.Length} waypoints  (90 km/h autovía)");
        Debug.Log($"    N1_Sur         : {GeoDataAlsasua.N1_Sur.Length} waypoints   (90 km/h autovía)");
        Debug.Log($"    NA120_Este     : {GeoDataAlsasua.NA120_Este.Length} waypoints   (60 km/h)");
        Debug.Log($"    NA120_Oeste    : {GeoDataAlsasua.NA120_Oeste.Length} waypoints   (60 km/h)");
        Debug.Log($"    Casco urbano   : {GeoDataAlsasua.CalleInteriorCasco.Length} waypoints   (30 km/h)");

        Debug.Log("\n  ── Rutas de patrulla ──");
        Debug.Log($"    Guardia Civil  : {GeoDataAlsasua.PatrullaGuardiaCivil.Length} waypoints " +
                  $"(cuartel Calle Ameztia → Herriko Plaza → vuelta)");
        Debug.Log($"    Policía Foral  : {GeoDataAlsasua.PatrullaPolForal.Length} waypoints " +
                  $"(comisaría norte → Herriko Plaza → polígono Ondarria)");

        Debug.Log("\n  ── Vía férrea ──");
        Debug.Log($"    Madrid-Irún    : {GeoDataAlsasua.ViaFerreaNorte.Length} waypoints " +
                  "(pasando por Estación Alsasua)");

        Debug.Log("\n  ── Puntos de spawn ──");
        Debug.Log($"    Principal      : {GeoDataAlsasua.SpawnPrincipal} (Herriko Plaza)");
        Debug.Log($"    Estación       : {GeoDataAlsasua.SpawnEstacion}");
        Debug.Log($"    Industrial     : {GeoDataAlsasua.SpawnIndustrial}");
        Debug.Log($"    Mirador        : {GeoDataAlsasua.SpawnMirador}");

        // Verificar coherencia con Cesium
        Debug.Log("\n  ── Verificación Cesium ──");
#if UNITY_EDITOR
        var geo = Object.FindFirstObjectByType<CesiumForUnity.CesiumGeoreference>();
        if (geo != null)
        {
            double dLat = System.Math.Abs(geo.latitude  - GeoDataAlsasua.LAT_CENTRO);
            double dLon = System.Math.Abs(geo.longitude - GeoDataAlsasua.LON_CENTRO);
            double errMetros = System.Math.Sqrt(
                dLat * dLat * GeoDataAlsasua.M_POR_GRADO_LAT * GeoDataAlsasua.M_POR_GRADO_LAT +
                dLon * dLon * GeoDataAlsasua.M_POR_GRADO_LON * GeoDataAlsasua.M_POR_GRADO_LON);

            if (errMetros < 50.0)
                Debug.Log($"    ✓ CesiumGeoreference alineado (error={errMetros:F0}m)");
            else
                Debug.LogWarning(
                    $"    ⚠ CesiumGeoreference desalineado: error={errMetros:F0}m — " +
                    $"Cesium apunta a lat={geo.latitude:F4} lon={geo.longitude:F4} " +
                    $"pero GeoData espera lat={GeoDataAlsasua.LAT_CENTRO:F4} " +
                    $"lon={GeoDataAlsasua.LON_CENTRO:F4}. " +
                    "Ejecuta 'Alsasua > ⚙ Configurar Escena Completa' para corregirlo.");
        }
        else
        {
            Debug.LogWarning("    ✗ CesiumGeoreference no encontrado — ejecuta '⚙ Configurar Escena Completa'.");
        }
#endif

        Debug.Log("══════════════════════════════════════════════════════════════");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HELPERS INTERNOS
    // ═══════════════════════════════════════════════════════════════════════

    private static GestorEscena AsegurarGestorEscena()
    {
        var gestor = Object.FindFirstObjectByType<GestorEscena>();
        if (gestor != null)
        {
            Debug.Log($"  ✓ GestorEscena ya existe: '{gestor.gameObject.name}'");
            return gestor;
        }

        // Crear un GO con GestorEscena
        var go = new GameObject("GestorEscena");
        gestor = go.AddComponent<GestorEscena>();
        Debug.Log("  ✓ Creado 'GestorEscena' con todos los sistemas de simulación.");
        Debug.Log("    En el Inspector, activa/desactiva: Multitud, Personajes,");
        Debug.Log("    Tráfico, Tren, Vegetación, Barricadas, Farolas, Edificios.");
        return gestor;
    }

    private static void CrearSpawn(Transform padre, string nombre, Vector3 posicion, string descripcion)
    {
        var existente = padre.Find(nombre);
        if (existente != null)
        {
            // Actualizar posición si ya existe
            existente.position = posicion;
            Debug.Log($"  ↻ Actualizado '{nombre}' → {posicion}");
            return;
        }

        var go = new GameObject(nombre);
        go.transform.SetParent(padre);
        go.transform.position = posicion;

        // Añadir un marcador visual (invisible en build, visible en editor)
#if UNITY_EDITOR
        // El gizmo solo se ve en editor; no hace falta ningún componente especial.
        // El nombre del GO con prefijo "Spawn_" es suficiente para ControladorJugador.
#endif

        Debug.Log($"  ✓ Creado '{nombre}' en {posicion}");
        Debug.Log($"    ({descripcion})");
    }
}
