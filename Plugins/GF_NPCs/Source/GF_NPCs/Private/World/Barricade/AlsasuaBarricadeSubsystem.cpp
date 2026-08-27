#include "World/Barricade/AlsasuaBarricadeSubsystem.h"
#include "World/Barricade/AlsasuaBarricadeActor.h"
#include "World/AlsasuaRedViaria.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaBarricadeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAlsasuaBarricadeSubsystem::Tick(float DeltaTime)
{
    LimpiarMuertas();
    SpawnCooldown = FMath::Max(0.f, SpawnCooldown - DeltaTime);
}

AAlsasuaBarricadeActor* UAlsasuaBarricadeSubsystem::ColocarBarricada(
    const FVector& Location, EBarricadeType Tipo, int32 TramoIdx)
{
    LimpiarMuertas();

    if (Barricadas.Num() >= MaxBarricadas) return nullptr;
    if (SpawnCooldown > 0.f) return nullptr;

    UWorld* W = GetWorld();
    if (!W) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AAlsasuaBarricadeActor* Barricada = W->SpawnActor<AAlsasuaBarricadeActor>(
        AAlsasuaBarricadeActor::StaticClass(), Location, FRotator::ZeroRotator, Params);

    if (Barricada)
    {
        Barricada->Tipo = Tipo;
        Barricada->TramoBloqueado = TramoIdx;
        Barricadas.Add(Barricada);
        SpawnCooldown = VidaEntreBarricadas;

        // Slight random rotation for visual variety
        Barricada->SetActorRotation(FRotator(0.f, FMath::FRandRange(-30.f, 30.f), 0.f));
    }

    return Barricada;
}

void UAlsasuaBarricadeSubsystem::LimpiarBarricadas()
{
    for (TWeakObjectPtr<AAlsasuaBarricadeActor>& WeakB : Barricadas)
    {
        if (WeakB.IsValid())
            WeakB->Destroy();
    }
    Barricadas.Empty();
}

int32 UAlsasuaBarricadeSubsystem::BloquearTramo(int32 TramoIdx, int32 NumBarricadas)
{
    UWorld* W = GetWorld();
    if (!W) return 0;

    // Get the road network to find segment positions
    UAlsasuaRedViaria* Red = W->GetSubsystem<UAlsasuaRedViaria>();
    if (!Red || !Red->EstaLista()) return 0;

    const FTramoViario& Tramo = Red->Tramo(TramoIdx);
    const FVector A = Red->PosicionNodo(Tramo.NodoA);
    const FVector B = Red->PosicionNodo(Tramo.NodoB);

    int32 Colocadas = 0;
    for (int32 i = 0; i < NumBarricadas; ++i)
    {
        const float Alpha = (i + 1.f) / (NumBarricadas + 1.f);
        const FVector Pos = FMath::Lerp(A, B, Alpha);
        const float Offset = FMath::FRandRange(-Tramo.AnchoCm * 0.3f, Tramo.AnchoCm * 0.3f);
        const FVector Perp = (B - A).GetSafeNormal().RotateAngleAxis(90.f, FVector::UpVector);
        const FVector FinalPos = Pos + Perp * Offset;

        // Random barricade type
        const EBarricadeType Tipo = static_cast<EBarricadeType>(FMath::RandRange(0, 4));

        if (ColocarBarricada(FinalPos, Tipo, TramoIdx))
            ++Colocadas;
    }

    return Colocadas;
}

bool UAlsasuaBarricadeSubsystem::EstaBloqueado(const FVector& Location, float Radius) const
{
    const float RadiusSq = FMath::Square(Radius);
    for (const TWeakObjectPtr<AAlsasuaBarricadeActor>& WeakB : Barricadas)
    {
        if (WeakB.IsValid() && !WeakB->EstaDestruida())
        {
            if (FVector::DistSquared(Location, WeakB->GetActorLocation()) < RadiusSq)
                return true;
        }
    }
    return false;
}

void UAlsasuaBarricadeSubsystem::LimpiarMuertas()
{
    for (int32 i = Barricadas.Num() - 1; i >= 0; --i)
    {
        if (!Barricadas[i].IsValid() || Barricadas[i]->EstaDestruida())
            Barricadas.RemoveAt(i);
    }
}
