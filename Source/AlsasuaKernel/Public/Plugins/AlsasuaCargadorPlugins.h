// AlsasuaCargadorPlugins.h
// Activa y desactiva de verdad los plugins GF_* en runtime vía el sistema
// GameFeatures: montar el bundle carga la DLL del módulo, instancia sus
// subsistemas y registra sus componentes; desactivar hace lo contrario.
//
// La lista vive en DefaultGame.ini ([/Script/AlsasuaKernel.AlsasuaCargadorPlugins],
// PluginsActivables=...). Sin entrada no se toca nada, así que un plugin puede
// quedarse compilado pero dormido.
//
// La activación es asíncrona por dentro; aquí se encadena una tras otra para
// que el log del resumen diga cuántas quedaron Active de verdad. Los
// subsistemas World de cada plugin se enganchan cuando su bundle llega a
// Active, aunque el mundo ya esté corriendo.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaCargadorPlugins.generated.h"

UCLASS(Config = Game)
class ALSASUAKERNEL_API UAlsasuaCargadorPlugins : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Activa todos los PluginsActivables del ini. Idempotente: los que ya
	 *  estan Active se saltan. Devuelve los que se pusieron en marcha. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Plugins")
	int32 ActivarTodos();

	/** Desactiva lo que este subsistema activó. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Plugins")
	void DesactivarTodos();

	/** "GF_Clima" -> URL de plugin del proyecto (via GetPluginURLByName). */
	static FString UrlDePlugin(const FString& Nombre);

private:
	UPROPERTY(Config)
	TArray<FString> PluginsActivables;

	/** URLs cuyo ciclo de vida gestiona esta clase (para no desactivar
	 *  plugins que otro activó). */
	TArray<FString> ActivadosAqui;

	bool bPendienteDeArranque = false;

	/** Gancho a FWorldDelegates::OnPostWorldInitialization: el primer mundo de
	 *  juego dispara la activación. */
	void AlIniciarMundo(UWorld* Mundo);

	FDelegateHandle GanchoMundo;
};
