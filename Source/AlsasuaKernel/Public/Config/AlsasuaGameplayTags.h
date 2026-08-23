#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace AlsasuaTags {
    // Estados del Jugador
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Recording);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_WithMegaphone);

    // Tipos de NPCs
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crowd_Role_Leader);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crowd_Role_Blocker);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Crowd_Role_Passive);

    // Eventos
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PoliceIncursion);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_MediaFrenzy);
}
