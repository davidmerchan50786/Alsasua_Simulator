#include "Interaction/AlsasuaSmartObject.h"
#include "Components/SceneComponent.h"

AAlsasuaSmartObject::AAlsasuaSmartObject()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    InteractionSlot = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionSlot"));
    InteractionSlot->SetupAttachment(RootComponent);
}
