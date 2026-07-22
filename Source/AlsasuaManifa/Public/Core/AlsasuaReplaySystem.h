#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaReplaySystem.generated.h"

USTRUCT(BlueprintType)
struct FReplaySnapshot
{
    GENERATED_BODY()

    UPROPERTY()
    float TimeStamp;

    // Posiciones de los líderes y jugadores
    UPROPERTY()
    TMap<AActor*, FTransform> ActorStates;

    // Estado global de la manifa en este instante
    UPROPERTY()
    float GlobalTension;
};

/** Sistema de persistencia para rebobinar o analizar la manifestación */
UCLASS()
class ALSASUAMANIFA_API UAlsasuaReplaySystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void StartRecording();

    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void StopRecording();

    // Exporta la sesión a un archivo binario ligero
    UFUNCTION(BlueprintCallable, Category = "AAA|Replay")
    void SaveReplayToFile(FString FileName);

private:
    bool bIsRecording = false;
    TArray<FReplaySnapshot> CurrentSession;

    void CaptureFrame();
};
