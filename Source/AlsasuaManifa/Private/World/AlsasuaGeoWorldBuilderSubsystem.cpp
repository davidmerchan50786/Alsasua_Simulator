#include "World/AlsasuaGeoWorldBuilderSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"

namespace
{
    FString GetGeoDataFilePath()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/AlsasuaGeoReference.json")));
    }

    FString GetGeoManifestFilePath()
    {
        return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/AlsasuaGeoSources.json")));
    }

    void AppendCoordinatePairs(const TArray<TSharedPtr<FJsonValue>>& CoordinatePairs, TArray<FGeoPoint>& OutPoints)
    {
        for (const TSharedPtr<FJsonValue>& CoordinateValue : CoordinatePairs)
        {
            const TArray<TSharedPtr<FJsonValue>> CoordinateArray = CoordinateValue->AsArray();
            if (CoordinateArray.Num() < 2)
            {
                continue;
            }

            FGeoPoint Point;
            Point.Longitude = CoordinateArray[0]->AsNumber();
            Point.Latitude = CoordinateArray[1]->AsNumber();
            OutPoints.Add(Point);
        }
    }

    FString ResolveGeoDataPath(const FString& Path)
    {
        if (Path.IsEmpty())
        {
            return FString();
        }

        if (Path.StartsWith(TEXT("Content/"), ESearchCase::IgnoreCase))
        {
            return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Path));
        }

        if (FPaths::IsRelative(Path))
        {
            return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), Path));
        }

        return FPaths::ConvertRelativePathToFull(Path);
    }

    FVector ConvertGeoToWorld(const FGeoPoint& Point, const FGeoLayerData& GeoData)
    {
        const double DegreesPerMeter = 111320.0;
        const double LatitudeScale = DegreesPerMeter;
        const double LongitudeScale = DegreesPerMeter * FMath::Cos(FMath::DegreesToRadians(GeoData.OriginLatitude));
        const double DeltaLatMeters = (Point.Latitude - GeoData.OriginLatitude) * LatitudeScale;
        const double DeltaLonMeters = (Point.Longitude - GeoData.OriginLongitude) * LongitudeScale;
        return FVector(static_cast<float>(DeltaLonMeters / GeoData.ScaleMetersPerUnit), static_cast<float>(DeltaLatMeters / GeoData.ScaleMetersPerUnit), static_cast<float>(Point.Altitude));
    }

    void AddFeatureToGeoData(const FString& Layer, const FString& KindName, const FString& Name, const TArray<FGeoPoint>& Points, FGeoLayerData& OutData)
    {
        const FString LayerName = Layer.IsEmpty() ? KindName : Layer;
        if (LayerName.Equals(TEXT("road"), ESearchCase::IgnoreCase))
        {
            OutData.Roads.Add(Points);
            if (!Name.IsEmpty())
            {
                FGeoNamedFeature NamedRoad;
                NamedRoad.Name = Name;
                NamedRoad.Points = Points;
                NamedRoad.Layer = LayerName;
                NamedRoad.Kind = KindName;
                OutData.NamedRoads.Add(NamedRoad);
            }
        }
        else if (LayerName.Equals(TEXT("rail"), ESearchCase::IgnoreCase) || LayerName.Equals(TEXT("railway"), ESearchCase::IgnoreCase))
        {
            OutData.Railways.Add(Points);
        }
        else if (LayerName.Equals(TEXT("building"), ESearchCase::IgnoreCase) || LayerName.Equals(TEXT("buildingFootprint"), ESearchCase::IgnoreCase))
        {
            OutData.Buildings.Add(Points);
            if (!Name.IsEmpty() && Name.Contains(TEXT("plaza"), ESearchCase::IgnoreCase))
            {
                FGeoNamedFeature Plaza;
                Plaza.Name = Name;
                Plaza.Points = Points;
                Plaza.Layer = LayerName;
                Plaza.Kind = KindName;
                OutData.NamedPlazas.Add(Plaza);
            }
        }
        else if (LayerName.Equals(TEXT("forest"), ESearchCase::IgnoreCase) || LayerName.Equals(TEXT("tree"), ESearchCase::IgnoreCase))
        {
            OutData.Forests.Add(Points);
        }
        else if (!Name.IsEmpty() && (LayerName.Equals(TEXT("neighborhood"), ESearchCase::IgnoreCase) || LayerName.Equals(TEXT("district"), ESearchCase::IgnoreCase)))
        {
            FGeoNamedFeature Neighborhood;
            Neighborhood.Name = Name;
            Neighborhood.Points = Points;
            Neighborhood.Layer = LayerName;
            Neighborhood.Kind = KindName;
            OutData.NamedNeighborhoods.Add(Neighborhood);
        }
    }

    bool TryParseGeoJsonFeatures(const TSharedPtr<FJsonObject>& RootObject, FGeoLayerData& OutData)
    {
        if (!RootObject.IsValid())
        {
            return false;
        }

        if (RootObject->HasField(TEXT("originLatitude")))
        {
            OutData.OriginLatitude = RootObject->GetNumberField(TEXT("originLatitude"));
        }

        if (RootObject->HasField(TEXT("originLongitude")))
        {
            OutData.OriginLongitude = RootObject->GetNumberField(TEXT("originLongitude"));
        }

        if (RootObject->HasField(TEXT("scaleMetersPerUnit")))
        {
            OutData.ScaleMetersPerUnit = FMath::Max(RootObject->GetNumberField(TEXT("scaleMetersPerUnit")), 0.1);
        }

        const TArray<TSharedPtr<FJsonValue>>* Features = nullptr;
        if (!RootObject->TryGetArrayField(TEXT("features"), Features))
        {
            return false;
        }

        for (const TSharedPtr<FJsonValue>& FeatureValue : *Features)
        {
            const TSharedPtr<FJsonObject> FeatureObject = FeatureValue->AsObject();
            if (!FeatureObject.IsValid())
            {
                continue;
            }

            const TSharedPtr<FJsonObject> GeometryObject = FeatureObject->GetObjectField(TEXT("geometry"));
            const TSharedPtr<FJsonObject> PropertiesObject = FeatureObject->GetObjectField(TEXT("properties"));
            const FString LayerName = PropertiesObject.IsValid() ? PropertiesObject->GetStringField(TEXT("layer")) : FString();
            const FString KindName = PropertiesObject.IsValid() ? PropertiesObject->GetStringField(TEXT("kind")) : FString();
            const FString Name = PropertiesObject.IsValid() ? PropertiesObject->GetStringField(TEXT("name")) : FString();
            const FString GeometryType = GeometryObject.IsValid() ? GeometryObject->GetStringField(TEXT("type")) : FString();

            if (GeometryType == TEXT("LineString") && GeometryObject.IsValid())
            {
                TArray<FGeoPoint> Points;
                const TArray<TSharedPtr<FJsonValue>>* CoordinatePairs = nullptr;
                if (!GeometryObject->TryGetArrayField(TEXT("coordinates"), CoordinatePairs))
                {
                    continue;
                }
                AppendCoordinatePairs(*CoordinatePairs, Points);
                if (Points.Num() >= 2)
                {
                    AddFeatureToGeoData(LayerName, KindName, Name, Points, OutData);
                }
            }
            else if (GeometryType == TEXT("MultiLineString") && GeometryObject.IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* LineStrings = nullptr;
                if (!GeometryObject->TryGetArrayField(TEXT("coordinates"), LineStrings))
                {
                    continue;
                }

                for (const TSharedPtr<FJsonValue>& LineStringValue : *LineStrings)
                {
                    TArray<FGeoPoint> Points;
                    const TArray<TSharedPtr<FJsonValue>> CoordinatePairs = LineStringValue->AsArray();
                    AppendCoordinatePairs(CoordinatePairs, Points);
                    if (Points.Num() >= 2)
                    {
                        AddFeatureToGeoData(LayerName, KindName, Name, Points, OutData);
                    }
                }
            }
            else if (GeometryType == TEXT("Polygon") && GeometryObject.IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* Rings = nullptr;
                if (!GeometryObject->TryGetArrayField(TEXT("coordinates"), Rings))
                {
                    continue;
                }
                if (Rings->Num() == 0)
                {
                    continue;
                }

                TArray<FGeoPoint> Points;
                const TArray<TSharedPtr<FJsonValue>> CoordinatePairs = (*Rings)[0]->AsArray();
                AppendCoordinatePairs(CoordinatePairs, Points);
                if (Points.Num() >= 3)
                {
                    AddFeatureToGeoData(LayerName, KindName, Name, Points, OutData);
                }
            }
            else if (GeometryType == TEXT("MultiPolygon") && GeometryObject.IsValid())
            {
                const TArray<TSharedPtr<FJsonValue>>* Polygons = nullptr;
                if (!GeometryObject->TryGetArrayField(TEXT("coordinates"), Polygons))
                {
                    continue;
                }

                for (const TSharedPtr<FJsonValue>& PolygonValue : *Polygons)
                {
                    const TArray<TSharedPtr<FJsonValue>> Rings = PolygonValue->AsArray();
                    if (Rings.Num() == 0)
                    {
                        continue;
                    }

                    TArray<FGeoPoint> Points;
                    const TArray<TSharedPtr<FJsonValue>> CoordinatePairs = Rings[0]->AsArray();
                    AppendCoordinatePairs(CoordinatePairs, Points);
                    if (Points.Num() >= 3)
                    {
                        AddFeatureToGeoData(LayerName, KindName, Name, Points, OutData);
                    }
                }
            }
        }

        OutData.bHasAnyData = OutData.Roads.Num() > 0 || OutData.Railways.Num() > 0 || OutData.Buildings.Num() > 0 || OutData.Forests.Num() > 0;
        return OutData.bHasAnyData;
    }

    bool TryLoadGeoSpatialDataFromFile(const FString& FilePath, FGeoLayerData& OutData)
    {
        if (FilePath.IsEmpty() || !FPaths::FileExists(FilePath))
        {
            return false;
        }

        FString JsonText;
        if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
        {
            return false;
        }

        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonText);
        if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
        {
            return false;
        }

        return TryParseGeoJsonFeatures(JsonObject, OutData);
    }

    bool TryLoadGeoSpatialData(FGeoLayerData& OutData)
    {
        bool bLoadedAny = false;
        FString ValidationMessage;

        FGeoLayerData ManifestData;
        if (UAlsasuaGeoWorldBuilderSubsystem::TryLoadGeoDataManifest(ManifestData))
        {
            for (const FGeoDataSource& Source : ManifestData.DataSources)
            {
                if (!Source.bEnabled)
                {
                    continue;
                }

                const bool bIsOfficialVectorSource = Source.Type.Equals(TEXT("vector"), ESearchCase::IgnoreCase) ||
                    Source.Type.Equals(TEXT("official_vector"), ESearchCase::IgnoreCase) ||
                    Source.Type.Equals(TEXT("official"), ESearchCase::IgnoreCase) ||
                    Source.Type.Equals(TEXT("cadastre"), ESearchCase::IgnoreCase) ||
                    Source.Type.Equals(TEXT("elevation"), ESearchCase::IgnoreCase) ||
                    Source.Type.Equals(TEXT("ortho"), ESearchCase::IgnoreCase);

                if (!bIsOfficialVectorSource)
                {
                    continue;
                }

                const FString ResolvedPath = ResolveGeoDataPath(Source.FilePath);
                if (ResolvedPath.IsEmpty())
                {
                    continue;
                }

                if (TryLoadGeoSpatialDataFromFile(ResolvedPath, OutData))
                {
                    bLoadedAny = true;
                }
            }
        }

        if (!bLoadedAny)
        {
            const FString FilePath = GetGeoDataFilePath();
            if (FPaths::FileExists(FilePath))
            {
                bLoadedAny = TryLoadGeoSpatialDataFromFile(FilePath, OutData);
            }
        }

        if (!bLoadedAny)
        {
            return false;
        }

        if (!UAlsasuaGeoWorldBuilderSubsystem::ValidateGeoDataAgainstOfficialBounds(OutData, ValidationMessage))
        {
            UE_LOG(LogTemp, Warning, TEXT("Geo data rejected: %s"), *ValidationMessage);
            OutData = FGeoLayerData();
            return false;
        }

        return true;
    }
}

bool UAlsasuaGeoWorldBuilderSubsystem::TryLoadGeoDataManifest(FGeoLayerData& OutData)
{
    const FString ManifestPath = GetGeoManifestFilePath();
    if (!FPaths::FileExists(ManifestPath))
    {
        return false;
    }

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ManifestPath))
    {
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(JsonReader, JsonObject))
    {
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Sources = nullptr;
    if (!JsonObject->TryGetArrayField(TEXT("sources"), Sources))
    {
        return false;
    }

    for (const TSharedPtr<FJsonValue>& SourceValue : *Sources)
    {
        const TSharedPtr<FJsonObject> SourceObject = SourceValue->AsObject();
        if (!SourceObject.IsValid())
        {
            continue;
        }

        const FString Name = SourceObject->GetStringField(TEXT("name"));
        const FString Type = SourceObject->GetStringField(TEXT("type"));
        const FString FilePath = SourceObject->GetStringField(TEXT("filePath"));
        const FString Url = SourceObject->GetStringField(TEXT("url"));
        const bool bEnabled = SourceObject->GetBoolField(TEXT("enabled"));

        if (!bEnabled)
        {
            continue;
        }

        UAlsasuaGeoWorldBuilderSubsystem::RegisterDataSource(OutData, Name, Type, FilePath, Url);
    }

    return OutData.DataSources.Num() > 0;
}

void UAlsasuaGeoWorldBuilderSubsystem::RegisterDataSource(FGeoLayerData& OutData, const FString& Name, const FString& Type, const FString& FilePath, const FString& Url)
{
    FGeoDataSource DataSource;
    DataSource.Name = Name;
    DataSource.Type = Type;
    DataSource.FilePath = FilePath;
    DataSource.Url = Url;
    DataSource.bEnabled = true;
    OutData.DataSources.Add(DataSource);
}

bool UAlsasuaGeoWorldBuilderSubsystem::ValidateGeoDataAgainstOfficialBounds(const FGeoLayerData& GeoData, FString& OutMessage)
{
    TArray<FGeoPoint> AllPoints;
    for (const TArray<FGeoPoint>& Points : GeoData.Roads)
    {
        AllPoints.Append(Points);
    }
    for (const TArray<FGeoPoint>& Points : GeoData.Railways)
    {
        AllPoints.Append(Points);
    }
    for (const TArray<FGeoPoint>& Points : GeoData.Buildings)
    {
        AllPoints.Append(Points);
    }
    for (const TArray<FGeoPoint>& Points : GeoData.Forests)
    {
        AllPoints.Append(Points);
    }

    if (AllPoints.Num() < 6)
    {
        OutMessage = TEXT("No sufficient official geometry points were loaded.");
        return false;
    }

    double MinLat = TNumericLimits<double>::Max();
    double MaxLat = TNumericLimits<double>::Lowest();
    double MinLon = TNumericLimits<double>::Max();
    double MaxLon = TNumericLimits<double>::Lowest();

    for (const FGeoPoint& Point : AllPoints)
    {
        MinLat = FMath::Min(MinLat, Point.Latitude);
        MaxLat = FMath::Max(MaxLat, Point.Latitude);
        MinLon = FMath::Min(MinLon, Point.Longitude);
        MaxLon = FMath::Max(MaxLon, Point.Longitude);
    }

    const double OfficialMinLat = 42.78;
    const double OfficialMaxLat = 43.02;
    const double OfficialMinLon = -2.60;
    const double OfficialMaxLon = -2.10;

    const bool bWithinBounds = MinLat >= OfficialMinLat && MaxLat <= OfficialMaxLat && MinLon >= OfficialMinLon && MaxLon <= OfficialMaxLon;
    if (!bWithinBounds)
    {
        OutMessage = FString::Printf(TEXT("Coordinates fell outside the official Alsasua/Navarre bounds: lat[%.4f, %.4f], lon[%.4f, %.4f]."), MinLat, MaxLat, MinLon, MaxLon);
        return false;
    }

    OutMessage = FString::Printf(TEXT("Official bounds accepted for Alsasua/Navarre: lat[%.4f, %.4f], lon[%.4f, %.4f]."), MinLat, MaxLat, MinLon, MaxLon);
    return true;
}

void UAlsasuaGeoWorldBuilderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BuildWorld();
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildWorld()
{
    if (!GetWorld())
    {
        return;
    }

    AActor* BuilderActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    BuilderActor->SetActorLabel(TEXT("GeoWorldBuilder"));

    BuildTerrainMesh(BuilderActor);

    FGeoLayerData GeoData;
    const bool bHasGeoData = TryLoadGeoSpatialData(GeoData);

    if (bHasGeoData)
    {
        for (const TArray<FGeoPoint>& Points : GeoData.Roads)
        {
            TArray<FVector> WorldPoints;
            WorldPoints.Reserve(Points.Num());
            for (const FGeoPoint& Point : Points)
            {
                WorldPoints.Add(ConvertGeoToWorld(Point, GeoData));
            }
            BuildRoadMesh(BuilderActor, WorldPoints, 320.f);
        }

        for (const TArray<FGeoPoint>& Points : GeoData.Railways)
        {
            TArray<FVector> WorldPoints;
            WorldPoints.Reserve(Points.Num());
            for (const FGeoPoint& Point : Points)
            {
                WorldPoints.Add(ConvertGeoToWorld(Point, GeoData));
            }
            BuildRailMesh(BuilderActor, WorldPoints, 70.f);
        }

        for (const TArray<FGeoPoint>& Points : GeoData.Buildings)
        {
            BuildGeoBuilding(BuilderActor, Points, GeoData);
        }

        for (const FGeoNamedFeature& Road : GeoData.NamedRoads)
        {
            BuildNamedRoadLabel(BuilderActor, Road, GeoData);
        }

        for (const FGeoNamedFeature& Plaza : GeoData.NamedPlazas)
        {
            BuildNamedPlazaLabel(BuilderActor, Plaza, GeoData);
        }

        for (const FGeoNamedFeature& Neighborhood : GeoData.NamedNeighborhoods)
        {
            BuildNamedNeighborhoodLabel(BuilderActor, Neighborhood, GeoData);
        }

        for (const TArray<FGeoPoint>& Points : GeoData.Forests)
        {
            BuildGeoForest(BuilderActor, Points, GeoData);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No official geo data was accepted for Alsasua; the world will remain terrain-only until verified official datasets are provided."));
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildTerrainMesh(AActor* Owner)
{
    if (!Owner)
    {
        return;
    }

    // Terreno base amplio con pendiente longitudinal y relieve suave para aproximar la orografía de Alsasua.
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
    Mesh->RegisterComponent();
    Mesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
    Mesh->SetWorldScale3D(FVector(14000.f, 14000.f, 220.f));
    Mesh->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
    Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));

    // Franjas de terreno para diferenciar valle, pendiente y zonas de bosque.
    for (int32 i = 0; i < 12; ++i)
    {
        FVector Position(-11000.f + i * 1900.f, 0.f, 0.f);
        float Elevation = 10.f + (i % 4) * 8.f + (i > 6 ? 8.f : 0.f);
        UStaticMeshComponent* Strip = NewObject<UStaticMeshComponent>(Owner);
        Strip->RegisterComponent();
        Strip->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Strip->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
        Strip->SetWorldScale3D(FVector(1300.f, 14000.f, 24.f));
        Strip->SetWorldLocation(Position + FVector(0.f, 0.f, Elevation));
        Strip->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
    }

    // Un corredor de baja cota para simular un río o vega longitudinal.
    UStaticMeshComponent* RiverStrip = NewObject<UStaticMeshComponent>(Owner);
    RiverStrip->RegisterComponent();
    RiverStrip->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    RiverStrip->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
    RiverStrip->SetWorldScale3D(FVector(3000.f, 14000.f, 12.f));
    RiverStrip->SetWorldLocation(FVector(-500.f, 0.f, 3.f));
    RiverStrip->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildRoadMesh(AActor* Owner, const TArray<FVector>& Points, float Width)
{
    if (!Owner || Points.Num() < 2)
    {
        return;
    }

    const float RoadHeight = 4.f;
    const float LaneWidth = 3.2f;
    const float CenterLineWidth = 0.3f;
    const float Shoulder = 2.f;

    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        FVector A = Points[i];
        FVector B = Points[i + 1];
        FVector Dir = (B - A).GetSafeNormal();
        FVector Right = FVector::CrossProduct(FVector(0.f, 0.f, 1.f), Dir).GetSafeNormal() * Width;

        UStaticMeshComponent* Asphalt = NewObject<UStaticMeshComponent>(Owner);
        Asphalt->RegisterComponent();
        Asphalt->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Asphalt->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Asphalt->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
        Asphalt->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
        Asphalt->SetWorldScale3D(FVector(Width * 0.01f, (B - A).Size() * 0.01f, RoadHeight * 0.01f));
        Asphalt->SetWorldLocation((A + B) * 0.5f + FVector(0.f, 0.f, RoadHeight * 0.5f));
        Asphalt->SetWorldRotation(FRotationMatrix::MakeFromX(B - A).Rotator());

        const int32 LaneCount = FMath::Max(1, FMath::RoundToInt(Width / (LaneWidth * 2.f)));
        for (int32 Lane = 0; Lane < LaneCount; ++Lane)
        {
            const float Offset = (Lane - (LaneCount - 1) * 0.5f) * (LaneWidth * 2.f);
            const FVector MarkerPos = (A + B) * 0.5f + Dir * (Offset * 0.5f) + FVector(0.f, 0.f, RoadHeight + 0.2f);
            UStaticMeshComponent* LaneMarker = NewObject<UStaticMeshComponent>(Owner);
            LaneMarker->RegisterComponent();
            LaneMarker->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            LaneMarker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
            LaneMarker->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
            LaneMarker->SetWorldScale3D(FVector(CenterLineWidth, (B - A).Size() * 0.01f, 0.05f));
            LaneMarker->SetWorldLocation(MarkerPos);
            LaneMarker->SetWorldRotation(FRotationMatrix::MakeFromX(B - A).Rotator());
        }

        const FVector ZebraStart = A + Dir * 120.f;
        const FVector ZebraEnd = B - Dir * 120.f;
        if ((B - A).Size() > 260.f)
        {
            for (int32 Step = 0; Step < 3; ++Step)
            {
                const FVector ZebraPos = FMath::Lerp(ZebraStart, ZebraEnd, (Step + 1) / 4.f);
                UStaticMeshComponent* Zebra = NewObject<UStaticMeshComponent>(Owner);
                Zebra->RegisterComponent();
                Zebra->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
                Zebra->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
                Zebra->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
                Zebra->SetWorldScale3D(FVector(4.f, 22.f, 0.05f));
                Zebra->SetWorldLocation(ZebraPos + FVector(0.f, 0.f, RoadHeight + 0.1f));
                Zebra->SetWorldRotation(FRotationMatrix::MakeFromX(B - A).Rotator());
            }
        }
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildRailMesh(AActor* Owner, const TArray<FVector>& Points, float Width)
{
    if (!Owner || Points.Num() < 2)
    {
        return;
    }

    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        FVector A = Points[i];
        FVector B = Points[i + 1];
        FVector Dir = (B - A).GetSafeNormal();
        FVector Right = FVector::CrossProduct(FVector(0.f, 0.f, 1.f), Dir).GetSafeNormal() * Width;
        FVector P1 = A + Right;
        FVector P2 = A - Right;
        FVector P3 = B + Right;
        FVector P4 = B - Right;

        UStaticMeshComponent* Segment = NewObject<UStaticMeshComponent>(Owner);
        Segment->RegisterComponent();
        Segment->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Segment->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
        Segment->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
        Segment->SetWorldScale3D(FVector(Width * 0.01f, (B - A).Size() * 0.01f, 3.f));
        Segment->SetWorldLocation((A + B) * 0.5f + FVector(0.f, 0.f, 1.5f));
        Segment->SetWorldRotation(FRotationMatrix::MakeFromX(B - A).Rotator());
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildBuildings(AActor* Owner)
{
    if (!Owner)
    {
        return;
    }

    const TArray<FVector> UrbanDistricts = {
        FVector(-1800.f, -400.f, 0.f),
        FVector(-1800.f, 1200.f, 0.f),
        FVector(400.f, -400.f, 0.f),
        FVector(400.f, 1200.f, 0.f),
        FVector(2600.f, 1200.f, 0.f),
        FVector(3600.f, 2600.f, 0.f)
    };

    const TArray<float> DistrictScales = {1.20f, 0.95f, 1.00f, 0.85f, 0.80f, 1.35f};

    for (int32 d = 0; d < UrbanDistricts.Num(); ++d)
    {
        FVector Center = UrbanDistricts[d];
        float Scale = DistrictScales[d];

        for (int32 row = 0; row < 4; ++row)
        {
            for (int32 col = 0; col < 4; ++col)
            {
                FVector Position = Center + FVector((col - 1.5f) * 240.f, (row - 1.5f) * 220.f, 0.f);
                FVector Extents(110.f * Scale + col * 8.f, 80.f * Scale + row * 6.f, 140.f + (row + col) * 20.f);

                UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
                Mesh->RegisterComponent();
                Mesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
                Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
                Mesh->SetWorldScale3D(Extents * 0.5f);
                Mesh->SetWorldLocation(Position + FVector(0.f, 0.f, Extents.Z));
                Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
            }
        }
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildForest(AActor* Owner)
{
    if (!Owner)
    {
        return;
    }

    const TArray<FVector> ForestBands = {
        FVector(-5800.f, -3300.f, 0.f),
        FVector(-4200.f, 3200.f, 0.f),
        FVector(5000.f, 2400.f, 0.f)
    };

    for (int32 b = 0; b < ForestBands.Num(); ++b)
    {
        FVector Origin = ForestBands[b];
        for (int32 i = 0; i < 48; ++i)
        {
            FVector Position = Origin + FVector(-1400.f + i * 90.f, -1000.f + ((i % 8) * 220.f), 0.f);
            UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
            Mesh->RegisterComponent();
            Mesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
            Mesh->SetWorldScale3D(FVector(28.f, 28.f, 160.f));
            Mesh->SetWorldLocation(Position + FVector(0.f, 0.f, 80.f));
            Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
        }
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildGeoBuilding(AActor* Owner, const TArray<FGeoPoint>& Points, const FGeoLayerData& GeoData)
{
    if (!Owner || Points.Num() < 3)
    {
        return;
    }

    TArray<FVector> WorldPoints;
    WorldPoints.Reserve(Points.Num());

    FVector Center = FVector::ZeroVector;
    float MinX = TNumericLimits<float>::Max();
    float MaxX = TNumericLimits<float>::Lowest();
    float MinY = TNumericLimits<float>::Max();
    float MaxY = TNumericLimits<float>::Lowest();

    for (const FGeoPoint& Point : Points)
    {
        const FVector WorldPoint = ConvertGeoToWorld(Point, GeoData);
        WorldPoints.Add(WorldPoint);
        Center += WorldPoint;
        MinX = FMath::Min(MinX, WorldPoint.X);
        MaxX = FMath::Max(MaxX, WorldPoint.X);
        MinY = FMath::Min(MinY, WorldPoint.Y);
        MaxY = FMath::Max(MaxY, WorldPoint.Y);
    }
    Center /= Points.Num();

    const float FootprintWidth = FMath::Max(80.f, FMath::Abs(MaxX - MinX) * 0.35f + 20.f);
    const float FootprintLength = FMath::Max(80.f, FMath::Abs(MaxY - MinY) * 0.35f + 20.f);
    const float Height = 24.f + FMath::Clamp(Points.Num(), 3, 12) * 10.f;
    const FVector Extents(FootprintWidth * 0.5f, FootprintLength * 0.5f, Height * 0.5f);

    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
    Mesh->RegisterComponent();
    Mesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
    Mesh->SetWorldScale3D(FVector(Extents.X / 100.f, Extents.Y / 100.f, Extents.Z / 100.f));
    Mesh->SetWorldLocation(Center + FVector(0.f, 0.f, Extents.Z));
    Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildGeoForest(AActor* Owner, const TArray<FGeoPoint>& Points, const FGeoLayerData& GeoData)
{
    if (!Owner || Points.Num() < 3)
    {
        return;
    }

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Points)
    {
        Center += ConvertGeoToWorld(Point, GeoData);
    }
    Center /= Points.Num();

    const int32 TreeCount = FMath::Clamp(6 + Points.Num() / 2, 6, 16);
    for (int32 i = 0; i < TreeCount; ++i)
    {
        const float Angle = (i / static_cast<float>(TreeCount)) * 2.f * PI;
        const float Radius = 120.f + (i % 5) * 35.f;
        const FVector Offset = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
        const float Width = 18.f + (i % 4) * 6.f;
        const float Height = 140.f + (i % 6) * 20.f;

        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
        Mesh->RegisterComponent();
        Mesh->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/SM_Cube.SM_Cube")));
        Mesh->SetWorldScale3D(FVector(Width, Width, Height));
        Mesh->SetWorldLocation(Center + Offset + FVector(0.f, 0.f, Height * 0.5f));
        Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedRoadLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData)
{
    if (!Owner || Feature.Points.Num() < 2) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point, GeoData);
    }
    Center /= Feature.Points.Num();
    AddLabel(Owner, Center + FVector(0.f, 0.f, 45.f), Feature.Name, 14.f, FColor::White);
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedPlazaLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData)
{
    if (!Owner || Feature.Points.Num() < 3) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point, GeoData);
    }
    Center /= Feature.Points.Num();
    AddLabel(Owner, Center + FVector(0.f, 0.f, 80.f), Feature.Name, 18.f, FColor::Yellow);
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedNeighborhoodLabel(AActor* Owner, const FGeoNamedFeature& Feature, const FGeoLayerData& GeoData)
{
    if (!Owner || Feature.Points.Num() < 3) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point, GeoData);
    }
    Center /= Feature.Points.Num();
    AddLabel(Owner, Center + FVector(0.f, 0.f, 120.f), Feature.Name, 16.f, FColor::Cyan);
}

void UAlsasuaGeoWorldBuilderSubsystem::AddLabel(AActor* Owner, const FVector& Location, const FString& LabelText, float Size, const FColor& Color)
{
    if (!Owner || LabelText.IsEmpty()) return;

    UTextRenderComponent* Label = NewObject<UTextRenderComponent>(Owner);
    Label->RegisterComponent();
    Label->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Label->SetText(FText::FromString(LabelText));
    Label->SetWorldSize(Size);
    Label->SetTextRenderColor(Color);
    Label->SetWorldLocation(Location);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
}

void UAlsasuaGeoWorldBuilderSubsystem::AddBoxToMeshData(
    TArray<FVector>& Vertices,
    TArray<int32>& Triangles,
    TArray<FVector>& Normals,
    TArray<FVector2D>& UVs,
    TArray<FColor>& Colors,
    const FVector& Center,
    const FVector& Extents,
    const FColor& Color,
    int32& StartIndex)
{
    (void)Vertices; (void)Triangles; (void)Normals; (void)UVs; (void)Colors; (void)Center; (void)Extents; (void)Color; (void)StartIndex;
}

void UAlsasuaGeoWorldBuilderSubsystem::AddQuadToMeshData(
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
    int32& StartIndex)
{
    (void)Vertices; (void)Triangles; (void)Normals; (void)UVs; (void)Colors; (void)A; (void)B; (void)C; (void)D; (void)Color; (void)StartIndex;
}
