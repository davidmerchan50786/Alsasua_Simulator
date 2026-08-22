// AlsasuaNightLightingSystem.h (capa MANIFA)
// Lleva el factor de noche del pueblo: 0 de día, 1 con la noche cerrada, con su
// transición al amanecer y al anochecer, contra el reloj de UTimeOfDayManager.
//
// Lleva el factor y nada más. Escribía además el emissive de las ventanas de
// todos los edificios, y eso ahora es de UAlsasuaBuildingEmissiveComponent, que
// va uno por AEdificioGenerado desde la fase 19. Dos escritores sobre el mismo
// parámetro de material es exactamente el fallo que este fichero ya describía
// para las farolas —"las dos escrituras se pisaban y todo el alumbrado latía"—,
// así que se queda con un dueño.
//
// Y de todas formas no llegaba a escribir en ninguno: cacheaba los edificios con
// GetAllActorsOfClass(AStaticMeshActor) filtrando por GetName(), y los 1030 son
// AEdificioGenerado, que deriva de AActor. Cero en la lista, y su malla es un
// ProceduralMeshComponent, que tampoco habría salido por
// GetComponents<UStaticMeshComponent>.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaNightLightingSystem.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaNightLightingSystem : public UActorComponent
{
    GENERATED_BODY()
public:
    UAlsasuaNightLightingSystem();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float SunsetHour = 20.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float SunriseHour = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Timing")
    float TransitionDuration = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Ambient")
    float NightAmbientIntensity = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Night|Ambient")
    FLinearColor NightAmbientColor = FLinearColor(0.05f, 0.05f, 0.15f);

    UFUNCTION(BlueprintCallable, Category = "Night")
    float GetCurrentNightFactor() const { return NightFactor; }

    UFUNCTION(BlueprintCallable, Category = "Night")
    bool IsNight() const { return NightFactor > 0.5f; }

private:
    float NightFactor = 0.0f;
    float CurrentHour = 12.0f;

    void UpdateNightFactor();
};
