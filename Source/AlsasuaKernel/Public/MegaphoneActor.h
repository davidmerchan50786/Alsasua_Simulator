#pragma once
#include "GameFramework/Actor.h"
#include "Interaction/InteractableInterface.h"
#include "MegaphoneActor.generated.h"

UCLASS()
class ALSASUAKERNEL_API AMegaphoneActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
public:
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionText_Implementation() const override { return FText::FromString("Usar Megáfono (Animar Multitud)"); }
};
