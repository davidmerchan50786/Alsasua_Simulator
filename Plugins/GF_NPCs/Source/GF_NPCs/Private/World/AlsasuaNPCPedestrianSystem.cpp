#include "World/AlsasuaNPCPedestrianSystem.h"
#include "World/AlsasuaRedViaria.h"
#include "AlsasuaServiceRegistry.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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
}

void UAlsasuaNPCPedestrianSystem::ActualizarNPCs(float DeltaTime)
{
    Tick(DeltaTime);
}

void UAlsasuaNPCPedestrianSystem::Tick(float DeltaTime)
{
    if (CallesCache.Num() == 0) CargarCallejero();
    if (NPCs.Num() == 0) return;

    // Get player position for LOD
    UWorld* W = GetWorld();
    FVector PlayerPos = FVector::ZeroVector;
    if (W)
    {
        if (APawn* P = UGameplayStatics::GetPlayerPawn(W, 0))
            PlayerPos = P->GetActorLocation();
    }

    // Round-robin: tick MaxTicksPerFrame NPCs per frame for amortized cost
    const int32 Total = NPCs.Num();
    const int32 TicksThisFrame = FMath::Min(MaxTicksPerFrame, Total);

    for (int32 i = 0; i < TicksThisFrame; ++i)
    {
        int32 Idx = (TickIndex + i) % Total;
        FNPCPedestrian& NPC = NPCs[Idx];

        ActualizarLOD(NPC, PlayerPos);

        // Skip hidden NPCs entirely
        if (NPC.LodActual == ENPCLod::Hidden) continue;

        NPC.TiempoEnActividad += DeltaTime;
        if (NPC.TiempoEnActividad >= NPC.DuracionActividad)
            CambiarActividad(NPC);

        // Full LOD: move and animate. Proxy: just update position.
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

            // Update full actor
            if (NPC.LodActual == ENPCLod::Full && NPC.ActorAsociado.IsValid())
            {
                ASkeletalMeshActor* SkelActor = Cast<ASkeletalMeshActor>(NPC.ActorAsociado.Get());
                if (SkelActor)
                {
                    SkelActor->SetActorLocation(NuevaPos);
                    SkelActor->SetActorRotation(NPC.DireccionMovimiento.Rotation());
                }
            }

            // Update proxy actor
            if (NPC.ProxyActor.IsValid())
            {
                NPC.ProxyActor->SetActorLocation(NuevaPos);
            }
        }
    }

    TickIndex = (TickIndex + TicksThisFrame) % Total;

    // ── NPC joining manifestation ──────────────────────────────────────────
    // Check every 0.5s (not every frame) for perf
    static float JoinTimer = 0.f;
    JoinTimer += DeltaTime;
    if (JoinTimer < 0.5f) return;
    JoinTimer = 0.f;

    UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
    if (!GI) return;

    UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>();
    if (!Reg) return;

    // Access ManifestacionSubsystem via service registry
    UObject* ManifObj = Reg->Pedir(FName("Manifestacion"));
    if (!ManifObj) return;

    // Check if active via reflection (Activa() -> bool)
    UFunction* ActivaFunc = ManifObj->FindFunction(TEXT("Activa"));
    if (!ActivaFunc) return;
    uint8 ActivaResult = 0;
    ManifObj->ProcessEvent(ActivaFunc, &ActivaResult);
    if (!ActivaResult) return;

    // Get centro via GetCentroActual() -> FVector
    FVector CentroManif = FVector::ZeroVector;
    UFunction* CentroFunc = ManifObj->FindFunction(TEXT("GetCentroActual"));
    if (CentroFunc)
        ManifObj->ProcessEvent(CentroFunc, &CentroManif);

    // Get crowd count via NumManifestantes() -> int32
    int32 ManifCount = 0;
    UFunction* NumFunc = ManifObj->FindFunction(TEXT("NumManifestantes"));
    if (NumFunc)
        ManifObj->ProcessEvent(NumFunc, &ManifCount);

    // Access apoyo via service registry
    UObject* ApoyoObj = Reg->Pedir(FName("ApoyoPopular"));
    float ApoyoNorm = 0.5f;
    if (ApoyoObj)
    {
        FProperty* ApoyoProp = ApoyoObj->GetClass()->FindPropertyByName(TEXT("Apoyo"));
        if (ApoyoProp)
        {
            FFloatProperty* FApoyo = CastField<FFloatProperty>(ApoyoProp);
            if (FApoyo)
                ApoyoNorm = FMath::Clamp(FApoyo->GetPropertyValue_InContainer(ApoyoObj) / 100.f, 0.f, 1.f);
        }
    }

    const float RadioJoinSq = FMath::Square(RadioGrito);

    for (FNPCPedestrian& NPC : NPCs)
    {
        if (NPC.bEsManifestante) continue;
        if (NPC.LodActual == ENPCLod::Hidden) continue;

        const float DistSq = FVector::DistSquared(NPC.PosicionInicio, CentroManif);
        if (DistSq > RadioJoinSq) continue;

        // Probability: base * apoyo * (1 + peer_pressure * crowd_size) * distance_falloff
        const float DistFactor = 1.f - FMath::Sqrt(DistSq) / RadioGrito;
        const float PeerPressure = 1.f + PresionGrupo * FMath::Min(ManifCount, 50);
        const float Prob = ProbabilidadUnirse * ApoyoNorm * PeerPressure * DistFactor * JoinTimer;

        if (FMath::FRand() < Prob)
        {
            UnirAManifestacion(&NPC - &NPCs[0]);
            ++ManifestantesUnidos;

            // Switch NPC to walk toward manifestation
            NPC.PosicionObjetivo = CentroManif + FMath::VRand() * FMath::Sqrt(DistSq) * 0.5f;
            NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NPC.PosicionInicio).GetSafeNormal();
            NPC.Velocidad = FMath::RandRange(60.f, 120.f);
            NPC.DuracionActividad = 999.f;

            // Show full actor for joining NPCs
            if (NPC.ActorAsociado.IsValid())
                NPC.ActorAsociado->SetActorHiddenInGame(false);
        }
    }
}

void UAlsasuaNPCPedestrianSystem::ActualizarLOD(FNPCPedestrian& NPC, const FVector& PlayerPos)
{
    const float DistSq = FVector::DistSquared(NPC.PosicionInicio, PlayerPos);
    const ENPCLod NuevoLod = (DistSq < LodFullDistance * LodFullDistance) ? ENPCLod::Full
        : (DistSq < LodProxyDistance * LodProxyDistance) ? ENPCLod::Proxy
        : ENPCLod::Hidden;

    if (NuevoLod == NPC.LodActual) return;

    // Transition: show/hide actors based on new LOD
    if (NuevoLod == ENPCLod::Full)
    {
        if (NPC.ActorAsociado.IsValid()) NPC.ActorAsociado->SetActorHiddenInGame(false);
        if (NPC.ProxyActor.IsValid()) NPC.ProxyActor->SetActorHiddenInGame(true);
    }
    else if (NuevoLod == ENPCLod::Proxy)
    {
        if (NPC.ActorAsociado.IsValid()) NPC.ActorAsociado->SetActorHiddenInGame(true);
        if (NPC.ProxyActor.IsValid()) NPC.ProxyActor->SetActorHiddenInGame(false);
    }
    else // Hidden
    {
        if (NPC.ActorAsociado.IsValid()) NPC.ActorAsociado->SetActorHiddenInGame(true);
        if (NPC.ProxyActor.IsValid()) NPC.ProxyActor->SetActorHiddenInGame(true);
    }

    NPC.LodActual = NuevoLod;
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
    int32 Descartadas = 0;

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

        FString Tipo;
        Road->TryGetStringField(TEXT("type"), Tipo);
        if (!UAlsasuaRedViaria::EsTransitableAPie(Tipo)) { ++Descartadas; continue; }

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr)) continue;
        if (PointsArr->Num() < 2) continue;

        TArray<FVector> PuntosCalle;
        for (int32 i = 0; i < PointsArr->Num(); i++)
        {
            const TSharedPtr<FJsonObject>& Pt = (*PointsArr)[i]->AsObject();
            if (!Pt) continue;
            PuntosCalle.Add(UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(),
                FVector(Pt->GetNumberField(TEXT("x")), 0.0f, Pt->GetNumberField(TEXT("z")))));
        }

        if (PuntosCalle.Num() >= 2)
            CallesCache.Add(PuntosCalle);
    }

    UE_LOG(LogTemp, Log,
        TEXT("NPCPedestrians: %d calles transitables a pie cacheadas (%d descartadas)"),
        CallesCache.Num(), Descartadas);
}

void UAlsasuaNPCPedestrianSystem::GenerarNPCs()
{
    if (CallesCache.Num() == 0) CargarCallejero();

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
            NPC.Nombre = FString::Printf(TEXT("NPC_%04d"), NPCCount);
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
            CrearProxyNPC(NPC);
            NPCs.Add(NPC);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: %d peatones generados (LOD: Full/Proxy/Hidden)"), NPCs.Num());
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
    NPCActor->SetActorLabel(*FString::Printf(TEXT("NPC_%s_%s"), *NPC.Nombre, *NPC.Barrio));
#endif

    NPC.ActorAsociado = NPCActor;
    // Start hidden — LOD system will show the right one
    NPCActor->SetActorHiddenInGame(true);
}

void UAlsasuaNPCPedestrianSystem::CrearProxyNPC(FNPCPedestrian& NPC)
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Simple static mesh capsule proxy for mid-range NPCs
    AStaticMeshActor* Proxy = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), NPC.PosicionInicio, FRotator::ZeroRotator);
    if (!Proxy) return;

    Proxy->GetRootComponent()->SetMobility(EComponentMobility::Movable);
    Proxy->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Use a simple capsule shape as proxy
    UStaticMesh* CapsuleMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CapsuleMesh)
        Proxy->GetStaticMeshComponent()->SetStaticMesh(CapsuleMesh);

    // Color by barrio for visual distinction
    FLinearColor ProxyColor = FLinearColor::Gray;
    if (NPC.Barrio == TEXT("Herriko")) ProxyColor = FLinearColor(0.8f, 0.2f, 0.2f);
    else if (NPC.Barrio == TEXT("Zelai")) ProxyColor = FLinearColor(0.2f, 0.8f, 0.2f);
    else if (NPC.Barrio == TEXT("Intxostia")) ProxyColor = FLinearColor(0.2f, 0.2f, 0.8f);
    else if (NPC.Barrio == TEXT("Harrobieta")) ProxyColor = FLinearColor(0.8f, 0.6f, 0.1f);

    Proxy->GetStaticMeshComponent()->SetVectorParameterValueOnMaterials(
        TEXT("Color"), FVector(ProxyColor.R, ProxyColor.G, ProxyColor.B));

    Proxy->SetActorScale3D(FVector(0.3f, 0.3f, 1.8f));
    Proxy->SetActorHiddenInGame(true);  // LOD will show

    NPC.ProxyActor = Proxy;
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
    FVector P = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));
    P.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), P.X, P.Y);
    return P;
}

//~ IAlsasuaPilarArranque / IAlsasuaPilarTiquear (fase 35 del antiguo Director)
int32 UAlsasuaNPCPedestrianSystem::EjecutarArranque()
{
	GenerarNPCs();
	return GetNPCs().Num();
}

FString UAlsasuaNPCPedestrianSystem::EtiquetaArranque() const
{
	return TEXT("peatones generados");
}

int32 UAlsasuaNPCPedestrianSystem::OrdenArranque() const
{
	return 350;
}

void UAlsasuaNPCPedestrianSystem::TiquearPilar(float DeltaTime)
{
	ActualizarNPCs(DeltaTime);
}

void UAlsasuaNPCPedestrianSystem::CambiarActividad(FNPCPedestrian& NPC)
{
    NPC.TiempoEnActividad = 0.0f;
    NPC.DuracionActividad = FMath::RandRange(3.0f, 15.0f);

    const int32 ActividadIdx = FMath::RandRange(0, 5);
    NPC.ActividadActual = static_cast<ENPCActivity>(ActividadIdx);

    if (NPC.LodActual != ENPCLod::Full) return;

    if (NPC.ActividadActual != ENPCActivity::Walk)
    {
        if (NPC.ActorAsociado.IsValid())
        {
            ASkeletalMeshActor* SkelActor = Cast<ASkeletalMeshActor>(NPC.ActorAsociado.Get());
            if (SkelActor && AnimIdle)
                SkelActor->GetSkeletalMeshComponent()->PlayAnimation(AnimIdle, true);
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
                SkelActor->GetSkeletalMeshComponent()->PlayAnimation(AnimCaminar, true);
        }
    }
}

TArray<int32> UAlsasuaNPCPedestrianSystem::GetNearbyNPCs(const FVector& Location, float Radius) const
{
    TArray<int32> Result;
    const float RadiusSq = FMath::Square(Radius);
    for (int32 i = 0; i < NPCs.Num(); ++i)
    {
        if (FVector::DistSquared(NPCs[i].PosicionInicio, Location) < RadiusSq)
            Result.Add(i);
    }
    return Result;
}

void UAlsasuaNPCPedestrianSystem::UnirAManifestacion(int32 Index)
{
    if (!NPCs.IsValidIndex(Index)) return;
    FNPCPedestrian& NPC = NPCs[Index];
    NPC.bEsManifestante = true;
    NPC.Velocidad = FMath::RandRange(60.0f, 120.0f);  // Slower in crowd
    NPC.DuracionActividad = 999.0f;  // Don't change activity while manifesting
}
