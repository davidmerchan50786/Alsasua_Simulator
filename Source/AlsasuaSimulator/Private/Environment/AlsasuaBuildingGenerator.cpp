#include "Environment/AlsasuaBuildingGenerator.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

AAlsasuaBuildingGenerator::AAlsasuaBuildingGenerator() {
    BuildingInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BuildingInstances"));
    BuildingInstances->SetMobility(EComponentMobility::Static);
    RootComponent = BuildingInstances;
}

void AAlsasuaBuildingGenerator::SpawnBuildingsFromData(FString JsonPath) {
    FString JsonContent;
    if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[AlsasuaEnv] No se pudo leer %s"), *JsonPath);
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AlsasuaEnv] JSON inválido en %s"), *JsonPath);
        return;
    }

    BuildingInstances->ClearInstances();

    const TArray<TSharedPtr<FJsonValue>>* BuildingsArray;
    if (!Root->TryGetArrayField(TEXT("buildings"), BuildingsArray))
    {
        UE_LOG(LogTemp, Warning, TEXT("[AlsasuaEnv] No se encontró array 'buildings' en %s"), *JsonPath);
        return;
    }

    int32 Spawned = 0;
    for (const TSharedPtr<FJsonValue>& Val : *BuildingsArray)
    {
        const TSharedPtr<FJsonObject>* Obj;
        if (!Val->TryGetObject(Obj)) continue;

        const double X = (*Obj)->GetNumberField(TEXT("x"));
        const double Y = (*Obj)->GetNumberField(TEXT("y"));
        const double Z = (*Obj)->GetNumberField(TEXT("z"));
        double Height; const bool bHasHeight = (*Obj)->TryGetNumberField(TEXT("height"), Height); Height = bHasHeight ? Height : 300.0;
        double Width; const bool bHasWidth = (*Obj)->TryGetNumberField(TEXT("width"), Width); Width = bHasWidth ? Width : 400.0;
        double Depth; const bool bHasDepth = (*Obj)->TryGetNumberField(TEXT("depth"), Depth); Depth = bHasDepth ? Depth : 400.0;
        double Rotation; const bool bHasRotation = (*Obj)->TryGetNumberField(TEXT("rotation"), Rotation); Rotation = bHasRotation ? Rotation : 0.0;

        FVector Location(X, Y, Z);
        FRotator Rot(0.0, Rotation, 0.0);
        FVector Scale(Width / 400.0, Depth / 400.0, Height / 300.0);

        FTransform T;
        T.SetLocation(Location);
        T.SetRotation(FQuat(Rot));
        T.SetScale3D(Scale);

        BuildingInstances->AddInstance(T, true);
        Spawned++;
    }

    UE_LOG(LogTemp, Log, TEXT("[AlsasuaEnv] %d edificios cargados desde %s"), Spawned, *JsonPath);
}
