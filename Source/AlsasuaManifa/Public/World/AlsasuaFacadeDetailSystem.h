#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaFacadeDetailSystem.generated.h"

/**
 * Sistema de detalles de fachada: balcones, persianas, macetas.
 * Añade micro-detalle a los edificios para dar realismo.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaFacadeDetailSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaFacadeDetailSystem();

	virtual void BeginPlay() override;

	// --- Balconies ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	bool bEnableBalconies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyProbability = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconySpacing = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyDepth = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyWidth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyHeight = 10.f;

	// --- Shutters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shutters")
	bool bEnableShutters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shutters")
	float ShutterProbability = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shutters")
	float ShutterWidth = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shutters")
	float ShutterHeight = 80.f;

	// --- Flower Pots ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Flowers")
	bool bEnableFlowerPots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Flowers")
	float FlowerPotProbability = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Flowers")
	float FlowerPotSize = 15.f;

	// --- Awnings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Awnings")
	bool bEnableAwnings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Awnings")
	float AwningProbability = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Awnings")
	float AwningWidth = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Awnings")
	float AwningProjection = 60.f;

	// --- AC Units ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|AC")
	bool bEnableACUnits = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|AC")
	float ACProbability = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|AC")
	float ACSize = 40.f;

	// --- Materials ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor ShutterColorBrown = FLinearColor(0.35f, 0.2f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor ShutterColorGreen = FLinearColor(0.1f, 0.35f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor ShutterColorBlue = FLinearColor(0.15f, 0.25f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor AwningColorRed = FLinearColor(0.7f, 0.1f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor AwningColorStripe = FLinearColor(0.9f, 0.9f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor FlowerColor1 = FLinearColor(0.9f, 0.2f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor FlowerColor2 = FLinearColor(1.f, 0.85f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor FlowerColor3 = FLinearColor(0.8f, 0.1f, 0.6f);

private:
	void SpawnFacadeDetails();

	int32 SpawnedDetailCount = 0;
};
