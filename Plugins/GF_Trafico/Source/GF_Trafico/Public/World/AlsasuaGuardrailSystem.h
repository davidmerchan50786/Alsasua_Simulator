// AlsasuaGuardrailSystem.h (capa MANIFA)
// Barandillas de protección en las vías de ladera.
//
// Iba a actor por tramo: 1647 sobre los ~819 draw calls de referencia. Ahora
// una capa instanciada.
//
// Y el criterio incluía Type == "residential", que son 1023 de esos 1647:
// quitamiedos de carretera a lo largo de todas las calles del casco, que no es
// lo que hay en Altsasu ni lo que la fase dice hacer ("puentes y zonas de
// riesgo"). Queda detrás de bEnCallesResidenciales, apagado. El tipo "bridge"
// que también miraba no existe en roads_unity.json: los puentes los construye
// UCargadorPuentes desde waterways_unity.json.
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AlsasuaGuardrailSystem.generated.h"

USTRUCT(BlueprintType)
struct FGuardrail
{
    GENERATED_BODY()
    FVector Inicio = FVector::ZeroVector;
    FVector Fin = FVector::ZeroVector;
    FString Tipo;
    FString Barrio;
};

UCLASS()
class GF_TRAFICO_API UAlsasuaGuardrailSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Guardrails")
    int32 ColocarBarandillas();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Guardrails")
    float AlturaBarandilla = 110.0f;

    /** Barandilla también en calles residenciales del casco. Apagado: eso son
     *  1023 de los 1647 tramos, y un quitamiedos de carretera a lo largo de
     *  todas las calles del pueblo no es lo que hay ni lo que la fase dice. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Guardrails")
    bool bEnCallesResidenciales = false;

    const TArray<FGuardrail>& GetBarandillas() const { return Barandillas; }

private:
    /** Actor que aloja la capa instanciada. */
    UPROPERTY() TObjectPtr<AActor> Host = nullptr;

    TArray<FGuardrail> Barandillas;
};
