#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KarmaSubsystem.generated.h"

UENUM(BlueprintType)
enum class EResistStyle : uint8 {
    Pacifist,     // Resistencia pasiva, documentación, desobediencia civil
    Pragmatic,    // Sabotaje material, defensa necesaria
    Militant      // Confrontación directa, respuesta violenta
};

UCLASS()
class GF_SYSTEMS_API UKarmaSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Puntos de Ética: Positivo = Pacifismo/Construcción, Negativo = Destrucción/Caos
    UPROPERTY(BlueprintReadOnly, Category="AAA|Ethics")
    float EthicalScore = 0.f;

    // Conteo de bajas o daños críticos
    UPROPERTY(BlueprintReadOnly, Category="AAA|Ethics")
    int32 StructuralDamageDealt = 0;

    UFUNCTION(BlueprintCallable, Category="AAA|Ethics")
    void RecordAction(FString ActionName, float EthicsDelta, bool bIsViolent);

    UFUNCTION(BlueprintPure, Category="AAA|Ethics")
    EResistStyle GetCurrentStyle() const;

    // Determina qué final de Altsasu se desbloquea
    UFUNCTION(BlueprintPure, Category="AAA|Ethics")
    FString GetEndingID() const;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStyleChanged, EResistStyle, NewStyle);
    UPROPERTY(BlueprintAssignable)
    FOnStyleChanged OnStyleChanged;

private:
    EResistStyle CachedStyle = EResistStyle::Pacifist;
};
