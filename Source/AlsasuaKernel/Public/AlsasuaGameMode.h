// AlsasuaGameMode.h
// ═══════════════════════════════════════════════════════════════════════════
//  GameMode principal del proyecto (nació en AlsasuaManifa, UE 5.4). Establece AAlsasuaCharacter
//  como pawn por defecto y un HUD por defecto (stub, sustituible por Blueprint).
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AlsasuaGameMode.generated.h"

UCLASS()
class ALSASUAKERNEL_API AAlsasuaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    // Constructor: fija DefaultPawnClass (AAlsasuaCharacter), PlayerControllerClass
    // (AAlsasuaPlayerController) y HUDClass (AAlsasuaHUD).
    AAlsasuaGameMode();
};
