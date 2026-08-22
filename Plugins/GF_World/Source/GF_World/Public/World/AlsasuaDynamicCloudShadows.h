#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaDynamicCloudShadows.generated.h"

/**
 * Proyecta sombras de nubes en movimiento sobre el terreno.
 * Usa un material dinámico con textura de nubes que se desplaza.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaDynamicCloudShadows : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaDynamicCloudShadows();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Cloud Shadow ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shadow")
	float CloudShadowIntensity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shadow")
	float CloudSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shadow")
	float CloudScale = 0.0001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shadow")
	float CloudDetailScale = 0.001f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shadow")
	float CloudDetailSpeed = 30.f;

	// --- Cloud Cover ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cover")
	float CloudCoverDay = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cover")
	float CloudCoverNight = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Cover")
	float CloudCoverRain = 0.9f;

	// --- Cloud Tint ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Tint")
	FLinearColor CloudColorDay = FLinearColor(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Tint")
	FLinearColor CloudColorSunset = FLinearColor(1.f, 0.7f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Tint")
	FLinearColor CloudColorNight = FLinearColor(0.1f, 0.12f, 0.2f);

private:
	void UpdateCloudShadows(float DeltaTime);

	float TimeAccum = 0.f;
	float CurrentCover = 0.4f;
};
