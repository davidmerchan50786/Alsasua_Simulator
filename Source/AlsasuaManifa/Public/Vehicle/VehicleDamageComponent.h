#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleDamageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVehicleDestroyed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTirePopped, int32, TireIndex);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSASUAMANIFA_API UVehicleDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVehicleDamageComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Status")
    float Health = 100.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AAA|Status")
    int32 IntactTires = 4;

    UPROPERTY(BlueprintAssignable)
    FOnVehicleDestroyed OnVehicleDestroyed;

    UPROPERTY(BlueprintAssignable)
    FOnTirePopped OnTirePopped;

    UFUNCTION(BlueprintCallable, Category="AAA|Damage")
    void ApplyVehicleDamage(float Amount);

    UFUNCTION(BlueprintCallable, Category="AAA|Damage")
    void PopTire();

private:
    void UpdateVehiclePerformance();
};
