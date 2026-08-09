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
	 * importándose y casi nada lo cargaba.
	 */
	const TCHAR* RaicesFab[] = {
		TEXT("/Game/Megascans"),
		TEXT("/Game/MSPresets"),
		TEXT("/Game/Fab"),
		TEXT("/Game/AssetsImportados"),
	};

	/**
	 * Palabras con las que viene nombrado cada tipo cuando se baja de Fab.
	 * Fab no usa los nombres de street_furniture.json, así que hay que mapear
	 * a los términos ingleses del catálogo.
	 */
	struct FClaves { const TCHAR* Tipo; const TCHAR* Claves; };

	const FClaves ClavesPorTipo[] = {
		{ TEXT("banco"),             TEXT("bench|park_bench|seat") },
		{ TEXT("papelera"),          TEXT("trash|litter|waste|bin|garbage") },
		{ TEXT("bollard"),           TEXT("bollard|post") },
		{ TEXT("parada_bus"),        TEXT("bus_stop|busstop|shelter") },
		{ TEXT("boca_incendio"),     TEXT("hydrant|fire_hydrant") },
		{ TEXT("tapa_alcantarilla"), TEXT("manhole|drain|sewer") },
		{ TEXT("maceta"),            TEXT("planter|flower_pot|flowerpot|pot") },
		{ TEXT("buzon_correos"),     TEXT("mailbox|postbox|post_box") },
		{ TEXT("fuente"),            TEXT("fountain|drinking_fountain") },
		{ TEXT("farola_decorativa"), TEXT("street_lamp|streetlamp|lamp_post|lantern") },
		{ TEXT("señal_stop"),        TEXT("stop_sign|road_sign|traffic_sign") },
		{ TEXT("cuadro_electrico"),  TEXT("electrical_box|utility_box|cabinet") },

		// Landmarks de landmarks_real.json.
		{ TEXT("iglesia"),           TEXT("church|chapel|cathedral") },
		{ TEXT("fronton"),           TEXT("pelota|fronton|handball_court") },
		{ TEXT("ayuntamiento"),      TEXT("town_hall|city_hall|townhall") },
		{ TEXT("estacion_tren"),     TEXT("train_station|railway_station|station") },
		{ TEXT("mercado"),           TEXT("market|market_hall") },
		{ TEXT("polideportivo"),     TEXT("sports_hall|gymnasium|sports_center") },
	};

	/**
	 * Claves de especie para lo descargado. Salen de los nombres reales del
	 * manifiesto: MeshyAI/Arbol_Roble, Mundo/pine/snow_pine_tree,
	 * Naturaleza/ForestPack/PP_Birch_Tree_05, Cypress/cypress...
	 */
	const FClaves ClavesArbol[] = {
		{ TEXT("QuercusRobur"), TEXT("arbol_roble|oak") },
		{ TEXT("Fagus"),        TEXT("arbol_haya|beech") },
		{ TEXT("Betula"),       TEXT("arbol_abedul|birch") },
		{ TEXT("Pinus"),        TEXT("pine_tree|snow_pine|pino") },
		{ TEXT("Populus"),      TEXT("aspen|poplar") },
		{ TEXT("Salix"),        TEXT("willow|sauce") },
		{ TEXT("Tilia"),        TEXT("linden|tilia") },
		{ TEXT("Platanus"),     TEXT("plane_tree|platanus") },
		{ TEXT("Acer"),         TEXT("maple|acer") },
		{ TEXT("Prunus"),       TEXT("cherry|prunus") },
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
