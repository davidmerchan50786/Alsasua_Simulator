// ApoyoPopularSubsystem.cpp
#include "ApoyoPopularSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaServiceRegistry.h"
#include "Engine/GameInstance.h"

void UApoyoPopularSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UGameInstance* GI = GetGameInstance())
		if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
			Reg->Publicar(FName("ApoyoPopular"), this);
}

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
	Apoyo = FMath::FInterpConstantTo(Apoyo, 50.f, DeltaTime, DecayApoyo);   // tiende a la media
	Paranoia = FMath::Max(0.f, Paranoia - DeltaTime);
	if (!FMath::IsNearlyEqual(Antes, Apoyo, 0.01f))
		OnApoyoCambia.Broadcast(Apoyo);
}
