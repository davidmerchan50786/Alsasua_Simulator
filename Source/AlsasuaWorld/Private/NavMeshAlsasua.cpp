// NavMeshAlsasua.cpp
#include "NavMeshAlsasua.h"
#include "NavigationInvokerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

void UNavMeshAlsasua::Tick(float DeltaTime)
{
	if (bHecho) return;
	UWorld* W = GetWorld();
	if (!W) return;

	APawn* Jug = UGameplayStatics::GetPlayerPawn(W, 0);
	if (!Jug) return;   // aún no hay jugador; reintenta el próximo frame

	// ¿ya tiene invoker?
	if (Jug->FindComponentByClass<UNavigationInvokerComponent>())
	{ bHecho = true; return; }

	UNavigationInvokerComponent* Inv = NewObject<UNavigationInvokerComponent>(Jug, TEXT("NavInvokerJugador"));
	Inv->SetGenerationRadii(RadioGeneracion, RadioEliminacion);
	Inv->RegisterComponent();

	bHecho = true;
	UE_LOG(LogTemp, Log, TEXT("[Nav] invoker del jugador registrado (gen %.0f / rem %.0f cm)"),
		RadioGeneracion, RadioEliminacion);
}
