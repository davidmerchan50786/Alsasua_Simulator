#pragma once
#include "CoreMinimal.h"
#include "Components/SplineMeshComponent.h"
#include "AlsasuaRoadComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUASIMULATOR_API UAlsasuaRoadComponent : public USplineMeshComponent {
    GENERATED_BODY()
public:
    UAlsasuaRoadComponent();
};
