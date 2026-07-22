#include "AI/Agents/AlsasuaChatManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void UAlsasuaChatManager::RequestNPCResponse(AActor* NPC, const FString& Prompt)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(TEXT("http://localhost:11434/api/generate"));
    Req->SetVerb(TEXT("POST"));
    Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Req->SetContentAsString(TEXT("{\"model\":\"llama3\",\"prompt\":\"") + Prompt + TEXT("\",\"stream\":false}"));

    Req->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr, FHttpResponsePtr Res, bool bSuccess)
    {
        if (bSuccess && Res.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("Agente Altsasu: %s"), *Res->GetContentAsString());
        }
    });

    Req->ProcessRequest();
}
