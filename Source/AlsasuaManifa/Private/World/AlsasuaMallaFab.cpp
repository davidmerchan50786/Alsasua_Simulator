#include "World/AlsasuaMallaFab.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Modules/ModuleManager.h"

namespace
{
	/**
	 * Carpetas de contenido descargado, por orden de preferencia.
	 *
	 * Las tres primeras son de Fab / launcher de Epic. AssetsImportados es lo
	 * que ya estaba descargado en el proyecto (552 mallas de packs de Unity,
	 * Meshy AI y modelos sueltos, según Datos/asset_manifest.json): estaba
	 * importándose y casi nada lo cargaba. ModelosDescargados son props CC0 de
	 * Poly Haven para los huecos que ninguna de las otras cubría.
	 */
	const TCHAR* RaicesFab[] = {
		TEXT("/Game/Megascans"),
		TEXT("/Game/MSPresets"),
		TEXT("/Game/Fab"),
		TEXT("/Game/AssetsImportados"),
		TEXT("/Game/ModelosDescargados"),
	};

	/**
	 * Palabras con las que viene nombrado cada tipo cuando se baja de Fab.
	 * Fab no usa los nombres de street_furniture.json, así que hay que mapear
	 * a los términos ingleses del catálogo.
	 */
	struct FClaves { const TCHAR* Tipo; const TCHAR* Claves; };

	const FClaves ClavesPorTipo[] = {
		// Cada tipo lleva primero el nombre español o euskera, que es como está
		// nombrado lo descargado en AssetsImportados (los 43 modelos de MeshyAI
		// se generaron para este pueblo: Ayuntamiento_Altsasu,
		// Iglesia_Jasokundeko, Fronton_Pelota...), y después el término inglés
		// del catálogo de Fab/Megascans. Con sólo el inglés fallaban 21 de 29.
		{ TEXT("banco"),             TEXT("banco_parque|banco|modular_street_seating|bench|park_bench") },
		{ TEXT("papelera"),          TEXT("papelera|contenedor_reciclaje|metal_trash_can|trash|litter|waste|bin") },
		{ TEXT("bollard"),           TEXT("bolardo|pilona|bollard") },
		{ TEXT("parada_bus"),        TEXT("parada_autobus|parada_bus|bus_stop|busstop|shelter") },
		{ TEXT("boca_incendio"),     TEXT("boca_incendio|hidrante|fire_hydrant|hydrant") },
		{ TEXT("tapa_alcantarilla"), TEXT("tapa_alcantarilla|alcantarilla|water_manhole_cover|manhole|rejilla|drain|sewer") },
		{ TEXT("maceta"),            TEXT("maceta|jardinera|seto_verde|planter|flower_pot") },
		{ TEXT("buzon_correos"),     TEXT("buzon_correos|buzon|mailbox|postbox") },
		{ TEXT("fuente"),            TEXT("fuente_agua|fuente_bebida|fuente|fountain") },
		{ TEXT("farola_decorativa"), TEXT("farola_clasica|farola_moderna|farola|prop_lamp_street|street_lamp|streetlamp|lantern") },
		{ TEXT("señal_stop"),        TEXT("stop_senal|senal_stop|stop_sign|road_sign") },
		{ TEXT("señal_velocidad"),   TEXT("senal_calle|senal_velocidad|speed_sign|traffic_sign") },
		{ TEXT("placa_calle"),       TEXT("placa_calle|senal_calle|street_plate|street_name") },
		{ TEXT("cuadro_electrico"),  TEXT("cuadro_electrico|deposito_agua|electrical_box|utility_box|cabinet") },
		{ TEXT("guarda_barandas"),   TEXT("barandilla_puente|barandilla|guardrail|railing") },
		{ TEXT("semaforo"),          TEXT("semaforo_urbano|semaforo|traffic_light") },

		// ── Vehículos ──────────────────────────────────────────────────────
		// Meshy generó cuatro para este pueblo y no los usaba nadie: no había
		// entrada, así que BuscarEnFab salía sin candidato y el coche acababa en
		// la forma básica del motor con el Coche_Sedan ya descargado al lado.
		{ TEXT("coche"),             TEXT("coche_sedan|coche|sedan|car|vehicle") },
		{ TEXT("coche_policia"),     TEXT("coche_policia|police_car|police") },
		{ TEXT("autobus"),           TEXT("autobus_urbano|autobus|bus|city_bus") },
		{ TEXT("furgoneta"),         TEXT("furgoneta_secundaria|furgoneta|van|truck") },

		// Tipos que trae street_furniture.json y que la tabla no conocía (17
		// piezas en total): sin entrada caían a primitiva aunque hubiera malla.
		// papelera_reciclaje tiene la suya propia de Meshy; el resto tira del
		// catálogo en inglés de Fab. El nombre con espacio de "rejilla
		// ventilacion" es el del dato, no un typo: tiene que casar literal.
		{ TEXT("papelera_reciclaje"),  TEXT("contenedor_reciclaje|reciclaje|recycling|recycle_bin") },
		{ TEXT("rejilla ventilacion"), TEXT("rejilla|ventilacion|vent_grate|grate|air_vent") },
		{ TEXT("espejo_seguridad"),    TEXT("espejo_seguridad|espejo|traffic_mirror|convex_mirror") },
		{ TEXT("resalto_vial"),        TEXT("resalto|baden|speed_bump|speed_hump") },
		{ TEXT("cruce_peatonal"),      TEXT("cruce_peatonal|paso_cebra|crosswalk|zebra_crossing") },
		{ TEXT("bici_arbol"),          TEXT("aparcabicis|bicicletero|bike_rack|bicycle_rack") },

		// Landmarks de landmarks_real.json. Meshy generó los cuatro
		// singulares del pueblo con su nombre propio.
		{ TEXT("iglesia"),           TEXT("iglesia_jasokundeko|iglesia|eliza|church|chapel") },
		{ TEXT("fronton"),           TEXT("fronton_pelota|fronton|pelota|handball_court") },
		{ TEXT("ayuntamiento"),      TEXT("ayuntamiento_altsasu|ayuntamiento|udaletxea|town_hall|city_hall") },
		{ TEXT("estacion_tren"),     TEXT("estacion_tren_altsasu|estacion_tren|estacion|train_station|railway_station") },
		{ TEXT("mercado"),           TEXT("mercado|azoka|market_hall|market") },
		{ TEXT("polideportivo"),     TEXT("nave_industrial|polideportivo|kiroldegia|sports_hall|gymnasium") },

		// ── Kit modular de Village (142 piezas) ────────────────────────────
		// Familias limpias por prefijo: Roof_ (29), Stucco_ (26), Stone_ (23),
		// Wood_ (16), Prop_ (13), Canopy_ (10), Waterwheel_ (9), Cobblestone_,
		// Wall_Prop_, Dirt_, Kit_Window_. Se apunta a la pieza recta o
		// "Full" de cada familia, que es la que sirve de caso general.
		{ TEXT("toldo"),             TEXT("canopy_full|canopy_side|canopy_top|awning") },
		{ TEXT("marquesina"),        TEXT("canopy_full|canopy_beam|canopy_corner") },
		{ TEXT("bordillo"),          TEXT("stone_curb|curb|kerb") },
		{ TEXT("acera_pieza"),       TEXT("cobblestone_floor|stone_floor|pp_floor_tile") },
		{ TEXT("puerta"),            TEXT("wall_prop_door_simple|wall_prop_door_ornate|stucco_doorway|stone_doorway|door") },
		{ TEXT("escaparate"),        TEXT("stucco_doorway_wide|harategia|okindegia|shop_front|storefront") },
		{ TEXT("ventana"),           TEXT("stucco_window_single|stucco_window_double|kit_window|ventana_basca|window") },
		{ TEXT("muro_piedra"),       TEXT("stone_wall|piedra_muro|stone_arch") },
		{ TEXT("tejado_pieza"),      TEXT("roof_straight_side|roof_concave_side|roof_convex_side") },
		{ TEXT("chimenea"),          TEXT("roof_prop_chimney_stone|roof_prop_chimney|chimenea_piedra|chimney") },

		// Piezas de remate que ensambla UAlsasuaTejadoModular. Sólo la familia
		// Straight: en este kit Concave/Convex son variantes de planta curva, y
		// los footprints del LIDAR son polígonos de lados rectos.
		{ TEXT("tejado_alero"),         TEXT("roof_straight_side_eave|roof_straight_side") },
		{ TEXT("tejado_esquina_ext"),   TEXT("roof_straight_corner_outer_eave|roof_straight_corner_outer") },
		{ TEXT("tejado_esquina_int"),   TEXT("roof_straight_corner_inner") },
		{ TEXT("tejado_cumbrera"),      TEXT("roof_accent_ridge") },
		{ TEXT("tejado_cumbrera_fin"),  TEXT("roof_accent_ridge_end") },
		{ TEXT("tejado_limatesa"),      TEXT("roof_accent_rake_straight|roof_accent_rake_eave") },
		{ TEXT("pilar"),             TEXT("stone_pillar|stucco_prop_support_pillar|wood_post") },
		{ TEXT("escalera"),          TEXT("stone_steps|wood_steps|stairs|steps") },
		{ TEXT("barril"),            TEXT("prop_barrel|barrel") },
		{ TEXT("caja_madera"),       TEXT("prop_crate|crate") },
		{ TEXT("escalera_mano"),     TEXT("prop_ladder|ladder") },
		{ TEXT("pozo"),              TEXT("prop_well|well") },
		{ TEXT("noria"),             TEXT("waterwheel_1|waterwheel|water_wheel") },
		{ TEXT("canal_agua"),        TEXT("waterwheel_flume_straight|flume") },
		{ TEXT("molino"),            TEXT("wall_prop_windmill|casa_chorro_errota|windmill") },
		{ TEXT("cartel_pared"),      TEXT("wall_prop_sign|sign") },
		{ TEXT("farola_pared"),      TEXT("wall_prop_lamp") },
		{ TEXT("valla"),             TEXT("pp_small_fence|small_fence|fence") },
		{ TEXT("puente_pieza"),      TEXT("pp_bridge_15_middle|pp_bridge|bridge") },

		// ── ForestPack (34 piezas) para el suelo del monte ────────────────
		{ TEXT("roca"),              TEXT("pp_rock_moss_grown|pp_rock_pile|roca_grande|rock_01|rock_03") },
		{ TEXT("musgo"),             TEXT("pp_forest_mountain_moss|moss") },
		{ TEXT("seta"),              TEXT("pp_mushroom|mushroom") },
		{ TEXT("flor"),              TEXT("pp_daffodil|pp_hyacinth|pp_sunflower|fleur") },
		{ TEXT("prado"),             TEXT("pp_meadow_07|pp_meadow_08|meadow") },
		{ TEXT("senda"),             TEXT("pp_meadow_path|path") },
		{ TEXT("hierba_pieza"),      TEXT("pp_grass|grass_07|hierba_larga|multi_stylized_grass") },
		{ TEXT("seto"),              TEXT("hedgelong|hedgesmall|seto_verde|hedge") },
		{ TEXT("tronco_caido"),      TEXT("tronco_caido|pine_roots|dead_tree_trunk") },
	};

	/**
	 * Claves de especie para lo descargado. Salen de los nombres reales del
	 * manifiesto: MeshyAI/Arbol_Roble, Mundo/pine/snow_pine_tree,
	 * Naturaleza/ForestPack/PP_Birch_Tree_05, Cypress/cypress...
	 */
	const FClaves ClavesArbol[] = {
		// arbol_* son los de MeshyAI; pp_*_tree los del pack de Naturaleza.
		{ TEXT("QuercusRobur"), TEXT("arbol_roble|roble|oak") },
		{ TEXT("Fagus"),        TEXT("arbol_haya|haya|beech") },
		{ TEXT("Betula"),       TEXT("arbol_abedul|abedul|birch") },
		{ TEXT("Pinus"),        TEXT("snow_pine|pine_tree|pino|cypress|cipres") },
		{ TEXT("Populus"),      TEXT("chopo|alamo|aspen|poplar") },
		{ TEXT("Salix"),        TEXT("sauce|willow") },
		{ TEXT("Tilia"),        TEXT("tilo|tilia|linden|pp_tree|huge_tree") },
		{ TEXT("Platanus"),     TEXT("platano|plane_tree|platanus") },
		{ TEXT("Acer"),         TEXT("arce|maple|acer") },
		{ TEXT("Prunus"),       TEXT("cerezo|cherry|prunus") },
	};

	/**
	 * Arquetipo de landmark para cada tipo. Quince tipos comparten siete
	 * mallas: una iglesia y una ikastola no pueden ser el mismo cubo, pero un
	 * juzgado y una biblioteca sí comparten volumen.
	 */
	const TCHAR* ArquetipoLandmarkDe(const FString& Tipo)
	{
		if (Tipo == TEXT("iglesia"))          return TEXT("/Game/Landmarks/SM_Iglesia.SM_Iglesia");
		if (Tipo == TEXT("fronton"))          return TEXT("/Game/Landmarks/SM_Fronton.SM_Fronton");
		if (Tipo == TEXT("ayuntamiento"))     return TEXT("/Game/Landmarks/SM_Ayuntamiento.SM_Ayuntamiento");
		if (Tipo == TEXT("estacion_tren"))    return TEXT("/Game/Landmarks/SM_Estacion.SM_Estacion");

		// Naves diáfanas de cubierta ligera.
		if (Tipo == TEXT("polideportivo") || Tipo == TEXT("mercado"))
			return TEXT("/Game/Landmarks/SM_Nave.SM_Nave");

		// Centros docentes y culturales: bloque en L de tres plantas.
		if (Tipo == TEXT("ikastola") || Tipo == TEXT("colegio") ||
		    Tipo == TEXT("escuela_publica") || Tipo == TEXT("casa_cultura") ||
		    Tipo == TEXT("gaztetxe"))
			return TEXT("/Game/Landmarks/SM_Escuela.SM_Escuela");

		// Resto de equipamiento: juzgado, centro de salud, biblioteca.
		if (Tipo == TEXT("juzgado") || Tipo == TEXT("centro_salud") || Tipo == TEXT("biblioteca"))
			return TEXT("/Game/Landmarks/SM_BloqueCivico.SM_BloqueCivico");

		// "parque" no es un edificio: no le corresponde malla.
		return nullptr;
	}

	/** Respaldo procedural de árbol (UCreadorMallaArbol) para cada arquetipo. */
	const TCHAR* MallaArbolDe(const FString& Arquetipo)
	{
		static const TCHAR* Conocidos[] = {
			TEXT("QuercusRobur"), TEXT("Pinus"), TEXT("Fagus"), TEXT("Betula"),
			TEXT("Populus"), TEXT("Salix"), TEXT("Prunus"), TEXT("Tilia"),
			TEXT("Platanus"), TEXT("Acer"),
		};
		for (const TCHAR* K : Conocidos)
		{
			if (Arquetipo == K)
			{
				// Cadena estática por arquetipo: se construye una vez y vive
				// mientras dure el proceso.
				static TMap<FString, FString> Rutas;
				FString& R = Rutas.FindOrAdd(Arquetipo);
				if (R.IsEmpty())
				{
					R = FString::Printf(TEXT("/Game/Meshes/Arboles/SM_%s.SM_%s"), K, K);
				}
				return *R;
			}
		}
		return nullptr;
	}

	/** Nombre de la malla propia para ese tipo, o null si no hay. */
	const TCHAR* MallaPropiaDe(const FString& Tipo)
	{
		if (Tipo == TEXT("banco"))             return TEXT("/Game/Mobiliario/SM_Banco.SM_Banco");
		if (Tipo == TEXT("papelera"))          return TEXT("/Game/Mobiliario/SM_Papelera.SM_Papelera");
		if (Tipo == TEXT("bollard"))           return TEXT("/Game/Mobiliario/SM_Bolardo.SM_Bolardo");
		if (Tipo == TEXT("maceta"))            return TEXT("/Game/Mobiliario/SM_Maceta.SM_Maceta");
		if (Tipo == TEXT("boca_incendio"))     return TEXT("/Game/Mobiliario/SM_BocaIncendio.SM_BocaIncendio");
		if (Tipo == TEXT("tapa_alcantarilla")) return TEXT("/Game/Mobiliario/SM_TapaAlcantarilla.SM_TapaAlcantarilla");
		if (Tipo == TEXT("buzon_correos"))     return TEXT("/Game/Mobiliario/SM_BuzonCorreos.SM_BuzonCorreos");
		if (Tipo == TEXT("parada_bus"))        return TEXT("/Game/Mobiliario/SM_ParadaBus.SM_ParadaBus");
		if (const TCHAR* Arbol = MallaArbolDe(Tipo)) return Arbol;
		return ArquetipoLandmarkDe(Tipo);
	}

	struct FResuelto
	{
		TWeakObjectPtr<UStaticMesh> Malla;
		bool bDeFab = false;
	};

	TMap<FString, FResuelto> Cache;
	bool bFabEscaneado = false;
	TArray<FAssetData> MallasFab;

	bool Contiene(const FString& Texto, const FString& Claves)
	{
		TArray<FString> Lista;
		Claves.ParseIntoArray(Lista, TEXT("|"), true);
		for (const FString& C : Lista)
		{
			if (Texto.Contains(C)) return true;
		}
		return false;
	}

	/** Escanea una sola vez las carpetas de Fab; si no hay, no toca /Game. */
	void EscanearFab()
	{
		if (bFabEscaneado) return;
		bFabEscaneado = true;

		FAssetRegistryModule* ARM = FModuleManager::GetModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (!ARM) return;
		IAssetRegistry& AR = ARM->Get();

		FARFilter Filtro;
		Filtro.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
		Filtro.bRecursivePaths = true;
		Filtro.bRecursiveClasses = true;

		for (const TCHAR* Raiz : RaicesFab)
		{
			Filtro.PackagePaths.Add(FName(Raiz));
		}

		AR.GetAssets(Filtro, MallasFab);

		if (MallasFab.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[MallaFab] %d mallas de Fab disponibles."), MallasFab.Num());
		}
	}

	/** Busca en lo bajado de Fab la mejor malla para el tipo. */
	UStaticMesh* BuscarEnFab(const FString& Tipo)
	{
		const FClaves* Entrada = nullptr;
		for (const FClaves& C : ClavesPorTipo)
		{
			if (Tipo == C.Tipo) { Entrada = &C; break; }
		}
		if (!Entrada)
		{
			for (const FClaves& C : ClavesArbol)
			{
				if (Tipo == C.Tipo) { Entrada = &C; break; }
			}
		}
		if (!Entrada) return nullptr;

		EscanearFab();

		const FString Claves(Entrada->Claves);

		// Primero nombre exacto, clave por clave y en el orden escrito. Sin
		// esto una clave que es prefijo de otra se lleva la malla equivocada
		// según el orden del registro: "roof_accent_ridge" también está dentro
		// de Roof_Accent_Ridge_End, y la cumbrera salía siendo la tapa final.
		TArray<FString> Lista;
		Claves.ParseIntoArray(Lista, TEXT("|"), true);
		for (const FString& Clave : Lista)
		{
			for (const FAssetData& Datos : MallasFab)
			{
				if (Datos.AssetName.ToString().ToLower() != Clave) continue;
				if (UStaticMesh* Malla = Cast<UStaticMesh>(Datos.GetAsset())) return Malla;
			}
		}

		for (const FAssetData& Datos : MallasFab)
		{
			const FString Nombre = Datos.AssetName.ToString().ToLower();
			const FString Paquete = Datos.PackageName.ToString().ToLower();

			// Megascans deja el nombre legible en la carpeta y un hash en la
			// malla, así que hay que mirar las dos.
			if (!Contiene(Nombre, Claves) && !Contiene(Paquete, Claves)) continue;

			// Los LOD altos de Megascans son siluetas sin detalle.
			if (Nombre.Contains(TEXT("lod")) && !Nombre.Contains(TEXT("lod0"))) continue;

			if (UStaticMesh* Malla = Cast<UStaticMesh>(Datos.GetAsset()))
			{
				return Malla;
			}
		}
		return nullptr;
	}
}

UStaticMesh* AlsasuaMallaFab::Resolver(const FString& Tipo, const TCHAR* FormaBasica)
{
	if (const FResuelto* Guardado = Cache.Find(Tipo))
	{
		if (Guardado->Malla.IsValid()) return Guardado->Malla.Get();
		Cache.Remove(Tipo);
	}

	FResuelto Resuelto;

	// 1. Fab, si está bajado.
	Resuelto.Malla = BuscarEnFab(Tipo);
	Resuelto.bDeFab = Resuelto.Malla.IsValid();

	// 2. La malla propia generada por UCreadorMallaMobiliario.
	if (!Resuelto.Malla.IsValid())
	{
		if (const TCHAR* Propia = MallaPropiaDe(Tipo))
		{
			Resuelto.Malla = LoadObject<UStaticMesh>(nullptr, Propia);
		}
	}

	// 3. Forma básica del motor, para que al menos se vea el bulto.
	if (!Resuelto.Malla.IsValid() && FormaBasica)
	{
		Resuelto.Malla = LoadObject<UStaticMesh>(nullptr, FormaBasica);
	}

	Cache.Add(Tipo, Resuelto);
	return Resuelto.Malla.Get();
}

bool AlsasuaMallaFab::VieneDeFab(const FString& Tipo)
{
	const FResuelto* Guardado = Cache.Find(Tipo);
	return Guardado && Guardado->bDeFab;
}

void AlsasuaMallaFab::LimpiarCache()
{
	Cache.Empty();
	MallasFab.Empty();
	bFabEscaneado = false;
}
