#include "World/AlsasuaTrafficLightSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GeoDataAlsasua.h"

void UAlsasuaTrafficLightSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaTrafficLightSystem::ColocarSemaforos()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Semaforos.Empty();

    const TArray<TPair<FString, FString>> Ubicaciones = {
        {TEXT("Foruen Plaza"), TEXT("Herriko")},
        {TEXT("Kale Nagusia"), TEXT("Herriko")},
        {TEXT("Iruñeko Etorbidea"), TEXT("Zelai")},
        {TEXT("San Pedro bidea"), TEXT("SanPedro")},
        {TEXT("Geltokia kalea"), TEXT("Ferroviario")},
        {TEXT("Errota kalea"), TEXT("Errota")},
        {TEXT("Harrobieta kalea"), TEXT("Harrobieta")},
        {TEXT("Zelai kalea"), TEXT("Zelai")},
        {TEXT("Kale Nagusia"), TEXT("Herriko")},
        {TEXT("Iruñeko Etorbidea"), TEXT("Intxostia")},
        {TEXT("San Pedro bidea"), TEXT("SanPedro")},
        {TEXT("Geltokia kalea"), TEXT("Ferroviario")},
    };

    int32 Placed = 0;

    for (int32 i = 0; i < MaxSemaforos && i < Ubicaciones.Num(); i++)
    {
        const auto& Ubic = Ubicaciones[i];
        FVector Pos;
        if (Ubic.Value == TEXT("Herriko"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f + FMath::RandRange(-1.5f, 1.5f),
                8568.5f + FMath::RandRange(-1.5f, 1.5f), 0));
        else if (Ubic.Value == TEXT("Zelai"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f + FMath::RandRange(-1.5f, 1.5f),
                8573.5f + FMath::RandRange(-1.5f, 1.5f), 0));
        else if (Ubic.Value == TEXT("SanPedro"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f + FMath::RandRange(-1.0f, 1.0f),
                8565.5f + FMath::RandRange(-1.0f, 1.0f), 0));
        else if (Ubic.Value == TEXT("Ferroviario"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1892.5f + FMath::RandRange(-1.0f, 1.0f),
                8571.5f + FMath::RandRange(-1.0f, 1.0f), 0));
        else if (Ubic.Value == TEXT("Errota"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f + FMath::RandRange(-1.0f, 1.0f),
                8570.5f + FMath::RandRange(-1.0f, 1.0f), 0));
        else if (Ubic.Value == TEXT("Harrobieta"))
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f + FMath::RandRange(-1.0f, 1.0f),
                8569.0f + FMath::RandRange(-1.0f, 1.0f), 0));
        else
            Pos = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f + FMath::RandRange(-2.0f, 2.0f),
                8572.0f + FMath::RandRange(-2.0f, 2.0f), 0));

        FTrafficLight Light;
        Light.Posicion = Pos;
        Light.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Light.Calle = Ubic.Key;
        Light.Barrio = Ubic.Value;
        Light.bActivo = true;

        float PostHeight = 350.0f;

        AStaticMeshActor* PostActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (PostActor)
        {
            PostActor->SetMobility(EComponentMobility::Static);
            PostActor->SetActorScale3D(FVector(0.08f, 0.08f, PostHeight / 100.0f));

            UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Cylinder"));
            if (CylinderMesh)
                PostActor->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);

            UMaterialInterface* MetalMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Metal_Negro"));
            if (MetalMat)
                PostActor->GetStaticMeshComponent()->SetMaterial(0, MetalMat);

#if WITH_EDITOR
            PostActor->SetActorLabel(*FString::Printf(TEXT("Semaforo_%s_%d"),
                *Ubic.Value.Left(6), i));
#endif
        }

        FVector LightPos = Pos;
        LightPos.Z += PostHeight;

        AStaticMeshActor* BoxActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), LightPos, FRotator::ZeroRotator);
        if (BoxActor)
        {
            BoxActor->SetMobility(EComponentMobility::Static);
            BoxActor->SetActorScale3D(FVector(0.25f, 0.25f, 0.7f));

            UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/EngineBasicShapes/Cube"));
            if (CubeMesh)
                BoxActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

            UMaterialInterface* BlackMat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/Materiales/M_Metal_Negro"));
            if (BlackMat)
                BoxActor->GetStaticMeshComponent()->SetMaterial(0, BlackMat);

#if WITH_EDITOR
            BoxActor->SetActorLabel(*FString::Printf(TEXT("SemaforoCaja_%s_%d"),
                *Ubic.Value.Left(6), i));
#endif
        }

        for (int32 L = 0; L < 3; L++)
        {
            FVector LedPos = LightPos;
            LedPos.Z -= (L * 25.0f);

            APointLight* Led = World->SpawnActor<APointLight>(
                APointLight::StaticClass(), LedPos, FRotator::ZeroRotator);
            if (Led)
            {
                FLinearColor LedColor;
                if (L == 0) LedColor = FLinearColor(0.8f, 0.0f, 0.0f);
                else if (L == 1) LedColor = FLinearColor(0.8f, 0.6f, 0.0f);
                else LedColor = FLinearColor(0.0f, 0.7f, 0.0f);

                Led->GetPointLightComponent()->SetIntensity(L == 0 ? 2000.0f : 200.0f);
                Led->GetPointLightComponent()->SetLightColor(LedColor);
                Led->GetPointLightComponent()->SetAttenuationRadius(200.0f);
                Led->GetPointLightComponent()->SetSourceRadius(5.0f);

#if WITH_EDITOR
                const TCHAR* LedName = (L == 0) ? TEXT("Rojo") : (L == 1) ? TEXT("Amarillo") : TEXT("Verde");
                Led->SetActorLabel(*FString::Printf(TEXT("SemaforoLed_%s_%s_%d"),
                    *Ubic.Value.Left(6), LedName, i));
#endif
            }
        }

        Semaforos.Add(Light);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficLights: %d semáforos en intersecciones principales"), Placed);
    return Placed;
}
