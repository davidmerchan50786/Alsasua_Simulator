#include "GeoDataAlsasua.h"

FVector UAlsasuaGeoData::UnityaUnreal(const FVector& UnityPos)
{
    return FVector(UnityPos.X * 100.0f, UnityPos.Y * 100.0f, UnityPos.Z * 100.0f);
}

FVector UAlsasuaGeoData::UnrealaUnity(const FVector& UnrealPos)
{
    return FVector(UnrealPos.X / 100.0f, UnrealPos.Y / 100.0f, UnrealPos.Z / 100.0f);
}

FVector UAlsasuaGeoData::HerrikoPlaza()
{
    return FVector(0.0, 0.0, 53194.0);
}
