#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AlsasuaBillboardImpostorComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaBillboardImpostorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaBillboardImpostorComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	float BillboardEnableDistance = 15000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	float BillboardDisableDistance = 12000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	UTexture2D* ImpostorTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	float BillboardWidth = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	float BillboardHeight = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	bool bCastShadows = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impostor")
	FLinearColor TintColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category="Impostor")
	bool bIsShowingBillboard = false;

	UFUNCTION(BlueprintCallable, Category="Impostor")
	void SetImpostorTexture(UTexture2D* Texture);

	UFUNCTION(BlueprintCallable, Category="Impostor")
	void SetBillboardSize(float Width, float Height);

	UFUNCTION(BlueprintPure, Category="Impostor")
	bool IsBillboardActive() const { return bIsShowingBillboard; }

private:
	UPROPERTY() UBillboardComponent* Billboard = nullptr;
	UPROPERTY() UStaticMeshComponent* OriginalMesh = nullptr;
	void ShowBillboard();
	void HideBillboard();
	void FindOriginalMesh();
	float GetDistanceToCamera() const;
};
