#include "World/AlsasuaBillboardImpostorComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UAlsasuaBillboardImpostorComponent::UAlsasuaBillboardImpostorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UAlsasuaBillboardImpostorComponent::BeginPlay()
{
	Super::BeginPlay();
	FindOriginalMesh();
}

void UAlsasuaBillboardImpostorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float Dist = GetDistanceToCamera();

	if (!bIsShowingBillboard && Dist > BillboardEnableDistance)
	{
		ShowBillboard();
	}
	else if (bIsShowingBillboard && Dist < BillboardDisableDistance)
	{
		HideBillboard();
	}
}

float UAlsasuaBillboardImpostorComponent::GetDistanceToCamera() const
{
	UWorld* W = GetWorld();
	if (!W) return 0.f;

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC || !PC->GetPawn()) return 0.f;

	return FVector::Dist(GetOwner()->GetActorLocation(), PC->GetPawn()->GetActorLocation());
}

void UAlsasuaBillboardImpostorComponent::FindOriginalMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UStaticMeshComponent*> Meshes;
	Owner->GetComponents<UStaticMeshComponent>(Meshes);
	if (Meshes.Num() > 0)
	{
		OriginalMesh = Meshes[0];
	}
}

void UAlsasuaBillboardImpostorComponent::ShowBillboard()
{
	if (bIsShowingBillboard) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	if (!Billboard)
	{
		Billboard = NewObject<UBillboardComponent>(Owner);
		Billboard->SetupAttachment(Owner->GetRootComponent());
		Billboard->RegisterComponent();
	}

	if (ImpostorTexture)
	{
		Billboard->SetSprite(ImpostorTexture);
	}

	Billboard->SetWorldScale3D(FVector(BillboardWidth / 100.f, BillboardHeight / 100.f, 1.f));
	Billboard->SetCastShadow(bCastShadows);
	Billboard->SetVisibility(true);
	Billboard->SetHiddenInGame(false);

	if (OriginalMesh)
	{
		OriginalMesh->SetVisibility(false);
		OriginalMesh->SetComponentTickEnabled(false);
	}

	bIsShowingBillboard = true;
}

void UAlsasuaBillboardImpostorComponent::HideBillboard()
{
	if (!bIsShowingBillboard) return;

	if (Billboard)
	{
		Billboard->SetVisibility(false);
		Billboard->SetHiddenInGame(true);
	}

	if (OriginalMesh)
	{
		OriginalMesh->SetVisibility(true);
		OriginalMesh->SetComponentTickEnabled(true);
	}

	bIsShowingBillboard = false;
}

void UAlsasuaBillboardImpostorComponent::SetImpostorTexture(UTexture2D* Texture)
{
	ImpostorTexture = Texture;
	if (Billboard && Texture)
	{
		Billboard->SetSprite(Texture);
	}
}

void UAlsasuaBillboardImpostorComponent::SetBillboardSize(float Width, float Height)
{
	BillboardWidth = Width;
	BillboardHeight = Height;
	if (Billboard)
	{
		Billboard->SetWorldScale3D(FVector(Width / 100.f, Height / 100.f, 1.f));
	}
}
