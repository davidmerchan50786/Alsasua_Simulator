// ArmasComponent.cpp
#include "ArmasComponent.h"
#include "DrogasSubsystem.h"
#include "WantedSubsystem.h"
#include "ConsecuenciasSubsystem.h"
#include "DisfrazSubsystem.h"
#include "AlsasuaPlayerCharacter.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UArmasComponent::UArmasComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UArmasComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Municion.Num() < 10) Municion.SetNumZeroed(10);
}

void UArmasComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (Cooldown > 0.f) Cooldown -= DeltaTime;
}

void UArmasComponent::RecogerArma(ETipoArma Arma, int32 Cantidad)
{
	const int32 i = (int32)Arma;
	if (Municion.Num() < 10) Municion.SetNumZeroed(10);
	if (Cantidad > 0) Municion[i] += Cantidad;
}

void UArmasComponent::UsarArma()
{
	if (Cooldown > 0.f) return;
	switch (ArmaActual)
	{
	case ETipoArma::Punos:    GolpearMelee(); break;
	case ETipoArma::Pistola:  DispararFuego(34, 1, 0.6f, 0.20f); break;
	case ETipoArma::Escopeta: DispararFuego(13, 8, 7.f,  0.75f); break;
	case ETipoArma::Fusil:    DispararFuego(26, 1, 1.3f, 0.11f); break;
	default: break;
	}
}

void UArmasComponent::GolpearMelee()
{
	Cooldown = 0.4f;
	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	if (GetWorld()->SweepSingleByChannel(Hit, Origen, Origen + Dir * 230.f, FQuat::Identity,
			ECC_Pawn, FCollisionShape::MakeSphere(50.f), Q))
	{
		if (IDamageable* D = Cast<IDamageable>(Hit.GetActor()))
			if (!D->EstaMuerto()) { D->RecibirDano(20, Hit.ImpactPoint, ETipoDano::Impacto); Consecuencias(Hit.GetActor()); }
	}
	SubirBusqueda(1);
}

void UArmasComponent::DispararFuego(int32 Dano, int32 Perdigones, float DispersionGrados, float Cadencia)
{
	const int32 i = (int32)ArmaActual;
	if (Municion.IsValidIndex(i) && Municion[i] <= 0) { Cooldown = 0.25f; return; }
	Cooldown = Cadencia;
	if (Municion.IsValidIndex(i)) Municion[i]--;

	// Disparar te delata (se cae el disfraz).
	if (const UWorld* W = GetWorld())
		if (UGameInstance* GI = W->GetGameInstance())
			if (UDisfrazSubsystem* Dis = GI->GetSubsystem<UDisfrazSubsystem>()) Dis->Delatar();

	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	// Sonido de disparo (si el asset existe).
	static USoundBase* SDisparo = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Disparo.SC_Disparo"));
	if (SDisparo && GetOwner()) UGameplayStatics::PlaySoundAtLocation(GetWorld(), SDisparo, GetOwner()->GetActorLocation());

	// Fogonazo en el cañón (socket "Muzzle_01" de la malla; si no, en el origen).
	static UNiagaraSystem* NSFogo = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Fogonazo.NS_Fogonazo"));
	if (NSFogo)
	{
		if (const ACharacter* Ch = Cast<ACharacter>(GetOwner()))
			UNiagaraFunctionLibrary::SpawnSystemAttached(NSFogo, Ch->GetMesh(), TEXT("Muzzle_01"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		else
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NSFogo, Origen + Dir * 60.f, Dir.Rotation());
	}

	const float Disp = DispersionGrados * MultDispersionActual();   // borrachera/drogas empeoran la puntería
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());

	for (int32 p = 0; p < FMath::Max(1, Perdigones); ++p)
	{
		FVector D = Dir;
		if (Disp > 0.f)
			D = FRotator(FMath::FRandRange(-Disp, Disp), FMath::FRandRange(-Disp, Disp), 0.f).RotateVector(Dir);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Origen, Origen + D * 30000.f, ECC_Visibility, Q))
		{
			if (IDamageable* Dmg = Cast<IDamageable>(Hit.GetActor()))
				if (!Dmg->EstaMuerto())
				{
					Dmg->RecibirDano(Dano, Hit.ImpactPoint, ETipoDano::Bala);
					Consecuencias(Hit.GetActor());
					// Sangre (si el asset existe).
					static UNiagaraSystem* NSSangre = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Sangre.NS_Sangre"));
					if (NSSangre) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NSSangre, Hit.ImpactPoint, (-D).Rotation());
				}

			// Impacto visible + sonido en la superficie (si los assets existen).
			static UNiagaraSystem* NSImpacto = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Impacto.NS_Impacto"));
			if (NSImpacto)
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), NSImpacto, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			static USoundBase* SImpacto = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Impacto.SC_Impacto"));
			if (SImpacto)
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), SImpacto, Hit.ImpactPoint);
		}
	}
	// Retroceso: patada hacia arriba + pequeño desvío lateral (menor al apuntar).
	if (APawn* P = Cast<APawn>(GetOwner()))
		if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			float Kick = 0.6f + DispersionGrados * 0.15f;          // armas más dispersas patean más
			if (const AAlsasuaPlayerCharacter* Ch = Cast<AAlsasuaPlayerCharacter>(P)) if (Ch->EstaApuntando()) Kick *= 0.5f;
			PC->AddPitchInput(-Kick);                               // hacia arriba
			PC->AddYawInput(FMath::FRandRange(-Kick, Kick) * 0.3f); // desvío lateral
		}

	SubirBusqueda(3);
}

bool UArmasComponent::ObtenerMira(FVector& OutOrigen, FVector& OutDir) const
{
	if (const APawn* P = Cast<APawn>(GetOwner()))
		if (const AController* C = P->GetController())
		{
			FRotator R; C->GetPlayerViewPoint(OutOrigen, R);
			OutDir = R.Vector();
			return true;
		}
	return false;
}

FString UArmasComponent::NombreArma() const
{
	switch (ArmaActual)
	{
	case ETipoArma::Punos:     return TEXT("Punos");
	case ETipoArma::Spray:     return TEXT("Spray");
	case ETipoArma::Tirachinas:return TEXT("Tirachinas");
	case ETipoArma::Molotov:   return TEXT("Molotov");
	case ETipoArma::Ikurrina:  return TEXT("Ikurrina");
	case ETipoArma::BombaLapa: return TEXT("Bomba lapa");
	case ETipoArma::CocheBomba:return TEXT("Coche bomba");
	case ETipoArma::Pistola:   return TEXT("Pistola");
	case ETipoArma::Escopeta:  return TEXT("Escopeta");
	case ETipoArma::Fusil:     return TEXT("Fusil");
	default:                   return TEXT("Arma");
	}
}

float UArmasComponent::MultDispersionActual() const
{
	float Mult = 1.f;
	if (const UWorld* W = GetWorld())
		if (const UGameInstance* GI = W->GetGameInstance())
			if (const UDrogasSubsystem* Dr = GI->GetSubsystem<UDrogasSubsystem>())
				Mult = Dr->MultDispersion();

	// Apuntar (ADS) mejora la puntería.
	if (const AAlsasuaPlayerCharacter* C = Cast<AAlsasuaPlayerCharacter>(GetOwner()))
		if (C->EstaApuntando()) Mult *= 0.35f;

	return Mult;
}

void UArmasComponent::SubirBusqueda(int32 Cantidad) const
{
	if (const UWorld* W = GetWorld())
		if (UGameInstance* GI = W->GetGameInstance())
			if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
				Wn->AumentarBusqueda(Cantidad);
}

void UArmasComponent::Consecuencias(AActor* Victima) const
{
	if (const UWorld* W = GetWorld())
		if (UGameInstance* GI = W->GetGameInstance())
			if (UConsecuenciasSubsystem* C = GI->GetSubsystem<UConsecuenciasSubsystem>())
				C->RegistrarDano(Victima);
}
