// AlsasuaNPC.cpp
#include "AlsasuaNPC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"

AAlsasuaNPC::AAlsasuaNPC()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAlsasuaNPC::RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo)
{
	if (bMuerto) return;
	Vida = FMath::Max(0, Vida - Cantidad);
	UltimoOrigenDano = Origen;
	if (Vida <= 0) Morir();
}

void AAlsasuaNPC::Morir()
{
	if (bMuerto) return;
	bMuerto = true;

	// Ragdoll + empuje en la dirección del disparo.
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetCollisionProfileName(TEXT("Ragdoll"));
		M->SetSimulatePhysics(true);
		FVector Dir = (GetActorLocation() - UltimoOrigenDano);
		Dir.Z = FMath::Abs(Dir.Z) + 200.f;   // algo hacia arriba
		M->AddImpulse(Dir.GetSafeNormal() * ImpulsoMuerte, NAME_None, true);
	}
	if (UCharacterMovementComponent* Mov = GetCharacterMovement()) Mov->DisableMovement();
	if (UCapsuleComponent* Cap = GetCapsuleComponent()) Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetLifeSpan(DuracionCadaver);   // limpiar el cadáver
}
