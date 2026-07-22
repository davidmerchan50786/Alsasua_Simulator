#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GeoDataAlsasua.generated.h"

USTRUCT(BlueprintType)
struct ALSASUACORE_API FAlsasuaGeoCoords
{
    GENERATED_BODY()
    UPROPERTY() double UTM_E = 567951.0;
    UPROPERTY() double UTM_N = 4749902.0;
    UPROPERTY() double OX = 1918.0;
    UPROPERTY() double OZ = 8570.0;
    UPROPERTY() double ScaleX = 1.0;
    UPROPERTY() double CotaPlaza = 531.94;
    UPROPERTY() double MToUU = 100.0;
};

UCLASS()
class ALSASUACORE_API UAlsasuaGeoData : public UObject
{
    GENERATED_BODY()
public:
    static FVector UnityaUnreal(const FVector& UnityPos);
    static FVector UnrealaUnity(const FVector& UnrealPos);
    static FVector HerrikoPlaza();

    static constexpr double OX = 1918.0;
    static constexpr double OZ = 8570.0;
};
