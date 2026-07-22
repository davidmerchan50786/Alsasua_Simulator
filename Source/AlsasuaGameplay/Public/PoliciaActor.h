// PoliciaActor.h (capa GAMEPLAY)
// Policía Foral jugable: NPC (Entities) + su IA con visión (Gameplay).
// Hacerlo en Gameplay permite enlazar el AIController sin romper capas.
#pragma once

#include "CoreMinimal.h"
#include "AlsasuaNPC.h"
#include "PoliciaActor.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API APoliciaActor : public AAlsasuaNPC
{
	GENERATED_BODY()

public:
	APoliciaActor();
};
