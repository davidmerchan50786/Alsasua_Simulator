#include "AI/AlsasuaAbility_Shout.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"

void UAlsasuaAbility_Shout::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
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

    if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
    {
        ACharacter* Player = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
        if (!Player)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        UWorld* W = GetWorld();
        if (!W)
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        FVector Origin = Player->GetActorLocation();
        UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();

        if (Sentiment)
        {
            Sentiment->TriggerSocialEvent(Origin, ShoutType == "Motivational" ? 0.3f : -0.2f, Radius);
            UE_LOG(LogTemp, Log, TEXT("Player shouted '%s'! Crowd sentiment affected in radius."), *ShoutType);
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
