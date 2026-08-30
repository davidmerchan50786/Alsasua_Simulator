// ArmasComponent.cpp
#include "ArmasComponent.h"
#include "DrogasSubsystem.h"
#include "WantedSubsystem.h"
#include "ConsecuenciasSubsystem.h"
#include "DisfrazSubsystem.h"
#include "AlsasuaPlayerCharacter.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "PoblacionSubsystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/OverlapResult.h"

UArmasComponent::UArmasComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UArmasComponent::BeginPlay()
{
	Super::BeginPlay();
	InitAmmoArrays();
	PreloadAssets();
}

void UArmasComponent::PreloadAssets()
{
	SDisparo = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Disparo.SC_Disparo"));
	SImpacto = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Impacto.SC_Impacto"));
	SMolotov = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Molotov.SC_Molotov"));
	SBombaColocar = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_BombaColocar.SC_BombaColocar"));
	SExplosion = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Explosion.SC_Explosion"));
	SExplosionGrande = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_ExplosionGrande.SC_ExplosionGrande"));

	NSFogonazo = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Fogonazo.NS_Fogonazo"));
	NSSangre = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Sangre.NS_Sangre"));
	NSImpacto = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Impacto.NS_Impacto"));
	NSSpray = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Spray.NS_Spray"));
	NSMolotov = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Molotov.NS_Molotov"));
	NSExplosion = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Explosion.NS_Explosion"));
	NSExplosionCoche = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_ExplosionCoche.NS_ExplosionCoche"));
}

void UArmasComponent::InitAmmoArrays()
{
	const int32 NumWeapons = 10;
	if (Municion.Num() < NumWeapons) Municion.SetNumZeroed(NumWeapons);
	if (MunicionMax.Num() < NumWeapons) MunicionMax.SetNum(NumWeapons);
	if (Reserva.Num() < NumWeapons) Reserva.SetNumZeroed(NumWeapons);
	if (TiempoRecarga.Num() < NumWeapons) TiempoRecarga.SetNum(NumWeapons);

	// Valores por defecto por arma.
	if (MunicionMax[(int32)ETipoArma::Pistola] == 0)  MunicionMax[(int32)ETipoArma::Pistola] = 12;
	if (MunicionMax[(int32)ETipoArma::Escopeta] == 0) MunicionMax[(int32)ETipoArma::Escopeta] = 8;
	if (MunicionMax[(int32)ETipoArma::Fusil] == 0)     MunicionMax[(int32)ETipoArma::Fusil] = 30;

	if (TiempoRecarga[(int32)ETipoArma::Pistola] == 0.f)  TiempoRecarga[(int32)ETipoArma::Pistola] = 1.5f;
	if (TiempoRecarga[(int32)ETipoArma::Escopeta] == 0.f) TiempoRecarga[(int32)ETipoArma::Escopeta] = 2.0f;
	if (TiempoRecarga[(int32)ETipoArma::Fusil] == 0.f)     TiempoRecarga[(int32)ETipoArma::Fusil] = 2.5f;

	// Munición inicial.
	if (Municion[(int32)ETipoArma::Pistola] == 0)  Municion[(int32)ETipoArma::Pistola] = 12;
	if (Municion[(int32)ETipoArma::Escopeta] == 0) Municion[(int32)ETipoArma::Escopeta] = 8;
	if (Municion[(int32)ETipoArma::Fusil] == 0)     Municion[(int32)ETipoArma::Fusil] = 30;

	// Reserva inicial.
	if (Reserva[(int32)ETipoArma::Pistola] == 0)  Reserva[(int32)ETipoArma::Pistola] = 60;
	if (Reserva[(int32)ETipoArma::Escopeta] == 0) Reserva[(int32)ETipoArma::Escopeta] = 32;
	if (Reserva[(int32)ETipoArma::Fusil] == 0)     Reserva[(int32)ETipoArma::Fusil] = 120;
}

void UArmasComponent::TickComponent(float DeltaTime, ELevelTick, FActorComponentTickFunction*)
{
	if (Cooldown > 0.f) Cooldown -= DeltaTime;

	// Recoil recovers when not firing.
	if (RecoilStack > 0.f) RecoilStack = FMath::Max(0.f, RecoilStack - RecoilDecay * DeltaTime);

	if (bReloading)
	{
		ReloadTimer -= DeltaTime;
		if (ReloadTimer <= 0.f)
		{
			FinishReload();
		}
	}
}

void UArmasComponent::NotifyNearbyGuards(FVector Location, float Loudness)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// Alert pedestrians to flee.
	UPoblacionSubsystem::OnLoudNoise.Broadcast(Location);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(3000.f);
	if (W->OverlapMultiByChannel(Overlaps, Location, FQuat::Identity, ECC_Pawn, Shape))
	{
		for (const FOverlapResult& Ov : Overlaps)
		{
			if (UGuardDetectionComponent* Det = Ov.GetActor()->FindComponentByClass<UGuardDetectionComponent>())
				Det->ReportNoise(Location, Loudness);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Munición
// ─────────────────────────────────────────────────────────────────────────────
int32 UArmasComponent::MunicionActual() const
{
	const int32 i = (int32)ArmaActual;
	return Municion.IsValidIndex(i) ? Municion[i] : 0;
}

int32 UArmasComponent::MunicionMaxActual() const
{
	const int32 i = (int32)ArmaActual;
	return MunicionMax.IsValidIndex(i) ? MunicionMax[i] : 0;
}

int32 UArmasComponent::ReservaActual() const
{
	const int32 i = (int32)ArmaActual;
	return Reserva.IsValidIndex(i) ? Reserva[i] : 0;
}

bool UArmasComponent::CanReload() const
{
	if (bReloading) return false;
	if (EsCuerpoACuerpo()) return false;

	const int32 i = (int32)ArmaActual;
	if (!Municion.IsValidIndex(i) || !MunicionMax.IsValidIndex(i) || !Reserva.IsValidIndex(i)) return false;

	return Municion[i] < MunicionMax[i] && Reserva[i] > 0;
}

void UArmasComponent::CambiarArma(ETipoArma Arma)
{
	if (bReloading) return; // No cambiar de arma durante recarga.
	ArmaActual = Arma;
	OnAmmoChanged.Broadcast(MunicionActual());
}

void UArmasComponent::RecogerArma(ETipoArma Arma, int32 Cantidad)
{
	const int32 i = (int32)Arma;
	if (i < 0 || i >= 10) return;
	if (Municion.Num() < 10) Municion.SetNumZeroed(10);
	if (Cantidad > 0) Municion[i] += Cantidad;
	OnAmmoChanged.Broadcast(MunicionActual());
}

void UArmasComponent::RecogerMunicion(ETipoArma Arma, int32 Cantidad)
{
	const int32 i = (int32)Arma;
	if (i < 0 || i >= 10) return;
	if (Reserva.Num() < 10) Reserva.SetNumZeroed(10);
	if (Cantidad > 0) Reserva[i] += Cantidad;
	OnAmmoChanged.Broadcast(MunicionActual());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Usar Arma
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::UsarArma()
{
	if (Cooldown > 0.f || bReloading) return;

	switch (ArmaActual)
	{
	case ETipoArma::Punos:     GolpearMelee(); break;
	case ETipoArma::Spray:     LanzarSpray(); break;
	case ETipoArma::Tirachinas:LanzarTirachinas(); break;
	case ETipoArma::Molotov:   LanzarMolotov(); break;
	case ETipoArma::Ikurrina:  GolpearIkurrina(); break;
	case ETipoArma::BombaLapa: ColocarBombaLapa(); break;
	case ETipoArma::CocheBomba:CocheBombaDetonar(); break;
	case ETipoArma::Pistola:   DispararFuego(34, 1, 0.6f, 0.20f); break;
	case ETipoArma::Escopeta:  DispararFuego(13, 8, 7.f,  0.75f); break;
	case ETipoArma::Fusil:     DispararFuego(26, 1, 1.3f, 0.11f); break;
	default: break;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Reload
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::Reload()
{
	if (!CanReload()) return;

	const int32 i = (int32)ArmaActual;
	const float Time = TiempoRecarga.IsValidIndex(i) ? TiempoRecarga[i] : 2.0f;

	bReloading = true;
	ReloadTimer = Time;
	OnReloadStarted.Broadcast();
}

void UArmasComponent::FinishReload()
{
	bReloading = false;

	const int32 i = (int32)ArmaActual;
	if (!Municion.IsValidIndex(i) || !MunicionMax.IsValidIndex(i) || !Reserva.IsValidIndex(i)) return;

	const int32 Needed = MunicionMax[i] - Municion[i];
	const int32 Available = Reserva[i];
	const int32 ToLoad = FMath::Min(Needed, Available);

	Municion[i] += ToLoad;
	Reserva[i] -= ToLoad;

	OnReloadFinished.Broadcast();
	OnAmmoChanged.Broadcast(MunicionActual());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Melee
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::GolpearMelee()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Cooldown = 0.4f;
	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	if (W->SweepSingleByChannel(Hit, Origen, Origen + Dir * 230.f, FQuat::Identity,
			ECC_Pawn, FCollisionShape::MakeSphere(50.f), Q))
	{
		if (IDamageable* D = Cast<IDamageable>(Hit.GetActor()))
			if (!D->EstaMuerto()) { D->RecibirDano(20, Hit.ImpactPoint, ETipoDano::Impacto); Consecuencias(Hit.GetActor()); }
	}
	SubirBusqueda(1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Disparo
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::DispararFuego(int32 Dano, int32 Perdigones, float DispersionGrados, float Cadencia)
{
	UWorld* W = GetWorld();
	if (!W) return;

	const int32 i = (int32)ArmaActual;
	if (Municion.IsValidIndex(i) && Municion[i] <= 0)
	{
		// Sin munición: auto-recargar si hay reserva.
		if (CanReload())
		{
			Reload();
		}
		Cooldown = 0.25f;
		return;
	}
	Cooldown = Cadencia;
	if (Municion.IsValidIndex(i)) Municion[i]--;
	OnAmmoChanged.Broadcast(MunicionActual());

	// Disparar te delata (se cae el disfraz).
	if (UGameInstance* GI = W->GetGameInstance())
		if (UDisfrazSubsystem* Dis = GI->GetSubsystem<UDisfrazSubsystem>()) Dis->Delatar();

	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	// Sonido de disparo.
	if (SDisparo && GetOwner()) UGameplayStatics::PlaySoundAtLocation(W, SDisparo, GetOwner()->GetActorLocation());

	// Notificar a guardias cercanos del sonido del disparo.
	if (GetOwner()) NotifyNearbyGuards(GetOwner()->GetActorLocation(), 1.0f);

	// Fogonazo en el cañón.
	if (NSFogonazo)
	{
		if (const ACharacter* Ch = Cast<ACharacter>(GetOwner()))
			UNiagaraFunctionLibrary::SpawnSystemAttached(NSFogonazo, Ch->GetMesh(), TEXT("Muzzle_01"), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		else
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSFogonazo, Origen + Dir * 60.f, Dir.Rotation());
	}

	const float Disp = DispersionGrados * MultDispersionActual();
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());

	for (int32 p = 0; p < FMath::Max(1, Perdigones); ++p)
	{
		FVector D = Dir;
		if (Disp > 0.f)
			D = FRotator(FMath::FRandRange(-Disp, Disp), FMath::FRandRange(-Disp, Disp), 0.f).RotateVector(Dir);

		// Bullet penetration: trace along the full path and process every hit.
		// Pass through up to MaxPenetration non-damageable obstacles (thin walls),
		// stop on the first damageable target like a real bullet.
		FHitResult Hit;
		FVector TraceStart = Origen;
		int32 Penetrated = 0;
		bool bHitSomething = false;

		while (true)
		{
			if (!W->LineTraceSingleByChannel(Hit, TraceStart, Origen + D * 30000.f, ECC_Visibility, Q))
				break;

			bHitSomething = true;
			TraceStart = Hit.ImpactPoint + D * 2.f;

			IDamageable* Dmg = Cast<IDamageable>(Hit.GetActor());
			const bool bIsDamageable = Dmg && !Dmg->EstaMuerto();

			if (bIsDamageable || !bStopOnFirstHit)
			{
				// Headshot: hit bone at or under "head". Damage falloff by distance.
				const float Dist = Hit.Distance;
				float FinalDano = (float)Dano;
				if (Dist > DamageFalloffStart)
					FinalDano *= FMath::Clamp(1.f - (Dist - DamageFalloffStart) / DamageFalloffRange, 0.25f, 1.f);

				bool bHeadshot = false;
				const FString BoneStr = Hit.BoneName.ToString();
				bHeadshot = BoneStr.Contains(TEXT("head"), ESearchCase::IgnoreCase)
					|| BoneStr.Contains(TEXT("cabeza"), ESearchCase::IgnoreCase)
					|| BoneStr.Contains(TEXT("neck"), ESearchCase::IgnoreCase);
				if (bHeadshot) FinalDano *= HeadshotMultiplier;

				if (bIsDamageable)
				{
					Dmg->RecibirDano((int32)FinalDano, Hit.ImpactPoint, ETipoDano::Bala);
				Consecuencias(Hit.GetActor());
				OnHitMark.Broadcast(bHeadshot);
				if (NSSangre) UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSSangre, Hit.ImpactPoint, (-D).Rotation());
				}

			if (NSImpacto)
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSImpacto, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			if (SImpacto)
				UGameplayStatics::PlaySoundAtLocation(W, SImpacto, Hit.ImpactPoint);
			}

			// Stop conditions: hit a damageable target, or exhausted penetration.
			if (bStopOnFirstHit && bIsDamageable)
				break;
			if (++Penetrated >= MaxPenetration)
				break;
		}
	}

	// Retroceso (progressive recoil pattern: climbs with continuous fire).
	if (APawn* P = Cast<APawn>(GetOwner()))
		if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			RecoilStack = FMath::Min(RecoilStack + RecoilPerShot, RecoilMax);
			float Kick = (0.6f + DispersionGrados * 0.15f) * (1.f + RecoilStack);
			if (const AAlsasuaPlayerCharacter* Ch = Cast<AAlsasuaPlayerCharacter>(P)) if (Ch->EstaApuntando()) Kick *= 0.5f;
			PC->AddPitchInput(-Kick);
			PC->AddYawInput(FMath::FRandRange(-Kick, Kick) * 0.3f);
		}

	SubirBusqueda(3);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utility
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  Spray (graffiti tagging — reduces suspicion in area)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::LanzarSpray()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Cooldown = 0.6f;
	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	if (W->LineTraceSingleByChannel(Hit, Origen, Origen + Dir * 500.f, ECC_Visibility, Q))
	{
		if (NSSpray)
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSSpray, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

		if (IDamageable* D = Cast<IDamageable>(Hit.GetActor()))
			if (!D->EstaMuerto())
				D->RecibirDano(0, Hit.ImpactPoint, ETipoDano::Impacto); // Tagging, no real damage.
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tirachinas (slingshot — silent, low damage ranged)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::LanzarTirachinas()
{
	UWorld* W = GetWorld();
	if (!W) return;

	const int32 i = (int32)ArmaActual;
	if (Municion.IsValidIndex(i) && Municion[i] <= 0)
	{
		if (CanReload()) Reload();
		Cooldown = 0.25f;
		return;
	}
	Cooldown = 0.45f;
	if (Municion.IsValidIndex(i)) Municion[i]--;
	OnAmmoChanged.Broadcast(MunicionActual());

	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	const float Disp = 0.8f * MultDispersionActual();
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());

	FVector D = Dir;
	if (Disp > 0.f)
		D = FRotator(FMath::FRandRange(-Disp, Disp), FMath::FRandRange(-Disp, Disp), 0.f).RotateVector(Dir);

	FHitResult Hit;
	if (W->LineTraceSingleByChannel(Hit, Origen, Origen + D * 15000.f, ECC_Visibility, Q))
	{
		if (IDamageable* Dmg = Cast<IDamageable>(Hit.GetActor()))
			if (!Dmg->EstaMuerto())
			{
				Dmg->RecibirDano(8, Hit.ImpactPoint, ETipoDano::Impacto);
				Consecuencias(Hit.GetActor());
			}

		if (NSImpacto)
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSImpacto, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Molotov (thrown fire bomb — area fire damage)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::LanzarMolotov()
{
	UWorld* W = GetWorld();
	if (!W) return;

	const int32 i = (int32)ArmaActual;
	if (Municion.IsValidIndex(i) && Municion[i] <= 0) { Cooldown = 0.25f; return; }
	Cooldown = 1.5f;
	if (Municion.IsValidIndex(i)) Municion[i]--;
	OnAmmoChanged.Broadcast(MunicionActual());

	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FVector ThrowEnd = Origen + Dir * 2000.f;
	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	FVector ImpactPoint = ThrowEnd;
	if (W->LineTraceSingleByChannel(Hit, Origen, ThrowEnd, ECC_Visibility, Q))
		ImpactPoint = Hit.ImpactPoint;

	if (NSMolotov)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSMolotov, ImpactPoint, FRotator::ZeroRotator, FVector(1.f));

	if (SMolotov)
		UGameplayStatics::PlaySoundAtLocation(W, SMolotov, ImpactPoint);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(400.f);
	if (W->OverlapMultiByChannel(Overlaps, ImpactPoint, FQuat::Identity, ECC_Pawn, Shape))
	{
		for (const FOverlapResult& Ov : Overlaps)
		{
			AActor* Target = Ov.GetActor();
			if (!Target || Target == GetOwner()) continue;
			if (IDamageable* Dmg = Cast<IDamageable>(Target))
				if (!Dmg->EstaMuerto())
				{
					Dmg->RecibirDano(40, ImpactPoint, ETipoDano::Fuego);
					Consecuencias(Target);
				}
		}
	}
		SubirBusqueda(4);
	NotifyNearbyGuards(ImpactPoint, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ikurrina (Basque flag melee — high damage + morale buff)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::GolpearIkurrina()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Cooldown = 0.6f;
	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	if (W->SweepSingleByChannel(Hit, Origen, Origen + Dir * 280.f, FQuat::Identity,
			ECC_Pawn, FCollisionShape::MakeSphere(60.f), Q))
	{
		if (IDamageable* D = Cast<IDamageable>(Hit.GetActor()))
			if (!D->EstaMuerto())
			{
				D->RecibirDano(35, Hit.ImpactPoint, ETipoDano::Impacto);
				Consecuencias(Hit.GetActor());
			}
	}
	SubirBusqueda(2);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bomba Lapa (limpet mine — place on surface, detonate later)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::ColocarBombaLapa()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Cooldown = 1.0f;

	FVector Origen, Dir;
	if (!ObtenerMira(Origen, Dir)) return;

	FHitResult Hit;
	FCollisionQueryParams Q; Q.AddIgnoredActor(GetOwner());
	if (W->LineTraceSingleByChannel(Hit, Origen, Origen + Dir * 300.f, ECC_Visibility, Q))
	{
		BombaLapaLocation = Hit.ImpactPoint;
		bBombaLapaActive = true;

		if (SBombaColocar)
			UGameplayStatics::PlaySoundAtLocation(W, SBombaColocar, Hit.ImpactPoint);
	}
}

void UArmasComponent::DetonarBombaLapa()
{
	if (!bBombaLapaActive) return;
	UWorld* W = GetWorld();
	if (!W) return;
	Cooldown = 0.5f;

	if (NSExplosion)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSExplosion, BombaLapaLocation, FRotator::ZeroRotator, FVector(1.2f));

	if (SExplosion)
		UGameplayStatics::PlaySoundAtLocation(W, SExplosion, BombaLapaLocation);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(800.f);
	if (W->OverlapMultiByChannel(Overlaps, BombaLapaLocation, FQuat::Identity, ECC_Pawn, Shape))
	{
		for (const FOverlapResult& Ov : Overlaps)
		{
			AActor* Target = Ov.GetActor();
			if (!Target || Target == GetOwner()) continue;
			if (IDamageable* Dmg = Cast<IDamageable>(Target))
				if (!Dmg->EstaMuerto())
				{
					Dmg->RecibirDano(80, BombaLapaLocation, ETipoDano::Explosion);
					Consecuencias(Target);
				}
		}
	}

	bBombaLapaActive = false;
	SubirBusqueda(5);
	NotifyNearbyGuards(BombaLapaLocation, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Coche Bomba (detonate a planted car bomb at nearest vehicle)
// ─────────────────────────────────────────────────────────────────────────────
void UArmasComponent::CocheBombaDetonar()
{
	UWorld* W = GetWorld();
	if (!W) return;

	Cooldown = 0.5f;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Find nearest vehicle (car pawn or any actor tagged "Vehicle").
	TArray<AActor*> Vehicles;
	UGameplayStatics::GetAllActorsOfClass(GetOwner(), APawn::StaticClass(), Vehicles);

	AActor* NearestVehicle = nullptr;
	float NearestDist = TNumericLimits<float>::Max();
	for (AActor* V : Vehicles)
	{
		if (V == Owner) continue;
		if (!V->ActorHasTag(FName("Vehicle"))) continue;
		float Dist = FVector::Dist(Owner->GetActorLocation(), V->GetActorLocation());
		if (Dist < NearestDist) { NearestDist = Dist; NearestVehicle = V; }
	}

	FVector BoomLocation = NearestVehicle ? NearestVehicle->GetActorLocation() : Owner->GetActorLocation() + Owner->GetActorForwardVector() * 300.f;

	if (NSExplosionCoche)
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NSExplosionCoche, BoomLocation, FRotator::ZeroRotator, FVector(2.f));

	if (SExplosionGrande)
		UGameplayStatics::PlaySoundAtLocation(W, SExplosionGrande, BoomLocation);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(1200.f);
	if (W->OverlapMultiByChannel(Overlaps, BoomLocation, FQuat::Identity, ECC_Pawn, Shape))
	{
		for (const FOverlapResult& Ov : Overlaps)
		{
			AActor* Target = Ov.GetActor();
			if (!Target || Target == Owner) continue;
			if (IDamageable* Dmg = Cast<IDamageable>(Target))
				if (!Dmg->EstaMuerto())
				{
					Dmg->RecibirDano(150, BoomLocation, ETipoDano::Explosion);
					Consecuencias(Target);
				}
		}
	}

	if (NearestVehicle && NearestVehicle->Implements<UDamageable>())
	{
		if (IDamageable* Dmg = Cast<IDamageable>(NearestVehicle))
			if (!Dmg->EstaMuerto())
				Dmg->RecibirDano(200, BoomLocation, ETipoDano::Explosion);

		// Carrero Blanco: enable physics + launch vehicle into the air.
		if (UStaticMeshComponent* Mesh = NearestVehicle->FindComponentByClass<UStaticMeshComponent>())
		{
			Mesh->SetSimulatePhysics(true);
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			const FVector UpImpulse = FVector(0, 0, 180000) + NearestVehicle->GetActorForwardVector() * 30000.f;
			Mesh->AddImpulse(UpImpulse, NAME_None, true);
			Mesh->AddAngularImpulseInDegrees(
				FVector(FMath::RandRange(-200.f, 200.f), FMath::RandRange(-200.f, 200.f), FMath::RandRange(400.f, 800.f)),
				NAME_None, true);
		}
	}

	// Radial impulse: launch nearby pawns.
	TArray<FOverlapResult> LaunchOverlaps;
	FCollisionShape LaunchShape = FCollisionShape::MakeSphere(1200.f);
	if (W->OverlapMultiByChannel(LaunchOverlaps, BoomLocation, FQuat::Identity, ECC_Pawn, LaunchShape))
	{
		for (const FOverlapResult& Ov : LaunchOverlaps)
		{
			AActor* Target = Ov.GetActor();
			if (!Target || Target == Owner) continue;
			const FVector Dir = (Target->GetActorLocation() - BoomLocation).GetSafeNormal();
			const float Dist = FVector::Dist(BoomLocation, Target->GetActorLocation());
			const float Force = FMath::Max(0.f, 100000.f * (1.f - Dist / 1200.f));
			if (APawn* P = Cast<APawn>(Target))
			{
				if (UCharacterMovementComponent* CM = P->FindComponentByClass<UCharacterMovementComponent>())
				{
					CM->SetMovementMode(MOVE_Falling);
					CM->Velocity = Dir * Force * 0.01f + FVector(0, 0, 900.f);
				}
			}
		}
	}

	SubirBusqueda(6);
	NotifyNearbyGuards(BoomLocation, 1.0f);
}
