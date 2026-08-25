// AlsasuaNPC.cpp
#include "AlsasuaNPC.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/AlsasuaParanoiaComponent.h"
#include "Audio/AlsasuaWhisperManager.h"
#include "Sound/SoundBase.h"

AAlsasuaNPC::AAlsasuaNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	ParanoiaComp = CreateDefaultSubobject<UAlsasuaParanoiaComponent>(TEXT("ParanoiaComp"));
	WhisperComp = CreateDefaultSubobject<UAlsasuaWhisperManager>(TEXT("WhisperComp"));
}

void AAlsasuaNPC::BeginPlay()
{
	Super::BeginPlay();
	if (ParanoiaComp)
	{
		ParanoiaComp->OnParanoiaChanged.AddDynamic(this, &AAlsasuaNPC::OnParanoiaLevelChanged);
	}
}

void AAlsasuaNPC::OnParanoiaLevelChanged(float NewLevel)
{
	// Whisper at paranoia thresholds — subtle audio feedback for nearby players.
	if (WhisperComp && !bMuerto)
	{
		static USoundBase* WhisperSound = nullptr;
		if (!WhisperSound)
			WhisperSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Whisper.SB_Whisper"));

		if (WhisperSound)
		{
			if (NewLevel > 75.f && LastWhisperThreshold < 75.f)
				WhisperComp->PlaySpatialWhisper(WhisperSound, 0.6f);
			else if (NewLevel > 50.f && LastWhisperThreshold < 50.f)
				WhisperComp->PlaySpatialWhisper(WhisperSound, 0.3f);
		}
	}
	LastWhisperThreshold = NewLevel;
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
