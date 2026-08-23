#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/AlsasuaLockpickTargetInterface.h"
#include "HideoutActor.generated.h"

UCLASS()
class GF_SYSTEMS_API AHideoutActor : public AActor, public IAlsasuaLockpickTarget {
    GENERATED_BODY()
public:
    AHideoutActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<class USceneComponent> RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<class UStaticMeshComponent> EntranceMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Forensics")
    TObjectPtr<class UEvidenceComponent> EvidenceComp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Zulo")
    bool bIsLocked = true;

    UFUNCTION(BlueprintCallable, Category="AAA|Zulo")
    void OpenZulo();

    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Zulo")
    void OnZuloOpened();

    //~ IAlsasuaLockpickTarget (contrato del Kernel)
    virtual bool EstaCerrado_Implementation() const override { return bIsLocked; }
    virtual void AlForzado_Implementation() override { OpenZulo(); }
};