#include "AI/Agents/AlsasuaAgentController.h"
#include "AI/Agents/AlsasuaChatManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void AAlsasuaAgentController::OnHearPlayerSpeech(const FString& SpeechText)
{
    if (UWorld* World = GetWorld())
    {
        if (UAlsasuaChatManager* ChatMgr = World->GetSubsystem<UAlsasuaChatManager>())
        {
            ChatMgr->RequestNPCResponse(GetPawn(), SpeechText);
        }
    }
}
