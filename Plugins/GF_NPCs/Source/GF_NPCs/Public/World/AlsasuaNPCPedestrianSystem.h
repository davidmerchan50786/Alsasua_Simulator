#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaNPCPedestrianSystem.generated.h"

class ASkeletalMeshActor;
class UStaticMeshComponent;
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

/** Distance-based LOD for NPC rendering */
UENUM(BlueprintType)
enum class ENPCLod : uint8
{
    Full,       // < 100m: skeletal mesh, full AI, animations
    Proxy,      // 100-300m: static mesh, no AI, simpler
    Hidden      // > 300m: invisible, no tick
};

USTRUCT(BlueprintType)
struct FNPCPedestrian
{
    GENERATED_BODY()
    FString Nombre;
    FString Barrio;
    ENPCActivity ActividadActual = ENPCActivity::Walk;
    ENPCLod LodActual = ENPCLod::Full;
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
    TWeakObjectPtr<AActor> ProxyActor;
    bool bEsManifestante = false;  // Joined a manifestation
};

UCLASS()
class GF_NPCS_API UAlsasuaNPCPedestrianSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear
{
    GENERATED_BODY()

public:
	virtual int32 EjecutarArranque() override;
	virtual FString EtiquetaArranque() const override;
	virtual int32 OrdenArranque() const override;
	virtual void TiquearPilar(float Dt) override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs")
    void GenerarNPCs();

    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs")
    void ActualizarNPCs(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs")
    int32 MaxNPCs = 600;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs")
    float SpawnRadius = 2000.0f;

    /** LOD distance thresholds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|LOD")
    float LodProxyDistance = 30000.0f;  // 300m

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|LOD")
    float LodFullDistance = 10000.0f;   // 100m

    /** NPCs per frame budget (full AI tick) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|Perf")
    int32 MaxTicksPerFrame = 80;

    const TArray<FNPCPedestrian>& GetNPCs() const { return NPCs; }

    /** Find nearby NPCs within radius */
    TArray<int32> GetNearbyNPCs(const FVector& Location, float Radius) const;

    /** Force NPC to join manifestation */
    void UnirAManifestacion(int32 Index);

    // El tiqueo real llega por IAlsasuaPilarTiquear::TiquearPilar, que el
    // DirectorArranque llama una vez por frame; Tick() es un metodo normal,
    // no FTickableGameObject (evitaria un doble tiqueo).
    void Tick(float DeltaTime);

private:
    TArray<FNPCPedestrian> NPCs;
    bool bInitialized = false;
    TArray<TArray<FVector>> CallesCache;
    int32 TickIndex = 0;  // Round-robin tick index

    UPROPERTY() USkeletalMesh* MeshHombre = nullptr;
    UPROPERTY() USkeletalMesh* MeshMujer = nullptr;
    UPROPERTY() UAnimSequence* AnimCaminar = nullptr;
    UPROPERTY() UAnimSequence* AnimIdle = nullptr;

    void CargarAssetsPersonaje();
    void CargarCallejero();
    void CrearNPCEnPunto(FNPCPedestrian& NPC);
    void CrearProxyNPC(FNPCPedestrian& NPC);
    void ActualizarLOD(FNPCPedestrian& NPC, const FVector& PlayerPos);
    FVector ObtenerPuntoCalle(const FString& Barrio);
    FVector ObtenerPuntoCalleAleatorio();
    void CambiarActividad(FNPCPedestrian& NPC);
};
