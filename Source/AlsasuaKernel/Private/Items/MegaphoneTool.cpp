#include "Items/MegaphoneTool.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "NPCGuardCharacter.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

AMegaphoneTool::AMegaphoneTool() { PrimaryActorTick.bCanEverTick = false; }

bool AMegaphoneTool::UseMegaphone(float Intensity) {
    if (bIsOnCooldown) return false;

    AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
    if (!Player) return false;

    const UAlsasuaAttributeSet* Attr = Player->GetAttributeSet();
    if (Attr && Attr->GetPopularSupport() < 5.f) return false;

    ApplyAudioInfluence(Intensity);

    bIsOnCooldown = true;
    GetWorldTimerManager().SetTimer(CooldownTimerHandle, this, &AMegaphoneTool::ResetCooldown, CooldownTime, false);

    UE_LOG(LogTemp, Warning, TEXT("Megáfono usado con intensidad: %f"), Intensity);
    return true;
}

void AMegaphoneTool::ApplyAudioInfluence(float Intensity) {
    TArray<AActor*> OverlappingActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    UKismetSystemLibrary::SphereOverlapActors(GetWorld(), GetActorLocation(), InfluenceRadius, ObjectTypes, nullptr, TArray<AActor*>(), OverlappingActors);

    for (AActor* Actor : OverlappingActors) {
        // 1. Aumentar moral de manifestantes.
        if (UAlsasuaCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UAlsasuaCrowdAgentComponent>()) {
            Crowd->Morale += 10.f * Intensity;
        }

        // 2. Reducir agresividad de la Guardia Civil.
        if (ANPCGuardCharacter* Guard = Cast<ANPCGuardCharacter>(Actor)) {
            Guard->ReduceAggression(15.f * Intensity);
        }
    }
}

void AMegaphoneTool::ResetCooldown() {
    bIsOnCooldown = false;
}
