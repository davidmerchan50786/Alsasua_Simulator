#include "Plugins/AlsasuaCargadorPlugins.h"
#include "GameFeaturesSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogCargadorPlugins, Log, All);

void UAlsasuaCargadorPlugins::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bPendienteDeArranque = true;

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

	UGameFeaturesSubsystem& GFS = UGameFeaturesSubsystem::Get();

	int32 Lanzadas = 0;
	for (const FString& Nombre : PluginsActivables)
	{
		FString URL;
		if (!GFS.GetPluginURLByName(Nombre, URL))
		{
			UE_LOG(LogCargadorPlugins, Warning,
				TEXT("[Plugins] %s no existe como plugin del proyecto."),
				*Nombre);
			continue;
		}
		if (GFS.IsGameFeaturePluginActive(URL, /*bCheckForActivating*/true))
		{
			continue;
		}

		++Lanzadas;
		GFS.LoadAndActivateGameFeaturePlugin(
			URL,
			FGameFeaturePluginLoadComplete::CreateLambda(
				[Nombre](const UE::GameFeatures::FResult& Resultado)
				{
					if (Resultado.HasError())
					{
						UE_LOG(LogCargadorPlugins, Warning,
							TEXT("[Plugins] %s no activo: %s"),
							*Nombre, *Resultado.GetError());
					}
				}));
		ActivadosAqui.AddUnique(URL);
	}

	UE_LOG(LogCargadorPlugins, Log,
		TEXT("[Plugins] %d plugins GF lanzados de %d configurados."),
		Lanzadas, PluginsActivables.Num());
	return Lanzadas;
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
