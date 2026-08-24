// AlsasuaTrafficLightSystem.h (capa MANIFA)
// Semáforos en las intersecciones reales de roads_unity.json, con su ciclo.
//
// Estaba escrito pero la fase 46 de ADirectorArranque no lo llamaba: se saltaba
// con un log que decía "skip semáforos para perfilado". Y colocarlos tal cual
// tampoco habría estado bien: encendía los tres LEDs a la vez —rojo, ámbar y
// verde simultáneos— con una APointLight por cada uno, o sea 36 luces para 12
// semáforos, y el bActivo de la estructura no lo leía nadie porque no había
// ciclo ninguno.
//
// Ahora es una luz por semáforo, cuyo color e intensidad cambian con la fase, y
// un ciclo de verdad con desfase por cruce para que no cambien todos a la vez.
// El tick va a 4 Hz como el resto de lo que toca render en este proyecto.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaTrafficLightSystem.generated.h"

class APointLight;

UENUM(BlueprintType)
enum class EFaseSemaforo : uint8
{
	Verde,
	Ambar,
	Rojo,
};

USTRUCT(BlueprintType)
struct FTrafficLight
{
	GENERATED_BODY()
	FVector Posicion = FVector::ZeroVector;
	float Rotacion = 0.0f;
	FString Calle;
	FString Barrio;
	bool bActivo = true;

	/** Segundos de desfase respecto al ciclo global. Sin esto los doce cruces
	 *  del pueblo cambiarían de color a la vez, que no pasa en ningún sitio. */
	float Desfase = 0.f;

	EFaseSemaforo Fase = EFaseSemaforo::Rojo;

	UPROPERTY() TObjectPtr<APointLight> Luz = nullptr;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaTrafficLightSystem : public UGameInstanceSubsystem, public FTickableGameObject, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override;
	virtual int32 OrdenArranque() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|TrafficLights")
	int32 ColocarSemaforos();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|TrafficLights")
	int32 MaxSemaforos = 12;

	/** Duración de cada fase (s). El ámbar corto, como en la calle. */
	UPROPERTY(EditAnywhere, Category = "Alsasua|TrafficLights") float SegVerde = 18.f;
	UPROPERTY(EditAnywhere, Category = "Alsasua|TrafficLights") float SegAmbar = 3.f;
	UPROPERTY(EditAnywhere, Category = "Alsasua|TrafficLights") float SegRojo  = 20.f;

	/** Fase de un cruce en un instante dado del ciclo. */
	EFaseSemaforo FaseEn(float TiempoCiclo) const;

	const TArray<FTrafficLight>& GetSemaforos() const { return Semaforos; }

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAlsasuaTrafficLightSystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate() && Semaforos.Num() > 0; }

private:
	/** Vuelca la fase de cada cruce a su luz. */
	void Aplicar();

	UPROPERTY() TArray<FTrafficLight> Semaforos;
	float Reloj = 0.f;
	float DesdeUltimoRefresco = 0.f;
};
