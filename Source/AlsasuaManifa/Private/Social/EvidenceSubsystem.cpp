#include "Social/EvidenceSubsystem.h"
#include "Politics/FactionSubsystem.h"

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
            if (UFactionSubsystem* FS = GetWorld()->GetSubsystem<UFactionSubsystem>())
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
