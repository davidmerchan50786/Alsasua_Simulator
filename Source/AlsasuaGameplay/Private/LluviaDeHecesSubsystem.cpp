// LluviaDeHecesSubsystem.cpp
#include "LluviaDeHecesSubsystem.h"
#include "Character/GameplayPostProcessComponent.h"
#include "PoblacionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

namespace LluviaDeHecesConsole
{
	int32 bActiva = false;
	float Intervalo = 0.4f;
	float Radio = 2500.f;
	int32 Cantidad = 10;
	float Altura = 2600.f;

	FAutoConsoleVariableRef CVarActiva(TEXT("Alsasua.LluviaDeHeces"), bActiva, TEXT("Activa la lluvia de heces (0|1)"));
	FAutoConsoleVariableRef CVarIntervalo(TEXT("Alsasua.LluviaDeHecesIntervalo"), Intervalo, TEXT("Segundos entre oleadas"));
	FAutoConsoleVariableRef CVarRadio(TEXT("Alsasua.LluviaDeHecesRadio"), Radio, TEXT("Radio del evento sobre el jugador (cm)"));
	FAutoConsoleVariableRef CVarCantidad(TEXT("Alsasua.LluviaDeHecesCantidad"), Cantidad, TEXT("Caídas por oleada"));
	FAutoConsoleVariableRef CVarAltura(TEXT("Alsasua.LluviaDeHecesAltura"), Altura, TEXT("Altura de lanzamiento (cm)"));
}

void ULluviaDeHecesSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

void ULluviaDeHecesSubsystem::Deinitialize()
{
	if (UWorld* W = GetWorld())
		if (APawn* P = UGameplayStatics::GetPlayerPawn(W, 0))
			if (UGameplayPostProcessComponent* PP = P->FindComponentByClass<UGameplayPostProcessComponent>())
				PP->SetFecesVision(false);
	Super::Deinitialize();
}

void ULluviaDeHecesSubsystem::Tick(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	APawn* Jugador = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jugador) return;

	UGameplayPostProcessComponent* PP = Jugador->FindComponentByClass<UGameplayPostProcessComponent>();
	const bool bActiva = LluviaDeHecesConsole::bActiva != 0;

	// Tint on/off + una huida de peatones al arrancar ("¡llega la mierda!").
	if (bActiva && !bLastActiva)
	{
		if (UGameInstance* GI = W->GetGameInstance())
			if (UPoblacionSubsystem* Pob = GI->GetSubsystem<UPoblacionSubsystem>())
				Pob->HuirDe(Jugador->GetActorLocation());
	}
	bLastActiva = bActiva;
	if (PP) PP->SetFecesVision(bActiva);

	if (!bActiva) { Acum = 0.f; return; }

	// Oleadas periódicas de caídas sobre el jugador.
	Acum += DeltaTime;
	if (Acum < LluviaDeHecesConsole::Intervalo) return;
	Acum = 0.f;

	UStaticMesh* Blob = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Blob) return;

	const FVector Centro = Jugador->GetActorLocation();
	const float Radio = LluviaDeHecesConsole::Radio;
	const float Altura = LluviaDeHecesConsole::Altura;

	for (int32 i = 0; i < LluviaDeHecesConsole::Cantidad; ++i)
	{
		const float Ang = FMath::FRandRange(0.f, 2.f * PI);
		const float R = FMath::Sqrt(FMath::FRand()) * Radio;
		const FVector Pos = Centro + FVector(FMath::Cos(Ang) * R, FMath::Sin(Ang) * R, Altura * (0.6f + FMath::FRand() * 0.6f));

		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AStaticMeshActor* BlobActor = W->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator, P))
		{
			BlobActor->GetStaticMeshComponent()->SetStaticMesh(Blob);
			BlobActor->SetMobility(EComponentMobility::Movable);
			const float S = FMath::FRandRange(25.f, 55.f);
			BlobActor->SetActorScale3D(FVector(S, S, S * FMath::FRandRange(0.5f, 1.3f)));
			BlobActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			BlobActor->GetStaticMeshComponent()->SetSimulatePhysics(true);
			BlobActor->GetStaticMeshComponent()->SetPhysicsLinearVelocity(FVector(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f), -FMath::FRandRange(50.f, 300.f)));

			// Desaparecen a los pocos segundos (actores sueltos, sin gestión de array).
			FTimerHandle Timer;
			W->GetTimerManager().SetTimer(Timer, [BlobActor]()
			{
				if (IsValid(BlobActor)) BlobActor->Destroy();
			}, 6.f, false);
		}
	}
}