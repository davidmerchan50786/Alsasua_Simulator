#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaPhotoComponent.generated.h"

USTRUCT(BlueprintType)
struct FPhotoSubject
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    AActor* Actor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    float Importance = 0.f;

    UPROPERTY(BlueprintReadOnly)
    FString Description;
};

/** Sistema de captura de hitos visuales para impacto social */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaPhotoComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaPhotoComponent();

    // Intenta tomar una foto de lo que hay frente a la cámara
    UFUNCTION(BlueprintCallable, Category = "AAA|Photo")
    void TakeSocialPhoto();

    // ¿Está el jugador mirando a través del visor?
    UPROPERTY(BlueprintReadWrite, Category = "AAA|Photo")
    bool bIsAimingCamera = false;

protected:
    // Detecta actores importantes en el cono de visión
    void AnalyzeFrame(TArray<FPhotoSubject>& OutSubjects);
};
