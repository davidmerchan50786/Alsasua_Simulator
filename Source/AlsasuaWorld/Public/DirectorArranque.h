#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectorArranque.generated.h"

class UAlsasuaNPCPedestrianSystem;
class UAlsasuaDynamicTrafficSystem;
class UAlsasuaWeatherSystem;
class UAlsasuaAmbientAudioSystem;

UCLASS()
class ALSASUAWORLD_API ADirectorArranque : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|World")
    void IniciarConstruccion();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Arranque")
    bool bCargarArboles = true;

private:
    bool bConstruccionCompleta = false;
    UPROPERTY() UAlsasuaNPCPedestrianSystem* NPCSystemRef = nullptr;
    UPROPERTY() UAlsasuaDynamicTrafficSystem* TrafficSystemRef = nullptr;
    UPROPERTY() UAlsasuaWeatherSystem* WeatherSystemRef = nullptr;
    UPROPERTY() UAlsasuaAmbientAudioSystem* AudioSystemRef = nullptr;
};
