#include "GF_VegetacionModule.h"

#define LOCTEXT_NAMESPACE "FGF_VegetacionModule"

void FGF_VegetacionModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Vegetacion: pillar loaded"));
}

void FGF_VegetacionModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("GF_Vegetacion: pillar unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGF_VegetacionModule, GF_Vegetacion)
