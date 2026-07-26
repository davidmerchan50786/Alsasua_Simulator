#include "World/AlsasuaNPCPedestrianSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaNPCPedestrianSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bInitialized = true;
    CargarAssetsPersonaje();
    CargarCallejero();
}

void UAlsasuaNPCPedestrianSystem::CargarAssetsPersonaje()
{
    MeshHombre = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/AssetsImportados/Characters/MeshyConfidence/Meshy_AI_Casual_Confidence_0421161928_texture_fbx/Meshy_AI_Casual_Confidence_0421161928_texture"));

    MeshMujer = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/AssetsImportados/Characters/MeshyOveralls/Meshy_AI_Casual_Denim_Overalls_0421162223_texture_fbx/Meshy_AI_Casual_Denim_Overalls_0421162223_texture"));

    AnimCaminar = LoadObject<UAnimSequence>(nullptr,
        TEXT("/Game/AssetsImportados/Characters/Meshy_AI_Animation_Walking_withSkin"));

    if (!AnimCaminar)
    {
        AnimCaminar = LoadObject<UAnimSequence>(nullptr,
            TEXT("/Game/AssetsImportados/Characters/X Bot@Walking"));
    }

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: Mesh hombre=%s, mujer=%s, anim=%s"),
        MeshHombre ? TEXT("OK") : TEXT("NULL"),
        MeshMujer ? TEXT("OK") : TEXT("NULL"),
        AnimCaminar ? TEXT("OK") : TEXT("NULL"));
}

void UAlsasuaNPCPedestrianSystem::CargarCallejero()
{
    CallesCache.Empty();

    TArray<FString> Lineas;
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("NPCPedestrians: No se pudo cargar roads_unity.json"));
        return;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> RoadsArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RoadsArr)) return;

    for (const auto& RoadVal : RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr)) continue;
        if (PointsArr->Num() < 2) continue;

        TArray<FVector> PuntosCalle;
        for (int32 i = 0; i < PointsArr->Num(); i++)
        {
            const TSharedPtr<FJsonObject>& Pt = (*PointsArr)[i]->AsObject();
            if (!Pt) continue;
            const float X = Pt->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
            const float Z = Pt->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
            PuntosCalle.Add(UAlsasuaGeoData::UnityaUnreal(FVector(X, 0.0f, Z)));
        }

        if (PuntosCalle.Num() >= 2)
            CallesCache.Add(PuntosCalle);
    }

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: %d calles cacheadas"), CallesCache.Num());
}

void UAlsasuaNPCPedestrianSystem::GenerarNPCs()
{
    if (!bInitialized) return;
    UWorld* World = GetWorld();
    if (!World) return;

    NPCs.Empty();

    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario"), TEXT("Monte")
    };

    for (int32 i = 0; i < MaxNPCs; i++)
    {
        FNPCPedestrian NPC;
        NPC.Nombre = FString::Printf(TEXT("Peaton_%03d"), i);
        NPC.Barrio = Barrios[i % Barrios.Num()];
        NPC.GrupoEdad = FMath::RandRange(0, 3);
        NPC.Velocidad = FMath::RandRange(80.0f, 180.0f);
        NPC.bLlevaCompras = (FMath::RandRange(0, 3) == 0);
        NPC.bMascota = (FMath::RandRange(0, 5) == 0);
        NPC.ActividadActual = ENPCActivity::Walk;
        NPC.DuracionActividad = FMath::RandRange(3.0f, 10.0f);

        NPC.PosicionInicio = ObtenerPuntoCalleAleatorio();
        NPC.PosicionObjetivo = ObtenerPuntoCalleAleatorio();
        NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NPC.PosicionInicio).GetSafeNormal();

        CrearNPCEnPunto(NPC);
        NPCs.Add(NPC);
    }

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: %d peatones generados en 8 barrios"), NPCs.Num());
}

void UAlsasuaNPCPedestrianSystem::ActualizarNPCs(float DeltaTime)
{
    for (FNPCPedestrian& NPC : NPCs)
    {
        NPC.TiempoEnActividad += DeltaTime;

        if (NPC.TiempoEnActividad >= NPC.DuracionActividad)
        {
            CambiarActividad(NPC);
        }

        if (NPC.ActividadActual == ENPCActivity::Walk)
        {
            FVector NuevaPos = NPC.PosicionInicio + NPC.DireccionMovimiento * NPC.Velocidad * DeltaTime;
            float DistObj = FVector::Distance(NuevaPos, NPC.PosicionObjetivo);

            if (DistObj < 200.0f)
            {
                NPC.PosicionObjetivo = ObtenerPuntoCalleAleatorio();
                NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NuevaPos).GetSafeNormal();
            }

            NPC.PosicionInicio = NuevaPos;

            if (NPC.ActorAsociado.IsValid())
            {
                ASkeletalMeshActor* SkelActor = Cast<ASkeletalMeshActor>(NPC.ActorAsociado.Get());
                if (SkelActor)
                {
                    SkelActor->SetActorLocation(NuevaPos);
                    FRotator LookAt = NPC.DireccionMovimiento.Rotation();
                    SkelActor->SetActorRotation(LookAt);
                }
            }
        }
    }
}

void UAlsasuaNPCPedestrianSystem::CambiarActividad(FNPCPedestrian& NPC)
{
    NPC.TiempoEnActividad = 0.0f;
    NPC.DuracionActividad = FMath::RandRange(3.0f, 15.0f);

    const int32 ActividadIdx = FMath::RandRange(0, 5);
    NPC.ActividadActual = static_cast<ENPCActivity>(ActividadIdx);

    if (NPC.ActividadActual != ENPCActivity::Walk)
    {
        if (NPC.ActorAsociado.IsValid())
        {
            ASkeletalMeshActor* SkelActor = Cast<ASkeletalMeshActor>(NPC.ActorAsociado.Get());
            if (SkelActor && AnimIdle)
            {
                SkelActor->GetSkeletalMeshComponent()->PlayAnimation(AnimIdle, true);
            }
        }
    }
    else
    {
        NPC.PosicionObjetivo = ObtenerPuntoCalleAleatorio();
        NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NPC.PosicionInicio).GetSafeNormal();

        if (NPC.ActorAsociado.IsValid())
        {
            ASkeletalMeshActor* SkelActor = Cast<ASkeletalMeshActor>(NPC.ActorAsociado.Get());
            if (SkelActor && AnimCaminar)
            {
                SkelActor->GetSkeletalMeshComponent()->PlayAnimation(AnimCaminar, true);
            }
        }
    }
}

void UAlsasuaNPCPedestrianSystem::CrearNPCEnPunto(FNPCPedestrian& NPC)
{
    UWorld* World = GetWorld();
    if (!World) return;

    ASkeletalMeshActor* NPCActor = World->SpawnActor<ASkeletalMeshActor>(
        ASkeletalMeshActor::StaticClass(), NPC.PosicionInicio, FRotator::ZeroRotator);
    if (!NPCActor) return;

    NPCActor->SetMobility(EComponentMobility::Movable);

    USkeletalMesh* MeshAUsar = nullptr;
    if (NPC.GrupoEdad <= 1)
        MeshAUsar = MeshHombre;
    else
        MeshAUsar = MeshMujer;

    if (!MeshAUsar) MeshAUsar = MeshHombre;
    if (!MeshAUsar) MeshAUsar = MeshMujer;

    if (MeshAUsar)
    {
        NPCActor->GetSkeletalMeshComponent()->SetSkeletalMesh(MeshAUsar);

        if (NPC.ActividadActual == ENPCActivity::Walk && AnimCaminar)
        {
            NPCActor->GetSkeletalMeshComponent()->PlayAnimation(AnimCaminar, true);
        }

        float Escala = (NPC.GrupoEdad == 3) ? 0.85f : 1.0f;
        NPCActor->SetActorScale3D(FVector(Escala));
    }
    else
    {
        NPCActor->SetActorScale3D(FVector(0.4f, 0.4f, 1.8f));
    }

#if WITH_EDITOR
    NPCActor->SetActorLabel(*FString::Printf(TEXT("Peaton_%s_%s"), *NPC.Nombre, *NPC.Barrio));
#endif

    NPC.ActorAsociado = NPCActor;
}

FVector UAlsasuaNPCPedestrianSystem::ObtenerPuntoCalleAleatorio()
{
    if (CallesCache.Num() == 0)
        return ObtenerPuntoCalle(TEXT("Herriko"));

    const int32 RandCalle = FMath::RandRange(0, CallesCache.Num() - 1);
    const TArray<FVector>& Calle = CallesCache[RandCalle];
    if (Calle.Num() == 0)
        return ObtenerPuntoCalle(TEXT("Herriko"));

    const int32 RandPt = FMath::RandRange(0, Calle.Num() - 1);
    return Calle[RandPt];
}

FVector UAlsasuaNPCPedestrianSystem::ObtenerPuntoCalle(const FString& Barrio)
{
    const TMap<FString, FVector> CentrosBarrios = {
        {TEXT("Herriko"), UAlsasuaGeoData::UnityaUnreal(FVector(1891.0f, 8568.0f, 0))},
        {TEXT("Zelai"), UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 8573.0f, 0))},
        {TEXT("Intxostia"), UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f, 8577.0f, 0))},
        {TEXT("SanPedro"), UAlsasuaGeoData::UnityaUnreal(FVector(1895.0f, 8565.0f, 0))},
        {TEXT("Errota"), UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f, 8570.0f, 0))},
        {TEXT("Harrobieta"), UAlsasuaGeoData::UnityaUnreal(FVector(1889.0f, 8569.0f, 0))},
        {TEXT("Ferroviario"), UAlsasuaGeoData::UnityaUnreal(FVector(1892.0f, 8571.0f, 0))},
        {TEXT("Monte"), UAlsasuaGeoData::UnityaUnreal(FVector(1894.0f, 8575.0f, 0))}
    };

    if (const FVector* Centro = CentrosBarrios.Find(Barrio))
        return *Centro;

    return UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 8572.0f, 0));
}
