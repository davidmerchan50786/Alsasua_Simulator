#include "Plugins/AlsasuaCargadorPlugins.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "GameFeaturesSubsystem.h"
#include "HAL/ConsoleManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogCargadorPlugins, Log, All);

namespace AlsasuaArranqueFlags
{
	bool bSemaforos = true;
}

/** Dependencias declaradas en el .uplugin del plugin dado. */
static void DependenciasDePlugin(const FString& Nombre,
	TSet<FString>& Fuera)
{
	const FString Archivo = FPaths::ProjectPluginsDir() / Nombre /
		(Nombre + TEXT(".uplugin"));
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Archivo))
	{
		return;
	}
	TSharedPtr<FJsonObject> Raiz;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Texto), Raiz) ||
		!Raiz.IsValid())
	{
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* Deps = nullptr;
	Raiz->TryGetArrayField(TEXT("Dependencies"), Deps);
	if (Deps == nullptr)
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Valor : *Deps)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		FString NombreDep;
		if (Valor->TryGetObject(Obj) && Obj && (*Obj)->TryGetStringField(TEXT("Name"), NombreDep))
		{
			Fuera.Add(NombreDep);
		}
		else if (Valor->TryGetString(NombreDep))
		{
			Fuera.Add(NombreDep);
		}
	}
}

FString UAlsasuaCargadorPlugins::UrlDePlugin(const FString& Nombre)
{
	// 1) Mapa nombre->URL del subsistema, si el estado del plugin ya se conoce.
	FString URL;
	if (UGameFeaturesSubsystem::Get().GetPluginURLByName(Nombre, URL))
	{
		return URL;
	}

	// 2) Resolucion directa: protocolo File sobre el .uplugin. Evita la carrera
	//    con OnGameFeatureStatusKnown en mundos que arrancan antes de que los
	//    state machines de los plugins built-in reporten estado.
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(Nombre))
	{
		const FString Descriptor = Plugin->GetDescriptorFileName();
		if (!Descriptor.IsEmpty())
		{
			return UGameFeaturesSubsystem::GetPluginURL_FileProtocol(Descriptor);
		}
	}
	return FString();
}

void UAlsasuaCargadorPlugins::UrlsDePlugin(const FString& Nombre, TArray<FString>& Fuera)
{
	Fuera.Reset();
	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	// Variante canonica por nombre (la que usa la autoactivacion del .uproject).
	FString PorNombre;
	if (GFS.GetPluginURLByName(Nombre, PorNombre))
	{
		Fuera.AddUnique(PorNombre);
	}

	// Variante File:// sobre el .uplugin (la nuestra, sin depender del mapa).
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(Nombre))
	{
		const FString Descriptor = Plugin->GetDescriptorFileName();
		if (!Descriptor.IsEmpty())
		{
			Fuera.AddUnique(
				UGameFeaturesSubsystem::GetPluginURL_FileProtocol(Descriptor));
		}
	}
}

void UAlsasuaCargadorPlugins::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bPendienteDeArranque = true;
	PilaresConocidos = PluginsActivables;

	// El mundo se crea despues del GameInstance, asi que en el arranque normal
	// aqui todavia no hay: esperamos a OnPostWorldInitialization. Los
	// subsistemas World de los plugins se enganchan durante esa inicializacion,
	// antes de cualquier BeginPlay (y antes que DirectorArranque).
	TWeakObjectPtr<UAlsasuaCargadorPlugins> DebilThis(this);
	GanchoMundo = FWorldDelegates::OnPostWorldInitialization.AddLambda(
		[DebilThis](UWorld* Mundo, const UWorld::InitializationValues&)
		{
			if (DebilThis.IsValid())
			{
				DebilThis->AlIniciarMundo(Mundo);
			}
		});

	// Por si este subsistema nace con un mundo de juego ya corriendo
	// (recarga de blueprint, inyeccion tardia).
	if (UWorld* W = GetGameInstance()->GetWorld())
	{
		AlIniciarMundo(W);
	}
}

void UAlsasuaCargadorPlugins::AlIniciarMundo(UWorld* Mundo)
{
	// Criterio 9 automatizado: -AlsasuaFugaPilar=GF_Clima lanza la verificacion
	// de fuga en el primer mundo de juego persistente (el -game crea un mundo
	// "Untitled" efimero antes del mapa real; su timer moriria con el).
	FString NombreFuga;
	if (!bFugaLanzada && Mundo && Mundo->IsGameWorld() &&
		!Mundo->GetName().Contains(TEXT("Untitled")) &&
		FParse::Value(FCommandLine::Get(), TEXT("-AlsasuaFugaPilar="), NombreFuga))
	{
		bFugaLanzada = true;
		VerificarFugaPilar(NombreFuga, Mundo);
	}

	if (!bPendienteDeArranque || !Mundo || !Mundo->IsGameWorld())
	{
		return;
	}
	bPendienteDeArranque = false;
	ActivarTodos();
}

void UAlsasuaCargadorPlugins::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(GanchoMundo);
	DesactivarTodos();
	Super::Deinitialize();
}

int32 UAlsasuaCargadorPlugins::ActivarTodos()
{
	bPendienteDeArranque = false;

	// Override de linea de comandos para la matriz de activacion (Fase 5) y
	// depuracion: -AlsasuaPlugins=GF_Clima,GF_Audio  activa SOLO esos;
	// -AlsasuaPlugins=Ninguno apaga todo lo conocido. Sin el parametro,
	// manda DefaultGame.ini.
	// Nota: FParse::Value corta el token en la coma, se lee la linea cruda.
	const FString Linea = FCommandLine::Get();
	const int32 Prefijo = Linea.Find(TEXT("AlsasuaPlugins="), ESearchCase::IgnoreCase);
	if (Prefijo != INDEX_NONE)
	{
		const int32 Ini = Prefijo + FCString::Strlen(TEXT("AlsasuaPlugins="));
		int32 Fin = Linea.Len();
		for (int32 i = Ini; i < Linea.Len(); ++i)
		{
			if (FChar::IsWhitespace(Linea[i]))
			{
				Fin = i;
				break;
			}
		}
		const FString Lista = Linea.Mid(Ini, Fin - Ini);

		PluginsActivables.Reset();
		if (!Lista.Equals(TEXT("Ninguno"), ESearchCase::IgnoreCase))
		{
			Lista.ParseIntoArray(PluginsActivables, TEXT(","), true);
		}
		UE_LOG(LogCargadorPlugins, Log,
			TEXT("[Plugins] override de consola: %d activables."), PluginsActivables.Num());
	}

	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	// Combo dinamico: todo pilar conocido que no este en la lista efectiva se
	// apaga (Deactivate+Unload), venga de donde venga su activacion. Asi el
	// override de consola manda aunque el .uproject monte los plugins al
	// arrancar, y la matriz de activacion (Fase 5) es real.
	int32 Apagadas = 0;
	for (const FString& Pilar : PilaresConocidos)
	{
		if (PluginsActivables.Contains(Pilar))
		{
			continue;
		}
		TArray<FString> URLs;
		UrlsDePlugin(Pilar, URLs);
		bool bApagado = false;
		for (const FString& URL : URLs)
		{
			if (GFS.IsGameFeaturePluginActive(URL, /*bCheckForActivating*/true))
			{
				GFS.DeactivateGameFeaturePlugin(URL);
				bApagado = true;
			}
			// El montaje del .uproject deja la maquina en estados previos a
			// Activo: Unload desde ahi desmonta modulos y contenido igualmente.
			GFS.UnloadGameFeaturePlugin(URL);
			ActivadosAqui.Remove(URL);
		}
		if (bApagado || URLs.Num() > 0)
		{
			++Apagadas;
		}
	}
	if (Apagadas > 0)
	{
		UE_LOG(LogCargadorPlugins, Log,
			TEXT("[Plugins] %d pilares apagados por combo."), Apagadas);
	}

	int32 Lanzadas = 0;

	// Orden topologico por Dependencies del .uplugin: un pilar se activa solo
	// despues de sus dependencias activables, para que las DLL base ya esten
	// cargadas cuando el cargador importe al dependiente.
	TMap<FString, TSet<FString>> Deps;
	for (const FString& Nombre : PluginsActivables)
	{
		TSet<FString> Brutas;
		DependenciasDePlugin(Nombre, Brutas);
		TSet<FString> Set;
		for (const FString& Dep : Brutas)
		{
			if (PluginsActivables.Contains(Dep))
			{
				Set.Add(Dep);
			}
		}
		Deps.Add(Nombre, MoveTemp(Set));
	}

	TArray<FString> Orden;
	TSet<FString> Hechos;
	for (int32 i = 0; i < PluginsActivables.Num(); ++i)
	{
		for (const FString& Nombre : PluginsActivables)
		{
			if (Hechos.Contains(Nombre))
			{
				continue;
			}
			bool bTodas = true;
			for (const FString& Dep : Deps[Nombre])
			{
				if (!Hechos.Contains(Dep))
				{
					bTodas = false;
					break;
				}
			}
			if (bTodas)
			{
				Orden.Add(Nombre);
				Hechos.Add(Nombre);
			}
		}
	}
	for (const FString& Nombre : PluginsActivables)
	{
		if (!Hechos.Contains(Nombre)) // ciclo: se activa igualmente al final
		{
			Orden.Add(Nombre);
		}
	}

	LanzadasTotales = 0;
	ColaActivacion.Reset();
	for (const FString& Nombre : Orden)
	{
		TArray<FString> URLs;
		UrlsDePlugin(Nombre, URLs);
		if (URLs.Num() == 0)
		{
			UE_LOG(LogCargadorPlugins, Warning,
				TEXT("[Plugins] %s no existe como plugin del proyecto."),
				*Nombre);
			continue;
		}
		if (URLs.ContainsByPredicate([&GFS](const FString& URL)
			{
				return GFS.IsGameFeaturePluginActive(URL, /*bCheckForActivating*/true);
			}))
		{
			continue; // ya activo (autoactivacion): no cuenta ni relanza
		}
		ColaActivacion.Add(URLs[0]);
		++Lanzadas;
	}

	UE_LOG(LogCargadorPlugins, Log,
		TEXT("[Plugins] %d plugins GF lanzados de %d configurados."),
		Lanzadas, PluginsActivables.Num());
	ActivarSiguiente();
	return Lanzadas;
}

void UAlsasuaCargadorPlugins::ActivarSiguiente()
{
	if (ColaActivacion.IsEmpty())
	{
		return;
	}
	const FString URL = ColaActivacion[0];
	ColaActivacion.RemoveAt(0);

	UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(
		URL,
	FGameFeaturePluginLoadComplete::CreateWeakLambda(this,
		[this, URL](const UE::GameFeatures::FResult& Resultado)
			{
				if (Resultado.HasError())
				{
					UE_LOG(LogCargadorPlugins, Warning,
						TEXT("[Plugins] %s no activo: %s"),
						*URL, *Resultado.GetError());
				}
				else
				{
					++LanzadasTotales;
					ActivadosAqui.AddUnique(URL);
				}
				ActivarSiguiente();
			}));
}

void UAlsasuaCargadorPlugins::DesactivarTodos()
{
	// Deinitialize() de un GameInstanceSubsystem puede llegar durante el
	// cierre del editor, después de que el motor ya haya empezado a destruir
	// sus subsistemas de Engine (GameFeatures es uno). UGameFeaturesSubsystem::Get()
	// no comprueba null: sobre un puntero de motor ya destruido, cualquier
	// llamada revienta con access violation. Sin nada que desactivar tampoco
	// hace falta arriesgarse.
	if (IsEngineExitRequested() || !GEngine || ActivadosAqui.IsEmpty())
	{
		ActivadosAqui.Reset();
		return;
	}

	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	int32 Descargadas = 0;
	for (const FString& URL : ActivadosAqui)
	{
		if (GFS.IsGameFeaturePluginActive(URL))
		{
			GFS.DeactivateGameFeaturePlugin(URL);
			GFS.UnloadGameFeaturePlugin(URL);
			++Descargadas;
		}
	}
	UE_LOG(LogCargadorPlugins, Log, TEXT("[Plugins] %d plugins descargados."),
		Descargadas);
	ActivadosAqui.Reset();
}

void UAlsasuaCargadorPlugins::ActivarPilar(const FString& Nombre)
{
	if (Nombre.IsEmpty())
	{
		return;
	}
	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	// Dependencias transitivas primero: el modulo de un pilar dependiente no
	// importa si la DLL de su base no esta cargada (ExplicitlyLoaded).
	TArray<FString> Cadena;
	TSet<FString> Vistos;
	TFunction<void(const FString&)> Visitar = [&](const FString& Actual)
	{
		if (Actual.IsEmpty() || !Actual.StartsWith(TEXT("GF_")) ||
			Vistos.Contains(Actual))
		{
			return;
		}
		Vistos.Add(Actual);
		TSet<FString> Deps;
		DependenciasDePlugin(Actual, Deps);
		for (const FString& Dep : Deps)
		{
			Visitar(Dep);
		}
		Cadena.AddUnique(Actual); // postorden: bases antes que dependientes
	};
	Visitar(Nombre);

	int32 EnCola = 0;
	for (const FString& Pilar : Cadena)
	{
		TArray<FString> URLs;
		UrlsDePlugin(Pilar, URLs);
		if (URLs.Num() == 0)
		{
			UE_LOG(LogCargadorPlugins, Warning,
				TEXT("[Plugins] ActivarPilar: %s no existe como plugin."),
				*Pilar);
			continue;
		}
		const bool bActivo = URLs.ContainsByPredicate([&GFS](const FString& URL)
			{
				return GFS.IsGameFeaturePluginActive(URL, /*bCheckForActivating*/true);
			});
		if (!bActivo)
		{
			ColaActivacion.Add(URLs[0]);
			++EnCola;
		}
	}
	UE_LOG(LogCargadorPlugins, Log,
		TEXT("[Plugins] ActivarPilar %s: %d en cola (%d con dependencias)."),
		*Nombre, EnCola, Cadena.Num());
	ActivarSiguiente();
}

void UAlsasuaCargadorPlugins::DesactivarPilar(const FString& Nombre)
{
	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	TArray<FString> URLs;
	UrlsDePlugin(Nombre, URLs);

	// Criterio 9: Deactivate/Unload de GFP no revoca los subsistemas ni los
	// componentes que el pilar coloco en mundos ya vivos (sobreviven al GC
	// por su cadena de Outer). Quitarlos aqui, generico por paquete
	// /Script/<Nombre>, mientras el modulo sigue cargado.
	const FString Paquete = TEXT("/Script/") + Nombre;
	TArray<UClass*> ClasesSubsistema;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->GetOutermost() && It->GetOutermost()->GetName() == Paquete &&
			It->IsChildOf<USubsystem>())
		{
			ClasesSubsistema.Add(*It);
		}
	}
	for (UClass* Clase : ClasesSubsistema)
	{
		FSubsystemCollectionBase::DeactivateExternalSubsystem(Clase);
	}
	for (TObjectIterator<UActorComponent> It; It; ++It)
	{
		if (!It->HasAnyFlags(RF_ClassDefaultObject | RF_BeginDestroyed) &&
			It->GetClass()->GetOutermost() &&
			It->GetClass()->GetOutermost()->GetName() == Paquete)
		{
			It->DestroyComponent();
		}
	}

	int32 Apagadas = 0;
	for (const FString& URL : URLs)
	{
		if (GFS.IsGameFeaturePluginActive(URL))
		{
			GFS.DeactivateGameFeaturePlugin(URL);
			GFS.UnloadGameFeaturePlugin(URL);
			++Apagadas;
		}
		ActivadosAqui.Remove(URL);
	}
	UE_LOG(LogCargadorPlugins, Log,
		TEXT("[Plugins] DesactivarPilar %s: %d descargados."), *Nombre, Apagadas);
}

void UAlsasuaCargadorPlugins::VerificarFugaPilar(const FString& Nombre, UWorld* Mundo)
{
	// Objetos cuyo paquete de clase es /Script/<Nombre>: instancias y CDO del
	// modulo. Tras Deactivate+Unload+GC no debe quedar ninguno.
	if (!Mundo)
	{
		return;
	}
	FugaPendiente = Nombre;
	Mundo->GetTimerManager().SetTimer(MangoFuga,
		FTimerDelegate::CreateUObject(this, &UAlsasuaCargadorPlugins::SondeoFuga,
			TWeakObjectPtr<UWorld>(Mundo)),
		0.5f, /*bLoop*/true);
}

void UAlsasuaCargadorPlugins::SondeoFuga(TWeakObjectPtr<UWorld> MundoDebil)
{
	UWorld* Mundo = MundoDebil.Get();
	if (!Mundo)
	{
		GetGameInstance()->GetTimerManager().ClearTimer(MangoFuga);
		FugaPendiente.Reset();
		return;
	}

	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();
	const FString URL = UrlDePlugin(FugaPendiente);
	if (!GFS.IsGameFeaturePluginActive(URL, /*bCheckForActivating*/true))
	{
		// La activacion es async: esperar a Active o el conteo saldria de un
		// pilar a medio nacer y daria LIMPIO falso.
		if (++IntentosFuga > 60) // 30 s a 0.5 s
		{
			UE_LOG(LogCargadorPlugins, Warning,
				TEXT("[Fuga] %s: no llego a Active en 30 s; prueba abortada."),
				*FugaPendiente);
			Mundo->GetTimerManager().ClearTimer(MangoFuga);
			FugaPendiente.Reset();
			IntentosFuga = 0;
		}
		return;
	}
	Mundo->GetTimerManager().ClearTimer(MangoFuga);
	IntentosFuga = 0;

	auto ObjetosDelModulo = [](const FString& Modulo)
	{
		const FString Paquete = TEXT("/Script/") + Modulo;
		int32 N = 0;
		for (TObjectIterator<UObject> It; It; ++It)
		{
			if (const UPackage* P = It->GetClass()->GetOutermost();
				P && P->GetName() == Paquete)
			{
				++N;
			}
		}
		return N;
	};

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	const int32 Antes = ObjetosDelModulo(FugaPendiente);

	DesactivarPilar(FugaPendiente);
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS); // 1a: destruye
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS); // 2a: purga pendientes

	// Red de seguridad: alguna instancia puede quedar fuera de toda coleccion
	// (RemoveAllInstances salta las que no tienen dueno). Marcarlas para que
	// el siguiente GC las recoja; si algo las referenciaba de verdad, el
	// conteo lo delata como RESIDUO.
	const FString PaqueteFuga = TEXT("/Script/") + FugaPendiente;
	int32 Huerfanos = 0;
	for (TObjectIterator<USubsystem> It; It; ++It)
	{
		if (!It->IsUnreachable() &&
			It->GetClass()->GetOutermost() &&
			It->GetClass()->GetOutermost()->GetName() == PaqueteFuga)
		{
			It->MarkAsGarbage();
			++Huerfanos;
		}
	}
	if (Huerfanos > 0)
	{
		UE_LOG(LogCargadorPlugins, Warning,
			TEXT("[Fuga] %s: %d subsistema(s) huerfano(s) fuera de coleccion, marcados."),
			*FugaPendiente, Huerfanos);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}

	const int32 Despues = ObjetosDelModulo(FugaPendiente);
	if (Despues > 0)
	{
		const FString PaqueteResiduo = TEXT("/Script/") + FugaPendiente;
		for (TObjectIterator<UObject> It; It; ++It)
		{
			if (const UPackage* P = It->GetClass()->GetOutermost();
				P && P->GetName() == PaqueteResiduo)
			{
				UWorld* MundoResiduo = GetGameInstance()->GetWorld();
				const bool bEnColeccion = MundoResiduo &&
					MundoResiduo->GetSubsystemBase(It->GetClass()) == *It;
				UE_LOG(LogCargadorPlugins, Log,
					TEXT("[Fuga] residuo: %s flags=%d enColeccion=%d"),
					*It->GetFullName(),
					(uint32)It->GetFlags(),
					bEnColeccion ? 1 : 0);
			}
		}
	}
	UE_LOG(LogCargadorPlugins, Log,
		TEXT("[Fuga] %s: antes=%d despues=%d %s"),
		*FugaPendiente, Antes, Despues,
		Despues == 0 ? TEXT("LIMPIO") : TEXT("RESIDUO"));
	FugaPendiente.Reset();
}

static UAlsasuaCargadorPlugins* CargadorDelMundo(UWorld* Mundo)
{
	return Mundo && Mundo->GetGameInstance()
		? Mundo->GetGameInstance()->GetSubsystem<UAlsasuaCargadorPlugins>()
		: nullptr;
}

FAutoConsoleCommandWithWorldAndArgs GCmdDesactivarPilar(
	TEXT("Alsasua.Pilar.Desactivar"),
	TEXT("Descarga un pilar GF_* en runtime. Uso: Alsasua.Pilar.Desactivar GF_Clima"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* Mundo)
		{
			if (UAlsasuaCargadorPlugins* C = CargadorDelMundo(Mundo); C && Args.Num() > 0)
			{
				C->DesactivarPilar(Args[0]);
			}
		}));

FAutoConsoleCommandWithWorldAndArgs GCmdFugaPilar(
	TEXT("Alsasua.Pilar.Fuga"),
	TEXT("Criterio 9: GC, desactiva el pilar, GC x2 y compara conteo de objetos. Uso: Alsasua.Pilar.Fuga GF_Clima"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* Mundo)
		{
			if (UAlsasuaCargadorPlugins* C = CargadorDelMundo(Mundo); C && Args.Num() > 0)
			{
				C->VerificarFugaPilar(Args[0], Mundo);
			}
		}));

FAutoConsoleCommandWithWorldAndArgs GCmdActivarPilar(
	TEXT("Alsasua.Pilar.Activar"),
	TEXT("Activa un pilar GF_* y sus dependencias en runtime. Uso: Alsasua.Pilar.Activar GF_Clima"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* Mundo)
		{
			if (UAlsasuaCargadorPlugins* C = CargadorDelMundo(Mundo); C && Args.Num() > 0)
			{
				C->ActivarPilar(Args[0]);
			}
		}));
