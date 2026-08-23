// ConsecuenciasSubsystem.h (capa GAMEPLAY)
// El apoyo popular tiene memoria: matar a un CIVIL baja el apoyo (daño
// colateral); matar a un GUARDIA paga recompensa. Puerto de
// SistemaConsecuencias + el recompensa de GuardiaCivilAI.Morir() del
// prototipo Unity. Lo invoca el arma al causar daño — AAlsasuaNPC vive en
// AlsasuaEntities, por debajo de AlsasuaGameplay en la capa, así que la
// consecuencia se decide aquí (arriba) leyendo su estado público
// (EstaMuerto/bEsPolicia), no dentro de NPC::Morir().
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConsecuenciasSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UConsecuenciasSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// $ al jugador por cada guardia civil abatido. Mismo valor que
	// GuardiaCivilAI.recompensa en el prototipo Unity (200).
	UPROPERTY(EditAnywhere, Category="Consecuencias") int32 RecompensaGuardia = 200;

	// Llamar tras aplicar daño a una víctima. Si acaba de morir: civil ->
	// penaliza apoyo popular; guardia -> paga recompensa. Una vez por víctima.
	void RegistrarDano(AActor* Victima);

private:
	UPROPERTY() TSet<uint32> MuertesContadas;   // una consecuencia por víctima
};
