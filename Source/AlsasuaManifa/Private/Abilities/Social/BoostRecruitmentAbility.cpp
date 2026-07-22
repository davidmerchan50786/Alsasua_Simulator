#include "Abilities/Social/BoostRecruitmentAbility.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "Kismet/GameplayStatics.h"

UBoostRecruitmentAbility::UBoostRecruitmentAbility() { InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor; }

void UBoostRecruitmentAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) {
    if (!ActorInfo)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Avatar = ActorInfo->AvatarActor.Get();
    if (Avatar) {
        TArray<AActor*> OverlappingActors;
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

        UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Avatar->GetActorLocation(), RecruitmentRadius, ObjectTypes, nullptr, TArray<AActor*>(), OverlappingActors);

        for (AActor* Actor : OverlappingActors) {
            if (UAlsasuaCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UAlsasuaCrowdAgentComponent>()) {
                Crowd->Morale += 20.f;
            }
        }
    }
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}