#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FAlsasuaContractsModule"

class FAlsasuaContractsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAlsasuaContractsModule, AlsasuaContracts)
