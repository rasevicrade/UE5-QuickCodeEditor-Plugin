// Copyright TechnicallyArtist 2025 All Rights Reserved.


#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextLayout.h"

#include "Editor/CustomTextBox/SyntaxHighlight/QCE_WordBackgroundHighlighter.h"
#include "Settings/UQCE_EditorSettings.h"

int32 FQCE_TextLayout::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                               const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
                               const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	return FSlateTextLayout::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                                 bParentEnabled);
}

void FQCE_TextLayout::HighlightWord(const FString& Word)
{
    // Get highlight color from editor settings
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    const FLinearColor BackgroundColor = Settings ? Settings->WordHighlightColor : FLinearColor(1.0f, 1.0f, 0.0f, 0.3f);

    // Set up search parameters
    ESearchCase::Type SearchCase = ESearchCase::CaseSensitive;
    const int32 SearchTextLength = Word.Len();
    constexpr int32 SearchHighlightZOrder = -9;  // Lower Z-order ensures highlights appear behind text

    // Create a highlighter object with color from settings
    TSharedRef<FQCE_WordBackgroundHighlighter> Highlighter = FQCE_WordBackgroundHighlighter::Create(BackgroundColor);
    
    // Get all text lines in the layout
    const TArray<FLineModel>& Lines = GetLineModels();
    for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
    {
        const FLineModel& Line = Lines[LineIndex];

        // Initialize search positions
        int32 FindBegin = 0;  // Position to start searching from
        int32 CurrentSearchBegin;  // Position where current match is found
        const int32 TextLength = Line.Text->Len();
        
        // Search for all occurrences of the word in current line
        while (FindBegin < TextLength && (CurrentSearchBegin = Line.Text->Find(Word, SearchCase, ESearchDir::FromStart, FindBegin)) != INDEX_NONE)
        {
            // Check if this is a whole word match by verifying boundaries
            bool bIsWordBoundary = true;
            
            // Check character before the word
            if (CurrentSearchBegin > 0 && FChar::IsAlnum((*Line.Text)[CurrentSearchBegin - 1]))
            {
                bIsWordBoundary = false;
            }
            
            // Check character after the word
            const int32 EndPos = CurrentSearchBegin + SearchTextLength;
            if (EndPos < TextLength && FChar::IsAlnum((*Line.Text)[EndPos]))
            {
                bIsWordBoundary = false;
            }

            // Move search position past current match
            FindBegin = CurrentSearchBegin + SearchTextLength;
            
            // Only add highlight if it's a whole word match
            if (bIsWordBoundary)
            {
                WordHighlights.Add(FTextLineHighlight(LineIndex, FTextRange(CurrentSearchBegin, FindBegin), SearchHighlightZOrder, Highlighter));
            }
        }
    }

    // Apply all collected highlights to the text layout
    for (const FTextLineHighlight& LineHighlight : WordHighlights)
    {
        AddLineHighlight(LineHighlight);
    }
}

void FQCE_TextLayout::HighlightSpecificOccurrence(const FString& Word, int32 LineIndex, int32 StartOffset, int32 EndOffset)
{
    // Get highlight color from editor settings
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    const FLinearColor BackgroundColor = Settings ? Settings->WordHighlightColor : FLinearColor(1.0f, 1.0f, 0.0f, 0.3f);

    constexpr int32 SearchHighlightZOrder = -9;  // Lower Z-order ensures highlights appear behind text

    // Create a highlighter object with color from settings
    TSharedRef<FQCE_WordBackgroundHighlighter> Highlighter = FQCE_WordBackgroundHighlighter::Create(BackgroundColor);
    
    // Get all text lines in the layout
    const TArray<FLineModel>& Lines = GetLineModels();
    
    // Validate the line index
    if (!Lines.IsValidIndex(LineIndex))
    {
        return;
    }

    // Add highlight for the specific occurrence
    WordHighlights.Add(FTextLineHighlight(LineIndex, FTextRange(StartOffset, EndOffset), SearchHighlightZOrder, Highlighter));
    
    // Apply the highlight to the text layout
    AddLineHighlight(WordHighlights.Last());
}

void FQCE_TextLayout::ClearHighlights()
{
    const TArray<FTextLayout::FLineModel>& Lines = GetLineModels();

    for (const FTextLineHighlight& LineHighlight : WordHighlights)
    {
        if (Lines.IsValidIndex(LineHighlight.LineIndex))
        {
            RemoveLineHighlight(LineHighlight);
        }
    }

    WordHighlights.Empty();
}