#include "World/AlsasuaBuildingInteriorSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaBuildingInteriorSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaBuildingInteriorSystem::GenerarInteriores()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* BuildingsArr;
    if (!RootVal->TryGetArray(BuildingsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    Interiores.Empty();

    for (const auto& BldVal : *BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->HasField(TEXT("id")) ? Bld->GetIntegerField(TEXT("id")) : -1;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("Herriko");
        const float Height = Bld->HasField(TEXT("height")) ? Bld->GetNumberField(TEXT("height")) : 10.0f;

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        float CX = 0, CZ = 0;
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            CX += Vert->GetNumberField(TEXT("x"));
            CZ += Vert->GetNumberField(TEXT("z"));
        }
        CX /= VertsArr->Num();
        CZ /= VertsArr->Num();

        FVector Center = UAlsasuaGeoData::RelLocalToUE5(FVector(CX, 0.0f, CZ));
        int32 NumPlantas = FMath::Max(1, FMath::CeilToInt(Height / 3.0f));

        FBuildingInterior Interior;
        Interior.BuildingId = Id;
        Interior.Barrio = Barrio;
        Interior.Uso = TEXT("residencial");
        Interior.Plantas = NumPlantas;
        Interior.bTieneLuz = (FMath::FRand() < ProbabilidadLuz);
        Interior.IntensidadLuz = FMath::FRandRange(0.5f, 2.0f);

        if (Barrio == TEXT("Herriko") || Barrio == TEXT("Harrobieta"))
            Interior.ColorLuz = FLinearColor(1.0f, 0.9f, 0.75f);
        else
            Interior.ColorLuz = FLinearColor(1.0f, 0.95f, 0.85f);

        if (Interior.bTieneLuz)
        {
            FVector LightPos = Center;
            LightPos.Z += NumPlantas * 300.0f * 0.5f;

            APointLight* Light = World->SpawnActor<APointLight>(
                APointLight::StaticClass(), LightPos, FRotator::ZeroRotator);
            if (Light)
            {
                UPointLightComponent* PLComp = Cast<UPointLightComponent>(Light->GetLightComponent());
                if (PLComp)
                {
                    PLComp->SetIntensity(Interior.IntensidadLuz * 500.0f);
                    PLComp->SetLightColor(Interior.ColorLuz);
                    PLComp->SetAttenuationRadius(600.0f);
                    PLComp->SetSourceRadius(10.0f);
                }

#if WITH_EDITOR
                Light->SetActorLabel(*FString::Printf(TEXT("LuzInterior_%d"), Id));
#endif
            }
        }

        Interiores.Add(Interior);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("BuildingInteriors: %d interiores con iluminación generados"), Placed);
    return Placed;
}
