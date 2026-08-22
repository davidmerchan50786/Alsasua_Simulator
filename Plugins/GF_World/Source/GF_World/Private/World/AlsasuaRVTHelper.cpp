#include "World/AlsasuaRVTHelper.h"
#include "CargarMaterialComun.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Engine/World.h"

void AAlsasuaRVTHelper::BeginPlay()
{
    Super::BeginPlay();
    CachedMPC = CargarMPCClima();
}

void AAlsasuaRVTHelper::PaintWetnessAtLocation(FVector Location, float Intensity, float Radius)
{
    UWorld* W = GetWorld();
    if (!W) return;

    // Push wetness data into the global MPC so the RVT material can sample it.
    if (CachedMPC)
    {
        if (UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(CachedMPC))
        {
            Inst->SetVectorParameterValue(FName("WPaintLocation"), FVector(Location.X, Location.Y, Intensity));
            Inst->SetScalarParameterValue(FName("WPaintRadius"), Radius);
            Inst->SetScalarParameterValue(FName("WPaintIntensity"), FMath::Clamp(Intensity, 0.f, 1.f));
        }
    }
}
