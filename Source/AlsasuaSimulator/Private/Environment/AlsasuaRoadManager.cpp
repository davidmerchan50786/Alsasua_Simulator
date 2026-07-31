#include "Environment/AlsasuaRoadManager.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAlsasuaRoadManager::AAlsasuaRoadManager() {
    RoadSpline = CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));
    RootComponent = RoadSpline;
}

void AAlsasuaRoadManager::GenerateRoadMesh()
{
    if (!RoadSpline) return;

    // Limpiar splinemeshes previos.
    TArray<USceneComponent*> ChildComps;
    RoadSpline->GetChildrenComponents(false, ChildComps);
    for (USceneComponent* Child : ChildComps)
    {
        if (USplineMeshComponent* SMC = Cast<USplineMeshComponent>(Child))
        {
            SMC->DestroyComponent();
        }
    }

    const int32 NumPoints = RoadSpline->GetNumberOfSplinePoints();
    if (NumPoints < 2) return;

    const int32 Segments = NumPoints - 1;

    for (int32 i = 0; i < Segments; ++i)
    {
        USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
        if (!SplineMesh) continue;

        SplineMesh->SetMobility(EComponentMobility::Static);
        SplineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        SplineMesh->SetCollisionObjectType(ECC_WorldStatic);
        SplineMesh->SetCastShadow(true);
        SplineMesh->SetAffectDistanceFieldLighting(true);

        SplineMesh->RegisterComponent();
        SplineMesh->AttachToComponent(RoadSpline, FAttachmentTransformRules::KeepRelativeTransform);

        FVector StartPos, StartTangent, EndPos, EndTangent;
        RoadSpline->GetLocationAndTangentAtSplinePoint(i, StartPos, StartTangent, ESplineCoordinateSpace::Local);
        RoadSpline->GetLocationAndTangentAtSplinePoint(i + 1, EndPos, EndTangent, ESplineCoordinateSpace::Local);

        SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);

        const float RoadWidth = 800.0f;
        const float RoadHeight = 20.0f;
        SplineMesh->SetStartScale(FVector2D(1.0f, 1.0f));
        SplineMesh->SetEndScale(FVector2D(1.0f, 1.0f));
        SplineMesh->SetStartOffset(FVector2D(0.0f, 0.0f));
        SplineMesh->SetEndOffset(FVector2D(0.0f, 0.0f));
        SplineMesh->SetForwardAxis(ESplineMeshAxis::X, true);
    }

    UE_LOG(LogTemp, Log, TEXT("[AlsasuaEnv] Carretera generada: %d segmentos desde %d puntos"),
        Segments, NumPoints);
}
