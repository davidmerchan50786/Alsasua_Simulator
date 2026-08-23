#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlsasuaKernel, Log, All);

class FAlsasuaKernelModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		UE_LOG(LogAlsasuaKernel, Display, TEXT("[Kernel] StartupModule"));
		FDefaultGameModuleImpl::StartupModule();
	}
};

IMPLEMENT_MODULE(FAlsasuaKernelModule, AlsasuaKernel)
