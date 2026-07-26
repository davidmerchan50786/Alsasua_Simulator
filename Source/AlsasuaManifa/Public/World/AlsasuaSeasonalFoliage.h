#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaSeasonalFoliage.generated.h"

/**
 * Cambia colores de foliage según la estación del año.
 * Modifica materiales dinámicos de árboles y arbustos.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaSeasonalFoliage : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaSeasonalFoliage();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Season Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Spring")
	FLinearColor SpringLeafColor = FLinearColor(0.3f, 0.7f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Spring")
	float SpringSaturation = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Summer")
	FLinearColor SummerLeafColor = FLinearColor(0.15f, 0.55f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Summer")
	float SummerSaturation = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Autumn")
	FLinearColor AutumnLeafColor1 = FLinearColor(0.8f, 0.4f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Autumn")
	FLinearColor AutumnLeafColor2 = FLinearColor(0.9f, 0.2f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Autumn")
	FLinearColor AutumnLeafColor3 = FLinearColor(0.6f, 0.5f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Autumn")
	float AutumnSaturation = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Winter")
	FLinearColor WinterLeafColor = FLinearColor(0.25f, 0.2f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Winter")
	float WinterSaturation = 0.4f;

	// --- Tree-specific ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Trees")
	float BeechAutumnProbability = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Trees")
	float OakAutumnProbability = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Trees")
	float BirchAutumnProbability = 0.9f;

	// --- Grass ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Grass")
	FLinearColor GrassColorSpring = FLinearColor(0.25f, 0.65f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Grass")
	FLinearColor GrassColorSummer = FLinearColor(0.2f, 0.5f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Grass")
	FLinearColor GrassColorAutumn = FLinearColor(0.45f, 0.4f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Season|Grass")
	FLinearColor GrassColorWinter = FLinearColor(0.3f, 0.28f, 0.2f);

private:
	void UpdateSeasonalColors(float DeltaTime);

	float CurrentTimeYear = 0.f;
};
