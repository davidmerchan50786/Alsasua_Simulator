#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGF_ClimaModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
