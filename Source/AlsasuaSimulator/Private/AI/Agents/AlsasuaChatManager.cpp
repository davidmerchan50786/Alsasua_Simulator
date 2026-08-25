#include "AI/Agents/AlsasuaChatManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void UAlsasuaChatManager::RequestNPCResponse(AActor* NPC, const FString& Prompt)
{
    if (!NPC) return;

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(TEXT("http://localhost:11434/api/generate"));
    Req->SetVerb(TEXT("POST"));
    Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Req->SetContentAsString(TEXT("{\"model\":\"llama3\",\"prompt\":\"") + Prompt + TEXT("\",\"stream\":false}"));

    Req->OnProcessRequestComplete().BindLambda([this, NPC](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
    {
        if (bSuccess && Res.IsValid())
        {
            FString ResponseText = Res->GetContentAsString();
            UE_LOG(LogTemp, Log, TEXT("NPC %s responde: %s"), *NPC->GetName(), *ResponseText);
            OnResponseReceived.Broadcast(NPC, ResponseText);
        }
    });

    Req->ProcessRequest();
}
