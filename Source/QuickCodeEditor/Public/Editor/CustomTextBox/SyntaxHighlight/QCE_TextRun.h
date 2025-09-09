// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once
#include "Framework/Text/SlateTextRun.h"
#include "Settings/UQCE_EditorSettings.h"

/**
 * Custom text run class that extends FSlateTextRun to provide custom tab character rendering.
 * 
 * This class is used by the QuickCodeEditor's syntax highlighting system to handle text measurement
 * and rendering for individual text runs within a syntax-highlighted text block. It specifically
 * addresses tab character handling and provides custom measurement logic for accurate text layout.
 */
class FQCE_TextRun : public FSlateTextRun
{
public:
    /** Factory method to create a new FQCE_TextRun instance. */
    static TSharedRef<FQCE_TextRun> Create(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange)
    {
        return MakeShareable(new FQCE_TextRun(InRunInfo, InText, InStyle, InRange));
    }

    /**
     * Measures the size of a portion of this text run.
     * 
     * This override provides custom measurement logic, particularly for handling tab characters
     * and ensuring proper spacing in the code editor's text layout.
     */
    virtual FVector2D Measure(int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const override;

protected:
    /**
     * Protected constructor for FQCE_TextRun.
     * Use the static Create method to instantiate this class.
     */
    FQCE_TextRun(const FRunInfo& InRunInfo, const TSharedRef<const FString>& InText, const FTextBlockStyle& InStyle, const FTextRange& InRange)
        : FSlateTextRun(InRunInfo, InText, InStyle, InRange)
        , TextBlockStyle(InStyle)
    {
    }

private:
    /** Reference to the text block style used for measuring and rendering this text run */
    const FTextBlockStyle& TextBlockStyle;
}; 