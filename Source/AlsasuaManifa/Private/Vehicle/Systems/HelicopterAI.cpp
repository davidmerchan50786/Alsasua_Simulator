#include "Vehicle/Systems/HelicopterAI.h"
#include "Components/SpotLightComponent.h"

AHelicopterAI::AHelicopterAI() {
    PrimaryActorTick.bCanEverTick = true;
    SearchLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SearchLight"));
    SearchLight->Intensity = 100000.f;
    SearchLight->OuterConeAngle = 30.f;
}

void AHelicopterAI::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
    if(Target) {
        FVector TargetLoc = Target->GetActorLocation();
        FRotator TargetRot = (TargetLoc - GetActorLocation()).Rotation();
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 2.0f));
        // El helicoptero siempre vuela alto sobre el jugador
        FVector DesiredLoc = TargetLoc + FVector(0,0, 2000.f);
        SetActorLocation(FMath::VInterpTo(GetActorLocation(), DesiredLoc, DeltaTime, 1.5f));
    }
}