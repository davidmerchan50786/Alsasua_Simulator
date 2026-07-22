#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaCrowdAgentComponent.generated.h"

UENUM(BlueprintType)
enum class ECrowdAgentState : uint8 { Neutral, Panicking, Resisting, Following };

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaCrowdAgentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaCrowdAgentComponent();

    // Recibe influencia de pánico o fuerza externa (Efecto Dominó)
    UFUNCTION(BlueprintCallable, Category = "AAA|AI")
    void ReceiveExternalPanic(float Intensity);

    // Estado actual para animaciones
    UPROPERTY(BlueprintReadOnly, Category = "AAA|AI")
    ECrowdAgentState CurrentState = ECrowdAgentState::Neutral;

    UFUNCTION(BlueprintCallable, Category = "AAA|AI")
    uint8 GetCurrentMood() const;

    // Intensidad del empuje actual (para aplicar a la velocidad del character)
    UPROPERTY(BlueprintReadOnly, Category = "AAA|AI")
    float PushForce = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category = "AAA|AI")
    float Morale = 50.0f;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float PanicLevel = 0.0f;
};
