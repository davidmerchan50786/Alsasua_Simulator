#include "AI/Agents/AlsasuaAgentController.h"
#include "AI/Agents/AlsasuaChatManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void AAlsasuaAgentController::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld()) {
        if (UAlsasuaChatManager* ChatMgr = World->GetSubsystem<UAlsasuaChatManager>()) {
            ChatMgr->OnResponseReceived.AddDynamic(this, &AAlsasuaAgentController::OnLLMResponse);
        }
    }
}

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

void AAlsasuaAgentController::HandleNPCResponse(const FString& ResponseText)
{
    UE_LOG(LogTemp, Log, TEXT("AgentController %s procesa respuesta: %s"), *GetName(), *ResponseText);
}

void AAlsasuaAgentController::OnLLMResponse(AActor* NPC, const FString& ResponseText)
{
    if (NPC == GetPawn()) {
        HandleNPCResponse(ResponseText);
    }
}
