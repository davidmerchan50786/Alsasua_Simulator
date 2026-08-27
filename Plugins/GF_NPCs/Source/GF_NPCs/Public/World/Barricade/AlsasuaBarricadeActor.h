#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaBarricadeActor.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;

UENUM(BlueprintType)
enum class EBarricadeType : uint8
{
    Contenedor,     // Overturned container
    Coche,          // Burnt car
    Neumaticos,     // Tire stack
    Escombros,      // Debris pile
    BarbacoaGrill   // Burning barrel
};

UCLASS()
class GF_NPCS_API AAlsasuaBarricadeActor : public AActor
{
    GENERATED_BODY()

public:
    AAlsasuaBarricadeActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    EBarricadeType Tipo = EBarricadeType::Contenedor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    bool bIsBurning = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barricade")
    float BurnDuration = 15.0f;

    UFUNCTION(BlueprintCallable, Category = "Barricade")
    void RecibirDano(float Cantidad);

    UFUNCTION(BlueprintCallable, Category = "Barricade")
    void PrenderFuego();

    UFUNCTION(BlueprintPure, Category = "Barricade")
    bool EstaDestruida() const { return CurrentHealth <= 0.f; }

    /** Road segment index this barricade blocks (-1 = none) */
    UPROPERTY(BlueprintReadWrite, Category = "Barricade")
    int32 TramoBloqueado = -1;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MeshComp;
    UPROPERTY() TObjectPtr<UNiagaraComponent> FireVFX;
    float BurnTimer = 0.f;

    void DestruirBarricada();
};
