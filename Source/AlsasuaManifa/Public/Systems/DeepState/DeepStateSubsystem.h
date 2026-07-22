#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DeepStateSubsystem.generated.h"

UENUM(BlueprintType)
enum class EDeepStateOp : uint8 {
    Disinformation, // Fake news para bajar el PopularSupport
    EvidencePlanting, // Plantar pruebas para subir el WantedLevel
    Assassination,   // Eliminar NPCs aliados del jugador
    AssetFreezing    // Bloquear acceso a fondos/zulos
};

USTRUCT(BlueprintType)
struct FDeepStateProject {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ProjectName;

    UPROPERTY(BlueprintReadOnly)
    float Progress = 0.f; // 0 a 100

    UPROPERTY(BlueprintReadOnly)
    EDeepStateOp OperationType;

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = false;
};

UCLASS()
class ALSASUAMANIFA_API UDeepStateSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Inicia una operación encubierta contra el jugador
    UFUNCTION(BlueprintCallable, Category="AAA|DeepState")
    void LaunchCovertOp(FName OpName, EDeepStateOp Type);

    // Permite al jugador sabotear las cloacas (hackeo, filtración a prensa)
    UFUNCTION(BlueprintCallable, Category="AAA|DeepState")
    void CounterOperation(FName OpName, float SabotageAmount);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeepStateAlert, FText, Warning, EDeepStateOp, Type);
    UPROPERTY(BlueprintAssignable)
    FOnDeepStateAlert OnDeepStateAlert;

private:
    UPROPERTY()
    TMap<FName, FDeepStateProject> ActiveProjects;
};