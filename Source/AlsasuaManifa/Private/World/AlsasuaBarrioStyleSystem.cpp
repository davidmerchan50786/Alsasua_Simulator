#include "World/AlsasuaBarrioStyleSystem.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/MeshComponent.h"

UAlsasuaBarrioStyleSystem::UAlsasuaBarrioStyleSystem()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaBarrioStyleSystem::BeginPlay()
{
	Super::BeginPlay();
	ApplyBarrioStyles();
}

void UAlsasuaBarrioStyleSystem::ApplyBarrioStyles()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// El barrio lo pone ADirectorArranque al colgar el componente: es él quien
	// es capa WORLD y ve el AEdificioGenerado del que sale.
	//
	// Antes se sacaba de Owner->GetName().ToLower(), y el nombre de objeto de un
	// AEdificioGenerado es "EdificioGenerado_42": no contiene "herriko" ni
	// ninguno de los otros, así que los 1030 caían al estilo por defecto y los
	// ocho barrios se veían iguales. Se cae a GetName() sólo por si esto acaba
	// colgado de algo que no es un edificio.
	FString Label = Barrio.ToLower();
	if (Label.IsEmpty()) Label = Owner->GetName().ToLower();

	FLinearColor FachadaColor = FLinearColor(0.75f, 0.72f, 0.65f); // Default: piedra
	FLinearColor TejadoColor = FLinearColor(0.7f, 0.35f, 0.15f);   // Default: terracota
	float AlturaMedia = 1000.f;

	if (Label.Contains(TEXT("herriko")) || Label.Contains(TEXT("casco")))
	{
		FachadaColor = HerrikoFachadaPiedra;
		TejadoColor = HerrikoTejadoTerracota;
		AlturaMedia = HerrikoAlturaMedia;
	}
	else if (Label.Contains(TEXT("zelai")))
	{
		FachadaColor = ZelaiFachadaHormigon;
		TejadoColor = ZelaiTejadoGris;
		AlturaMedia = ZelaiAlturaMedia;
	}
	else if (Label.Contains(TEXT("intxostia")))
	{
		FachadaColor = IntxostiaFachadaHormigon;
		TejadoColor = IntxostiaTejadoPlano;
		AlturaMedia = IntxostiaAlturaMedia;
	}
	else if (Label.Contains(TEXT("errota")) || Label.Contains(TEXT("molina")))
	{
		FachadaColor = ErrotaFachadaLadrillo;
		TejadoColor = ErrotaTejadoTerracota;
		AlturaMedia = ErrotaAlturaMedia;
	}
	else if (Label.Contains(TEXT("sanpedro")) || Label.Contains(TEXT("estacion")))
	{
		FachadaColor = FMath::Lerp(SanPedroFachadaPiedra, SanPedroFachadaHormigon, 0.5f);
		TejadoColor = FLinearColor(0.6f, 0.55f, 0.5f); // Mixto
		AlturaMedia = SanPedroAlturaMedia;
	}
	else if (Label.Contains(TEXT("harrobieta")) || Label.Contains(TEXT("mercado")))
	{
		FachadaColor = HarrobietaFachadaPiedra;
		TejadoColor = HarrobietaTejadoTerracota;
		AlturaMedia = HarrobietaAlturaMedia;
	}
	else if (Label.Contains(TEXT("ferroviario")) || Label.Contains(TEXT("vias")))
	{
		FachadaColor = FerroviarioFachadaLadrillo;
		TejadoColor = FerroviarioTejadoOxidado;
		AlturaMedia = FerroviarioAlturaMedia;
	}
	else if (Label.Contains(TEXT("monte")) || Label.Contains(TEXT("ladera")))
	{
		FachadaColor = MonteFachadaPiedraRustica;
		TejadoColor = MonteTejadoPizarra;
		AlturaMedia = MonteAlturaMedia;
	}

	// Aplicar colores a materiales del edificio
	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim) continue;
		UMeshComponent* Mesh = Cast<UMeshComponent>(Prim);
		if (!Mesh) continue;

		const int32 NumMats = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			// El material del edificio no es un MID: lo pone el cargador tal
			// cual. Con `if (!MID) continue` no se escribía en ninguno, así que
			// aunque el barrio se hubiera detectado bien no se habría visto.
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
			if (!MID)
			{
				UMaterialInterface* Base = Mesh->GetMaterial(i);
				if (!Base) continue;
				MID = UMaterialInstanceDynamic::Create(Base, this);
				if (!MID) continue;
				Mesh->SetMaterial(i, MID);
			}

			// Detectar si es tejado o fachada por índice
			if (i == 0)
			{
				// Material principal = fachada
				MID->SetVectorParameterValue(FName("BaseColor"), FachadaColor);
			}
			else if (i == 1)
			{
				// Segundo material = tejado
				MID->SetVectorParameterValue(FName("BaseColor"), TejadoColor);
			}
		}
	}
}
