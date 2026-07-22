#include "AlsasuaDebugComponent.h"
#include "DrawDebugHelpers.h"
#include "AlsasuaCharacter.h"
#include "NPCGuardCharacter.h"
#include "AI/AlsasuaAIController.h"

UAlsasuaDebugComponent::UAlsasuaDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAlsasuaDebugComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	DrawAIDebug();
	DrawCharacterDebug();
#endif
}

void UAlsasuaDebugComponent::DrawAIDebug()
{
	ANPCGuardCharacter* Guard = Cast<ANPCGuardCharacter>(GetOwner());
	if (Guard && bShowAIFieldOfView)
	{
		// Dibujar cono de visión aproximado
		DrawDebugCone(GetWorld(), Guard->GetActorLocation(), Guard->GetActorForwardVector(), 3000.f, FMath::DegreesToRadians(45.f), FMath::DegreesToRadians(45.f), 12, FColor::Red, false, -1.f, 0, 2.f);
	}
}

void UAlsasuaDebugComponent::DrawCharacterDebug()
{
	AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(GetOwner());
	if (Player)
	{
		// Mostrar valores de GAS sobre la cabeza del jugador
		FString DebugText = FString::Printf(TEXT("Health: %.1f | Stamina: %.1f | Support: %.1f"), Player->GetHealth(), Player->GetStamina(), Player->GetPopularSupport());
		DrawDebugString(GetWorld(), FVector(0, 0, 100), DebugText, Player, FColor::Cyan, 0.01f, true);
	}
}
