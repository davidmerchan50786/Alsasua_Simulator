#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ManifaFormationManager.generated.h"

UENUM(BlueprintType)
enum class EFormationType : uint8 {
    Line,
    Wedge,
    Column,
    Circle
};

USTRUCT(BlueprintType)
struct FFormationData {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EFormationType Type = EFormationType::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Spacing = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxAgents = 20;
};

UCLASS()
class ALSASUAMANIFA_API UManifaFormationManager : public UWorldSubsystem {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="AAA|Manifa")
    void CreateFormation(AActor* Leader, EFormationType Type);

    UFUNCTION(BlueprintCallable, Category="AAA|Manifa")
    void UpdateFormations(float DeltaTime);

private:
    TMap<AActor*, TArray<AActor*>> ActiveFormations;
    TMap<AActor*, EFormationType> FormationTypes;
    TArray<FVector> CalculateFormationOffsets(EFormationType Type, int32 Count, float Spacing);
};
