// ApoyoPopularSubsystem.cpp
#include "ApoyoPopularSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaNPC.h"
#include "Components/AlsasuaParanoiaComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UApoyoPopularSubsystem::SumarApoyo(float Cantidad, const FString& Razon)
{
	Apoyo = FMath::Clamp(Apoyo + Cantidad, 0.f, 100.f);
	OnApoyoCambia.Broadcast(Apoyo);
	if (!Razon.IsEmpty())
		UE_LOG(LogAlsasua, Log, TEXT("Apoyo +%.0f (%s) -> %.0f%%"), Cantidad, *Razon, Apoyo);
}

void UApoyoPopularSubsystem::RestarApoyo(float Cantidad, const FString& Razon)
{
	Apoyo = FMath::Clamp(Apoyo - Cantidad, 0.f, 100.f);
	OnApoyoCambia.Broadcast(Apoyo);
	if (!Razon.IsEmpty())
		UE_LOG(LogAlsasua, Log, TEXT("Apoyo -%.0f (%s) -> %.0f%%"), Cantidad, *Razon, Apoyo);
}

void UApoyoPopularSubsystem::SumarParanoia(float Cantidad)  { Paranoia = FMath::Clamp(Paranoia + Cantidad, 0.f, 100.f); }
void UApoyoPopularSubsystem::RestarParanoia(float Cantidad) { Paranoia = FMath::Clamp(Paranoia - Cantidad, 0.f, 100.f); }

void UApoyoPopularSubsystem::Tick(float DeltaTime)
{
	const float Antes = Apoyo;
	Apoyo = FMath::FInterpConstantTo(Apoyo, 50.f, DeltaTime, DecayApoyo);
	Paranoia = FMath::Max(0.f, Paranoia - DeltaTime);

	// Aggregate per-NPC paranoia into global level every tick (cheap — NPCs are nearby).
	UWorld* W = GetWorld();
	if (W)
	{
		float TotalParanoia = 0.f;
		int32 NPCCount = 0;
		for (TActorIterator<AAlsasuaNPC> It(W); It; ++It)
		{
			if (*It && !It->bMuerto)
			{
				if (UAlsasuaParanoiaComponent* PC = It->FindComponentByClass<UAlsasuaParanoiaComponent>())
				{
					TotalParanoia += PC->GetParanoiaLevel();
					NPCCount++;
				}
			}
		}
		if (NPCCount > 0)
		{
			const float AvgParanoia = TotalParanoia / NPCCount;
			Paranoia = FMath::FInterpTo(Paranoia, AvgParanoia, DeltaTime, 0.5f);
		}
	}

	if (!FMath::IsNearlyEqual(Antes, Apoyo, 0.01f))
		OnApoyoCambia.Broadcast(Apoyo);
}
