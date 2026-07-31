#include "World/AlsasuaNPCPedestrianSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Animation/SkeletalMeshActor.h"
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
            PuntosCalle.Add(UAlsasuaGeoData::RelLocalToUE5(FVector(Pt->GetNumberField(TEXT("x")), 0.0f, Pt->GetNumberField(TEXT("z")))));
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

    struct FBarrioNPC { FString Nombre; float Peso; };
    TArray<FBarrioNPC> Barrios;

    const FString NPath = FPaths::ProjectContentDir() + TEXT("Datos/nighborhoods.json");
    TArray<FString> Lines;
    if (FFileHelper::LoadFileToStringArray(Lines, *NPath))
    {
        FString Js;
        for (const FString& L : Lines) Js += L;
        TSharedPtr<FJsonObject> Root;
        TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
        if (FJsonSerializer::Deserialize(Rd, Root) && Root.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* BarArr;
            if (Root->TryGetArrayField(TEXT("barrios"), BarArr))
            {
                for (const auto& BV : *BarArr)
                {
                    const TSharedPtr<FJsonObject>& BO = BV->AsObject();
                    if (!BO) continue;
                    const FString Nombre = BO->GetStringField(TEXT("id"));
                    const FString Den = BO->GetStringField(TEXT("densidad_edificios"));
                    float Peso = 1.0f;
                    if (Den == TEXT("Alta")) Peso = 3.0f;
                    else if (Den == TEXT("Media")) Peso = 2.0f;
                    else if (Den == TEXT("Baja")) Peso = 1.0f;
                    else if (Den == TEXT("Muy Baja")) Peso = 0.5f;
                    Barrios.Add({Nombre, Peso});
                }
            }
        }
    }

    if (Barrios.Num() == 0)
    {
        Barrios.Add({TEXT("Herriko"), 3.0f});
        Barrios.Add({TEXT("Zelai"), 2.0f});
        Barrios.Add({TEXT("Intxostia"), 3.0f});
        Barrios.Add({TEXT("Errota"), 1.0f});
        Barrios.Add({TEXT("SanPedro"), 2.0f});
        Barrios.Add({TEXT("Harrobieta"), 2.0f});
        Barrios.Add({TEXT("Ferroviario"), 1.0f});
        Barrios.Add({TEXT("Monte"), 0.5f});
    }

    float PesoTotal = 0.0f;
    for (const FBarrioNPC& B : Barrios) PesoTotal += B.Peso;

    int32 NPCCount = 0;
    for (const FBarrioNPC& B : Barrios)
    {
        const int32 N = FMath::Max(1, FMath::RoundToInt32(MaxNPCs * B.Peso / PesoTotal));
        for (int32 i = 0; i < N && NPCCount < MaxNPCs; ++i, ++NPCCount)
        {
            FNPCPedestrian NPC;
            NPC.Nombre = FString::Printf(TEXT("Peaton_%03d"), NPCCount);
            NPC.Barrio = B.Nombre;
            NPC.GrupoEdad = FMath::RandRange(0, 3);
            NPC.Velocidad = FMath::RandRange(80.0f, 180.0f);
            NPC.bLlevaCompras = (FMath::RandRange(0, 3) == 0);
            NPC.bMascota = (FMath::RandRange(0, 5) == 0);
            NPC.ActividadActual = ENPCActivity::Walk;
            NPC.DuracionActividad = FMath::RandRange(3.0f, 10.0f);

            NPC.PosicionInicio = ObtenerPuntoCalle(B.Nombre);
            NPC.PosicionObjetivo = ObtenerPuntoCalleAleatorio();
            NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NPC.PosicionInicio).GetSafeNormal();

            CrearNPCEnPunto(NPC);
            NPCs.Add(NPC);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: %d peatones generados distribuidos por densidad"), NPCs.Num());
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

    NPCActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);

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
    return UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));
}
