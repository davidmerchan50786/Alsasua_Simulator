#include "World/AlsasuaMallaFab.h"
#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Modules/ModuleManager.h"

namespace
{
	/** Carpetas donde aterrizan las descargas de Fab y del launcher de Epic. */
	const TCHAR* RaicesFab[] = {
		TEXT("/Game/Megascans"),
		TEXT("/Game/MSPresets"),
		TEXT("/Game/Fab"),
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
	};

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
		return nullptr;
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
