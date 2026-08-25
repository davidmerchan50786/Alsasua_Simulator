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
#include "Engine/EngineTypes.h"
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

	/** Activa un pilar y sus dependencias GF_* en runtime (criterio del plan
	 *  §9: activar/desactivar sin reiniciar). Asíncrono vía cola común. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Plugins")
	void ActivarPilar(const FString& Nombre);

	/** Descarga un pilar en runtime: Deactivate+Unload de sus URLs. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Plugins")
	void DesactivarPilar(const FString& Nombre);

	/** Criterio §9: espera a que el pilar esté Active, cuenta objetos C++ del
	 *  módulo, desactiva, GC x2 y vuelve a contar. Log
	 *  "[Fuga] X: antes=N despues=0 LIMPIO|RESIDUO". */
	void VerificarFugaPilar(const FString& Nombre, UWorld* Mundo);

	/** "GF_Clima" -> URL de plugin del proyecto (via GetPluginURLByName). */
	static FString UrlDePlugin(const FString& Nombre);
	static void UrlsDePlugin(const FString& Nombre, TArray<FString>& Fuera);

private:
	UPROPERTY(Config)
	TArray<FString> PluginsActivables;

	/** Copia del ini sin el override de consola: define el universo de
	 *  pilares que este cargador puede apagar en un combo. */
	TArray<FString> PilaresConocidos;

	/** URLs cuyo ciclo de vida gestiona esta clase (para no desactivar
	 *  plugins que otro activó). */
	TArray<FString> ActivadosAqui;

	/** Cola de la activación secuencial: dependencias primero, para que las
	 *  DLL de los pilares base estén cargadas cuando las importen los
	 *  dependientes. */
	TArray<FString> ColaActivacion;
	int32 LanzadasTotales = 0;
	void ActivarSiguiente();

	bool bPendienteDeArranque = false;

	/** La verificacion de fuga (-AlsasuaFugaPilar=) se lanza una sola vez. */
	bool bFugaLanzada = false;

	/** Estado del sondeo de fuga: el nombre vive aqui y no en el delegado
	 *  del timer (las capturas grandes viajan mal hasta su primer tick). */
	FString FugaPendiente;
	int32 IntentosFuga = 0;
	FTimerHandle MangoFuga;
	void SondeoFuga(TWeakObjectPtr<UWorld> MundoDebil);

	/** Gancho a FWorldDelegates::OnPostWorldInitialization: el primer mundo de
	 *  juego dispara la activación. */
	void AlIniciarMundo(UWorld* Mundo);

	FDelegateHandle GanchoMundo;
};
