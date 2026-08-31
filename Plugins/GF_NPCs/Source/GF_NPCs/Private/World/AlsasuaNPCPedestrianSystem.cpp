#include "World/AlsasuaNPCPedestrianSystem.h"
#include "World/AlsasuaRedViaria.h"
#include "AlsasuaServiceRegistry.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioDeviceManager.h"

// Paleta de ropa realista por estilo (colores sobrios, típicos del pueblo).
static FLinearColor ColorRopaPara(ENPCEstilo Estilo, bool bMujer)
{
    // Trabajador: azul/marrón trabajo. Jubilado: tonos tierra. Estudiante: vivos.
    switch (Estilo)
    {
    case ENPCEstilo::Trabajador: return bMujer ? FLinearColor(0.18f, 0.27f, 0.42f) : FLinearColor(0.22f, 0.32f, 0.42f);
    case ENPCEstilo::Jubilado:   return bMujer ? FLinearColor(0.62f, 0.52f, 0.36f) : FLinearColor(0.45f, 0.42f, 0.38f);
    case ENPCEstilo::Estudiante: return bMujer ? FLinearColor(0.55f, 0.28f, 0.30f) : FLinearColor(0.20f, 0.40f, 0.48f);
    case ENPCEstilo::Comerciante:return bMujer ? FLinearColor(0.35f, 0.42f, 0.30f) : FLinearColor(0.30f, 0.34f, 0.46f);
    case ENPCEstilo::Deportista: return bMujer ? FLinearColor(0.40f, 0.10f, 0.30f) : FLinearColor(0.10f, 0.35f, 0.25f);
    case ENPCEstilo::Obrero:     return bMujer ? FLinearColor(0.60f, 0.55f, 0.35f) : FLinearColor(0.55f, 0.48f, 0.24f);
    case ENPCEstilo::Nocturno:   return bMujer ? FLinearColor(0.16f, 0.16f, 0.18f) : FLinearColor(0.12f, 0.12f, 0.16f);
    default:                     return bMujer ? FLinearColor(0.4f, 0.3f, 0.3f) : FLinearColor(0.3f, 0.3f, 0.4f);
    }
}

// Proclividad a sumarse a la protesta según la forma de ser: el Rebelde se
// apunta, el Tímido se echa atrás. Multiplica la probabilidad base de unirse.
static float ProbabilidadProtesta(ENPCPersonalidad P)
{
	switch (P)
	{
	case ENPCPersonalidad::Rebelde:   return 2.0f;
	case ENPCPersonalidad::Amable:    return 1.4f;
	case ENPCPersonalidad::Sociable:  return 1.4f;
	case ENPCPersonalidad::Serio:     return 1.0f;
	case ENPCPersonalidad::Nervioso:  return 0.7f;
	case ENPCPersonalidad::Grumpy:    return 0.7f;
	case ENPCPersonalidad::Timido:    return 0.5f;
	default:                          return 1.0f;
	}
}

void UAlsasuaNPCPedestrianSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bInitialized = true;
    CargarAssetsPersonaje();

    if (UAlsasuaServiceRegistry* Reg = GetGameInstance() ? GetGameInstance()->GetSubsystem<UAlsasuaServiceRegistry>() : nullptr)
        Reg->Publicar(FName("NPCPedestrians"), this);
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

    // ── Social interaction: NPC-NPC conversations ─────────────────────────
    ProcesarSocial(DeltaTime);
    TiempoLinea += DeltaTime;

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
        // Cada persona reacciona según su forma de ser: el Rebelde se apunta, el
        // Tímido se echa atrás. Cierra el bucle persona -> protesta.
        const float Prob = ProbabilidadUnirse * ApoyoNorm * PeerPressure * DistFactor * JoinTimer
            * ProbabilidadProtesta(NPC.Persona.Personalidad);

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
    // Mannequin standard (UE) ya importado con sus animaciones de caminar/
    // idle bajo el mismo esqueleto: funciona sin importar FBX externos.
    // SKM_Manny_Simple es el que resuelve su skeleton; SK_Mannequin no, así
    // que ambos géneros usan Manny (el mannequin es andrógino de todos modos:
    // la distinción real hombre/mujer va por nombre/persona/voz/pitch).
    MeshHombre = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/FreeAnimationLibrary/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
    MeshMujer = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/FreeAnimationLibrary/Demo/Characters/Mannequins/Meshes/SKM_Manny_Simple"));

    // 2º cuerpo: personaje skinned de Fab (meta/AssetsImportados). Si carga,
    // da una 2ª silueta para romper la uniformidad de los clones. GPU bajo.
    MeshFab = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/AssetsImportados/Fab/Personaje_Riggeado"));

    AnimCaminar = LoadObject<UAnimSequence>(nullptr,
        TEXT("/Game/FreeAnimationLibrary/Animations/Walk/anim_Walk_Fwd_Loop_R"));
    AnimIdle = LoadObject<UAnimSequence>(nullptr,
        TEXT("/Game/FreeAnimationLibrary/Animations/Idle/anim_Idle"));

    UE_LOG(LogTemp, Log, TEXT("NPCPedestrians: Mesh hombre=%s, mujer=%s, fab=%s, anim=%s, idle=%s"),
        MeshHombre ? TEXT("OK") : TEXT("NULL"),
        MeshMujer ? TEXT("OK") : TEXT("NULL"),
        MeshFab ? TEXT("OK") : TEXT("NULL"),
        AnimCaminar ? TEXT("OK") : TEXT("NULL"),
        AnimIdle ? TEXT("OK") : TEXT("NULL"));
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

            GenerarPersona(NPC);

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

// ── Persona y vida social ───────────────────────────────────────────────────

void UAlsasuaNPCPedestrianSystem::GenerarPersona(FNPCPedestrian& NPC)
{
    // Cargar pool de nombres la primera vez
    if (!bNombresCargados)
    {
        NombresHombre = {
            TEXT("Mikel"), TEXT("Ander"), TEXT("Iker"), TEXT("Jon"), TEXT("Aitor"),
            TEXT("Unai"), TEXT("Asier"), TEXT("Beñat"), TEXT("Gorka"), TEXT("Iñaki"),
            TEXT("Xabi"), TEXT("Julen"), TEXT("Markel"), TEXT("Unax"), TEXT("Aimar"),
            TEXT("Koldobika"), TEXT("Iñigo"), TEXT("Eneko"), TEXT("Mattin"), TEXT("Peio")
        };
        NombresMujer = {
            TEXT("Ane"), TEXT("Maite"), TEXT("Nerea"), TEXT("Leire"), TEXT("Amaia"),
            TEXT("Naroa"), TEXT("Olatz"), TEXT("Garazi"), TEXT("June"), TEXT("Iraia"),
            TEXT("Eider"), TEXT("Nagore"), TEXT("Uxue"), TEXT("Ainhoa"), TEXT("Lide"),
            TEXT("Maddi"), TEXT("Haizea"), TEXT("Itziar"), TEXT("Kattalin"), TEXT("Arrate")
        };
        bNombresCargados = true;
    }

    // Edad desde el grupo existente (3=jubilado, 0=joven)
    const int32 Edad = (NPC.GrupoEdad == 0) ? FMath::RandRange(16, 30)
        : (NPC.GrupoEdad == 1) ? FMath::RandRange(30, 50)
        : (NPC.GrupoEdad == 2) ? FMath::RandRange(50, 70)
        : FMath::RandRange(70, 90);

    // Nombre y sexo
    const bool bMujer = (FMath::RandRange(0, 1) == 0);
    NPC.Persona.Nombre = bMujer ? NombresMujer[FMath::RandRange(0, NombresMujer.Num() - 1)]
                                : NombresHombre[FMath::RandRange(0, NombresHombre.Num() - 1)];
    NPC.Nombre = NPC.Persona.Nombre;  // reemplaza NPC_xxxx
    NPC.Persona.Edad = Edad;

    // Personalidad (7 tipos)
    NPC.Persona.Personalidad = static_cast<ENPCPersonalidad>(FMath::RandRange(0, 6));

    // Estilo según edad
    if (Edad >= 70)
        NPC.Persona.Estilo = ENPCEstilo::Jubilado;
    else if (Edad <= 25 && FMath::RandRange(0, 1))
        NPC.Persona.Estilo = ENPCEstilo::Estudiante;
    else
        NPC.Persona.Estilo = static_cast<ENPCEstilo>(FMath::RandRange(0, 6));

    // Voz: pitch distinto por persona (0.75-1.3)
    NPC.Persona.VozPitch = FMath::FRandRange(0.75f, 1.3f);

    // Frase favorita según personalidad
    NPC.Persona.FraseFavorita = FraseFavoritaPara(NPC.Persona.Personalidad);

    // Generosidad: más probable en Amable/Sociable
    NPC.Persona.bEsGeneroso = (NPC.Persona.Personalidad == ENPCPersonalidad::Amable ||
                               NPC.Persona.Personalidad == ENPCPersonalidad::Sociable) &&
                              (FMath::RandRange(0, 2) != 0);

    // Velocidad según persona
    if (NPC.Persona.Personalidad == ENPCPersonalidad::Nervioso) NPC.Velocidad *= 1.2f;
    else if (NPC.Persona.Personalidad == ENPCPersonalidad::Serio) NPC.Velocidad *= 0.9f;
}

void UAlsasuaNPCPedestrianSystem::ProcesarSocial(float DeltaTime)
{
    if (NPCs.Num() < 2) return;
    UWorld* W = GetWorld();
    if (!W) return;
    APawn* Player = UGameplayStatics::GetPlayerPawn(W, 0);
    if (!Player) return;
    const FVector PlayerPos = Player->GetActorLocation();

    TimerSocial += DeltaTime;
    if (TimerSocial < 1.0f) return;
    TimerSocial = 0.f;

    // Window: check a slice of NPCs each second for pair conversations
    const int32 Total = NPCs.Num();
    const int32 SliceSize = FMath::Min(40, Total);
    for (int32 k = 0; k < SliceSize; ++k)
    {
        SocialIndex = (SocialIndex + 1) % Total;
        FNPCPedestrian& A = NPCs[SocialIndex];
        if (A.LodActual != ENPCLod::Full) continue;

        // Only sociable/talkative NPCs initiate near player
        const float PlayerDistSq = FVector::DistSquared(A.PosicionInicio, PlayerPos);
        if (PlayerDistSq > FMath::Square(10000.f)) continue;  // 100m

        // Find another full-LOD NPC nearby
        for (int32 j = 0; j < Total; ++j)
        {
            FNPCPedestrian& B = NPCs[j];
            if (&B == &A) continue;
            if (B.LodActual != ENPCLod::Full) continue;

            const float DistSq = FVector::DistSquared(A.PosicionInicio, B.PosicionInicio);
            if (DistSq > FMath::Square(RadioConversacion)) continue;

            const float Now = W->GetTimeSeconds();
            // Cooldown per NPC to avoid dialog spam
            if (Now - A.UltimaConversacion < 8.f) continue;
            if (Now - B.UltimaConversacion < 8.f) continue;

            TradeConversacion(A, B);
            A.UltimaConversacion = Now;
            B.UltimaConversacion = Now;
            break;
        }
    }
}

void UAlsasuaNPCPedestrianSystem::TradeConversacion(FNPCPedestrian& A, FNPCPedestrian& B)
{
    // First NPC speaks based on its persona
    const FString LineaA = LineaDeConversacion(A.Persona, A.Persona.Edad != B.Persona.Edad);
    OnNPCHabla.Broadcast(A.Persona.Nombre, LineaA, A.Persona.VozPitch);
    UltimoHablante = A.Persona.Nombre; UltimaLinea = LineaA; UltimoPitch = A.Persona.VozPitch; TiempoLinea = 0.f;
    ReproducirVoz(A.PosicionInicio, A.Persona.VozPitch);

    // Second NPC responds (different line, own persona)
    const FString LineaB = LineaDeConversacion(B.Persona, A.Persona.Edad != B.Persona.Edad);
    OnNPCHabla.Broadcast(B.Persona.Nombre, LineaB, B.Persona.VozPitch);
    UltimoHablante = B.Persona.Nombre; UltimaLinea = LineaB; UltimoPitch = B.Persona.VozPitch; TiempoLinea = 0.f;
    ReproducirVoz(B.PosicionInicio, B.Persona.VozPitch);
}

FString UAlsasuaNPCPedestrianSystem::LineaDeConversacion(const FNPCPersona& P, bool bRangoEdadDiferente) const
{
    static const TMap<ENPCPersonalidad, TArray<FString>> Lineas = {
        { ENPCPersonalidad::Amable, {
            TEXT("Egun on! ¿Cómo va el día?"),
            TEXT("¡Vaya tiempo tan bueno hace hoy!"),
            TEXT("¿Necesitas ayuda con algo?"),
            TEXT("Hay que ser buena gente, ¿no crees?"),
            TEXT("Todo irá bien, seguro."),
        }},
        { ENPCPersonalidad::Timido, {
            TEXT("...hola."),
            TEXT("Eh... sí... bueno..."),
            TEXT("No sé, yo qué sé..."),
            TEXT("Perdona, que tengo prisa..."),
            TEXT("Uhm, no suelo hablar mucho..."),
        }},
        { ENPCPersonalidad::Grumpy, {
            TEXT("Qué país este, todo carísimo."),
            TEXT("Los jóvenes de hoy no tienen respeto."),
            TEXT("Todo está peor que antes, ya os digo."),
            TEXT("A mí nadie me cuenta nada."),
            TEXT("Cuánto impuesto y para nada."),
        }},
        { ENPCPersonalidad::Sociable, {
            TEXT("¡Egun on! ¿Habéis visto lo del pleno ayer?"),
            TEXT("Oye oye, ¡qué ganas de fiesta este finde!"),
            TEXT("Yo conozco a todo el mundo por aquí."),
            TEXT("¡Un placer verte por el barrio!"),
            TEXT("¿Has ido a la feria? ¡Está genial!"),
        }},
        { ENPCPersonalidad::Serio, {
            TEXT("Buenos días. Sin novedad."),
            TEXT("El trabajo es el trabajo."),
            TEXT("Conviene ser prudente."),
            TEXT("Cada uno a lo suyo."),
            TEXT("No me gusta perder el tiempo."),
        }},
        { ENPCPersonalidad::Nervioso, {
            TEXT("Oye, ¿has oído eso? Me ha parecido algo."),
            TEXT("Tengo que ir ya, que llego tarde, siempre llego tarde."),
            TEXT("¿Estamos seguros de que esto es seguro?"),
            TEXT("Uf, uf, qué agobio de día."),
            TEXT("¡Cuidado que viene un coche!"),
        }},
        { ENPCPersonalidad::Rebelde, {
            TEXT("El pueblo manda, ¿eh? ¡Asamblea!"),
            TEXT("Ni un paso atrás, compañero."),
            TEXT("Los de arriba que tiemblen."),
            TEXT("La voz del barrio no se silencia."),
            TEXT("¡Pan, tierra y libertad!"),
        }},
    };

    const TArray<FString>* Pool = Lineas.Find(P.Personalidad);
    if (Pool && Pool->Num() > 0)
        return (*Pool)[FMath::RandRange(0, Pool->Num() - 1)];

    return TEXT("¿Qué tal?");
}

FString UAlsasuaNPCPedestrianSystem::FraseFavoritaPara(ENPCPersonalidad P) const
{
    switch (P)
    {
    case ENPCPersonalidad::Amable:    return TEXT("Un saludo no cuesta nada.");
    case ENPCPersonalidad::Timido:    return TEXT("...");
    case ENPCPersonalidad::Grumpy:    return TEXT("Tiempos pasados fueron mejores.");
    case ENPCPersonalidad::Sociable:  return TEXT("Aquí todos nos conocemos.");
    case ENPCPersonalidad::Serio:     return TEXT("El que calla, otorga.");
    case ENPCPersonalidad::Nervioso:  return TEXT("Mira bien antes de cruzar.");
    case ENPCPersonalidad::Rebelde:   return TEXT("El pueblo, unido, jamás será vencido.");
    default:                          return TEXT("Egun on.");
    }
}

int32 UAlsasuaNPCPedestrianSystem::GetNearestNPC(const FVector& Location, float MaxRadius) const
{
    int32 BestIdx = -1;
    float BestDistSq = FMath::Square(MaxRadius);
    for (int32 i = 0; i < NPCs.Num(); ++i)
    {
        if (NPCs[i].LodActual != ENPCLod::Full) continue;
        const float DistSq = FVector::DistSquared(NPCs[i].PosicionInicio, Location);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestIdx = i;
        }
    }
    return BestIdx;
}

int32 UAlsasuaNPCPedestrianSystem::ReclutarPropensos(const FVector& Punto, float Radio, int32 MaxN)
{
    if (MaxN <= 0) return 0;
    const float RadioSq = FMath::Square(Radio);

    // Indices Full-LOD, manifestantes o propensos a protestar (factor >= 1.0
    // en ProbabilidadProtesta), ordenados por distancia al punto.
    TArray<int32> Candidatos;
    for (int32 i = 0; i < NPCs.Num(); ++i)
    {
        const FNPCPedestrian& NPC = NPCs[i];
        if (NPC.LodActual != ENPCLod::Full) continue;
        if (NPC.bEsManifestante) continue;
        if (ProbabilidadProtesta(NPC.Persona.Personalidad) < 1.0f) continue;
        if (FVector::DistSquared(NPC.PosicionInicio, Punto) > RadioSq) continue;
        Candidatos.Add(i);
    }
    Candidatos.Sort([&](int32 A, int32 B)
    {
        return FVector::DistSquared(NPCs[A].PosicionInicio, Punto)
             < FVector::DistSquared(NPCs[B].PosicionInicio, Punto);
    });

    int32 Reclutados = 0;
    for (const int32 i : Candidatos)
    {
        if (Reclutados >= MaxN) break;
        FNPCPedestrian& NPC = NPCs[i];
        NPC.bEsManifestante = true;
        NPC.PosicionObjetivo = Punto + FMath::VRand() * FMath::Min(Radio, 400.f) * 0.4f;
        NPC.DireccionMovimiento = (NPC.PosicionObjetivo - NPC.PosicionInicio).GetSafeNormal();
        NPC.Velocidad = FMath::RandRange(80.f, 140.f);
        NPC.DuracionActividad = 999.f;
        if (NPC.ActorAsociado.IsValid()) NPC.ActorAsociado->SetActorHiddenInGame(false);
        ReproducirVoz(NPC.PosicionInicio, NPC.Persona.VozPitch);
        OnNPCHabla.Broadcast(NPC.Persona.Nombre, NPC.Persona.FraseFavorita, NPC.Persona.VozPitch);
        ReclutadosTemporales.Add(i);
        ++Reclutados;
    }
    return Reclutados;
}

void UAlsasuaNPCPedestrianSystem::LiberarReclutados()
{
    for (const int32 i : ReclutadosTemporales)
    {
        if (!NPCs.IsValidIndex(i)) continue;
        FNPCPedestrian& NPC = NPCs[i];
        NPC.bEsManifestante = false;
        NPC.DuracionActividad = FMath::RandRange(1.f, 3.f); // retoma la rutina pronto
        NPC.Velocidad = FMath::RandRange(40.f, 90.f);
    }
    ReclutadosTemporales.Reset();
}

const FNPCPersona& UAlsasuaNPCPedestrianSystem::GetPersona(int32 Index) const
{
    static const FNPCPersona DefaultPersona;
    if (NPCs.IsValidIndex(Index))
        return NPCs[Index].Persona;
    return DefaultPersona;
}

FString UAlsasuaNPCPedestrianSystem::HablarConNPC(int32 Index)
{
    if (!NPCs.IsValidIndex(Index)) return FString();
    FNPCPedestrian& NPC = NPCs[Index];
    if (NPC.LodActual != ENPCLod::Full) return FString();

    // The NPC responds to the player based on persona, mood, and manifestation state
    FString Linea;
    if (NPC.bEsManifestante)
    {
        static const TArray<FString> ManifLines = {
            TEXT("¡Únete a la asamblea, compañero!"),
            TEXT("El pueblo está despertando!"),
            TEXT("Ni un paso atrás!"),
            TEXT("Esto va en serio: justicia y vivienda digna!"),
        };
        Linea = ManifLines[FMath::RandRange(0, ManifLines.Num() - 1)];
    }
    else
    {
        Linea = LineaDeConversacion(NPC.Persona, false);
    }

    UWorld* W = GetWorld();
    if (W)
    {
        NPC.UltimaConversacion = W->GetTimeSeconds();
        OnNPCHabla.Broadcast(NPC.Persona.Nombre, Linea, NPC.Persona.VozPitch);
        UltimoHablante = NPC.Persona.Nombre; UltimaLinea = Linea; UltimoPitch = NPC.Persona.VozPitch; TiempoLinea = 0.f;
        ReproducirVoz(NPC.PosicionInicio, NPC.Persona.VozPitch);
    }
    return Linea;
}

void UAlsasuaNPCPedestrianSystem::ReproducirVoz(const FVector& Posicion, float Pitch)
{
    UWorld* W = GetWorld();
    if (!W) return;

    // Sintetiza y reproduce un breve "murmullo" humano (voz) en memoria:
    // 0.35s con formantes + vibrato, sin assets dependientes.
    const int32 SampleRate = 22050;
    const int32 NumSamples = (int32)(0.35f * SampleRate);

    TArray<int16> Raw;
    Raw.SetNumUninitialized(NumSamples);
    for (int32 i = 0; i < NumSamples; ++i)
    {
        const float t = (float)i / SampleRate;
        float f1 = 0.55f * FMath::Sin(2.f * PI * 120.f * t);      // formante bajo
        float f2 = 0.30f * FMath::Sin(2.f * PI * 240.f * t);      // formante medio
        float f3 = 0.15f * FMath::Sin(2.f * PI * 480.f * t);      // formante alto
        float vib = 1.f + 0.06f * FMath::Sin(2.f * PI * 5.f * t); // vibrato
        float env = FMath::Min(1.f, t * 40.f) * FMath::Min(1.f, (0.35f - t) * 60.f);
        Raw[i] = (int16)((f1 + f2 + f3) * vib * env * 9000.f);
    }

    USoundWaveProcedural* Voz = NewObject<USoundWaveProcedural>(this);
    Voz->SetSampleRate(SampleRate);
    Voz->QueueAudio(reinterpret_cast<const uint8*>(Raw.GetData()), Raw.Num() * sizeof(int16));

    // Reproduce con tono por persona (0.75-1.3): voz distinta y audible por NPC
    UGameplayStatics::PlaySoundAtLocation(
        W, Voz, Posicion, FRotator::ZeroRotator, 1.f, Pitch, 0.f);
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
    // ~30% de los NPCs usan el cuerpo de Fab (2ª silueta) para romper los clones;
    // el resto alterna mannequin por grupo de edad como antes.
    if (MeshFab && (GetTypeHash(NPC.Nombre) % 10) < 3)
        MeshAUsar = MeshFab;
    else if (NPC.GrupoEdad <= 1)
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

        // Variación visual: tinte de ropa por estilo + género sobre el mannequin.
        // Ropa física distinta se añadirá cuando haya meshes de NPC; hasta entonces
        // el color rompe la uniformidad de los 600 clonados.
        {
            const FLinearColor Ropa = ColorRopaPara(NPC.Persona.Estilo, NPC.GrupoEdad >= 2);
            const FLinearColor Piel = FLinearColor::LerpUsingHSV(
                FLinearColor(0.85f, 0.69f, 0.55f), FLinearColor(0.42f, 0.28f, 0.2f),
                FMath::FRand());
            UMaterialInstanceDynamic* MID = NPCActor->GetSkeletalMeshComponent()->CreateAndSetMaterialInstanceDynamic(0);
            if (MID)
            {
                MID->SetVectorParameterValue(TEXT("BODY_COLOR"), Ropa);
                MID->SetVectorParameterValue(TEXT("SKIN_COLOR"), Piel);
            }
        }

        float Escala = (NPC.GrupoEdad == 3) ? 0.85f : (NPC.GrupoEdad == 0 ? 0.92f : (NPC.GrupoEdad == 2 ? 0.97f : 1.0f));
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

    // Color por estilo para coherencia con el full-LOD (antes por barrio).
    const FLinearColor ProxyColor = ColorRopaPara(NPC.Persona.Estilo, NPC.GrupoEdad >= 2);

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
