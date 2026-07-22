#include "Core/AlsasuaSaveGame.h"

UAlsasuaSaveGame::UAlsasuaSaveGame()
{
	SaveSlotName = TEXT("AlsasuaDefaultSlot");
	UserIndex = 0;
	SavedPopularSupport = 10.0f;
	SavedWantedLevel = 0.0f;
	SavedCurrentTime = 12.0f;
}
