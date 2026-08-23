#include "Plugins/AlsasuaCargadorPlugins.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "GameFeaturesSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
