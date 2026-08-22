#include "GF_TraficoModule.h"

#define LOCTEXT_NAMESPACE "FGF_TraficoModule"

void FGF_TraficoModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Trafico: pillar loaded"));
}

void FGF_TraficoModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Trafico: pillar unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_TraficoModule, GF_Trafico)
