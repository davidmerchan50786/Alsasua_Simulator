#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaBuildingEmissiveComponent.generated.h"

/**
 * Componente que controla el brillo emissivo de ventanas de edificios según la hora del día.
 * Se añade a cada edificio generado y actualiza los materiales dinámicamente.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_EDIFICIOS_API UAlsasuaBuildingEmissiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaBuildingEmissiveComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	FLinearColor WindowColorWarm = FLinearColor(1.0f, 0.85f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	FLinearColor WindowColorCool = FLinearColor(0.7f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	FLinearColor WindowOffColor = FLinearColor(0.02f, 0.02f, 0.03f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	float MaxEmissiveIntensity = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	float NightOnProbability = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	float FlickerSpeed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	int32 WindowRows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Emissive")
	int32 WindowCols = 4;

private:
	void UpdateEmissiveWindows(float DeltaTime);
	void SetupWindowMaterials();

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WindowMaterials;

	float CurrentEmissiveStrength = 0.f;
	float FlickerPhase = 0.f;
	TArray<bool> WindowOnOff;
	TArray<float> WindowColorTemp;  // Kelvin por ventana (2700-6500K)
	bool bInitialized = false;

	static FLinearColor ColorTempToRGB(float Kelvin);
};
