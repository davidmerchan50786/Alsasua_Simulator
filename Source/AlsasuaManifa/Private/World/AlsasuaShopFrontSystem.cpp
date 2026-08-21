#include "World/AlsasuaShopFrontSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "World/AlsasuaMuros.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaShopFrontSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarTiendas();
}

void UAlsasuaShopFrontSystem::Deinitialize()
{
    Tiendas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaShopFrontSystem::CargarTiendas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/signage_data.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath)) return false;

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Arr)) return false;

    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Tipo = Obj->GetStringField(TEXT("tipo"));
        if (Tipo != TEXT("señal_comercio") && Tipo != TEXT("comercial")) continue;

        FShopFront Shop;
        Shop.Nombre = Obj->GetStringField(TEXT("texto"));
        Shop.Tipo = TEXT("tienda");
        // Sin "barrio" se dejaba "Herriko" por defecto, que es inventarle el
        // barrio a 16 de las 56 y además tapar el hueco: ahora queda vacío y lo
        // resuelve —o no— la colocación, diciendo cuántas se quedan fuera.
        Obj->TryGetStringField(TEXT("barrio"), Shop.Barrio);
        Obj->TryGetStringField(TEXT("calle"), Shop.Calle);
        Shop.X = Obj->HasField(TEXT("x")) ? Obj->GetNumberField(TEXT("x")) : 1891.5;
        Shop.Z = Obj->HasField(TEXT("z")) ? Obj->GetNumberField(TEXT("z")) : 8572.0;
        // La rotación la fija el muro donde acabe, en ColocarTiendasEnMundo.
        Shop.Rotacion = 0.0f;
        Shop.AnchoM = 4.0f;
        Shop.AlturaM = 3.0f;
        Shop.ColorFachada = TEXT("blanco");

        const FString LowerName = Shop.Nombre.ToLower();
        Shop.bConToldo = LowerName.Contains(TEXT("taberna")) || LowerName.Contains(TEXT("cafe"))
            || LowerName.Contains(TEXT("bar")) || LowerName.Contains(TEXT("pintxos"));
        Shop.ColorToldo = LowerName.Contains(TEXT("okindegia")) ? TEXT("blanco") : TEXT("rojo");
        Shop.bConRotulo = true;
        Shop.Horario = TEXT("10:00-14:00 / 17:00-21:00");

        Tiendas.Add(Shop);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("ShopFront: %d tiendas reales cargadas"), Tiendas.Num());
    return true;
}

namespace
{
    /**
     * Semilla estable a partir del nombre del comercio (FNV-1a de 32 bits).
     *
     * No vale GetTypeHash: su valor no está garantizado entre compilaciones, y
     * lo que se busca aquí es justo lo contrario — que Amaia Okindegia salga
     * siempre en la misma fachada.
     */
    uint32 SemillaDe(const FString& Texto)
    {
        uint32 H = 2166136261u;
        for (TCHAR C : Texto) { H ^= static_cast<uint32>(C); H *= 16777619u; }
        return H;
    }
}

int32 UAlsasuaShopFrontSystem::ColocarTiendasEnMundo()
{
    if (!bCargado && !CargarTiendas()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    // Las coordenadas de estos 56 comercios no sirven, y no lo dicen.
    //
    // 40 vienen amontonados en un radio de diez metros alrededor de
    // (1891.5, 8572.0) —que es OriginLocalX/Z, la constante de centrado, no una
    // dirección— a 126 m del edificio más cercano. Los otros 16 son de los que
    // el generador situaba "por dirección" y están a hasta 216 km. Colocarlos
    // donde dice el dato deja las 56 tiendas en un corro en mitad de un prado.
    //
    // Lo que sí es bueno del dato es el contenido: nombres reales en euskera,
    // tipo de negocio y el barrio (o la calle, que en cuatro de los seis casos
    // es también un nombre de barrio). Así que la tienda va a una fachada de su
    // barrio, elegida por el nombre del comercio para que no se mueva entre
    // arranques, y se dice cuántas no se han podido situar.
    const TArray<AlsasuaMuros::FMuro>& Muros = AlsasuaMuros::Todos();

    TMap<FString, TArray<int32>> PorBarrio;
    for (int32 i = 0; i < Muros.Num(); ++i)
    {
        if (Muros[i].LargoM < 5.0f || Muros[i].Barrio.IsEmpty()) continue;
        PorBarrio.FindOrAdd(Muros[i].Barrio).Add(i);
    }
    if (PorBarrio.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShopFront: sin fachadas donde poner escaparate"));
        return 0;
    }

    // Fuera del bucle: se resolvían una vez por tienda.
    UStaticMesh* Malla = AlsasuaMallaFab::Resolver(TEXT("escaparate"),
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Malla) return 0;
    UMaterialInterface* MatTienda = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Tienda"));

    int32 Placed = 0, SinBarrio = 0;
    for (FShopFront& Tienda : Tiendas)
    {
        // El campo "calle" trae un nombre de barrio en cuatro de los seis casos
        // (Herriko, Zelai, Errota, Intxostia); los otros dos —Kale Nagusia y
        // Avenida Pamplona— son calles de verdad que roads_unity.json no tiene
        // con ese nombre, y adivinarles un barrio sería inventarlo.
        const TArray<int32>* Candidatos = PorBarrio.Find(Tienda.Barrio);
        if (!Candidatos) Candidatos = PorBarrio.Find(Tienda.Calle);
        if (!Candidatos || Candidatos->Num() == 0) { ++SinBarrio; continue; }

        FRandomStream Sorteo(static_cast<int32>(SemillaDe(Tienda.Nombre)));
        const AlsasuaMuros::FMuro& Muro = Muros[(*Candidatos)[Sorteo.RandHelper(Candidatos->Num())]];

        const float AnchoM = FMath::Min(Tienda.AnchoM, Muro.LargoM - 1.0f);
        const float MargenT = (AnchoM * 0.5f) / Muro.LargoM;
        const float T = Sorteo.FRandRange(MargenT, 1.0f - MargenT);

        const FVector2D XZ = Muro.A + (Muro.B - Muro.A) * T
                           + Muro.Fuera * (Tienda.AnchoM * 0.0f + 0.10f);

        FVector Loc = UAlsasuaGeoData::RelLocalToUE5(FVector(XZ.X, 0.0f, XZ.Y));
        // El cubo se centra en su origen: apoyado en el suelo, el centro va a
        // media altura. Antes se ponía la Z en el suelo y medio escaparate
        // quedaba enterrado.
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y) + Tienda.AlturaM * 50.0f;

        Tienda.Rotacion = Muro.Yaw;
        Tienda.X = XZ.X + UAlsasuaGeoData::OX;
        Tienda.Z = XZ.Y + UAlsasuaGeoData::OZ;

        AStaticMeshActor* ShopActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0.f, Muro.Yaw, 0.f));
        if (!ShopActor) continue;

        ShopActor->SetMobility(EComponentMobility::Static);
        // El eje local X mira hacia afuera del muro: el fondo del escaparate va
        // en X y el ancho en Y. Estaba al revés, con el ancho metido en el muro.
        ShopActor->SetActorScale3D(FVector(0.35f, AnchoM, Tienda.AlturaM));
        ShopActor->GetStaticMeshComponent()->SetStaticMesh(Malla);
        if (MatTienda) ShopActor->GetStaticMeshComponent()->SetMaterial(0, MatTienda);

#if WITH_EDITOR
        ShopActor->SetActorLabel(*FString::Printf(TEXT("Tienda_%s_%s"),
            *Tienda.Nombre.Left(15), *Tienda.Tipo));
#endif
        Placed++;
    }

    UE_LOG(LogTemp, Log,
        TEXT("ShopFront: %d escaparates en fachada de su barrio; %d sin barrio conocido (la coordenada del dato no sirve)"),
        Placed, SinBarrio);
    return Placed;
}
