#include "Systemics/Sabotage/SabotageComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMeshActor.h"

USabotageComponent::USabotageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USabotageComponent::BeginSabotage(EDamageType Type, float Intensity)
{
    if (Type == EDamageType::None || Intensity <= 0.f) return;
    const float Old = CurrentDamage;
    ActiveDamage = Type;
    CurrentDamage = FMath::Clamp(CurrentDamage + Intensity, 0.f, 100.f);
    ApplyVisualDamage();
    BroadcastIfChanged(Old);
}

void USabotageComponent::Repair(float DeltaTime)
{
    if (!bCanRepair || CurrentDamage <= 0.f) return;
    const float Old = CurrentDamage;
    CurrentDamage = FMath::Max(0.f, CurrentDamage - RepairRate * DeltaTime);
    BroadcastIfChanged(Old);
    if (CurrentDamage <= 0.f) { ClearVisualDamage(); ActiveDamage = EDamageType::None; }
}

void USabotageComponent::ApplyVisualDamage()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;
    if (!CachedMesh) CachedMesh = Owner->FindComponentByClass<UStaticMeshComponent>();
    UStaticMeshComponent* Mesh = CachedMesh;
    if (!Mesh) return;

    if (!bOriginalCaptured)
    {
        OriginalMaterials = Mesh->GetMaterials();
        OriginalScale = Mesh->GetRelativeScale3D();
        bOriginalCaptured = true;
    }

    if (DamageMIDs.Num() != OriginalMaterials.Num())
    {
        DamageMIDs.SetNum(OriginalMaterials.Num());
        for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
        {
            if (!OriginalMaterials[i]) continue;
            DamageMIDs[i] = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, OriginalMaterials[i]);
        }
    }

    const float D = CurrentDamage / 100.f;
    const FLinearColor DamagedColor(D - 1.f, D - 1.f, D - 1.f);
    for (UMaterialInstanceDynamic* MID : DamageMIDs)
    {
        if (MID) MID->SetVectorParameterValue(FName("BaseColor"), DamagedColor);
    }

    UWorld* World = GetWorld();
    if (!World) return;

    if (ActiveDamage == EDamageType::Fire && !FireIndicator)
    {
        FVector FirePos = Mesh->Bounds.GetBox().GetCenter();
        FirePos.Z += Mesh->Bounds.SphereRadius;
        AStaticMeshActor* FireCube = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), FirePos, FRotator::ZeroRotator);
        if (FireCube)
        {
            FireCube->SetMobility(EComponentMobility::Movable);
            FireCube->GetStaticMeshComponent()->SetStaticMesh(
                LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
            UMaterialInstanceDynamic* FireMID = FireCube->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0);
            if (FireMID) FireMID->SetVectorParameterValue(FName("BaseColor"), FLinearColor(1.f, 0.3f, 0.f));
            float R = Mesh->Bounds.SphereRadius / 100.f;
            FireCube->SetActorScale3D(FVector(R * 0.5f, R * 0.5f, R));
            FireIndicator = FireCube;
        }
    }
    else if (ActiveDamage == EDamageType::RoadBlock)
    {
        const FVector S = OriginalScale;
        Mesh->SetRelativeScale3D(FVector(S.X, S.Y, S.Z * (1.f - 0.5f * D)));
    }
}

void USabotageComponent::ClearVisualDamage()
{
    UStaticMeshComponent* Mesh = CachedMesh;
    if (Mesh && bOriginalCaptured)
    {
        for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
            Mesh->SetMaterial(i, OriginalMaterials[i]);
        Mesh->SetRelativeScale3D(OriginalScale);
    }
    if (FireIndicator) { FireIndicator->Destroy(); FireIndicator = nullptr; }
    DamageMIDs.Empty();
}

void USabotageComponent::BroadcastIfChanged(float OldDamage)
{
    if (!FMath::IsNearlyEqual(OldDamage, CurrentDamage))
        OnDamageChanged.Broadcast(CurrentDamage);
}
