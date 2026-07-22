#include "AlsasuaLegacyGameMode.h"
#include "AlsasuaPlayerCharacter.h"
#include "AlsasuaLegacyPlayerController.h"

AAlsasuaLegacyGameMode::AAlsasuaLegacyGameMode()
{
	DefaultPawnClass = AAlsasuaPlayerCharacter::StaticClass();
	PlayerControllerClass = AAlsasuaLegacyPlayerController::StaticClass();
}
