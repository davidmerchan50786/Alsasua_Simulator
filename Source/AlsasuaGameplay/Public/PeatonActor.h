// PeatonActor.h (capa GAMEPLAY)
// Peatón civil: NPC (Entities) + IA de deambular (Gameplay). Como PoliciaActor,
// vive en Gameplay para enlazar el AIController sin romper capas.
#pragma once

#include "CoreMinimal.h"
#include "AlsasuaNPC.h"
#include "PeatonActor.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API APeatonActor : public AAlsasuaNPC
{
	GENERATED_BODY()

public:
	APeatonActor();
};
