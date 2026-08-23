#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaNPCPedestrianSystem.generated.h"

class ASkeletalMeshActor;
class USkeletalMesh;
class UAnimSequence;

UENUM(BlueprintType)
enum class ENPCActivity : uint8
{
    Walk,
    Stand,
    Sit,
    LookAround,
    EnterShop,
    WaitAtBusStop,
    AtBench
};

USTRUCT(BlueprintType)
struct FNPCPedestrian
{
    GENERATED_BODY()
    FString Nombre;
    FString Barrio;
    ENPCActivity ActividadActual = ENPCActivity::Walk;
    FVector PosicionInicio = FVector::ZeroVector;
    FVector PosicionObjetivo = FVector::ZeroVector;
    FVector DireccionMovimiento = FVector::ForwardVector;
    float Velocidad = 150.0f;
    int32 GrupoEdad = 0;
    bool bLlevaCompras = false;
    bool bMascota = false;
    float TiempoEnActividad = 0.0f;
    float DuracionActividad = 5.0f;
    TWeakObjectPtr<ASkeletalMeshActor> ActorAsociado;
};

UCLASS()
class GF_NPCS_API UAlsasuaNPCPedestrianSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs")
    void GenerarNPCs();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs")
    void ActualizarNPCs(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs")
    int32 MaxNPCs = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs")
    float SpawnRadius = 2000.0f;

    const TArray<FNPCPedestrian>& GetNPCs() const { return NPCs; }

private:
    TArray<FNPCPedestrian> NPCs;
    bool bInitialized = false;
    TArray<TArray<FVector>> CallesCache;

    UPROPERTY() USkeletalMesh* MeshHombre = nullptr;
    UPROPERTY() USkeletalMesh* MeshMujer = nullptr;
    UPROPERTY() UAnimSequence* AnimCaminar = nullptr;
    UPROPERTY() UAnimSequence* AnimIdle = nullptr;

    void CargarAssetsPersonaje();
    void CargarCallejero();
    void CrearNPCEnPunto(FNPCPedestrian& NPC);
    FVector ObtenerPuntoCalle(const FString& Barrio);
    FVector ObtenerPuntoCalleAleatorio();
    void CambiarActividad(FNPCPedestrian& NPC);
};
