// AlsasuaInputIDs.h (capa MANIFA)
// Identificadores de entrada para las habilidades de GAS. Puerto del enum de
// Unity; hoy no lo incluye nadie —AlsasuaGameplayAbility guarda su
// AbilityInputID como int32 pelado— pero se conserva por si se vuelve a atar el
// binding por enum, que es lo que hace GAS normalmente.
//
// Llevaba UENUM(BlueprintType) sin el #include del .generated.h. UHT lo exige
// para cualquier tipo reflejado de una cabecera, y lo pide con error, no con
// aviso: sin él ni siquiera se llega al compilador.
#pragma once

#include "CoreMinimal.h"
#include "AlsasuaInputIDs.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaAbilityInputID : uint8
{
	None,
	Confirm,
	Cancel,
	Sprint,
	Rally,
	Shout,
	Interaction
};

typedef EAlsasuaAbilityInputID AbilityInputID;
