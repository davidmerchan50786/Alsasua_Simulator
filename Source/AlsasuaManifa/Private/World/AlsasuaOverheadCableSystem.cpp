#include "World/AlsasuaOverheadCableSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

namespace
{
    /**
     * Calles con tendido aéreo.
     *
     * Se tendía sobre cualquier vía con nombre, y las 489 de roads_unity.json
     * incluyen la A-10, la N-1 y sus 50 enlaces: postes de hormigón y cable a
     * cinco metros por encima de la autovía. El tendido aéreo de Altsasu va por
     * las calles del casco.
     */
    bool LlevaTendido(const FString& Tipo)
    {
        return Tipo == TEXT("residential") || Tipo == TEXT("tertiary");
    }
}

void UAlsasuaOverheadCableSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaOverheadCableSystem::ColocarCables()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    TArray<TSharedPtr<FJsonValue>> Vias;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Vias, { TEXT("roads") }))
        return 0;

    Cables.Empty();

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("TendidoAereo"));
#endif

    UStaticMesh* Cubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Cilindro = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (!Cubo || !Cilindro) return 0;

    // Fuera del bucle: eran cuatro LoadObject por vano.
    auto CrearCapa = [&](const TCHAR* Nombre, UStaticMesh* M, const TCHAR* RutaMat, bool bColision)
        -> UHierarchicalInstancedStaticMeshComponent*
    {
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(M);
        if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, RutaMat))
            C->SetMaterial(0, Mat);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // El cable no tiene colisión: es un hilo a cinco metros y engancharía
        // los LineTrace de altura de todo lo que se apoye por raycast.
        C->SetCollisionEnabled(bColision ? ECollisionEnabled::QueryAndPhysics
                                         : ECollisionEnabled::NoCollision);
        C->RegisterComponent();
        return C;
    };

    UHierarchicalInstancedStaticMeshComponent* CapaCables =
        CrearCapa(TEXT("ISM_Cables"), Cubo, TEXT("/Game/Materiales/M_Metal_Negro"), /*bColision=*/false);
    UHierarchicalInstancedStaticMeshComponent* CapaPostes =
        CrearCapa(TEXT("ISM_Postes"), Cilindro, TEXT("/Game/Materiales/M_Hormigon"), /*bColision=*/true);

    int32 Vanos = 0, Postes = 0;

    for (const TSharedPtr<FJsonValue>& VV : Vias)
    {
        if (Vanos >= MaxCables) break;

        const TSharedPtr<FJsonObject> Via = VV->AsObject();
        if (!Via.IsValid()) continue;

        FString Tipo, Calle;
        Via->TryGetStringField(TEXT("type"), Tipo);
        Via->TryGetStringField(TEXT("name"), Calle);
        if (!LlevaTendido(Tipo) || Calle.IsEmpty()) continue;

        const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
        if (!Via->TryGetArrayField(TEXT("points"), Pts) || !Pts || Pts->Num() < 2) continue;

        // Polilínea de la calle en local relativo.
        TArray<FVector2D> Eje;
        Eje.Reserve(Pts->Num());
        for (const TSharedPtr<FJsonValue>& PV : *Pts)
        {
            const TSharedPtr<FJsonObject> PO = PV->AsObject();
            if (!PO.IsValid()) continue;
            Eje.Add(FVector2D(PO->GetNumberField(TEXT("x")), PO->GetNumberField(TEXT("z"))));
        }
        if (Eje.Num() < 2) continue;

        float LargoTotalM = 0.0f;
        for (int32 i = 0; i + 1 < Eje.Num(); ++i) LargoTotalM += FVector2D::Distance(Eje[i], Eje[i + 1]);

        const float VanoM = FMath::Max(5.0f, VanoCm * 0.01f);
        const int32 NumPostes = FMath::FloorToInt(LargoTotalM / VanoM) + 1;
        if (NumPostes < 2) continue;

        // Los postes se calculan una vez para toda la calle y los vanos van de
        // uno al siguiente. Antes cada vano plantaba sus tres postes, así que
        // los extremos compartidos salían duplicados, uno dentro de otro.
        TArray<FVector> Cabezas;   // cima del poste, donde engancha el cable
        Cabezas.Reserve(NumPostes);

        for (int32 p = 0; p < NumPostes; ++p)
        {
            const float S = (NumPostes > 1) ? LargoTotalM * p / (NumPostes - 1) : 0.0f;

            // Punto a distancia S por la polilínea.
            float Acum = 0.0f;
            FVector2D XZ = Eje.Last();
            for (int32 i = 0; i + 1 < Eje.Num(); ++i)
            {
                const float L = FVector2D::Distance(Eje[i], Eje[i + 1]);
                if (Acum + L >= S && L > KINDA_SMALL_NUMBER)
                {
                    XZ = Eje[i] + (Eje[i + 1] - Eje[i]) / L * (S - Acum);
                    break;
                }
                Acum += L;
            }

            // Cada poste muestrea SU suelo. Antes los tres del vano usaban la
            // cota del primer punto, así que en una calle con pendiente el
            // segundo y el tercero salían flotando o medio enterrados.
            FVector Base = UAlsasuaGeoData::RelLocalToUE5(FVector(XZ.X, 0.0f, XZ.Y));
            Base.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Base.X, Base.Y);

            // El cilindro del motor mide 100 uu de alto y se centra en su
            // origen: apoyado en el suelo, el centro va a media altura.
            CapaPostes->AddInstance(FTransform(
                FRotator::ZeroRotator,
                Base + FVector(0.0f, 0.0f, AlturaCables * 0.5f),
                FVector(0.12f, 0.12f, AlturaCables / 100.0f)), /*bWorldSpace=*/true);
            ++Postes;

            Cabezas.Add(Base + FVector(0.0f, 0.0f, AlturaCables));
        }

        for (int32 p = 0; p + 1 < Cabezas.Num() && Vanos < MaxCables; ++p)
        {
            const FVector A = Cabezas[p];
            const FVector B = Cabezas[p + 1];
            const float Largo = FVector::Distance(A, B);
            if (Largo < 100.0f) continue;

            // El cable llevaba sólo yaw, así que entre dos postes a cotas
            // distintas salía horizontal y sin tocar ninguno de los dos.
            //
            // Y la caída se aplicaba bajando el centro de una barra recta, lo
            // que despega los dos extremos de sus postes en vez de curvar el
            // cable. Con dos medios vanos que se encuentran en el punto bajo, la
            // barra sigue siendo recta pero engancha en los tres sitios.
            const float CaidaCm = 30.0f;
            const FVector Bajo = (A + B) * 0.5f - FVector(0.0f, 0.0f, CaidaCm);

            const FVector Tramos[2][2] = { { A, Bajo }, { Bajo, B } };
            for (const FVector (&T)[2] : Tramos)
            {
                const float L = FVector::Distance(T[0], T[1]);
                if (L < KINDA_SMALL_NUMBER) continue;
                CapaCables->AddInstance(FTransform(
                    (T[1] - T[0]).Rotation(), (T[0] + T[1]) * 0.5f,
                    FVector(L / 100.0f, 0.02f, 0.02f)), /*bWorldSpace=*/true);
            }

            FOverheadCable Cable;
            Cable.Inicio = A;
            Cable.Fin = B;
            Cable.Caida = CaidaCm;
            Cable.Tipo = TEXT("electrico");
            Cable.Calle = Calle;
            Cables.Add(MoveTemp(Cable));
            ++Vanos;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Cables: %d vanos sobre %d postes, en 2 capas instanciadas"),
        Vanos, Postes);
    return Vanos;
}
