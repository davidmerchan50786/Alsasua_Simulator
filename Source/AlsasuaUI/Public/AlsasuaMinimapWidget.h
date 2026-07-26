#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AlsasuaMinimapWidget.generated.h"

USTRUCT(BlueprintType)
struct FMinimapIcon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector WorldPosition = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor IconColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float IconRadius = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bVisible = true;
};

UCLASS()
class ALSASUAUI_API UAlsasuaMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void AddPOI(const FVector& WorldPos, const FLinearColor& Color, const FString& Label, float Radius = 8.f);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void RemovePOI(const FString& Label);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void ClearPOIs();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void SetWaypoint(const FVector& WorldPos);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void ClearWaypoint();

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void SetMinimapZoom(float NewZoom);

	UFUNCTION(BlueprintCallable, Category = "Alsasua|Minimap")
	void ToggleMinimap();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	float MinimapRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	float MapSize = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.05f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	FLinearColor PlayerColor = FLinearColor(0.2f, 1.f, 0.4f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	FLinearColor WaypointColor = FLinearColor(1.f, 0.8f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	FLinearColor CompassColor = FLinearColor(1.f, 1.f, 1.f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	float CompassHeight = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap|Style")
	float NorthArrowSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap")
	bool bShowCompass = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Minimap")
	bool bRotateWithPlayer = true;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	FVector2D WorldToMinimap(const FVector& WorldPos, const FVector2D& Center, float Zoom) const;
	FVector2D GetPlayerLocation() const;
	float GetPlayerYaw() const;

	UPROPERTY()
	TArray<FMinimapIcon> POIIcons;

	FVector WaypointLocation = FVector::ZeroVector;
	bool bHasWaypoint = false;
	float CurrentZoom = 1.f;
	bool bMinimapVisible = true;
};
