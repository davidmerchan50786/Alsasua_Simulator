#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "AlsasuaGeoWorldBuilderSubsystem.generated.h"

class UPrimitiveComponent;

struct FGeoPoint
{
    double Latitude = 0.0;
    double Longitude = 0.0;
    double Altitude = 0.0;
};

struct FGeoNamedFeature
{
    FString Name;
    TArray<FGeoPoint> Points;
    FString Layer;
    FString Kind;
};

struct FGeoDataSource
{
    FString Name;
    FString Type;
    FString FilePath;
    FString Url;
    bool bEnabled = false;
};

struct FGeoLayerData
{
    TArray<TArray<FGeoPoint>> Roads;
    TArray<TArray<FGeoPoint>> Railways;
    TArray<TArray<FGeoPoint>> Buildings;
    TArray<TArray<FGeoPoint>> Forests;
    TArray<FGeoPoint> ElevationSamples;
    TArray<FGeoNamedFeature> NamedRoads;
    TArray<FGeoNamedFeature> NamedPlazas;
    TArray<FGeoNamedFeature> NamedNeighborhoods;
    TArray<FGeoDataSource> DataSources;
    double OriginLatitude = 42.84;
    double OriginLongitude = -2.46;
    double ScaleMetersPerUnit = 1.0;
    bool bHasAnyData = false;
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaGeoWorldBuilderSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    static bool TryLoadGeoDataManifest(FGeoLayerData& OutData);
    static bool TryLoadGeoSpatialDataFromFile(const FString& FilePath, FGeoLayerData& OutData);
    static bool ValidateGeoDataAgainstOfficialBounds(const FGeoLayerData& GeoData, FString& OutMessage);
    static void RegisterDataSource(FGeoLayerData& OutData, const FString& Name, const FString& Type, const FString& FilePath, const FString& Url);

private:
    void BuildWorld();
    void BuildTerrainMesh(AActor* Owner);
    void BuildRoadMesh(AActor* Owner, const TArray<FVector>& Points, float Width);
    void BuildRailMesh(AActor* Owner, const TArray<FVector>& Points, float Width);
    void BuildBuildings(AActor* Owner);
    void BuildForest(AActor* Owner);
    void BuildGeoBuilding(AActor* Owner, const TArray<FGeoPoint>& Points, const FGeoLayerData& GeoData);
    void BuildGeoForest(AActor* Owner, const TArray<FGeoPoint>& Points, const FGeoLayerData& GeoData);
    void BuildNamedRoadLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData);
    void BuildNamedPlazaLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData);
    void BuildNamedNeighborhoodLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData);
    void AddLabel(AActor* Owner, const FVector& Location, const FString& LabelText, float Size, const FColor& Color);

    void AddBoxToMeshData(
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles,
        TArray<FVector>& Normals,
        TArray<FVector2D>& UVs,
        TArray<FColor>& Colors,
        const FVector& Center,
        const FVector& Extents,
        const FColor& Color,
        int32& StartIndex);

    void AddQuadToMeshData(
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles,
        TArray<FVector>& Normals,
        TArray<FVector2D>& UVs,
        TArray<FColor>& Colors,
        const FVector& A,
        const FVector& B,
        const FVector& C,
        const FVector& D,
        const FColor& Color,
        int32& StartIndex);
};
