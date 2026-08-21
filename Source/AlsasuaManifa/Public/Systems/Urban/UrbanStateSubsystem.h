#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UrbanStateSubsystem.generated.h"

UENUM(BlueprintType)
enum class ESectorTension : uint8 {
    Calm,
    Uneasy,
    Disturbance,
    Riot,
    MartialLaw
};

USTRUCT(BlueprintType)
struct FSectorState {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName SectorName;

    UPROPERTY(BlueprintReadWrite)
    float TensionLevel = 0.f; // 0 a 100

    UPROPERTY(BlueprintReadOnly)
    ESectorTension CurrentState = ESectorTension::Calm;

    UPROPERTY(BlueprintReadWrite)
    bool bControlledByPolice = true;
};

UCLASS()
class ALSASUAMANIFA_API UUrbanStateSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="AAA|Urban")
    void IncreaseTension(FName SectorName, float Amount);

    UFUNCTION(BlueprintPure, Category="AAA|Urban")
    FSectorState GetSectorState(FName SectorName) const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSectorStateChanged, FName, SectorName, ESectorTension, NewState);
    UPROPERTY(BlueprintAssignable)
    FOnSectorStateChanged OnSectorStateChanged;

private:
    UPROPERTY()
    TMap<FName, FSectorState> Sectors;

    // Había aquí un UpdateSectorVisuals(FName) que no definía nadie. Y era
    // redundante además de muerto: la respuesta visual al cambio de tensión
    // la da OnSectorStateChanged, que IncreaseTension ya emite.
};