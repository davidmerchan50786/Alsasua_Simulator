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
    FLinearColor GradingColor; // Tinte de color: Cálido (victoria), Frío/Azul (represión), Rojo (caos)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionEndingReached, FEndGameCinematicData, CinematicData);

UCLASS()
class ALSASUAMANIFA_API ULegacySubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Dispara el cierre del juego basándose en la "huella" dejada por el jugador
    UFUNCTION(BlueprintCallable, Category="AAA|Legacy")
    void TriggerMissionEnd();

    UPROPERTY(BlueprintAssignable)
    FOnMissionEndingReached OnMissionEndingReached;
};