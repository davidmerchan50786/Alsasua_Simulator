#include "Environment/TreeSpawnerComponent.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

UTreeSpawnerComponent::UTreeSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTreeSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		SpawnAllTrees();
	}
}

void UTreeSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllTrees();
	Super::EndPlay(EndPlayReason);
}

void UTreeSpawnerComponent::LoadFromJson(const FString& JsonFilePath)
{
	ParseJsonFile(JsonFilePath);
}

void UTreeSpawnerComponent::LoadFromCSV(const FString& CSVFilePath)
{
	ParseCSVFile(CSVFilePath);
}

void UTreeSpawnerComponent::SpawnAllTrees()
{
	UE_LOG(LogTemp, Log, TEXT("TreeSpawnerComponent: spawning %d species"), TreeData.Num());
	for (const auto& Pair : TreeData)
	{
		SpawnTreeSpecies(Pair.Key);
	}
}

void UTreeSpawnerComponent::SpawnTreeSpecies(const FString& Species)
{
	if (!TreeData.Contains(Species))
		return;

	UE_LOG(LogTemp, Log, TEXT("TreeSpawnerComponent: spawning species=%s count=%d"), *Species, TreeData[Species].Num());
	CreateHISMForSpecies(Species);
	PlaceTreeInstances(Species, TreeData[Species]);
}

void UTreeSpawnerComponent::ClearAllTrees()
{
	for (auto& Pair : HISMComponents)
	{
		if (Pair.Value)
		{
			Pair.Value->ClearInstances();
			Pair.Value->DestroyComponent();
		}
	}
	HISMComponents.Empty();

	for (AActor* Holder : HISMHolders)
	{
		if (Holder)
			Holder->Destroy();
	}
	HISMHolders.Empty();

	TreeData.Empty();
}

void UTreeSpawnerComponent::ClearTreeSpecies(const FString& Species)
{
	if (HISMComponents.Contains(Species))
	{
		if (HISMComponents[Species])
		{
			HISMComponents[Species]->ClearInstances();
			HISMComponents[Species]->DestroyComponent();
		}
		HISMComponents.Remove(Species);
	}

	if (TreeData.Contains(Species))
		TreeData.Remove(Species);
}

void UTreeSpawnerComponent::CreateHISMForSpecies(const FString& Species)
{
	if (!SpeciesMeshMap.Contains(Species))
		return;

	UStaticMesh* Mesh = SpeciesMeshMap[Species].Mesh;
	if (!Mesh)
		return;

	GetOrCreateHISM(Species, Mesh);
}

UHierarchicalInstancedStaticMeshComponent* UTreeSpawnerComponent::GetOrCreateHISM(const FString& Species, UStaticMesh* Mesh)
{
	if (HISMComponents.Contains(Species) && HISMComponents[Species])
		return HISMComponents[Species];

	AActor* Holder = GetWorld()->SpawnActor<AActor>();
	if (Holder)
	{
#if WITH_EDITOR
		Holder->SetActorLabel(FString::Printf(TEXT("TreeHISM_%s"), *Species));
#endif
		UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(Holder);
		if (HISM)
		{
			HISM->SetStaticMesh(Mesh);
			HISM->SetFlags(RF_Transactional);
			HISM->SetMobility(EComponentMobility::Static);
			HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HISM->SetCanEverAffectNavigation(false);
			HISM->SetVisibility(true);
			HISM->SetHiddenInGame(false);
			HISM->SetupAttachment(Holder->GetRootComponent());
			HISM->RegisterComponent();
			HISMComponents.Add(Species, HISM);
			HISMHolders.Add(Holder);
			UE_LOG(LogTemp, Log, TEXT("TreeSpawnerComponent: created HISM for species=%s mesh=%s"), *Species, *Mesh->GetName());
		}
		return HISM;
	}
	return nullptr;
}

void UTreeSpawnerComponent::PlaceTreeInstances(const FString& Species, const TArray<FTreePlacementData>& Trees)
{
	UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Species, nullptr);
	if (!HISM)
		return;

	for (const FTreePlacementData& Tree : Trees)
	{
		FTransform Transform;
		FVector Location = Tree.Position;
		if (bApplyWorldOffset)
			Location += OriginOffset.GetLocation();

		Transform.SetLocation(Location + FVector(0.0f, 0.0f, HeightOffset));
		Transform.SetRotation(FRotator(0.0f, bRandomizeRotation ? FMath::FRand() * 360.0f : Tree.RotationDegrees, 0.0f).Quaternion());

		float Scale = 1.0f;
		if (bRandomizeScale && SpeciesMeshMap.Contains(Species))
		{
			Scale = FMath::FRandRange(SpeciesMeshMap[Species].ScaleRange.X, SpeciesMeshMap[Species].ScaleRange.Y);
		}
		Transform.SetScale3D(FVector(Scale));

		HISM->AddInstance(Transform);
	}
	UE_LOG(LogTemp, Log, TEXT("TreeSpawnerComponent: species=%s placed=%d instances"), *Species, Trees.Num());
}

bool UTreeSpawnerComponent::ParseJsonFile(const FString& FilePath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
		return false;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, Root))
		return false;

	const TArray<TSharedPtr<FJsonValue>>* TreesArray;
	if (!Root->TryGetArrayField(TEXT("trees"), TreesArray))
		return false;

	TreeData.Empty();

	for (const TSharedPtr<FJsonValue>& Val : *TreesArray)
	{
		const TSharedPtr<FJsonObject>* Obj;
		if (!Val->TryGetObject(Obj))
			continue;

		FTreePlacementData Data;
		Data.Position.X = (*Obj)->GetNumberField(TEXT("x"));
		Data.Position.Y = (*Obj)->GetNumberField(TEXT("y"));
		Data.Position.Z = (*Obj)->GetNumberField(TEXT("z"));
		Data.Height = (*Obj)->GetNumberField(TEXT("height"));
		Data.CrownRadius = (*Obj)->GetNumberField(TEXT("crown_radius"));
		Data.RotationDegrees = (*Obj)->GetNumberField(TEXT("rotation"));
		Data.Species = (*Obj)->GetStringField(TEXT("species"));

		TreeData.FindOrAdd(Data.Species).Add(Data);
	}

	return true;
}

bool UTreeSpawnerComponent::ParseCSVFile(const FString& FilePath)
{
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *FilePath))
		return false;

	TArray<FString> Lines;
	CSVContent.ParseIntoArrayLines(Lines);

	if (Lines.Num() < 2)
		return false;

	TreeData.Empty();

	for (int32 i = 1; i < Lines.Num(); ++i)
	{
		TArray<FString> Fields;
		Lines[i].ParseIntoArray(Fields, TEXT(","), true);

		if (Fields.Num() < 6)
			continue;

		FTreePlacementData Data;
		Data.Position.X = FCString::Atof(*Fields[0]);
		Data.Position.Y = FCString::Atof(*Fields[1]);
		Data.Position.Z = FCString::Atof(*Fields[2]);
		Data.Height = FCString::Atof(*Fields[3]);
		Data.CrownRadius = FCString::Atof(*Fields[4]);
		Data.RotationDegrees = FCString::Atof(*Fields[5]);
		Data.Species = Fields.Num() > 6 ? Fields[6] : TEXT("unknown");

		TreeData.FindOrAdd(Data.Species).Add(Data);
	}

	return true;
}
