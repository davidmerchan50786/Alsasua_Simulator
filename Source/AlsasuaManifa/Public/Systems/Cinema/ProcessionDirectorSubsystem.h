#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProcessionDirectorSubsystem.generated.h"

UENUM(BlueprintType)
enum class EProcessionOutcome : uint8 {
    Undetermined,
    PublicJustice,    // Las cloacas son expuestas ante el pueblo
    StateRepression,  // Las cloacas ganan el relato y culpan al jugador
    BloodySunday      // El atentado sucede con víctimas masivas
};

UCLASS()
class ALSASUAMANIFA_API UProcessionDirectorSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Inicia el gran evento final
    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void StartProcessionEvent();

    // El jugador filtra el "Ledger" (Contabilidad) de las cloacas en el clímax
    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void PlayerAction_ExposeCloacas();

    // El jugador falla en detener el explosivo
    UFUNCTION(BlueprintCallable, Category="AAA|SetPiece")
    void TriggerBombFailure();

    UPROPERTY(BlueprintReadOnly, Category="AAA|SetPiece")
    EProcessionOutcome CurrentOutcome = EProcessionOutcome::Undetermined;

private:
    void UpdateCityStateByOutcome();
};