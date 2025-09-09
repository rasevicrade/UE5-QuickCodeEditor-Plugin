// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/SyntaxHighlight/QCE_WordBackgroundHighlighter.h"

#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"

TSharedRef<FQCE_WordBackgroundHighlighter> FQCE_WordBackgroundHighlighter::Create(const FLinearColor& InBackgroundColor)
{
	return MakeShareable(new FQCE_WordBackgroundHighlighter(InBackgroundColor));
}

FQCE_WordBackgroundHighlighter::FQCE_WordBackgroundHighlighter(const FLinearColor& InBackgroundColor)
	: BackgroundColor(InBackgroundColor)
{
}

int32 FQCE_WordBackgroundHighlighter::OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Location(Line.Offset.X + OffsetX, Line.Offset.Y);
	const FVector2D Size(Width, Line.TextHeight);

	// The block size and offset values are pre-scaled, so we need to account for that when converting the block offsets into paint geometry
	const float InverseScale = Inverse(AllottedGeometry.Scale);

	if (Width > 0.0f)
	{
		// Draw the background rectangle
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++LayerId,
			AllottedGeometry.ToPaintGeometry(
				TransformVector(InverseScale, Size), 
				FSlateLayoutTransform(TransformPoint(InverseScale, Location))
			),
			&DefaultStyle.HighlightShape,
			bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
			BackgroundColor
		);
	}

	return LayerId;
}

void FQCE_WordBackgroundHighlighter::SetBackgroundColor(const FLinearColor& InBackgroundColor)
{
	BackgroundColor = InBackgroundColor;
}