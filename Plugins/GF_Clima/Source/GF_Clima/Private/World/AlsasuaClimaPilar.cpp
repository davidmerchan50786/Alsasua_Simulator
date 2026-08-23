#include "World/AlsasuaClimaPilar.h"
#include "World/AlsasuaWeatherSystem.h"
#include "Engine/World.h"
#include "Engine/WorldSettings.h"

bool UAlsasuaClimaPilar::ShouldCreateSubsystem(UObject* Outer) const
{
	// Solo en mundos de partida; en el editor cambiaria el cielo al abrir nivel.
	UWorld* W = Cast<UWorld>(Outer->GetWorld());
	return W && W->WorldType == EWorldType::GameOrPIE && Super::ShouldCreateSubsystem(Outer);
}

void UAlsasuaClimaPilar::Deinitialize()
{
	if (Componente)
	{
		Componente->DestroyComponent();
		Componente = nullptr;
	}
	Super::Deinitialize();
}

int32 UAlsasuaClimaPilar::EjecutarArranque()
{
	if (!Componente)
	{
		Componente = NewObject<UAlsasuaWeatherSystem>(GetWorld()->GetWorldSettings());
		Componente->RegisterComponent();
	}
	return Componente ? 1 : -1;
}

bool UAlsasuaClimaPilar::EstaLloviendo() const
{
	return Componente && Componente->IsRaining();
}

bool UAlsasuaClimaPilar::HayTormenta() const
{
	// El enum EWeatherState es del pilar; fuera de el solo viaja un bool.
	return Componente && Componente->GetWeather() == EWeatherState::Storm;
}

float UAlsasuaClimaPilar::HoraDeJuego() const
{
	return Componente ? Componente->GameTimeHour : 12.f;
}

float UAlsasuaClimaPilar::VelocidadViento() const
{
	return Componente ? Componente->WindSpeed : 0.f;
}

float UAlsasuaClimaPilar::IntensidadLluvia() const
{
	return Componente ? Componente->RainIntensity : 0.f;
}
