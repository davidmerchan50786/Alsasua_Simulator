#include "Mass/AlsasuaMassParallelManager.h"
#include "Core/AlsasuaBudgetManager.h"
#include "Core/AlsasuaHitchProtector.h"

void UAlsasuaMassParallelManager::Tick(float DeltaTime)
{
	UAlsasuaBudgetManager* Budget = GetWorld()->GetSubsystem<UAlsasuaBudgetManager>();
	UAlsasuaHitchProtector* Hitch = GetWorld()->GetSubsystem<UAlsasuaHitchProtector>();

	if (Budget && !Budget->CanExecute(EBudgetCategory::Simulation)) return;

	float LODScale = Hitch ? Hitch->GetGlobalLODScale() : 1.0f;

	// Solo actualizamos una fracción de los proxies si estamos en modo ahorro o pánico
	int32 TotalToUpdate = FMath::RoundToInt(Proxies.Num() * LODScale);

	for (int32 Index = 0; Index < TotalToUpdate; Index++)
	{
		FMassProtesterProxy& P = Proxies[Index];
		P.Position += P.Rotation.Vector() * (10.f * DeltaTime);
	}
}
