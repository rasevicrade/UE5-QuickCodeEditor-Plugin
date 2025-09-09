// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once
#include "Framework/Text/SlateTextLayout.h"
#include "Settings/UQCE_EditorSettings.h"

/**
 * Custom text layout class that extends FSlateTextLayout to provide word highlighting functionality.
 * This class manages the rendering and highlighting of text in the code editor.
 */
class FQCE_TextLayout : public FSlateTextLayout
{
public:
	/** Creates and initializes a new text layout instance. */
	static TSharedRef<FQCE_TextLayout> Create(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle)
	{
		TSharedRef<FQCE_TextLayout> Layout = MakeShareable(new FQCE_TextLayout(InOwner, MoveTemp(InDefaultTextStyle)));
		Layout->AggregateChildren();
		return Layout;
	}

	/** Handles the painting of text and word highlights.*/
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	
	/** 
	 * Add word background highlighting for a specific word.
	 * Uses the WordHighlightColor from editor settings.
	 */
	void HighlightWord(const FString& Word);

	/**  Highlights a specific occurrence of a word at the given position.*/
	void HighlightSpecificOccurrence(const FString& Word, int32 LineIndex, int32 StartOffset, int32 EndOffset);

	/** Clears all active word highlights. */
	void ClearHighlights();

protected:
	/** Protected constructor to ensure instances are created through the Create function. */
	FQCE_TextLayout(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle)
		: FSlateTextLayout(InOwner, MoveTemp(InDefaultTextStyle))
	{
	}

private:
	/** Store active word highlights */
	TArray<FTextLineHighlight> WordHighlights;
};
