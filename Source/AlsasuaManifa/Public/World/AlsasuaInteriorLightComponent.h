#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaInteriorLightComponent.generated.h"

class UPointLightComponent;

/**
 * Componente de luz interior para edificios. Genera luces puntuales
 * dentro de ventanas visibles desde el exterior, creando la ilusión
 * de habitaciones iluminadas de noche.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaInteriorLightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaInteriorLightComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Config ---
	/**
	 * Techo de luces por planta y de plantas. Son TOPES, no cantidades fijas:
	 * antes se creaban MaxFloors * NumLightsPerFloor luces en todos los
	 * edificios, midiera lo que midiera el edificio. Con los 1030 footprints del
	 * pueblo eso son 12 360 UPointLightComponent, y a un caserío de una planta
	 * le salían cuatro pisos de luces flotando sobre el tejado.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	int32 NumLightsPerFloor = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	int32 MaxFloors = 4;

	/**
	 * Se lo pasa ADirectorArranque antes de RegisterComponent, igual que el
	 * Barrio de UAlsasuaBarrioStyleSystem: MANIFA no puede ver a
	 * AEdificioGenerado porque la dependencia va WORLD → MANIFA. Sin esto el
	 * componente no sabe ni lo alto ni lo ancho que es su edificio.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interior|Light")
	void Configurar(int32 EnPlantas, float EnAnchoCm, int32 EnSemilla);

	/** Más allá de esto no se encienden ni se tican. El pueblo son 7,2 km y de
	 *  noche se veían las 12 000 luces a la vez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float DistanciaMaximaCm = 25000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float FloorHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float LightIntensity = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float LightRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	FLinearColor WarmLightColor = FLinearColor(1.f, 0.85f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	FLinearColor CoolLightColor = FLinearColor(0.7f, 0.8f, 1.f);

	// --- Behavior ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float OnProbability = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float TurnOnHour = 17.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float TurnOffHour = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float WarmCoolBlend = 0.5f;

	// --- Activity (walking lights) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	bool bEnableActivity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	float ActivitySpeed = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	float ActivityRadius = 80.f;

private:
	void SetupInteriorLights();

	int32 PlantasReales = 0;   // del edificio, se las pasa el director
	float AnchoCm = 0.f;
	int32 Semilla = 0;

	/** Última intensidad y color escritos. Escribir en un componente de luz
	 *  invalida estado de render, así que sólo se escribe cuando cambia de
	 *  verdad — CLAUDE.md §8.2. Antes se llamaba a SetIntensity y SetLightColor
	 *  para las 12 360 luces cinco veces por segundo, también de día con la
	 *  intensidad clavada en cero. */
	TArray<float> UltimaIntensidad;
	bool bLejos = false;
	void UpdateInteriorLights(float DeltaTime);

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> InteriorLights;

	UPROPERTY()
	TArray<bool> LightActive;

	UPROPERTY()
	TArray<float> LightFlickerPhase;

	float CurrentBlend = 0.f;
	bool bInitialized = false;
};
