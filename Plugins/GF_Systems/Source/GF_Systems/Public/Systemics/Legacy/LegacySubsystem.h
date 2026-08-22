#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LegacySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FEndGameCinematicData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FText CinematicTitle;

    UPROPERTY(BlueprintReadOnly)
    FName SequenceToPlay; // El nombre del level sequence que el artista debe disparar

    UPROPERTY(BlueprintReadOnly)
    FLinearColor GradingColor = FLinearColor::White; // Tinte de color: Cálido (victoria), Frío/Azul (represión), Rojo (caos)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEndingReached, FEndGameCinematicData, CinematicData);

UCLASS()
class GF_SYSTEMS_API ULegacySubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Dispara el cierre del juego basándose en la "huella" dejada por el jugador
    UFUNCTION(BlueprintCallable, Category="AAA|Legacy")
    void TriggerMissionEnd();

    UPROPERTY(BlueprintAssignable)
    FOnMissionEndingReached OnMissionEndingReached;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Legacy")
    float FinalPopularSupport = 50.f;
};