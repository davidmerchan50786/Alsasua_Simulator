// ProgresionSubsystem.cpp
#include "ProgresionSubsystem.h"
#include "ApoyoPopularSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlsasuaProg, Log, All);

void UProgresionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
		{
			Apoyo->OnApoyoCambia.AddDynamic(this, &UProgresionSubsystem::OnApoyo);
			OnApoyo(Apoyo->Apoyo);
		}
	}
}

void UProgresionSubsystem::Deinitialize()
{
	if (UGameInstance* GI = GetGameInstance())
		if (UApoyoPopularSubsystem* Apoyo = GI->GetSubsystem<UApoyoPopularSubsystem>())
			Apoyo->OnApoyoCambia.RemoveDynamic(this, &UProgresionSubsystem::OnApoyo);
	Super::Deinitialize();
}

void UProgresionSubsystem::OnApoyo(float Apoyo)
{
	const int32 Nuevo = FMath::Clamp((int32)(Apoyo / 20.f), 0, 5);
	if (Nuevo == Nivel) return;
	Nivel = Nuevo;
	OnNivelCambiado.Broadcast(Nivel);
	UE_LOG(LogAlsasuaProg, Log, TEXT("Nivel del movimiento: %d"), Nivel);
}
