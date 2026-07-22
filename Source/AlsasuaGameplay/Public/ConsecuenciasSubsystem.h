// ConsecuenciasSubsystem.h (capa GAMEPLAY)
// El apoyo popular tiene memoria: matar a un CIVIL baja el apoyo (daño colateral).
// Puerto de SistemaConsecuencias (la parte de colaterales). Lo invoca el arma
// al causar daño.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConsecuenciasSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UConsecuenciasSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Llamar tras aplicar daño a una víctima. Si acaba de morir y es civil, penaliza.
	void RegistrarDano(AActor* Victima);

private:
	UPROPERTY() TSet<uint32> MuertesContadas;   // una penalización por víctima
};
