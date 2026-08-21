#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaBuildingFacadeSystem.generated.h"

/**
 * Sistema de fachadas procedurales: ventanas, puertas, escaparates.
 * Genera geometría de fachada basada en el tipo de edificio.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaBuildingFacadeSystem : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaBuildingFacadeSystem();

	virtual void BeginPlay() override;

	// --- Building Type ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Type")
	bool bIsResidential = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Type")
	bool bIsCommercial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Type")
	bool bIsIndustrial = false;

	// --- Windows ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowWidth = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowHeight = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowSpacingX = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowSpacingY = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowSillDepth = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Windows")
	float WindowFrameWidth = 5.f;

	// --- Doors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Doors")
	float DoorWidth = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Doors")
	float DoorHeight = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Doors")
	float DoorSpacing = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Doors")
	float DoorFrameWidth = 8.f;

	// --- Shop Fronts ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shop")
	float ShopFrontWidth = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shop")
	float ShopFrontHeight = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shop")
	float ShopGlassHeight = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Shop")
	float ShopSignHeight = 30.f;

	// --- Balconies ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyDepth = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyRailHeight = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Balconies")
	float BalconyRailSpacing = 20.f;

	// --- Materials ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor WindowGlassColor = FLinearColor(0.3f, 0.4f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor WindowFrameColor = FLinearColor(0.8f, 0.8f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor DoorColor = FLinearColor(0.4f, 0.25f, 0.1f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor ShopFrontColor = FLinearColor(0.9f, 0.9f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facade|Materials")
	FLinearColor BalconyColor = FLinearColor(0.3f, 0.3f, 0.3f);

private:
	void SpawnFacadeElements();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedElements;
};
