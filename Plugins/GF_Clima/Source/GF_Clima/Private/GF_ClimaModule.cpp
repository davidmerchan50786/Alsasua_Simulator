#include "GF_ClimaModule.h"

#define LOCTEXT_NAMESPACE "FGF_ClimaModule"

void FGF_ClimaModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Clima: pillar loaded"));
}

void FGF_ClimaModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Clima: pillar unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_ClimaModule, GF_Clima)
