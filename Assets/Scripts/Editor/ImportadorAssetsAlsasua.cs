// Assets/Scripts/Editor/ImportadorAssetsAlsasua.cs
// ═══════════════════════════════════════════════════════════════════════════
//  Importador automático de assets desde E:\assets al proyecto Alsasua.
//
//  USO:
//    Menú Unity → Alsasua → 📦 Importar Assets Seleccionados
//      → Importa solo los paquetes marcados como "esenciales" para Alsasua.
//
//    Menú Unity → Alsasua → 📦 Importar TODOS los Assets Útiles
//      → Importa todo el catálogo relevante (más lento).
//
//    Menú Unity → Alsasua → 📦 Verificar Assets Importados
//      → Muestra en consola qué paquetes ya están y cuáles faltan.
//
//  RUTA BASE: configura RUTA_ASSETS con la ruta real de tu carpeta E:\assets.
//  Por defecto apunta a E:\assets (Windows). Cambia la barra si usas Mac/Linux.
//
//  Los paquetes se importan de forma interactiva (interactive: true) para que
//  puedas revisar qué se importa. Usa interactive: false solo si ya conoces bien
//  el contenido de cada paquete.
// ═══════════════════════════════════════════════════════════════════════════

#if UNITY_EDITOR

using UnityEngine;
using UnityEditor;
using System.IO;
using System.Collections.Generic;

public static class ImportadorAssetsAlsasua
{
    // ───────────────────────────────────────────────────────────────────────
    //  CONFIGURACIÓN — ajusta estas rutas si tus carpetas assets están en otro lugar
    // ───────────────────────────────────────────────────────────────────────
    private const string PREF_RUTA_ASSETS = "Alsasua_RutaAssets";
    private const string PREF_RUTA_DOWNLOADS = "Alsasua_RutaDownloads";

    private static string RutaBase =>
        EditorPrefs.GetString(PREF_RUTA_ASSETS, @"E:\assets");

    private static string RutaDownloads =>
        EditorPrefs.GetString(PREF_RUTA_DOWNLOADS, @"C:\Users\coperenea\Downloads");

    // ───────────────────────────────────────────────────────────────────────
    //  DEFINICIÓN DE PAQUETES
    // ───────────────────────────────────────────────────────────────────────

    private enum Prioridad { Esencial, Recomendado, Opcional }

    private struct PaqueteInfo
    {
        public string rutaRelativa;   // relativa a RutaBase
        public string nombre;         // nombre amigable
        public string descripcion;    // para qué sirve en Alsasua
        public Prioridad prioridad;
        public string carpetaDestino; // carpeta Assets/ donde quedará tras importar
    }

    private static readonly PaqueteInfo[] PAQUETES = new PaqueteInfo[]
    {
        // ── ESENCIALES: impacto directo en la simulación ─────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Rossendy Brito\3D Models\Concrete Barricades.unitypackage",
            nombre        = "Concrete Barricades",
            descripcion   = "Barricadas de hormigón para SistemaBarricadas (complementa Abandoned World)",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/BarrierPack_Rossendy",
        },
        new PaqueteInfo {
            rutaRelativa  = @"GBAndrewGB\3D ModelsCharacters\VILLAGE HOUSES PACK.unitypackage",
            nombre        = "Village Houses Pack",
            descripcion   = "Casas del pueblo para poblar Alsasua (SistemaEdificios)",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/VillageHouses",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Mehdi Rabiee\3D ModelsEnvironments\House Pack.unitypackage",
            nombre        = "House Pack",
            descripcion   = "Casas adicionales para SistemaEdificios (variedad urbana)",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/HousePack",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Aleksey Kozhemyakin\3D ModelsProps\Modular self-stand fence.unitypackage",
            nombre        = "Modular Fence",
            descripcion   = "Vallas modulares para perímetro policial y zonas acordonadas",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/ModularFence",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Nick Veselov NVJOB\Shaders\Fast Dynamic Sky.unitypackage",
            nombre        = "Fast Dynamic Sky",
            descripcion   = "Cielo dinámico para SistemaAtmosfera (amanecer → noche)",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/FastDynamicSky",
        },

        // ── RECOMENDADOS: mejoran visualmente la escena ──────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Nobiax Yughues\3D ModelsVegetationPlants\Yughues Free Bushes.unitypackage",
            nombre        = "Yughues Free Bushes",
            descripcion   = "Arbustos y matorrales para SistemaVegetacion",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Vegetacion/Arbustos",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Pure Poly\3D ModelsEnvironmentsLandscapes\Free Low Poly Nature Forest.unitypackage",
            nombre        = "Free Low Poly Nature Forest",
            descripcion   = "Árboles low poly para SistemaVegetacion (complementa pino/roble)",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Vegetacion/LowPolyForest",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Laxer\3D ModelsVegetationTrees\Mobile Tree Package.unitypackage",
            nombre        = "Mobile Tree Package",
            descripcion   = "Árboles optimizados para móvil, útiles en SistemaVegetacion",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Vegetacion/MobileTrees",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Olivier Girardot\AudioSound FX\Free Sound Effects Pack.unitypackage",
            nombre        = "Free Sound Effects Pack",
            descripcion   = "Megapack SFX: pasos, impactos, motores → rellena AudioManager",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/SFX/FreeSoundEffects",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Goumain Antoine\AudioSound FX\Nature Sounds Pack - Free.unitypackage",
            nombre        = "Nature Sounds Pack",
            descripcion   = "Sonidos exteriores (viento, pájaros) para ambiente diurno",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/SFX/NatureSounds",
        },
        new PaqueteInfo {
            rutaRelativa  = @"NoxSound\AudioAmbientNature\Nature - Essentials.unitypackage",
            nombre        = "Nature - Essentials (NoxSound)",
            descripcion   = "Ambiente sonoro exterior complementario a Gregor Quendel",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/SFX/NatureEssentials",
        },

        // ── VFX ADICIONALES ──────────────────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Core games studio\Particle SystemsFire\Fire Explosion VFX.unitypackage",
            nombre        = "Fire Explosion VFX",
            descripcion   = "VFX fuego+explosión adicional para barricadas y bombas molotov",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/VFX/FireExplosion",
        },
        new PaqueteInfo {
            rutaRelativa  = @"FXKandolPack\Particle SystemsFire\FX FumeFx.unitypackage",
            nombre        = "FX FumeFx",
            descripcion   = "Humo volumétrico para barricadas (complementa Free Fire VFX)",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/VFX/FumeFX",
        },
        new PaqueteInfo {
            rutaRelativa  = @"SOLODREAM CREATION\Particle SystemsFire\Free Asset VFX Particles Fireball Pack.unitypackage",
            nombre        = "VFX Fireball Pack",
            descripcion   = "Cócteles molotov y focos de fuego sobre barricadas",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/VFX/Fireball",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Nova Sound\AudioSound FX\Free Fireworks - Fire FX - Nova Sound.unitypackage",
            nombre        = "Free Fireworks SFX",
            descripcion   = "SFX chispas y fuego para barricadas ardiendo",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/SFX/FireworksSFX",
        },

        // ── OPCIONALES: mejoras visuales ─────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Galactic Studios\Shaders\Ultra Skybox Fog.unitypackage",
            nombre        = "Ultra Skybox Fog",
            descripcion   = "Niebla volumétrica para mañanas invernales en Alsasua",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Shaders/SkyboxFog",
        },
        new PaqueteInfo {
            rutaRelativa  = @"DigitalKonstrukt\3D Models\Prototyping Pack Free.unitypackage",
            nombre        = "Prototyping Pack Free",
            descripcion   = "Primitivas de desarrollo para pruebas rápidas en escena",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Dev/Prototyping",
        },

        // ── ASSETS DESDE CARPETA DOWNLOADS ─────────────────────────────────
        // NOTA: Estos assets vienen extraídos manualmente de Downloads.
        // Cópialos a Assets/Downloads/{Vegetation,Buildings,Weapons,Animals,etc}
        // para que el importador los encuentre.

        // ── Vegetación Downloads ─────────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"bushes.7z",  // Requiere descomprimir primero
            nombre        = "Bushes - Downloads",
            descripcion   = "Arbustos variados desde Downloads para SistemaVegetacion",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/Bushes",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Free Hedges.7z",
            nombre        = "Free Hedges - Downloads",
            descripcion   = "Setos y cercas vivas para SistemaVegetacion",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/Hedges",
        },
        new PaqueteInfo {
            rutaRelativa  = @"grass_02.7z",
            nombre        = "Grass 02 - Downloads",
            descripcion   = "Pastos variados para cobertura vegetal",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/Grass",
        },
        new PaqueteInfo {
            rutaRelativa  = @"plant-pack.zip",
            nombre        = "Plant Pack - Downloads",
            descripcion   = "Colección completa de plantas para ambiente exterior",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/PlantPack",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Stylish plants.7z",
            nombre        = "Stylish Plants - Downloads",
            descripcion   = "Plantas estilizadas para decoración ambient",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/StylishPlants",
        },
        new PaqueteInfo {
            rutaRelativa  = @"jkm_oaktree.zip",
            nombre        = "Oak Tree - Downloads",
            descripcion   = "Árbol roble individual para areas arboladas",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Vegetation/OakTree",
        },
        new PaqueteInfo {
            rutaRelativa  = @"modular_village_collection.zip",
            nombre        = "Modular Village Collection - Downloads",
            descripcion   = "Colección modular: plantas + casas + decoración aldea",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/Downloads/Vegetation/VillageCollection",
        },
        new PaqueteInfo {
            rutaRelativa  = @"tiny weeds 2.7z",
            nombre        = "Tiny Weeds 2 - Downloads",
            descripcion   = "Maleza detalle para zonas degradadas",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Vegetation/TinyWeeds2",
        },

        // ── Edificios Downloads ─────────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Fantasy Ruins Pack.zip",
            nombre        = "Fantasy Ruins Pack - Downloads",
            descripcion   = "Ruinas y estructuras destruidas para ambiente post-crisis",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/Downloads/Buildings/FantasyRuins",
        },
        new PaqueteInfo {
            rutaRelativa  = @"House.zip",
            nombre        = "House - Downloads",
            descripcion   = "Casa individual para variedad en SistemaEdificios",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Buildings/House",
        },
        new PaqueteInfo {
            rutaRelativa  = @"hq_building.gltf_.zip",
            nombre        = "HQ Building GLTF - Downloads",
            descripcion   = "Edificio de cuartel general o administración",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Buildings/HQBuilding",
        },

        // ── Armas y Equipamiento Downloads ─────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"elven_weapon_set_by_pfunked.zip",
            nombre        = "Elven Weapon Set - Downloads",
            descripcion   = "Set de armas variadas para equipar NPCs/personajes",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/Downloads/Weapons/ElvenWeapons",
        },
        new PaqueteInfo {
            rutaRelativa  = @"smallarms.zip",
            nombre        = "Small Arms - Downloads",
            descripcion   = "Armas ligeras, pistolas, rifles para personajes",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/Downloads/Weapons/SmallArms",
        },
        new PaqueteInfo {
            rutaRelativa  = @"smg_12b4.7z",
            nombre        = "SMG 12B4 - Downloads",
            descripcion   = "Metralleta/submunición para NPCs armados",
            prioridad     = Prioridad.Esencial,
            carpetaDestino = "Assets/Downloads/Weapons/SMG",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Hatchet.zip",
            nombre        = "Hatchet - Downloads",
            descripcion   = "Hacha/martillo para herramientas o armas improvisadas",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Weapons/Hatchet",
        },

        // ── Fauna/Animales Downloads ─────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"riggedHorse.blend",
            nombre        = "Rigged Horse - Downloads",
            descripcion   = "Caballo rigged para nuevo SistemaFauna",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Animals/Horse",
        },
        new PaqueteInfo {
            rutaRelativa  = @"rabbit.blend",
            nombre        = "Rabbit - Downloads",
            descripcion   = "Conejo para fauna ambiental",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Animals/Rabbit",
        },
        new PaqueteInfo {
            rutaRelativa  = @"chicken.zip",
            nombre        = "Chicken - Downloads",
            descripcion   = "Pollo para ambiente rural",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Animals/Chicken",
        },
        new PaqueteInfo {
            rutaRelativa  = @"rooster.zip",
            nombre        = "Rooster - Downloads",
            descripcion   = "Gallo para zona agrícola",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Animals/Rooster",
        },
        new PaqueteInfo {
            rutaRelativa  = @"Wolf.zip",
            nombre        = "Wolf - Downloads",
            descripcion   = "Lobo para fauna silvestre amenazante",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Animals/Wolf",
        },
        new PaqueteInfo {
            rutaRelativa  = @"sheepies.blend",
            nombre        = "Sheepies - Downloads",
            descripcion   = "Ovejas para zona pastoril",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Animals/Sheep",
        },

        // ── Ruinas y Destrucción Downloads ─────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"destroyedwalls_by_lfa.zip",
            nombre        = "Destroyed Walls - Downloads",
            descripcion   = "Muros y paredes destruidas para escenario post-crisis",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Destruction/DestroyedWalls",
        },

        // ── Iluminación/Fuego Downloads ─────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"torch.7z",
            nombre        = "Torch - Downloads",
            descripcion   = "Antorcha modelo 3D para iluminación ambiente",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Effects/Torch",
        },

        // ── Personajes Downloads ─────────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"female.blend",
            nombre        = "Female Character - Downloads",
            descripcion   = "Personaje femenino para civiles/manifestantes",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Characters/Female",
        },
        new PaqueteInfo {
            rutaRelativa  = @"male.blend",
            nombre        = "Male Character - Downloads",
            descripcion   = "Personaje masculino para civiles/manifestantes",
            prioridad     = Prioridad.Recomendado,
            carpetaDestino = "Assets/Downloads/Characters/Male",
        },
        new PaqueteInfo {
            rutaRelativa  = @"MONK.blend",
            nombre        = "Monk Character - Downloads",
            descripcion   = "Monje para civiles especiales",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Characters/Monk",
        },
        new PaqueteInfo {
            rutaRelativa  = @"forest-monster.7z",
            nombre        = "Forest Monster - Downloads",
            descripcion   = "Monstruo para enemigos/hostiles",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Characters/Monsters",
        },

        // ── Audio Downloads ─────────────────────────────────────
        new PaqueteInfo {
            rutaRelativa  = @"Game Over Glow.mp3",
            nombre        = "Game Over Glow Audio - Downloads",
            descripcion   = "SFX ambiental para AlertaManager o eventos",
            prioridad     = Prioridad.Opcional,
            carpetaDestino = "Assets/Downloads/Audio",
        },
    };

    // ───────────────────────────────────────────────────────────────────────
    //  MENÚ PRINCIPAL
    // ───────────────────────────────────────────────────────────────────────

    [MenuItem("Alsasua/📦 Importar Assets Esenciales", priority = 50)]
    public static void ImportarEsenciales()
    {
        ImportarPorPrioridad(Prioridad.Esencial, "ESENCIALES");
    }

    [MenuItem("Alsasua/📦 Importar Assets Recomendados", priority = 51)]
    public static void ImportarRecomendados()
    {
        ImportarPorPrioridad(Prioridad.Recomendado, "RECOMENDADOS");
    }

    [MenuItem("Alsasua/📦 Importar TODOS los Assets Útiles", priority = 52)]
    public static void ImportarTodos()
    {
        var todos = new List<PaqueteInfo>(PAQUETES);
        ImportarLista(todos, "TODOS");
    }

    [MenuItem("Alsasua/📦 Verificar Assets Importados", priority = 53)]
    public static void VerificarAssets()
    {
        int ok = 0, falta = 0;
        var sb = new System.Text.StringBuilder();
        sb.AppendLine("════════════════════════════════════════════════════");
        sb.AppendLine("  [Alsasua] ESTADO DE ASSETS EN E:\\assets");
        sb.AppendLine("════════════════════════════════════════════════════");

        foreach (var p in PAQUETES)
        {
            string rutaFisica  = Path.Combine(RutaBase, p.rutaRelativa);
            bool   archExiste  = File.Exists(rutaFisica);
            bool   yaImportado = AssetDatabase.IsValidFolder(p.carpetaDestino);

            string estado = yaImportado ? "✓ IMPORTADO"
                          : archExiste  ? "⚠ DISPONIBLE (no importado)"
                                        : "✗ NO ENCONTRADO";

            sb.AppendLine($"  [{p.prioridad,-12}] {p.nombre,-35} {estado}");
            if (!archExiste)
                sb.AppendLine($"              Ruta buscada: {rutaFisica}");

            if (yaImportado || archExiste) ok++;
            else falta++;
        }

        sb.AppendLine("════════════════════════════════════════════════════");
        sb.AppendLine($"  Disponibles/importados: {ok}  |  No encontrados: {falta}");
        sb.AppendLine($"  Ruta base configurada: {RutaBase}");
        sb.AppendLine("  → Cambia la ruta: Edit → Preferences → Alsasua Assets Path");
        sb.AppendLine("════════════════════════════════════════════════════");
        Debug.Log(sb.ToString());
    }

    [MenuItem("Alsasua/📦 Configurar Ruta de Assets", priority = 54)]
    public static void ConfigurarRuta()
    {
        string nuevaRuta = EditorUtility.OpenFolderPanel(
            "Seleccionar carpeta de assets (E:\\assets)",
            RutaBase, "");

        if (!string.IsNullOrEmpty(nuevaRuta))
        {
            EditorPrefs.SetString(PREF_RUTA_ASSETS, nuevaRuta);
            Debug.Log($"[Alsasua] Ruta de assets actualizada: {nuevaRuta}");
        }
    }

    [MenuItem("Alsasua/📦 Configurar Ruta de Downloads", priority = 55)]
    public static void ConfigurarRutaDownloads()
    {
        string nuevaRuta = EditorUtility.OpenFolderPanel(
            "Seleccionar carpeta Downloads",
            RutaDownloads, "");

        if (!string.IsNullOrEmpty(nuevaRuta))
        {
            EditorPrefs.SetString(PREF_RUTA_DOWNLOADS, nuevaRuta);
            Debug.Log($"[Alsasua] Ruta de Downloads actualizada: {nuevaRuta}");
        }
    }

    [MenuItem("Alsasua/📦 Importar Downloads - Esenciales", priority = 56)]
    public static void ImportarDownloadsEsenciales()
    {
        var lista = new List<PaqueteInfo>();
        foreach (var p in PAQUETES)
        {
            if (p.carpetaDestino.Contains("Downloads") && p.prioridad == Prioridad.Esencial)
                lista.Add(p);
        }
        ImportarDownloads(lista, "DOWNLOADS ESENCIALES");
    }

    [MenuItem("Alsasua/📦 Importar Downloads - Recomendados", priority = 57)]
    public static void ImportarDownloadsRecomendados()
    {
        var lista = new List<PaqueteInfo>();
        foreach (var p in PAQUETES)
        {
            if (p.carpetaDestino.Contains("Downloads") && p.prioridad == Prioridad.Recomendado)
                lista.Add(p);
        }
        ImportarDownloads(lista, "DOWNLOADS RECOMENDADOS");
    }

    [MenuItem("Alsasua/📦 Importar TODOS los Downloads", priority = 58)]
    public static void ImportarTodosDownloads()
    {
        var lista = new List<PaqueteInfo>();
        foreach (var p in PAQUETES)
        {
            if (p.carpetaDestino.Contains("Downloads"))
                lista.Add(p);
        }
        ImportarDownloads(lista, "TODOS LOS DOWNLOADS");
    }

    // ───────────────────────────────────────────────────────────────────────
    //  LÓGICA DE IMPORTACIÓN
    // ───────────────────────────────────────────────────────────────────────

    private static void ImportarPorPrioridad(Prioridad prioridad, string etiqueta)
    {
        var lista = new List<PaqueteInfo>();
        foreach (var p in PAQUETES)
            if (p.prioridad == prioridad) lista.Add(p);
        ImportarLista(lista, etiqueta);
    }

    private static void ImportarLista(List<PaqueteInfo> lista, string etiqueta)
    {
        // Verificar que la ruta base existe
        if (!Directory.Exists(RutaBase))
        {
            EditorUtility.DisplayDialog(
                "Ruta no encontrada",
                $"No se encontró la carpeta de assets:\n{RutaBase}\n\n" +
                "Usa Alsasua → 📦 Configurar Ruta de Assets para apuntar a tu carpeta E:\\assets.",
                "OK");
            return;
        }

        // Filtrar los que existen en disco y no están ya importados
        var pendientes = new List<PaqueteInfo>();
        var yaImportados = new List<string>();
        var noEncontrados = new List<string>();

        foreach (var p in lista)
        {
            string ruta = Path.Combine(RutaBase, p.rutaRelativa);
            if (AssetDatabase.IsValidFolder(p.carpetaDestino))
                yaImportados.Add(p.nombre);
            else if (File.Exists(ruta))
                pendientes.Add(p);
            else
                noEncontrados.Add($"{p.nombre} → {ruta}");
        }

        // Informe previo
        var msg = new System.Text.StringBuilder();
        msg.AppendLine($"Assets {etiqueta} — Resumen:");
        msg.AppendLine($"  · A importar:       {pendientes.Count}");
        msg.AppendLine($"  · Ya importados:    {yaImportados.Count}");
        msg.AppendLine($"  · No encontrados:   {noEncontrados.Count}");

        if (pendientes.Count == 0)
        {
            EditorUtility.DisplayDialog($"Importar Assets {etiqueta}", msg.ToString() +
                "\nNo hay nada nuevo que importar.", "OK");
            return;
        }

        msg.AppendLine("\nSe importarán:");
        foreach (var p in pendientes)
            msg.AppendLine($"  · {p.nombre}  ({p.descripcion})");

        if (noEncontrados.Count > 0)
        {
            msg.AppendLine("\nNo encontrados (omitidos):");
            foreach (var n in noEncontrados)
                msg.AppendLine($"  · {n}");
        }

        bool confirmar = EditorUtility.DisplayDialog(
            $"Importar Assets {etiqueta}",
            msg.ToString(),
            "Importar", "Cancelar");

        if (!confirmar) return;

        // Importar uno a uno
        int importados = 0;
        int errores    = 0;

        for (int i = 0; i < pendientes.Count; i++)
        {
            var p = pendientes[i];
            string ruta = Path.Combine(RutaBase, p.rutaRelativa);

            EditorUtility.DisplayProgressBar(
                $"Importando assets Alsasua ({i + 1}/{pendientes.Count})",
                $"{p.nombre}...",
                (float)(i + 1) / pendientes.Count);

            try
            {
                // interactive: true → Unity abre el diálogo de selección de assets del paquete
                AssetDatabase.ImportPackage(ruta, interactive: true);
                importados++;
                Debug.Log($"[Alsasua] ✓ Importado: {p.nombre}");
            }
            catch (System.Exception ex)
            {
                errores++;
                Debug.LogError($"[Alsasua] ✗ Error importando {p.nombre}: {ex.Message}");
            }
        }

        EditorUtility.ClearProgressBar();
        AssetDatabase.Refresh();

        Debug.Log(
            "════════════════════════════════════════════════════\n" +
           $"  [Alsasua] Importación {etiqueta} completada\n" +
           $"  · Importados: {importados}/{pendientes.Count}\n" +
           $"  · Errores: {errores}\n" +
            "  → Asigna los prefabs en el Inspector de SistemaAssets\n" +
            "    o ejecuta Alsasua → ⚙ Autoconectar Assets Importados\n" +
            "════════════════════════════════════════════════════");

        if (importados > 0)
        {
            // Sugerir autoconexión
            bool autoconectar = EditorUtility.DisplayDialog(
                "Importación completada",
                $"Se importaron {importados} paquete(s).\n\n" +
                "¿Ejecutar Autoconectar Assets ahora para enlazarlos automáticamente con los sistemas?",
                "Sí, autoconectar", "No por ahora");

            if (autoconectar)
                AutoconectarAssetsImportados();
        }
    }

    private static void ImportarDownloads(List<PaqueteInfo> lista, string etiqueta)
    {
        // Verificar que la ruta de Downloads existe
        if (!Directory.Exists(RutaDownloads))
        {
            EditorUtility.DisplayDialog(
                "Ruta no encontrada",
                $"No se encontró la carpeta de Downloads:\n{RutaDownloads}\n\n" +
                "Usa Alsasua → 📦 Configurar Ruta de Downloads para apuntar a tu carpeta Downloads.",
                "OK");
            return;
        }

        // Filtrar archivos que existan y carpetas destino
        var copiar = new List<(string origen, string destino, string nombre)>();
        var yaExisten = new List<string>();
        var noEncontrados = new List<string>();

        foreach (var p in lista)
        {
            string origen = Path.Combine(RutaDownloads, p.rutaRelativa);

            if (AssetDatabase.IsValidFolder(p.carpetaDestino))
            {
                yaExisten.Add(p.nombre);
            }
            else if (File.Exists(origen))
            {
                copiar.Add((origen, p.carpetaDestino, p.nombre));
            }
            else if (Directory.Exists(origen))
            {
                copiar.Add((origen, p.carpetaDestino, p.nombre));
            }
            else
            {
                noEncontrados.Add($"{p.nombre} → {origen}");
            }
        }

        // Resumen
        var msg = new System.Text.StringBuilder();
        msg.AppendLine($"Assets {etiqueta} — Resumen:");
        msg.AppendLine($"  · A copiar:         {copiar.Count}");
        msg.AppendLine($"  · Ya existentes:    {yaExisten.Count}");
        msg.AppendLine($"  · No encontrados:   {noEncontrados.Count}");

        if (copiar.Count == 0)
        {
            EditorUtility.DisplayDialog($"Copiar Assets {etiqueta}", msg.ToString() +
                "\nNo hay nada nuevo que copiar.", "OK");
            return;
        }

        msg.AppendLine("\nSe copiarán:");
        foreach (var (_, _, nombre) in copiar)
            msg.AppendLine($"  · {nombre}");

        if (noEncontrados.Count > 0)
        {
            msg.AppendLine("\nNo encontrados (omitidos):");
            foreach (var n in noEncontrados)
                msg.AppendLine($"  · {n}");
        }

        msg.AppendLine("\nNOTA: Asegúrate de haber descomprimido los archivos .7z y .zip");
        msg.AppendLine("en la carpeta Assets/Downloads/{categoria} antes de continuar.");

        bool confirmar = EditorUtility.DisplayDialog(
            $"Copiar Assets {etiqueta}",
            msg.ToString(),
            "Continuar", "Cancelar");

        if (!confirmar) return;

        // Crear carpetas destino y copiar
        int copiados = 0;
        int errores = 0;

        for (int i = 0; i < copiar.Count; i++)
        {
            var (origen, destino, nombre) = copiar[i];

            EditorUtility.DisplayProgressBar(
                $"Copiando assets {etiqueta} ({i + 1}/{copiar.Count})",
                nombre,
                (float)(i + 1) / copiar.Count);

            try
            {
                // Crear carpeta destino si no existe
                if (!Directory.Exists(destino))
                    Directory.CreateDirectory(destino);

                // Copiar archivo o carpeta
                if (File.Exists(origen))
                {
                    string nombreArchivo = Path.GetFileName(origen);
                    string destFile = Path.Combine(destino, nombreArchivo);
                    File.Copy(origen, destFile, overwrite: true);
                    copiados++;
                    Debug.Log($"[Alsasua] ✓ Copiado: {nombre}");
                }
                else if (Directory.Exists(origen))
                {
                    CopiarDirectorio(origen, destino, true);
                    copiados++;
                    Debug.Log($"[Alsasua] ✓ Copiada carpeta: {nombre}");
                }
            }
            catch (System.Exception ex)
            {
                errores++;
                Debug.LogError($"[Alsasua] ✗ Error copiando {nombre}: {ex.Message}");
            }
        }

        EditorUtility.ClearProgressBar();
        AssetDatabase.Refresh();

        Debug.Log(
            "════════════════════════════════════════════════════\n" +
           $"  [Alsasua] Copia {etiqueta} completada\n" +
           $"  · Copiados: {copiados}/{copiar.Count}\n" +
           $"  · Errores: {errores}\n" +
            "  → Ubicación: Assets/Downloads/{categoria}\n" +
            "════════════════════════════════════════════════════");
    }

    // Helper para copiar directorios recursivamente
    private static void CopiarDirectorio(string origen, string destino, bool sobrescribir)
    {
        var dirInfo = new DirectoryInfo(origen);
        if (!Directory.Exists(destino))
            Directory.CreateDirectory(destino);

        foreach (FileInfo archivo in dirInfo.GetFiles())
        {
            string destFile = Path.Combine(destino, archivo.Name);
            archivo.CopyTo(destFile, sobrescribir);
        }

        foreach (DirectoryInfo subDir in dirInfo.GetDirectories())
        {
            string destSubDir = Path.Combine(destino, subDir.Name);
            CopiarDirectorio(subDir.FullName, destSubDir, sobrescribir);
        }
    }

    // ───────────────────────────────────────────────────────────────────────
    //  AUTOCONEXIÓN DE ASSETS A SISTEMAS
    // ───────────────────────────────────────────────────────────────────────

    [MenuItem("Alsasua/📦 Autoconectar Assets Importados", priority = 55)]
    public static void AutoconectarAssetsImportados()
    {
        Debug.Log("[Alsasua] Buscando assets importados para autoconectar…");

        var sistemaAssets = FindSistemaAssets();
        if (sistemaAssets == null)
        {
            Debug.LogWarning("[Alsasua] AutoconectarAssets: SistemaAssets no encontrado en la escena. " +
                             "Abre la escena ESCENA_ALSASUA y vuelve a ejecutar.");
            return;
        }

        var so   = new SerializedObject(sistemaAssets);
        int hits = 0;

        // ── Buscar prefab de barricada concreto (Rossendy Brito) ──
        hits += AsignarPrimerPrefab(so, "prefabBarricadaPack",
            new[] {
                "Assets/BarrierPack_Rossendy",
                "Assets/BarrierPack",
                "Assets/Rossendy Brito",
            }, "*.prefab");

        // ── Buscar prefab de farola ──
        hits += AsignarPrimerPrefab(so, "prefabFarola",
            new[] { "Assets/SpaceZeta_StreetLamps2/Prefabs" }, "StreetLampRound1A.prefab");

        // ── Buscar prefab barricada hormigón (Abandoned World) ──
        hits += AsignarPrimerPrefab(so, "prefabBarricadaHormigon",
            new[] { "Assets/Abandoned World/Metal and Concrete Barrier/Prefabs" },
            "Concrete_Barrier_1.prefab");

        // ── Buscar prefab barricada metal ──
        hits += AsignarPrimerPrefab(so, "prefabBarricadaMetal",
            new[] { "Assets/Abandoned World/Metal and Concrete Barrier/Prefabs" },
            "Metal_Barrier_1.prefab");

        // ── Buscar prefab policía (Interceptor) ──
        hits += AsignarPrimerPrefab(so, "prefabPatrullaGC",
            new[] { "Assets/Police Car & Helicopter/Prefabs" }, "Interceptor.prefab");

        // ── Buscar prefab helicóptero ──
        hits += AsignarPrimerPrefab(so, "prefabHelicoptero",
            new[] { "Assets/Police Car & Helicopter/Prefabs" }, "Helicopter.prefab");

        // ── Buscar prefab explosión (Mirza Beig) ──
        hits += AsignarPrimerPrefab(so, "prefabExplosion",
            new[] { "Assets/Mirza Beig/Cinematic Explosions FREE/Prefabs/Explosions" },
            "*.prefab");

        // ── Buscar prefab VFX fuego (Free Fire VFX) ──
        hits += AsignarPrimerPrefab(so, "prefabVFXFuego",
            new[] { "Assets/Vefects/Free Fire VFX/Particles" },
            "VFX_Fire_Floor_01.prefab");

        // ── Buscar prefab VFX fuego lite (LiteFireEffect) ──
        hits += AsignarPrimerPrefab(so, "prefabVFXFuegoLite",
            new[] { "Assets/LiteFireEffect/Prafab" }, "BaseFire000.prefab");

        // ── Buscar prefab soldado (LowPolySoldiers) ──
        hits += AsignarPrimerPrefab(so, "prefabSoldado",
            new[] { "Assets/LowPolySoldiers_demo/models" }, "Soldier_demo.FBX");

        // ── Audio: multitud (Gregor Quendel) ──
        hits += AsignarPrimerAudio(so, "audioMultitud",
            new[] { "Assets/Gregor Quendel - Free General Ambience Sounds" },
            "*Cheering*Ambience*.wav");

        // ── Audio: tráfico ──
        hits += AsignarPrimerAudio(so, "audioTraficoAmbiente",
            new[] { "Assets/Gregor Quendel - Free General Ambience Sounds" },
            "*Traffic*Street*Cars*.wav");

        so.ApplyModifiedProperties();
        EditorUtility.SetDirty(sistemaAssets);
        AssetDatabase.SaveAssets();

        Debug.Log($"[Alsasua] ✓ Autoconexión completada: {hits} campos asignados en SistemaAssets.");

        if (hits == 0)
            Debug.LogWarning("[Alsasua] No se encontró ningún asset para asignar. " +
                "Asegúrate de haber importado los paquetes primero (Alsasua → 📦 Importar Assets).");
    }

    // ───────────────────────────────────────────────────────────────────────
    //  HELPERS
    // ───────────────────────────────────────────────────────────────────────

    private static Component FindSistemaAssets()
    {
        // Buscar en la escena activa
        foreach (var go in Object.FindObjectsByType<GameObject>(FindObjectsSortMode.None))
        {
            var comp = go.GetComponent("SistemaAssets");
            if (comp != null) return comp;
        }
        return null;
    }

    /// <summary>
    /// Busca el primer prefab que coincida con el patrón en las carpetas indicadas
    /// y lo asigna al campo serializado. Devuelve 1 si asignó, 0 si no.
    /// </summary>
    private static int AsignarPrimerPrefab(SerializedObject so, string campo,
        string[] carpetas, string patron)
    {
        var prop = so.FindProperty(campo);
        if (prop == null) return 0;
        if (prop.objectReferenceValue != null) return 0;  // ya asignado, respetar

        foreach (string carpeta in carpetas)
        {
            if (!AssetDatabase.IsValidFolder(carpeta)) continue;
            string[] guids = AssetDatabase.FindAssets("t:Prefab t:Model", new[] { carpeta });
            foreach (string guid in guids)
            {
                string ruta = AssetDatabase.GUIDToAssetPath(guid);
                string archivo = Path.GetFileName(ruta);

                // Coincidir patrón (glob simple: * = comodín)
                if (PatronCoincide(archivo, patron))
                {
                    var obj = AssetDatabase.LoadAssetAtPath<GameObject>(ruta);
                    if (obj != null)
                    {
                        prop.objectReferenceValue = obj;
                        Debug.Log($"[Alsasua] Auto-asignado {campo} → {ruta}");
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    private static int AsignarPrimerAudio(SerializedObject so, string campo,
        string[] carpetas, string patron)
    {
        var prop = so.FindProperty(campo);
        if (prop == null) return 0;
        if (prop.objectReferenceValue != null) return 0;

        foreach (string carpeta in carpetas)
        {
            if (!AssetDatabase.IsValidFolder(carpeta)) continue;
            string[] guids = AssetDatabase.FindAssets("t:AudioClip", new[] { carpeta });
            foreach (string guid in guids)
            {
                string ruta    = AssetDatabase.GUIDToAssetPath(guid);
                string archivo = Path.GetFileName(ruta);
                if (PatronCoincide(archivo, patron))
                {
                    var clip = AssetDatabase.LoadAssetAtPath<UnityEngine.AudioClip>(ruta);
                    if (clip != null)
                    {
                        prop.objectReferenceValue = clip;
                        Debug.Log($"[Alsasua] Auto-asignado {campo} → {ruta}");
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    /// <summary>
    /// Coincidencia de patrón glob simple (solo * como comodín, sin sensibilidad a mayúsculas).
    /// </summary>
    private static bool PatronCoincide(string nombre, string patron)
    {
        // Si el patrón no tiene comodín, comparación directa
        if (!patron.Contains("*"))
            return string.Equals(nombre, patron, System.StringComparison.OrdinalIgnoreCase);

        // Convertir patrón glob a regex
        string regex = "^" + System.Text.RegularExpressions.Regex.Escape(patron)
                                 .Replace("\\*", ".*") + "$";
        return System.Text.RegularExpressions.Regex.IsMatch(
            nombre, regex, System.Text.RegularExpressions.RegexOptions.IgnoreCase);
    }
}

#endif
