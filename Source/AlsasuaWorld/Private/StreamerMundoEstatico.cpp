// StreamerMundoEstatico.cpp
#include "StreamerMundoEstatico.h"
#include "GobernadorRender.h"
#include "EngineUtils.h"                 // TActorIterator
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"

FString UStreamerMundoEstatico::GetDebugSummary() const
{
	return FString::Printf(TEXT("Registered=%d | Tick=%s"), Registro.Num(), IsTickable() ? TEXT("on") : TEXT("off"));
}

void UStreamerMundoEstatico::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Auto-recoge el mundo estático ya colocado por etiqueta.
	for (TActorIterator<AActor> It(&InWorld); It; ++It)
		for (const FName& T : Tags)
			if (It->ActorHasTag(T)) { Registrar(*It); break; }
	UE_LOG(LogTemp, Log, TEXT("StreamerMundoEstatico: registered %d actors"), Registro.Num());
}

void UStreamerMundoEstatico::Registrar(AActor* A)
{
	if (!A) return;
	FRegistroMundo R; R.Actor = A; R.Banda = EBandaMundo::Activo;
	Registro.Add(R);
}

void UStreamerMundoEstatico::Tick(float DeltaTime)
{
	Acum += DeltaTime;
	if (Acum < PeriodoReclasif) return;
	Acum = 0.f;
	Reclasificar();
}

void UStreamerMundoEstatico::Reclasificar()
{
	UGobernadorRender* Gob = GetWorld() ? GetWorld()->GetSubsystem<UGobernadorRender>() : nullptr;
	const float RAct = Gob ? Gob->RadioActivacion() : 30000.f;
	const float RImp = Gob ? Gob->RadioImpostor()   : 48000.f;

	APawn* Jug = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (!Jug) return;
	const FVector P = Jug->GetActorLocation();

	for (FRegistroMundo& R : Registro)
	{
		AActor* A = R.Actor.Get();
		if (!A) continue;
		const float D = FVector::Dist(P, A->GetActorLocation());

		// histéresis: el umbral de subir es más exigente que el de bajar
		const float H = (R.Banda == EBandaMundo::Activo)   ? Histeresis : 0.f;
		const float H2 = (R.Banda == EBandaMundo::Impostor) ? Histeresis : 0.f;

		EBandaMundo Nueva;
		if (D <= RAct + H)        Nueva = EBandaMundo::Activo;
		else if (D <= RImp + H2)  Nueva = EBandaMundo::Impostor;
		else                      Nueva = EBandaMundo::Oculto;

		if (Nueva != R.Banda) AplicarBanda(R, Nueva);
	}
	UE_LOG(LogTemp, Verbose, TEXT("StreamerMundoEstatico: reclasified %d actors"), Registro.Num());
}

void UStreamerMundoEstatico::AplicarBanda(FRegistroMundo& R, EBandaMundo Nueva)
{
	AActor* A = R.Actor.Get();
	if (!A) return;
	R.Banda = Nueva;

	switch (Nueva)
	{
	case EBandaMundo::Activo:
		A->SetActorHiddenInGame(false);
		A->SetActorEnableCollision(true);
		A->ForEachComponent<UStaticMeshComponent>(false, [](UStaticMeshComponent* C)
		{ C->SetCastShadow(true); C->SetForcedLodModel(0); });   // 0 = LOD automático
		break;

	case EBandaMundo::Impostor:   // "impostor-lite": visible, sin sombras, LOD más bajo
		A->SetActorHiddenInGame(false);
		A->SetActorEnableCollision(false);
		A->ForEachComponent<UStaticMeshComponent>(false, [](UStaticMeshComponent* C)
		{
			C->SetCastShadow(false);
			const int32 N = C->GetStaticMesh() ? C->GetStaticMesh()->GetNumLODs() : 0;
			if (N > 0) C->SetForcedLodModel(N);   // fuerza el LOD menos detallado (1-based)
		});
		break;

	case EBandaMundo::Oculto:
		A->SetActorHiddenInGame(true);
		A->SetActorEnableCollision(false);
		break;
	}
}
