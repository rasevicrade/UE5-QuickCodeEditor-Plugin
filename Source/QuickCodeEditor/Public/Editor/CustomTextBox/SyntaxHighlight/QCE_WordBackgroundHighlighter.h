// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once
#include "Framework/Text/ISlateLineHighlighter.h"

/**
 * A word background highlighter that provides background color highlighting for text in Slate text layouts.
 * Implements ISlateLineHighlighter to draw colored backgrounds behind text content.
 */
class FQCE_WordBackgroundHighlighter: public ISlateLineHighlighter
{
public:
	/**
	 * Creates a new word background highlighter instance.
	 * @param InBackgroundColor The color to use for background highlighting
	 * @return Shared reference to the created highlighter
	 */
	static TSharedRef<FQCE_WordBackgroundHighlighter> Create(const FLinearColor& InBackgroundColor);

	/**
	 * Paints the background highlight for a line of text.
	 * Called by the Slate framework during text rendering to draw background elements.
	 * @return Updated layer ID after painting
	 */
	virtual int32 OnPaint(const FPaintArgs& Args, const FTextLayout::FLineView& Line, const float OffsetX, const float Width, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/**
	 * Sets the background color for highlighting.
	 */
	void SetBackgroundColor(const FLinearColor& InBackgroundColor);

protected:
	/**
	 * Protected constructor for creating highlighter instances.
	 * Use the Create static method to instantiate this class.
	 */
	FQCE_WordBackgroundHighlighter(const FLinearColor& InBackgroundColor);

private:
	/** The background color used for highlighting text. Default is purple (0.54, 0.3, 0.83) */
	FLinearColor BackgroundColor =FLinearColor(0.54f, 0.3f, 0.83f);
};
