// VehiculoJugable.cpp
#include "VehiculoJugable.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "AlsasuaTypes.h"            // IDamageable
#include "AlsasuaNPC.h"
#include "ConsecuenciasSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "AlsasuaLegacyPlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Controller.h"

AVehiculoJugable::AVehiculoJugable()
{
	PrimaryActorTick.bCanEverTick = true;

	Cuerpo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cuerpo"));
	RootComponent = Cuerpo;
	Cuerpo->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Cuerpo->SetCollisionProfileName(TEXT("Pawn"));
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		Cuerpo->SetStaticMesh(M);
	Cuerpo->SetRelativeScale3D(FVector(4.6f, 1.9f, 1.4f));   // ~coche
	Tags.Add(TEXT("VehiculoJugable"));

	Brazo = CreateDefaultSubobject<USpringArmComponent>(TEXT("Brazo"));
	Brazo->SetupAttachment(RootComponent);
	Brazo->TargetArmLength = 650.f;
	Brazo->SocketOffset = FVector(0, 0, 200.f);
	Brazo->bEnableCameraLag = true;
	Brazo->CameraLagSpeed = 6.f;
	Brazo->bUsePawnControlRotation = false;   // cámara sigue al coche, no al ratón

	Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Brazo, USpringArmComponent::SocketName);
}

float AVehiculoJugable::AlturaSuelo(const FVector& Pos) const
{
	const UWorld* W = GetWorld();
	if (!W) return Pos.Z;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(SueloCoche), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, Pos + FVector(0, 0, 300.f), Pos - FVector(0, 0, 1000.f), ECC_Visibility, Q))
		return Hit.Location.Z;
	return Pos.Z;
}

void AVehiculoJugable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Aceleración / rozamiento / freno.
	if (FMath::Abs(Acelerador) > 0.05f)
		Vel += Acelerador * Aceleracion * DeltaTime;
	else
		Vel = FMath::FInterpTo(Vel, 0.f, DeltaTime, 1.2f);   // rueda libre (coasting)
	if (bFreno) Vel = FMath::FInterpTo(Vel, 0.f, DeltaTime, 6.f);   // freno fuerte
	Vel = FMath::Clamp(Vel, -VelMax * 0.4f, VelMax);

	// Giro proporcional a la velocidad (no giras parado).
	const float fVel = FMath::Clamp(FMath::Abs(Vel) / 250.f, 0.f, 1.f);
	const float Yaw = Volante * GiroGrados * DeltaTime * FMath::Sign(Vel) * fVel;
	AddActorWorldRotation(FRotator(0.f, Yaw, 0.f));

	// Avance con barrido (choca con muros).
	const float VelAntes = Vel;
	FHitResult Hit;
	AddActorWorldOffset(GetActorForwardVector() * Vel * DeltaTime, true, &Hit);
	if (Hit.bBlockingHit)
	{
		Vel *= 0.25f;
		// Daño por colisión proporcional a la velocidad del impacto.
		if (FMath::Abs(VelAntes) > 500.f)
			RecibirDano(FMath::RoundToInt(FMath::Abs(VelAntes) / 30.f), Hit.ImpactPoint, ETipoDano::Impacto);
	}

	// Drapeado sobre el terreno (mantiene las ruedas en el suelo).
	FVector P = GetActorLocation();
	const float zSuelo = AlturaSuelo(P) + 75.f;
	P.Z = FMath::FInterpTo(P.Z, zSuelo, DeltaTime, 10.f);
	SetActorLocation(P);

	Atropellar();
}

void AVehiculoJugable::RecibirDano(int32 C, FVector Origen, ETipoDano Tipo)
{
	if (bExplotado) return;
	Vida = FMath::Max(0, Vida - C);

	// Humo cuando el motor está tocado (< 40%).
	if (!bHumo && Vida < VidaMaxima * 0.4f)
	{
		bHumo = true;
		if (UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Humo.NS_Humo")))
			Humo = UNiagaraFunctionLibrary::SpawnSystemAttached(NS, Cuerpo, NAME_None, FVector(0, 0, 60.f), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}
	if (Vida <= 0) Explotar();
}

void AVehiculoJugable::Explotar()
{
	if (bExplotado) return;
	bExplotado = true;
	Vel = 0.f; Acelerador = 0.f; Volante = 0.f;
	UWorld* W = GetWorld();
	if (!W) return;

	// VFX + sonido.
	if (UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Explosion.NS_Explosion")))
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NS, GetActorLocation(), FRotator::ZeroRotator);
	if (USoundBase* S = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Explosion.SC_Explosion")))
		UGameplayStatics::PlaySoundAtLocation(W, S, GetActorLocation());

	// Daño radial a IDamageables cercanos (NPCs, jugador, otros coches).
	TArray<FOverlapResult> Ov;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Explosion), false); Q.AddIgnoredActor(this);
	W->OverlapMultiByChannel(Ov, GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(500.f), Q);
	for (const FOverlapResult& R : Ov)
		if (IDamageable* D = Cast<IDamageable>(R.GetActor()))
			if (!D->EstaMuerto())
			{
				const float dist = FVector::Dist(R.GetActor()->GetActorLocation(), GetActorLocation());
				D->RecibirDano(FMath::RoundToInt(FMath::Lerp(120.f, 30.f, FMath::Clamp(dist / 500.f, 0.f, 1.f))), GetActorLocation(), ETipoDano::Explosion);
			}

	// Expulsa al jugador si va dentro.
	if (AAlsasuaLegacyPlayerController* PC = Cast<AAlsasuaLegacyPlayerController>(GetController()))
		PC->SalirVehiculo(this);

	SetLifeSpan(8.f);   // chasis quemado un rato y se limpia
}

void AVehiculoJugable::Atropellar()
{
	if (FMath::Abs(Vel) < 400.f) return;   // hace falta velocidad
	UWorld* W = GetWorld();
	if (!W) return;

	const FVector Frente = GetActorLocation() + GetActorForwardVector() * 250.f;
	TArray<FOverlapResult> Ov;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(Atropello), false); Q.AddIgnoredActor(this);
	W->OverlapMultiByChannel(Ov, Frente, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(200.f), Q);

	const int32 Dano = FMath::Clamp(FMath::RoundToInt(FMath::Abs(Vel) / 15.f), 15, 120);

	for (const FOverlapResult& R : Ov)
	{
		AAlsasuaNPC* NPC = Cast<AAlsasuaNPC>(R.GetActor());
		if (!NPC || YaAtropellados.Contains(NPC)) continue;
		YaAtropellados.Add(NPC);

		if (IDamageable* D = Cast<IDamageable>(NPC))
			if (!D->EstaMuerto()) D->RecibirDano(Dano, GetActorLocation(), ETipoDano::Impacto);

		// empuje del cuerpo y registro para las consecuencias (apoyo popular).
		if (USkeletalMeshComponent* M = NPC->GetMesh())
			if (M->IsSimulatingPhysics())
				M->AddImpulse((GetActorForwardVector() + FVector(0, 0, 0.4f)).GetSafeNormal() * 60000.f, NAME_None, true);
		if (UConsecuenciasSubsystem* Cs = GetGameInstance() ? GetGameInstance()->GetSubsystem<UConsecuenciasSubsystem>() : nullptr)
			Cs->RegistrarDano(NPC);
	}
}

void AVehiculoJugable::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent) return;
	// Reusa los ejes clásicos (DefaultInput.ini): W/S y A/D (+ sticks si están mapeados).
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AVehiculoJugable::Acelerar);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"),   this, &AVehiculoJugable::Girar);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed,  this, &AVehiculoJugable::FrenoOn);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AVehiculoJugable::FrenoOff);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed,  this, &AVehiculoJugable::FrenoOn);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Released, this, &AVehiculoJugable::FrenoOff);
}
