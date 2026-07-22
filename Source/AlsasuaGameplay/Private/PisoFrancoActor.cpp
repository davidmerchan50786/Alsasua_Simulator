// PisoFrancoActor.cpp
#include "PisoFrancoActor.h"
#include "RespawnSubsystem.h"
#include "WantedSubsystem.h"
#include "AlsasuaTypes.h"        // IDamageable
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

APisoFrancoActor::APisoFrancoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Zona = CreateDefaultSubobject<UBoxComponent>(TEXT("Zona"));
	RootComponent = Zona;
	Zona->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	Zona->SetCollisionProfileName(TEXT("Trigger"));
	Zona->OnComponentBeginOverlap.AddDynamic(this, &APisoFrancoActor::OnEntra);
	Zona->OnComponentEndOverlap.AddDynamic(this, &APisoFrancoActor::OnSale);
}

void APisoFrancoActor::OnEntra(UPrimitiveComponent*, AActor* Otro, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (Otro != UGameplayStatics::GetPlayerPawn(this, 0)) return;
	bJugadorDentro = true;
	FijarRespawn(Otro);   // visitar el piso fija reaparición
}

void APisoFrancoActor::OnSale(UPrimitiveComponent*, AActor* Otro, UPrimitiveComponent*, int32)
{
	if (Otro == UGameplayStatics::GetPlayerPawn(this, 0)) bJugadorDentro = false;
}

void APisoFrancoActor::FijarRespawn(AActor* Jugador)
{
	if (const UWorld* W = GetWorld())
		if (UGameInstance* GI = W->GetGameInstance())
			if (URespawnSubsystem* R = GI->GetSubsystem<URespawnSubsystem>())
				R->FijarPunto(Jugador->GetActorLocation());
}

void APisoFrancoActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bJugadorDentro) return;

	APawn* Jugador = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Jugador) return;

	// curación gradual
	if (IDamageable* D = Cast<IDamageable>(Jugador))
		if (!D->EstaMuerto() && D->GetVida() < D->GetVidaMax())
			D->Curar(FMath::CeilToInt(CuracionPorSeg * DeltaTime));

	// enfriar el calor: acumula y resta estrellas enteras
	AcumBajada += CalorPorSeg * DeltaTime;
	if (AcumBajada >= 1.f)
	{
		const int32 Baja = FMath::FloorToInt(AcumBajada);
		AcumBajada -= Baja;
		if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
			if (UWantedSubsystem* Wn = GI->GetSubsystem<UWantedSubsystem>())
				Wn->AumentarBusqueda(-Baja);
	}
}
