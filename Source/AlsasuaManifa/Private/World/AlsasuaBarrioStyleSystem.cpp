#include "World/AlsasuaBarrioStyleSystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/Engine.h"

namespace
{
	static void ApplyMaterialStyle(UPrimitiveComponent* Primitive, const FLinearColor& FachadaColor, const FLinearColor& TejadoColor)
	{
		if (!Primitive)
		{
			return;
		}

		const int32 NumMaterials = Primitive->GetNumMaterials();
		for (int32 Index = 0; Index < NumMaterials; ++Index)
		{
			UMaterialInterface* Material = Primitive->GetMaterial(Index);
			if (!Material)
			{
				continue;
			}

			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
			if (!MID)
			{
				MID = UMaterialInstanceDynamic::Create(Material, Primitive);
				if (!MID)
				{
					continue;
				}
				Primitive->SetMaterial(Index, MID);
			}

			FLinearColor ColorForSlot = (Index == 1) ? TejadoColor : FachadaColor;
			if (Index > 1)
			{
				ColorForSlot = FachadaColor;
			}

			MID->SetVectorParameterValue(FName("BaseColor"), ColorForSlot);
			MID->SetVectorParameterValue(FName("TintColor"), ColorForSlot);
			MID->SetVectorParameterValue(FName("WallColor"), FachadaColor);
			MID->SetVectorParameterValue(FName("RoofColor"), TejadoColor);
			MID->SetVectorParameterValue(FName("Color"), ColorForSlot);
		}
	}
}

UAlsasuaBarrioStyleSystem::UAlsasuaBarrioStyleSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaBarrioStyleSystem::BeginPlay()
{
	Super::BeginPlay();
	ApplyBarrioStyles();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		UpdateColorGradingForLocation(Owner->GetActorLocation());
	}
}

void UAlsasuaBarrioStyleSystem::ApplyBarrioStyles()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FString Label = Owner->GetName().ToLower();
	EBarrioStyle Barrio = EBarrioStyle::Herriko;

	FLinearColor FachadaColor = FLinearColor(0.75f, 0.72f, 0.65f);
	FLinearColor TejadoColor = FLinearColor(0.7f, 0.35f, 0.15f);

	if (Label.Contains(TEXT("herriko")) || Label.Contains(TEXT("casco")))
	{
		Barrio = EBarrioStyle::Herriko;
		FachadaColor = HerrikoFachadaPiedra;
		TejadoColor = HerrikoTejadoTerracota;
	}
	else if (Label.Contains(TEXT("zelai")))
	{
		Barrio = EBarrioStyle::Zelai;
		FachadaColor = ZelaiFachadaHormigon;
		TejadoColor = ZelaiTejadoGris;
	}
	else if (Label.Contains(TEXT("intxostia")))
	{
		Barrio = EBarrioStyle::Intxostia;
		FachadaColor = IntxostiaFachadaHormigon;
		TejadoColor = IntxostiaTejadoPlano;
	}
	else if (Label.Contains(TEXT("errota")) || Label.Contains(TEXT("molina")))
	{
		Barrio = EBarrioStyle::Errota;
		FachadaColor = ErrotaFachadaLadrillo;
		TejadoColor = ErrotaTejadoTerracota;
	}
	else if (Label.Contains(TEXT("sanpedro")) || Label.Contains(TEXT("estacion")))
	{
		Barrio = EBarrioStyle::SanPedro;
		FachadaColor = FMath::Lerp(SanPedroFachadaPiedra, SanPedroFachadaHormigon, 0.5f);
		TejadoColor = FLinearColor(0.6f, 0.55f, 0.5f);
	}
	else if (Label.Contains(TEXT("harrobieta")) || Label.Contains(TEXT("mercado")))
	{
		Barrio = EBarrioStyle::Harrobieta;
		FachadaColor = HarrobietaFachadaPiedra;
		TejadoColor = HarrobietaTejadoTerracota;
	}
	else if (Label.Contains(TEXT("ferroviario")) || Label.Contains(TEXT("vias")))
	{
		Barrio = EBarrioStyle::Ferroviario;
		FachadaColor = FerroviarioFachadaLadrillo;
		TejadoColor = FerroviarioTejadoOxidado;
	}
	else if (Label.Contains(TEXT("monte")) || Label.Contains(TEXT("ladera")))
	{
		Barrio = EBarrioStyle::Monte;
		FachadaColor = MonteFachadaPiedraRustica;
		TejadoColor = MonteTejadoPizarra;
	}

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || Prim->IsPendingKill())
		{
			continue;
		}
		ApplyMaterialStyle(Prim, FachadaColor, TejadoColor);
	}

	UE_LOG(LogTemp, Log, TEXT("BarrioStyleSystem: owner=%s barrio=%d colors=%s"), *Owner->GetName(), (int32)Barrio, *GetDebugStyleSummary());
}

void UAlsasuaBarrioStyleSystem::ApplyBarrioLUT(EBarrioStyle Barrio)
{
	if (!bEnableColorMatching)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UTexture2D* LUT = BarrioLUTs.FindRef(Barrio);
	if (!LUT)
	{
		const FString AssetPath = DefaultLUTPath;
		LUT = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *AssetPath));
		if (!LUT)
		{
			const FSoftObjectPath SoftPath(AssetPath);
			LUT = Cast<UTexture2D>(SoftPath.TryLoad());
		}
	}
	if (!LUT)
	{
		UE_LOG(LogTemp, Warning, TEXT("BarrioStyleSystem: LUT missing for barrio=%d owner=%s"), (int32)Barrio, *Owner->GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("BarrioStyleSystem: applying LUT for barrio=%d owner=%s"), (int32)Barrio, *Owner->GetName());

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim)
		{
			continue;
		}

		const int32 NumMaterials = Prim->GetNumMaterials();
		for (int32 Index = 0; Index < NumMaterials; ++Index)
		{
			UMaterialInterface* Material = Prim->GetMaterial(Index);
			if (!Material)
			{
				continue;
			}

			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Material);
			if (!MID)
			{
				MID = UMaterialInstanceDynamic::Create(Material, Prim);
				if (!MID)
				{
					continue;
				}
				Prim->SetMaterial(Index, MID);
			}

			MID->SetTextureParameterValue(FName("BarrioLUT"), LUT);
			MID->SetTextureParameterValue(FName("LUTTexture"), LUT);
			MID->SetTextureParameterValue(FName("ColorLUT"), LUT);
			MID->SetScalarParameterValue(FName("ColorMatchingIntensity"), ColorMatchingIntensity);
			MID->SetScalarParameterValue(FName("LUTIntensity"), ColorMatchingIntensity);
		}
	}
}

EBarrioStyle UAlsasuaBarrioStyleSystem::GetBarrioStyleAtLocation(const FVector& WorldLocation)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return EBarrioStyle::Herriko;
	}

	const FString Label = Owner->GetName().ToLower();
	if (Label.Contains(TEXT("herriko")) || Label.Contains(TEXT("casco")))
	{
		return EBarrioStyle::Herriko;
	}
	if (Label.Contains(TEXT("zelai")))
	{
		return EBarrioStyle::Zelai;
	}
	if (Label.Contains(TEXT("intxostia")))
	{
		return EBarrioStyle::Intxostia;
	}
	if (Label.Contains(TEXT("errota")) || Label.Contains(TEXT("molina")))
	{
		return EBarrioStyle::Errota;
	}
	if (Label.Contains(TEXT("sanpedro")) || Label.Contains(TEXT("estacion")))
	{
		return EBarrioStyle::SanPedro;
	}
	if (Label.Contains(TEXT("harrobieta")) || Label.Contains(TEXT("mercado")))
	{
		return EBarrioStyle::Harrobieta;
	}
	if (Label.Contains(TEXT("ferroviario")) || Label.Contains(TEXT("vias")))
	{
		return EBarrioStyle::Ferroviario;
	}
	if (Label.Contains(TEXT("monte")) || Label.Contains(TEXT("ladera")))
	{
		return EBarrioStyle::Monte;
	}

	return EBarrioStyle::Herriko;
}

FString UAlsasuaBarrioStyleSystem::GetDebugStyleSummary() const
{
	AActor* Owner = GetOwner();
	const FString OwnerName = Owner ? Owner->GetName() : TEXT("<none>");
	return FString::Printf(TEXT("Owner=%s | ColorMatching=%s | LUT=%s | Intensity=%.2f"),
		*OwnerName,
		bEnableColorMatching ? TEXT("on") : TEXT("off"),
		*DefaultLUTPath,
		ColorMatchingIntensity);
}

void UAlsasuaBarrioStyleSystem::UpdateColorGradingForLocation(const FVector& PlayerLocation)
{
	ApplyBarrioStyles();
	ApplyBarrioLUT(GetBarrioStyleAtLocation(PlayerLocation));

	if (AActor* Owner = GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("BarrioStyleSystem: applied style for %s"), *Owner->GetName());
	}
}
