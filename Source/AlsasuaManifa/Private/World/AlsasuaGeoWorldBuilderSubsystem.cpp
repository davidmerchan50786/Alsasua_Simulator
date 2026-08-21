#include "World/AlsasuaGeoWorldBuilderSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Logging/LogMacros.h"
#include "HAL/ConsoleManager.h"
#include "GeoDataAlsasua.h"

static TAutoConsoleVariable<int32> CVarSkipGeoWorldBuild(
    TEXT("alsasua.SkipGeoWorldBuild"),
    0,
    TEXT("Skips geo world builder mesh generation for profiling"),
    ECVF_Cheat);

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

    /**
     * Lat/lon a coordenadas del mundo con la misma conversión que usa todo lo
     * demás (UTM 30N de UAlsasuaGeoData).
     *
     * Antes lo hacía con una equirectangular propia y un origen 42.84/−2.46,
     * que no es Alsasua sino un punto 25 km al oeste, y además dividía metros
     * por ScaleMetersPerUnit=1 para dar centímetros de Unreal: cualquier dato
     * que entrara por aquí caía fuera del pueblo y a 1/100 de su tamaño.
     */
    FVector ConvertGeoToWorld(const FGeoPoint& Point)
    {
        return UAlsasuaGeoData::LatLonToUE5(Point.Latitude, Point.Longitude, Point.Altitude);
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

        // originLatitude/originLongitude/scaleMetersPerUnit ya no se leen: el
        // anclaje del mundo lo fija UAlsasuaGeoData, y un origen distinto por
        // fichero sólo puede dejar estas capas descolocadas del resto.

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

// El header declara este static y sólo existía la función libre de arriba, que
// vive en el namespace anónimo: enlazado interno, invisible fuera de esta
// unidad de traducción. Sus tres hermanas —TryLoadGeoDataManifest,
// ValidateGeoDataAgainstOfficialBounds y RegisterDataSource— sí están definidas
// como miembro; ésta no, así que la primera llamada desde fuera habría sido un
// error de enlazado, no de compilación, y por tanto no lo canta ni el editor.
bool UAlsasuaGeoWorldBuilderSubsystem::TryLoadGeoSpatialDataFromFile(
    const FString& FilePath, FGeoLayerData& OutData)
{
    return ::TryLoadGeoSpatialDataFromFile(FilePath, OutData);
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

    if (CVarSkipGeoWorldBuild.GetValueOnAnyThread() != 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Geo world build skipped by alsasua.SkipGeoWorldBuild"));
        return;
    }

    // Los datos primero: sin GeoJSON oficial este subsistema no tiene nada que
    // construir y no debe dejar nada en el mundo. Antes spawneaba el actor y un
    // terreno de relleno (una losa de 1,4 km y doce tiras de "montes" con la
    // malla y el material por defecto del motor) en todos los mundos, encima
    // del terreno real que ya genera ATerrenoGenerado con el heightmap del IGN.
    FGeoLayerData GeoData;
    if (!TryLoadGeoSpatialData(GeoData))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Geo] sin datos oficiales en Datos/AlsasuaGeoSources.json; el mundo lo construyen los cargadores de Datos/*_unity.json"));
        return;
    }

    AActor* BuilderActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
#if WITH_EDITOR
    BuilderActor->SetActorLabel(TEXT("GeoWorldBuilder"));
#endif

    UInstancedStaticMeshComponent* RoadISM = nullptr;
    UInstancedStaticMeshComponent* BuildingISM = nullptr;
    UInstancedStaticMeshComponent* ForestISM = nullptr;

    auto CreateISM = [&](const TCHAR* Name, UInstancedStaticMeshComponent*& OutComp)
    {
        if (OutComp) return;
        OutComp = NewObject<UInstancedStaticMeshComponent>(BuilderActor, UInstancedStaticMeshComponent::StaticClass(), FName(Name));
        if (!OutComp) return;
        OutComp->RegisterComponent();
        OutComp->AttachToComponent(BuilderActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        OutComp->SetMobility(EComponentMobility::Static);
        OutComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OutComp->SetCanEverAffectNavigation(false);
        // /Engine/EngineMeshes/SM_Cube no existe: era otro LoadObject a null,
        // y estos ISM se quedaban sin malla sin decir nada.
        OutComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        OutComp->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")));
    };

    CreateISM(TEXT("GeoRoadISM"), RoadISM);
    CreateISM(TEXT("GeoBuildingISM"), BuildingISM);
    CreateISM(TEXT("GeoForestISM"), ForestISM);

    for (const TArray<FGeoPoint>& Points : GeoData.Roads)
    {
        TArray<FVector> WorldPoints;
        WorldPoints.Reserve(Points.Num());
        for (const FGeoPoint& Point : Points)
        {
            WorldPoints.Add(ConvertGeoToWorld(Point));
        }
        BuildRoadMesh(BuilderActor, WorldPoints, 320.f, RoadISM);
    }

    for (const TArray<FGeoPoint>& Points : GeoData.Railways)
    {
        TArray<FVector> WorldPoints;
        WorldPoints.Reserve(Points.Num());
        for (const FGeoPoint& Point : Points)
        {
            WorldPoints.Add(ConvertGeoToWorld(Point));
        }
        BuildRailMesh(BuilderActor, WorldPoints, 70.f, RoadISM);
    }

    for (const TArray<FGeoPoint>& Points : GeoData.Buildings)
    {
        BuildGeoBuilding(BuilderActor, Points, BuildingISM);
    }

    for (const FGeoNamedFeature& Road : GeoData.NamedRoads)
    {
        BuildNamedRoadLabel(BuilderActor, Road);
    }

    for (const FGeoNamedFeature& Plaza : GeoData.NamedPlazas)
    {
        BuildNamedPlazaLabel(BuilderActor, Plaza);
    }

    for (const FGeoNamedFeature& Neighborhood : GeoData.NamedNeighborhoods)
    {
        BuildNamedNeighborhoodLabel(BuilderActor, Neighborhood);
    }

    for (const TArray<FGeoPoint>& Points : GeoData.Forests)
    {
        BuildGeoForest(BuilderActor, Points, ForestISM);
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildRoadMesh(AActor* Owner, const TArray<FVector>& Points, float Width, UInstancedStaticMeshComponent* RoadISM)
{
    if (!Owner || !RoadISM || Points.Num() < 2)
    {
        return;
    }

    const float RoadHeight = 4.f;
    const float LaneWidth = 3.2f;
    const float CenterLineWidth = 0.3f;

    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        FVector A = Points[i];
        FVector B = Points[i + 1];
        FVector Dir = (B - A).GetSafeNormal();
        FTransform AsphaltTransform;
        AsphaltTransform.SetLocation((A + B) * 0.5f + FVector(0.f, 0.f, RoadHeight * 0.5f));
        AsphaltTransform.SetRotation(FQuat(FRotationMatrix::MakeFromX(B - A).Rotator()));
        AsphaltTransform.SetScale3D(FVector(Width * 0.01f, (B - A).Size() * 0.01f, RoadHeight * 0.01f));
        RoadISM->AddInstance(AsphaltTransform, true);

        const int32 LaneCount = FMath::Max(1, FMath::RoundToInt(Width / (LaneWidth * 2.f)));
        for (int32 Lane = 0; Lane < LaneCount; ++Lane)
        {
            const float Offset = (Lane - (LaneCount - 1) * 0.5f) * (LaneWidth * 2.f);
            const FVector MarkerPos = (A + B) * 0.5f + Dir * (Offset * 0.5f) + FVector(0.f, 0.f, RoadHeight + 0.2f);
            FTransform LaneTransform;
            LaneTransform.SetLocation(MarkerPos);
            LaneTransform.SetRotation(FQuat(FRotationMatrix::MakeFromX(B - A).Rotator()));
            LaneTransform.SetScale3D(FVector(CenterLineWidth, (B - A).Size() * 0.01f, 0.05f));
            RoadISM->AddInstance(LaneTransform, true);
        }

        const FVector ZebraStart = A + Dir * 120.f;
        const FVector ZebraEnd = B - Dir * 120.f;
        if ((B - A).Size() > 260.f)
        {
            for (int32 Step = 0; Step < 3; ++Step)
            {
                const FVector ZebraPos = FMath::Lerp(ZebraStart, ZebraEnd, (Step + 1) / 4.f);
                FTransform ZebraTransform;
                ZebraTransform.SetLocation(ZebraPos + FVector(0.f, 0.f, RoadHeight + 0.1f));
                ZebraTransform.SetRotation(FQuat(FRotationMatrix::MakeFromX(B - A).Rotator()));
                ZebraTransform.SetScale3D(FVector(4.f, 22.f, 0.05f));
                RoadISM->AddInstance(ZebraTransform, true);
            }
        }
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildRailMesh(AActor* Owner, const TArray<FVector>& Points, float Width, UInstancedStaticMeshComponent* RoadISM)
{
    if (!Owner || !RoadISM || Points.Num() < 2)
    {
        return;
    }

    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        FVector A = Points[i];
        FVector B = Points[i + 1];
        FTransform SegmentTransform;
        SegmentTransform.SetLocation((A + B) * 0.5f + FVector(0.f, 0.f, 1.5f));
        SegmentTransform.SetRotation(FQuat(FRotationMatrix::MakeFromX(B - A).Rotator()));
        SegmentTransform.SetScale3D(FVector(Width * 0.01f, (B - A).Size() * 0.01f, 3.f));
        RoadISM->AddInstance(SegmentTransform, true);
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildGeoBuilding(AActor* Owner, const TArray<FGeoPoint>& Points, UInstancedStaticMeshComponent* BuildingISM)
{
    if (!Owner || !BuildingISM || Points.Num() < 3)
    {
        return;
    }

    FVector Center = FVector::ZeroVector;
    float MinX = TNumericLimits<float>::Max();
    float MaxX = TNumericLimits<float>::Lowest();
    float MinY = TNumericLimits<float>::Max();
    float MaxY = TNumericLimits<float>::Lowest();

    for (const FGeoPoint& Point : Points)
    {
        const FVector WorldPoint = ConvertGeoToWorld(Point);
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

    FTransform BuildingTransform;
    BuildingTransform.SetLocation(Center + FVector(0.f, 0.f, Extents.Z));
    BuildingTransform.SetScale3D(FVector(Extents.X / 100.f, Extents.Y / 100.f, Extents.Z / 100.f));
    BuildingISM->AddInstance(BuildingTransform, true);
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildGeoForest(AActor* Owner, const TArray<FGeoPoint>& Points, UInstancedStaticMeshComponent* ForestISM)
{
    if (!Owner || !ForestISM || Points.Num() < 3)
    {
        return;
    }

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Points)
    {
        Center += ConvertGeoToWorld(Point);
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

        FTransform TreeTransform;
        TreeTransform.SetLocation(Center + Offset + FVector(0.f, 0.f, Height * 0.5f));
        TreeTransform.SetScale3D(FVector(Width, Width, Height));
        ForestISM->AddInstance(TreeTransform, true);
    }
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedRoadLabel(AActor* Owner, const FGeoNamedFeature& Feature)
{
    if (!Owner || Feature.Points.Num() < 2) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point);
    }
    Center /= Feature.Points.Num();
    AddLabel(Owner, Center + FVector(0.f, 0.f, 45.f), Feature.Name, 14.f, FColor::White);
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedPlazaLabel(AActor* Owner, const FGeoNamedFeature& Feature)
{
    if (!Owner || Feature.Points.Num() < 3) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point);
    }
    Center /= Feature.Points.Num();
    AddLabel(Owner, Center + FVector(0.f, 0.f, 80.f), Feature.Name, 18.f, FColor::Yellow);
}

void UAlsasuaGeoWorldBuilderSubsystem::BuildNamedNeighborhoodLabel(AActor* Owner, const FGeoNamedFeature& Feature)
{
    if (!Owner || Feature.Points.Num() < 3) return;

    FVector Center = FVector::ZeroVector;
    for (const FGeoPoint& Point : Feature.Points)
    {
        Center += ConvertGeoToWorld(Point);
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

