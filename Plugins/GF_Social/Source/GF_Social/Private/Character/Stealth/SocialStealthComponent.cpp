#include "Character/Stealth/SocialStealthComponent.h"
#include "ApoyoPopularSubsystem.h"
#include "Engine/GameInstance.h"
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

    // Scale crowd requirements with popular support.
    // High apoyo (70+): NPCs actively help hide you (fewer needed, wider radius).
    // Low apoyo (<30): NPCs avoid helping (more needed, tighter radius).
    float EffectiveRadius = CrowdDetectionRadius;
    int32 EffectiveMinCrowd = MinCrowdSizeToHide;

    if (UGameInstance* GI = Owner->GetWorld() ? Owner->GetWorld()->GetGameInstance() : nullptr)
    {
        if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
        {
            const float A = Apoyo->Apoyo;
            if (A >= 70.f)
            {
                // High support: radius +50%, crowd needed reduced by 1.
                EffectiveRadius *= 1.5f;
                EffectiveMinCrowd = FMath::Max(1, MinCrowdSizeToHide - 1);
            }
            else if (A < 30.f)
            {
                // Low support: radius -30%, crowd needed +1.
                EffectiveRadius *= 0.7f;
                EffectiveMinCrowd = MinCrowdSizeToHide + 1;
            }
        }
    }

    TArray<AActor*> OverlappingActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    TArray<AActor*> IgnoreActors;
    IgnoreActors.Add(Owner);

    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        Owner->GetActorLocation(),
        EffectiveRadius,
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

    bool bNowHidden = (ValidNPCsNearby >= EffectiveMinCrowd);
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
