#include "Character/Stealth/SocialStealthComponent.h"
// forward-declared
#include "Kismet/KismetSystemLibrary.h"

USocialStealthComponent::USocialStealthComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USocialStealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateStealthStatus();
}

void USocialStealthComponent::UpdateStealthStatus()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    TArray<AActor*> IgnoreActors;
    IgnoreActors.Add(Owner);

    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Owner->GetActorLocation(),
        CrowdDetectionRadius,
        ObjectTypes,
        nullptr,
        IgnoreActors,
        OverlappingActors
    );

    int32 ValidNPCsNearby = 0;
    for (AActor* Actor : OverlappingActors)
    {
        if (Actor->ActorHasTag("CrowdAgent"))
        {
            ValidNPCsNearby++;
        }
    }

    bool bNowHidden = (ValidNPCsNearby >= MinCrowdSizeToHide);
    if (bNowHidden != bIsHiddenInCrowd)
    {
        bIsHiddenInCrowd = bNowHidden;
        OnStealthStateChanged.Broadcast(bIsHiddenInCrowd);

        if (bIsHiddenInCrowd)
        {
            Owner->Tags.AddUnique(FName("HiddenInCrowd"));
        }
        else
        {
            Owner->Tags.Remove(FName("HiddenInCrowd"));
        }
    }
}
