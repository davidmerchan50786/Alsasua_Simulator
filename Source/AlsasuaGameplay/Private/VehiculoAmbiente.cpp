// VehiculoAmbiente.cpp
#include "VehiculoAmbiente.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "Engine/GameInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AVehiculoAmbiente::AVehiculoAmbiente()
{
	PrimaryActorTick.bCanEverTick = true;
	Cuerpo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cuerpo"));
	RootComponent = Cuerpo;
	Cuerpo->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Cuerpo->SetCollisionResponseToAllChannels(ECR_Overlap);
	Cuerpo->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);   // para que las balas lo alcancen
	if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		Cuerpo->SetStaticMesh(M);
	Cuerpo->SetRelativeScale3D(FVector(4.5f, 1.8f, 1.4f));   // ~coche (m); el cubo mide 1 m
	Tags.Add(TEXT("Vehiculo"));
}

void AVehiculoAmbiente::Iniciar(const TArray<FVector2D>& Eje, float AnchoCalzadaCm, int32 IndiceInicio, int32 Sentido, float VelocidadCmS)
{
	const int32 N = Eje.Num();
	Ruta.Reset(N);
	int32 Inicio = IndiceInicio;
	if (Sentido < 0)
	{
		// invierte la polilínea para marchar siempre "hacia delante"
		for (int32 i = N - 1; i >= 0; --i) Ruta.Add(Eje[i]);
		Inicio = (N - 1) - IndiceInicio;
	}
	else Ruta = Eje;

	Seg = FMath::Clamp(Inicio, 0, FMath::Max(0, Ruta.Num() - 2));
	DistEnSeg = 0.f;
	Velocidad = VelocidadCmS;
	// Carril derecho: centro de la mitad derecha de la calzada (clamp razonable).
	OffsetCarril = FMath::Clamp(AnchoCalzadaCm * 0.25f, 120.f, 450.f);
	bTerminado = (Ruta.Num() < 2);
}

float AVehiculoAmbiente::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaVehiculo), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void AVehiculoAmbiente::RecibirDano(int32 C, FVector Origen, ETipoDano Tipo)
{
	if (bExplotado) return;
	Vida = FMath::Max(0, Vida - C);
	if (!bHumo && Vida < VidaMaxima * 0.4f)
	{
		bHumo = true;
		if (UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Humo.NS_Humo")))
			Humo = UNiagaraFunctionLibrary::SpawnSystemAttached(NS, Cuerpo, NAME_None, FVector(0, 0, 60.f), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
	}
	if (Vida <= 0) Explotar();
}

void AVehiculoAmbiente::Explotar()
{
	if (bExplotado) return;
	bExplotado = true;
	bTerminado = true;   // deja de circular
	UWorld* W = GetWorld();
	if (!W) return;

	if (UNiagaraSystem* NS = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_Explosion.NS_Explosion")))
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(W, NS, GetActorLocation(), FRotator::ZeroRotator);
	if (USoundBase* S = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Explosion.SC_Explosion")))
		UGameplayStatics::PlaySoundAtLocation(W, S, GetActorLocation());

	// Daño radial a IDamageables cercanos (incluye otros coches -> cadena).
	TArray<FOverlapResult> Ov;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ExplosionAmb), false); Q.AddIgnoredActor(this);
	W->OverlapMultiByChannel(Ov, GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(500.f), Q);
	for (const FOverlapResult& R : Ov)
		if (IDamageable* D = Cast<IDamageable>(R.GetActor()))
			if (!D->EstaMuerto())
			{
				const float dist = FVector::Dist(R.GetActor()->GetActorLocation(), GetActorLocation());
				D->RecibirDano(FMath::RoundToInt(FMath::Lerp(120.f, 30.f, FMath::Clamp(dist / 500.f, 0.f, 1.f))), GetActorLocation(), ETipoDano::Explosion);
			}

	SetLifeSpan(8.f);
}

void AVehiculoAmbiente::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bTerminado || Ruta.Num() < 2) return;

	float mov = Velocidad * DeltaTime;
	while (mov > 0.f && Seg < Ruta.Num() - 1)
	{
		const FVector2D A = Ruta[Seg], B = Ruta[Seg + 1];
		const float segLen = FVector2D::Distance(A, B);
		const float rem = segLen - DistEnSeg;
		if (segLen < 1.f) { ++Seg; DistEnSeg = 0.f; continue; }
		if (mov < rem) { DistEnSeg += mov; mov = 0.f; }
		else { mov -= rem; ++Seg; DistEnSeg = 0.f; }
	}
	if (Seg >= Ruta.Num() - 1) { bTerminado = true; return; }

	const FVector2D A = Ruta[Seg], B = Ruta[Seg + 1];
	const FVector2D dir = (B - A).GetSafeNormal();
	// Normal derecha respecto al sentido de marcha (circulación por la derecha).
	const FVector2D derecha(dir.Y, -dir.X);
	const FVector2D pos = A + dir * DistEnSeg + derecha * OffsetCarril;
	const float z = AlturaSuelo(pos) + 40.f;   // despeje sobre el firme

	const FRotator rot = FRotationMatrix::MakeFromX(FVector(dir.X, dir.Y, 0.f)).Rotator();
	SetActorLocationAndRotation(FVector(pos.X, pos.Y, z), rot);
}
