#include "AI/Crowd/ManifaFormationManager.h"
#include "AI/AlsasuaCrowdAgentComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"

void UManifaFormationManager::CreateFormation(AActor* Leader, EFormationType Type) {
    if (!Leader) return;

    TArray<AActor*> Followers;
    TArray<AActor*> OverlappingActors;
    Leader->GetOverlappingActors(OverlappingActors);

    for (AActor* Actor : OverlappingActors) {
        if (Actor == Leader) continue;
        if (UAlsasuaCrowdAgentComponent* Crowd = Actor->FindComponentByClass<UAlsasuaCrowdAgentComponent>()) {
            if (Followers.Num() < 20) {
                Followers.Add(Actor);
                Crowd->CurrentState = ECrowdAgentState::Following;
            }
        }
    }

    ActiveFormations.Add(Leader, Followers);
    UE_LOG(LogTemp, Warning, TEXT("Formaci�n creada con %d seguidores."), Followers.Num());
}

TArray<FVector> UManifaFormationManager::CalculateFormationOffsets(EFormationType Type, int32 Count, float Spacing) {
    TArray<FVector> Offsets;
    for (int32 i = 0; i < Count; i++) {
        FVector Offset = FVector::ZeroVector;
        switch (Type) {
            case EFormationType::Line:
                Offset = FVector(0, (i - Count/2) * Spacing, 0);
                break;
            case EFormationType::Wedge:
                {
                    int32 Row = FMath::FloorToInt(FMath::Sqrt(static_cast<float>(2 * i + 0.25f)) - 0.5f);
                    int32 Col = i - (Row * (Row + 1) / 2);
                    Offset = FVector(-Row * Spacing, (Col - Row/2.f) * Spacing, 0);
                }
                break;
            case EFormationType::Column:
                Offset = FVector(-(i + 1) * Spacing, 0, 0);
                break;
        }
        Offsets.Add(Offset);
    }
    return Offsets;
}

void UManifaFormationManager::UpdateFormations(float DeltaTime) {
    for (auto& Elem : ActiveFormations) {
        AActor* Leader = Elem.Key;
        TArray<AActor*>& Followers = Elem.Value;
        if (!Leader) continue;

        TArray<FVector> Offsets = CalculateFormationOffsets(EFormationType::Wedge, Followers.Num(), 150.f);
        FVector LeaderForward = Leader->GetActorForwardVector();
        FVector LeaderRight = Leader->GetActorRightVector();
        FVector LeaderLoc = Leader->GetActorLocation();

        for (int32 i = 0; i < Followers.Num(); i++) {
            if (!Followers[i]) continue;

            FVector TargetLoc = LeaderLoc + (LeaderForward * Offsets[i].X) + (LeaderRight * Offsets[i].Y);

            DrawDebugSphere(GetWorld(), TargetLoc, 50.f, 8, FColor::Green, false, 0.1f);

            if (APawn* Pawn = Cast<APawn>(Followers[i])) {
                if (AAIController* AIC = Cast<AAIController>(Pawn->GetController())) {
                    AIC->MoveToLocation(TargetLoc, 50.f, true, true, false, true);
                }
            }
        }
    }
}
