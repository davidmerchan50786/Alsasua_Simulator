#include "Social/EvidenceSubsystem.h"
#include "Politics/FactionSubsystem.h"
#include "EconomiaCriminalSubsystem.h"

void UEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UEconomiaCriminalSubsystem::OnCriminalActivity.AddDynamic(this, &UEvidenceSubsystem::OnCriminalActivity);
}

void UEvidenceSubsystem::OnCriminalActivity(FName ActivityType, int32 Severity)
{
    FEvidenceItem NewEvidence;
    NewEvidence.EvidenceId = *FString::Printf(TEXT("Crime_%s_%d"), *ActivityType.ToString(), FMath::RandRange(0, 9999));
    NewEvidence.Title = FString::Printf(TEXT("Criminal: %s (sev %d)"), *ActivityType.ToString(), Severity);
    NewEvidence.ImpactPower = (float)Severity * 0.3f;
    CollectEvidence(NewEvidence);
}

void UEvidenceSubsystem::CollectEvidence(FEvidenceItem NewEvidence)
{
    CollectedEvidence.Add(NewEvidence);
    UE_LOG(LogTemp, Log, TEXT("Evidencia recogida: %s"), *NewEvidence.Title);
}

void UEvidenceSubsystem::PublishToPress(FName EvidenceId)
{
    for (int32 i = 0; i < CollectedEvidence.Num(); ++i)
    {
        if (CollectedEvidence[i].EvidenceId == EvidenceId)
        {
            // Aplicar impacto al mundo mediante el FactionSubsystem
            UWorld* W = GetWorld();
            if (UFactionSubsystem* FS = W ? W->GetSubsystem<UFactionSubsystem>() : nullptr)
            {
                // Publicar evidencia debilita la influencia de "El Centro" (Estado)
                FS->PublishEvidence(FName("ElCentro"), CollectedEvidence[i].ImpactPower);

                // Aumenta el apoyo popular de "La Asamblea"
                FS->RecordPoliticalEvent(FName("LaAsamblea"), FName("ElCentro"), CollectedEvidence[i].ImpactPower * 0.5f);
            }

            OnEvidencePublished.Broadcast(EvidenceId);
            CollectedEvidence.RemoveAt(i);
            break;
        }
    }
}
