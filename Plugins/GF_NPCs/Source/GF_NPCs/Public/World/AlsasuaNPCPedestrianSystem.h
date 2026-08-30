#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "Services/INPCSocialService.h"
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

/** Personalidad del NPC: moldea decisiones, rutas, humor y diálogo */
UENUM(BlueprintType)
enum class ENPCPersonalidad : uint8
{
    Amable,     // saluda, sonríe, ayuda
    Timido,     // evita conversación, habla bajo
    Grumpy,     // quejumbroso, negativo
    Sociable,   // inicia conversaciones, hablador
    Serio,      // formal, lacónico
    Nervioso,   // ansioso, salta a la conclusión
    Rebelde     // anti-autoridad, reivindicativo
};

/** Modo de ser / estilo de vida del NPC */
UENUM(BlueprintType)
enum class ENPCEstilo : uint8
{
    Trabajador,     // trabaja, rutas al trabajo/taller
    Jubilado,       // pasea, bancos, plazas
    Estudiante,     // instituto, biblioteca, jóvenes
    Comerciante,    // tienda, mercado
    Deportista,     // corre, parque
    Obrero,         // obra, industrial
    Nocturno        // bares, sale de noche
};

UENUM(BlueprintType)
enum class ENPCHumor : uint8
{
    Neutral,
    Feliz,
    Tenso,
    Enfadado,
    Asustado,
    Entusiasmado
};

USTRUCT(BlueprintType)
struct FNPCPersona
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString Nombre;
    UPROPERTY(BlueprintReadOnly) int32 Edad = 30;
    UPROPERTY(BlueprintReadOnly) ENPCPersonalidad Personalidad = ENPCPersonalidad::Amable;
    UPROPERTY(BlueprintReadOnly) ENPCEstilo Estilo = ENPCEstilo::Trabajador;
    UPROPERTY(BlueprintReadOnly) float VozPitch = 1.0f;   // tono de voz (0.7-1.3)
    UPROPERTY(BlueprintReadOnly) FString FraseFavorita;  // línea distintiva
    UPROPERTY(BlueprintReadOnly) bool bEsGeneroso = false;  // propenso a ayudar al jugador
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

    /** Persona individual: vida, estilo, forma de ser, voz, diálogo */
    FNPCPersona Persona;

    /** Humor actual (cambia con contexto social/manifestación) */
    ENPCHumor HumorActual = ENPCHumor::Neutral;

    /** Última vez que participó en una conversación (evita spam) */
    float UltimaConversacion = -999.f;
};

UCLASS()
class GF_NPCS_API UAlsasuaNPCPedestrianSystem : public UGameInstanceSubsystem, public IAlsasuaPilarArranque, public IAlsasuaPilarTiquear, public INPCSocialService
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

    /** Nearest visible NPC to a location (for player interaction), -1 if none */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs|Social")
    virtual int32 GetNearestNPC(const FVector& Location, float MaxRadius = 300.f) const override;

    /** Get NPC persona by index */
    UFUNCTION(BlueprintPure, Category = "Alsasua|NPCs|Social")
    const FNPCPersona& GetPersona(int32 Index) const;

    // INPCSocialService: nombre de la persona por reflexion, sin exponer
    // FNPCPersona (que vive solo en GF_NPCs) al otro lado del contrato.
    virtual FString GetPersonaNombre(int32 Index) const override { return GetPersona(Index).Nombre; }

    // INPCSocialService: clave de diálogo por personalidad (Amable, Rebelde...),
    // denota el fichero Content/Dialogs/<clave>.json compartido por todos los
    // NPC de esa forma de ser. El nombre real es único por NPC, la personalidad no.
    virtual FString GetPersonaClave(int32 Index) const override
    {
        static const TMap<ENPCPersonalidad, FString> Claves = {
            { ENPCPersonalidad::Amable,   TEXT("Amable") },
            { ENPCPersonalidad::Timido,   TEXT("Timido") },
            { ENPCPersonalidad::Grumpy,   TEXT("Grumpy") },
            { ENPCPersonalidad::Sociable, TEXT("Sociable") },
            { ENPCPersonalidad::Serio,    TEXT("Serio") },
            { ENPCPersonalidad::Nervioso, TEXT("Nervioso") },
            { ENPCPersonalidad::Rebelde,  TEXT("Rebelde") },
        };
        const FString* K = Claves.Find(GetPersona(Index).Personalidad);
        return K ? *K : FString();
    }

    /** Player talks to nearest NPC — returns the spoken line (or empty) */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|NPCs|Social")
    virtual FString HablarConNPC(int32 Index) override;

    /** NPC-NPC conversation radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|Social")
    float RadioConversacion = 350.0f;

    /** Delegate: NPC spoke a line (SpeakerName, Line, VoicePitch) — for subtitles/voice */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNPCHabla, FString, Nombre, FString, Linea, float, Pitch);
    UPROPERTY(BlueprintAssignable, Category = "Alsasua|NPCs|Social")
    FOnNPCHabla OnNPCHabla;

    // Pollable last-spoken state for HUD subtitles
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|NPCs|Social")
    FString UltimoHablante;
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|NPCs|Social")
    FString UltimaLinea;
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|NPCs|Social")
    float UltimoPitch = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|NPCs|Social")
    float TiempoLinea = 0.f;   // seconds since last line, for fade

    /** Force NPC to join manifestation */
    void UnirAManifestacion(int32 Index);

    /** How many NPCs joined this session */
    UPROPERTY(BlueprintReadOnly, Category = "Alsasua|NPCs|Manifestacion")
    int32 ManifestantesUnidos = 0;

    /** Sound/shouting propagation radius (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|Sonido")
    float RadioGrito = 5000.0f;

    /** Base probability per second for nearby NPC to join (scales with apoyo) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|Manifestacion")
    float ProbabilidadUnirse = 0.15f;

    /** Extra join probability per nearby manifestante (peer pressure) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPCs|Manifestacion")
    float PresionGrupo = 0.05f;

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

    void ProcesarSocial(float DeltaTime);
    void GenerarPersona(FNPCPedestrian& NPC);
    void TradeConversacion(FNPCPedestrian& A, FNPCPedestrian& B);
    FString LineaDeConversacion(const FNPCPersona& Persona, bool bRangoEdadDiferente) const;
    FString NombreAleatorio(bool bMujer) const;
    FString FraseFavoritaPara(ENPCPersonalidad P) const;
    void ReproducirVoz(const FVector& Posicion, float Pitch);

    // Social tick state
    int32 SocialIndex = 0;
    float TimerSocial = 0.f;

    // Persona name pools
    TArray<FString> NombresHombre;
    TArray<FString> NombresMujer;
    bool bNombresCargados = false;
};
