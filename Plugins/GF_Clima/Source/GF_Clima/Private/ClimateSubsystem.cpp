#include "ClimateSubsystem.h"

void UClimateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentTime = 12.0f; // Empezar al mediodía
	bIsRaining = false;
}

void UClimateSubsystem::UpdateClimate(float DeltaTime)
{
	CurrentTime += DeltaTime * 0.1f; // El tiempo avanza
	if (CurrentTime > 24.0f) CurrentTime = 0.0f;
}

float UClimateSubsystem::GetVisibilityMultiplier() const
{
	float Multiplier = 1.0f;

	// Si es de noche (de 21:00 a 06:00), reducir visibilidad
	if (CurrentTime > 21.0f || CurrentTime < 6.0f)
	{
		Multiplier *= 0.6f;
	}

	// Si llueve, reducir otro poco
	if (bIsRaining)
	{
		Multiplier *= 0.8f;
	}

	return Multiplier;
}
