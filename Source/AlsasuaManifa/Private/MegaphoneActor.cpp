#include "MegaphoneActor.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "AlsasuaAttributeSet.h"
#include "AbilitySystemGlobals.h"

void AMegaphoneActor::Interact_Implementation(AActor* Interactor)
{
    if (!Interactor)
    {
        return;
    }

    UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Interactor);
    if (!ASC) return;

    UAlsasuaAttributeSet* AS = ASC->GetSet<UAlsasuaAttributeSet>();
    if (AS)
    {
        ASC->ApplyModToAttribute(AS->GetPopularSupportAttribute(), EGameplayModOp::Additive, 5.f);
    }
}
