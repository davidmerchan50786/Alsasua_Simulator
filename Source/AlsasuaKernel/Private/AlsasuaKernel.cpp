// AlsasuaKernel.cpp (capa KERNEL)
// Punto de entrada del módulo. Sin esto el DLL carga pero no exporta
// InitializeModule(), y el motor lo rechaza con FailedToInitialize sin decir
// por qué: el arranque muere con "The game module 'AlsasuaKernel' could not be
// successfully initialized after it was loaded." y ni una línea de log en medio.
// Era el único módulo del proyecto que no lo tenía.
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, AlsasuaKernel)
