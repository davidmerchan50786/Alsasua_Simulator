#include "AlsasuaMinimapWidget.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "Rendering/DrawElements.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UAlsasuaMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	POIIcons.Empty();
}

void UAlsasuaMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

FVector2D UAlsasuaMinimapWidget::GetPlayerLocation() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->GetPawn()) return FVector2D::ZeroVector;
	const FVector Loc = PC->GetPawn()->GetActorLocation();
	return FVector2D(Loc.X, Loc.Y);
}

float UAlsasuaMinimapWidget::GetPlayerYaw() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->GetPawn()) return 0.f;
	return PC->GetPawn()->GetActorRotation().Yaw;
}

void UAlsasuaMinimapWidget::AddPOI(const FVector& WorldPos, const FLinearColor& Color, const FString& Label, float Radius)
{
	FMinimapIcon Icon;
	Icon.WorldPosition = WorldPos;
	Icon.IconColor = Color;
	Icon.Label = Label;
	Icon.IconRadius = Radius;
	Icon.bVisible = true;
	POIIcons.Add(MoveTemp(Icon));
}

void UAlsasuaMinimapWidget::RemovePOI(const FString& Label)
{
	POIIcons.RemoveAll([&Label](const FMinimapIcon& I) { return I.Label == Label; });
}

void UAlsasuaMinimapWidget::ClearPOIs()
{
	POIIcons.Empty();
}

void UAlsasuaMinimapWidget::SetWaypoint(const FVector& WorldPos)
{
	WaypointLocation = WorldPos;
	bHasWaypoint = true;
}

void UAlsasuaMinimapWidget::ClearWaypoint()
{
	bHasWaypoint = false;
}

void UAlsasuaMinimapWidget::SetMinimapZoom(float NewZoom)
{
	CurrentZoom = FMath::Clamp(NewZoom, 0.25f, 4.f);
}

void UAlsasuaMinimapWidget::ToggleMinimap()
{
	bMinimapVisible = !bMinimapVisible;
	SetVisibility(bMinimapVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

FVector2D UAlsasuaMinimapWidget::WorldToMinimap(const FVector& WorldPos, const FVector2D& Center, float Zoom) const
{
	const FVector2D PlayerLoc = GetPlayerLocation();
	const float Scale = MapSize * 0.5f / (MinimapRadius / CurrentZoom);
	return FVector2D(
		Center.X + (WorldPos.X - PlayerLoc.X) * Scale,
		Center.Y - (WorldPos.Y - PlayerLoc.Y) * Scale
	);
}

FReply UAlsasuaMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D Center(InGeometry.GetLocalSize().X * 0.5f, InGeometry.GetLocalSize().Y * 0.5f);
		const float Scale = MinimapRadius / (MapSize * 0.5f) / CurrentZoom;

		const FVector PlayerLoc = GetOwningPlayer() && GetOwningPlayer()->GetPawn()
			? GetOwningPlayer()->GetPawn()->GetActorLocation() : FVector::ZeroVector;

		const float WorldX = PlayerLoc.X + (LocalPos.X - Center.X) * Scale;
		const float WorldY = PlayerLoc.Y - (LocalPos.Y - Center.Y) * Scale;

		SetWaypoint(FVector(WorldX, WorldY, PlayerLoc.Z));
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

int32 UAlsasuaMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!bMinimapVisible) return LayerId;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center = Size * 0.5f;
	const float HalfMap = MapSize * 0.5f;
	const float Zoom = CurrentZoom;

	const FSlateBrush* WhiteBox = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox"));

	// Background border
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(Center.X - HalfMap - 2.f, Center.Y - HalfMap - 2.f),
			FVector2D(MapSize + 4.f, MapSize + 4.f)),
		WhiteBox, ESlateDrawEffect::None, FLinearColor::Black);

	// Background fill
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(Center.X - HalfMap, Center.Y - HalfMap),
			FVector2D(MapSize, MapSize)),
		WhiteBox, ESlateDrawEffect::None, BackgroundColor);

	// --- Draw roads ---
	APlayerController* PC = GetOwningPlayer();
	const FVector PlayerLoc = (PC && PC->GetPawn()) ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const FVector2D PlayerLoc2D = GetPlayerLocation();

	// roads_unity.json es un array en la raíz. Esto lo leía como objeto con campo
	// "roads": la deserialización fallaba y el minimapa salía sin una sola calle.
	// Los puntos son relativos, así que hay que sumar OX/OZ antes de convertir; sin
	// eso el trazado saldría desplazado 1918 m al oeste y 8570 al sur.
	{
		TArray<TSharedPtr<FJsonValue>> Segs;
		if (JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Segs, { TEXT("roads") }))
		{
			const FLinearColor RoadColor(0.4f, 0.4f, 0.45f, 0.7f);

			for (const auto& SegVal : Segs)
			{
				const TSharedPtr<FJsonObject> Seg = SegVal->AsObject();
				if (!Seg.IsValid()) continue;

				const TArray<TSharedPtr<FJsonValue>>* Pts;
				if (!Seg->TryGetArrayField(TEXT("points"), Pts)) continue;

				TArray<FVector2D> SegLine;
				for (const auto& PtVal : *Pts)
				{
					const TSharedPtr<FJsonObject> Pt = PtVal->AsObject();
					if (!Pt.IsValid()) continue;

					double Rx = 0, Rz = 0;
					Pt->TryGetNumberField(TEXT("x"), Rx);
					Pt->TryGetNumberField(TEXT("z"), Rz);

					const FVector UnrealPt = UAlsasuaGeoData::RelLocalToUE5(FVector(Rx, 0.0, Rz));
					const FVector2D MapPos = WorldToMinimap(UnrealPt, Center, Zoom);

					if (FMath::Abs(MapPos.X - Center.X) <= HalfMap + 10.f &&
						FMath::Abs(MapPos.Y - Center.Y) <= HalfMap + 10.f)
					{
						SegLine.Add(MapPos);
					}
				}

				if (SegLine.Num() >= 2)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(), SegLine,
						ESlateDrawEffect::None, RoadColor, false, 2.f);
				}
			}
		}
	}

	// --- Draw rivers ---
	// Igual que las calles: array en la raíz, y encima el campo que se buscaba
	// ("rivers") no existe en el dataset ni aunque la raíz fuera un objeto. A
	// diferencia de las calles, los cauces vienen en absoluto y no llevan OX/OZ.
	{
		TArray<TSharedPtr<FJsonValue>> Rivers;
		if (JsonDatos::CargarArray(TEXT("Datos/waterways_unity.json"), Rivers,
				{ TEXT("waterways"), TEXT("rivers") }))
		{
			const FLinearColor RiverColor(0.2f, 0.4f, 0.9f, 0.7f);

			for (const auto& RivVal : Rivers)
			{
				const TSharedPtr<FJsonObject> Riv = RivVal->AsObject();
				if (!Riv.IsValid()) continue;

				const TArray<TSharedPtr<FJsonValue>>* Pts;
				if (!Riv->TryGetArrayField(TEXT("pts"), Pts)) continue;

				TArray<FVector2D> RivLine;
				for (int32 i = 0; i + 2 < Pts->Num(); i += 3)
				{
					double Rx = 0, Ry = 0, Rz = 0;
					(*Pts)[i]->TryGetNumber(Rx);
					(*Pts)[i + 1]->TryGetNumber(Ry);
					(*Pts)[i + 2]->TryGetNumber(Rz);

					const FVector UnrealPt = UAlsasuaGeoData::UnityaUnreal(FVector(Rx, 0.0, Rz));
					const FVector2D MapPos = WorldToMinimap(UnrealPt, Center, Zoom);

					if (FMath::Abs(MapPos.X - Center.X) <= HalfMap + 10.f &&
						FMath::Abs(MapPos.Y - Center.Y) <= HalfMap + 10.f)
					{
						RivLine.Add(MapPos);
					}
				}

				if (RivLine.Num() >= 2)
				{
					FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
						AllottedGeometry.ToPaintGeometry(), RivLine,
						ESlateDrawEffect::None, RiverColor, false, 2.5f);
				}
			}
		}
	}

	// --- Draw POIs ---
	for (const FMinimapIcon& Icon : POIIcons)
	{
		if (!Icon.bVisible) continue;

		const FVector2D MapPos = WorldToMinimap(Icon.WorldPosition, Center, Zoom);
		if (FMath::Abs(MapPos.X - Center.X) > HalfMap + 10.f ||
			FMath::Abs(MapPos.Y - Center.Y) > HalfMap + 10.f) continue;

		const float Dist = FVector::Dist2D(Icon.WorldPosition, PlayerLoc);
		if (Dist > MinimapRadius * 1.5f) continue;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(
				MapPos - FVector2D(Icon.IconRadius), FVector2D(Icon.IconRadius * 2.f)),
			WhiteBox, ESlateDrawEffect::None, Icon.IconColor);

		FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 8);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(
				MapPos + FVector2D(Icon.IconRadius + 3.f, -6.f), FVector2D(200.f, 14.f)),
			FText::FromString(Icon.Label),
			FontInfo, ESlateDrawEffect::None, Icon.IconColor);
	}

	// --- Draw waypoint ---
	if (bHasWaypoint)
	{
		const FVector2D WPos = WorldToMinimap(WaypointLocation, Center, Zoom);
		if (FMath::Abs(WPos.X - Center.X) <= HalfMap + 20.f &&
			FMath::Abs(WPos.Y - Center.Y) <= HalfMap + 20.f)
		{
			const float Pulse = 6.f + FMath::Sin(FPlatformTime::Seconds() * 3.f) * 2.f;

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(WPos - FVector2D(Pulse), FVector2D(Pulse * 2.f)),
				WhiteBox, ESlateDrawEffect::None,
				FLinearColor(WaypointColor.R, WaypointColor.G, WaypointColor.B, 0.3f));

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(WPos - FVector2D(3.f), FVector2D(6.f)),
				WhiteBox, ESlateDrawEffect::None, WaypointColor);

			const float Distance = FVector::Dist(PlayerLoc, WaypointLocation);
			const FString DistStr = Distance > 1000.f
				? FString::Printf(TEXT("%.1f km"), Distance / 1000.f)
				: FString::Printf(TEXT("%.0f m"), Distance);

			FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 9);
			FSlateDrawElement::MakeText(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(WPos + FVector2D(8.f, -6.f), FVector2D(120.f, 14.f)),
				FText::FromString(DistStr),
				FontInfo, ESlateDrawEffect::None, WaypointColor);
		}
	}

	// --- Draw player ---
	const float DotSize = 6.f;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Center - FVector2D(DotSize + 2.f), FVector2D((DotSize + 2.f) * 2.f)),
		WhiteBox, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.5f));

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(Center - FVector2D(DotSize), FVector2D(DotSize * 2.f)),
		WhiteBox, ESlateDrawEffect::None, PlayerColor);

	if (bRotateWithPlayer)
	{
		const float Yaw = GetPlayerYaw();
		const float Rad = FMath::DegreesToRadians(Yaw);
		const float ArrowLen = 14.f;

		const FVector2D Tip(Center.X + FMath::Cos(Rad) * ArrowLen,
			Center.Y - FMath::Sin(Rad) * ArrowLen);
		const FVector2D L(Center.X + FMath::Cos(Rad + 2.5f) * ArrowLen * 0.4f,
			Center.Y - FMath::Sin(Rad + 2.5f) * ArrowLen * 0.4f);
		const FVector2D R(Center.X + FMath::Cos(Rad - 2.5f) * ArrowLen * 0.4f,
			Center.Y - FMath::Sin(Rad - 2.5f) * ArrowLen * 0.4f);

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>{ Tip, L, R, Tip },
			ESlateDrawEffect::None, PlayerColor, true, 1.f);
	}

	// --- Compass ---
	if (bShowCompass)
	{
		const float MapTop = Center.Y - HalfMap - CompassHeight - 8.f;
		const float CompassWidth = MapSize * 0.8f;
		const float CompassLeft = Center.X - CompassWidth * 0.5f;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2D(CompassLeft, MapTop),
				FVector2D(CompassWidth, CompassHeight)),
			WhiteBox, ESlateDrawEffect::None, FLinearColor(0.05f, 0.05f, 0.1f, 0.7f));

		const float Yaw = GetPlayerYaw();
		const FString Dirs[] = { TEXT("E"), TEXT("NE"), TEXT("N"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("S"), TEXT("SE") };
		const float Step = CompassWidth / 8.f;

		FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 10);

		for (int32 i = 0; i < 8; ++i)
		{
			const float Offset = ((i * 45.f - Yaw) / 45.f);
			const float Wrapped = FMath::Fmod(Offset + 12.f, 8.f) - 4.f;
			if (FMath::Abs(Wrapped) > 4.5f) continue;

			const float X = CompassLeft + CompassWidth * 0.5f + Wrapped * Step;
			const bool bN = (Dirs[i] == TEXT("N"));

			FSlateDrawElement::MakeText(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2D(X - 5.f, MapTop + 4.f),
					FVector2D(20.f, CompassHeight - 4.f)),
				FText::FromString(Dirs[i]),
				FontInfo, ESlateDrawEffect::None,
				bN ? FLinearColor(1.f, 0.2f, 0.2f, 1.f) : CompassColor);
		}

		const float ArrowY = MapTop + CompassHeight + 2.f;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>{
				FVector2D(Center.X, ArrowY + NorthArrowSize),
				FVector2D(Center.X - 4.f, ArrowY),
				FVector2D(Center.X + 4.f, ArrowY),
				FVector2D(Center.X, ArrowY + NorthArrowSize)
			},
			ESlateDrawEffect::None, FLinearColor(1.f, 0.2f, 0.2f, 0.8f), true, 1.5f);
	}

	// Circular clip mask
	const FVector2D MaskMin(Center.X - HalfMap, Center.Y - HalfMap);
	const FVector2D MaskMax(Center.X + HalfMap, Center.Y + HalfMap);
	OutDrawElements.PushClip(FSlateClippingZone(FSlateRect(MaskMin, MaskMax)));

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		TArray<FVector2D>{ MaskMin, FVector2D(MaskMax.X, MaskMin.Y),
			MaskMax, FVector2D(MaskMin.X, MaskMax.Y), MaskMin },
		ESlateDrawEffect::None, FLinearColor::White, true, 1.5f);

	OutDrawElements.PopClip();

	return LayerId + 2;
}
