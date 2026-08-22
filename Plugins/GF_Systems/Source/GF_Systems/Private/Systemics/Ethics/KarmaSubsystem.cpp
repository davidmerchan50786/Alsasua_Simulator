#include "Systemics/Ethics/KarmaSubsystem.h"

void UKarmaSubsystem::RecordAction(FString ActionName, float EthicsDelta, bool bIsViolent)
{
    EthicalScore += EthicsDelta;
    if (bIsViolent) StructuralDamageDealt++;

    EResistStyle NewStyle = GetCurrentStyle();
    if (NewStyle != CachedStyle)
    {
        CachedStyle = NewStyle;
        OnStyleChanged.Broadcast(CachedStyle);
    }

    UE_LOG(LogTemp, Warning, TEXT("Acción Ética: %s | Score Global: %f"), *ActionName, EthicalScore);
}

EResistStyle UKarmaSubsystem::GetCurrentStyle() const
{
    if (EthicalScore > 50.f && StructuralDamageDealt == 0) return EResistStyle::Pacifist;
    if (EthicalScore < -50.f || StructuralDamageDealt > 10) return EResistStyle::Militant;
    return EResistStyle::Pragmatic;
}

FString UKarmaSubsystem::GetEndingID() const
{
    EResistStyle Style = GetCurrentStyle();
    if (Style == EResistStyle::Pacifist) return "ENDING_DIGNITY"; // Altsasu gana por apoyo internacional
    if (Style == EResistStyle::Militant) return "ENDING_REPRESSION"; // La fuerza justifica la bota
    return "ENDING_EXODUS"; // Victoria amarga, el pueblo queda dividido
}
