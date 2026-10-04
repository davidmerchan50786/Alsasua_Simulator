// GuardianIluminacion.h (capa WORLD)
// Deja la escena con una iluminación jugable al empezar la partida, venga como
// venga el mapa: un solo sol por encima del horizonte y con intensidad, un solo
// cielo/luz de cielo/niebla, y ningún post-process con la exposición rota. Todo
// lo que corrige lo dice en el log (prefijo "GuardianIluminacion:").
//
// Existe porque L_Alsasua llegó con cuatro soles, cuatro cielos, cuatro nieblas
// y cuatro luces de cielo guardados dentro, y la partida salía negra con el mundo
// presente (viewmode unlit lo enseñaba). Con -AlsasuaPlugins=Ninguno no corre
// nadie que mueva el sol (eso es GF_Clima), así que la luz es la que trae el mapa.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GuardianIluminacion.generated.h"

UCLASS()
class ALSASUAWORLD_API UGuardianIluminacion : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    void UnoDeCada(UWorld& W);
    void SolJugable(UWorld& W);
    void ExposicionSana(UWorld& W);
};
