#include "World/AlsasuaAudioPilar.h"
#include "World/AlsasuaAmbientAudioSystem.h"
#include "Engine/World.h"
#include "Engine/WorldSettings.h"

bool UAlsasuaAudioPilar::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* W = Cast<UWorld>(Outer->GetWorld());
	return W && W->WorldType == EWorldType::GameOrPIE && Super::ShouldCreateSubsystem(Outer);
}

void UAlsasuaAudioPilar::Deinitialize()
{
	if (Componente)
	{
		Componente->DestroyComponent();
		Componente = nullptr;
	}
	Super::Deinitialize();
}

int32 UAlsasuaAudioPilar::EjecutarArranque()
{
	if (!Componente)
	{
		Componente = NewObject<UAlsasuaAmbientAudioSystem>(GetWorld()->GetWorldSettings());
		Componente->RegisterComponent();
	}
	return Componente ? 1 : -1;
}

void UAlsasuaAudioPilar::TiquearPilar(float DeltaTime)
{
	if (!Componente)
	{
		return;
	}
	IAlsasuaEstadoClima* Clima = nullptr;
	if (USubsystem* Candidato = FuenteClima.Get())
	{
		Clima = Cast<IAlsasuaEstadoClima>(Candidato);
	}
	else if (UWorld* W = GetWorld())
	{
		W->ForEachSubsystem<UWorldSubsystem>([this, &Clima](UWorldSubsystem* Sub)
		{
			if (!FuenteClima.IsValid() && Cast<IAlsasuaEstadoClima>(Sub))
			{
				FuenteClima = Sub;
				Clima = Cast<IAlsasuaEstadoClima>(Sub);
			}
		});
	}
	if (!Clima)
	{
		return; // pilar de clima ausente: silencio, como toca
	}
	const bool bRaining = Clima->EstaLloviendo();
	const bool bStorm = Clima->HayTormenta();
	Componente->SetWeatherState(bRaining, bStorm, false);
	Componente->UpdateAmbientAudio(Clima->HoraDeJuego(),
		Clima->VelocidadViento(), Clima->IntensidadLluvia(), 5000.0f);
}
