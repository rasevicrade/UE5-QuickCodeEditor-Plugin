// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/SyntaxHighlight/QCE_TextRun.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

FVector2D FQCE_TextRun::Measure(int32 StartIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const
{
    // Get the base measurement from the parent class
    FVector2D BaseSize = FSlateTextRun::Measure(StartIndex, EndIndex, Scale, TextContext);
    
    const UQCE_EditorSettings* Settings = GetDefault<UQCE_EditorSettings>();
    check(Settings);
    
    // Count tab characters in the measured range
    int32 TabCount = 0;
    const FString& Text2 = *Text;
    for (int32 i = StartIndex; i < EndIndex && i < Text2.Len(); ++i)
    {
        if (Text2[i] == TEXT('\t'))
        {
            TabCount++;
        }
    }
    
    if (TabCount > 0)
    {
        const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const float SpaceWidth = FontMeasure->Measure(TEXT(" "), Style.Font).X;
        BaseSize.X = SpaceWidth * Settings->TabSpaceCount * TabCount;
    }
    
    return BaseSize;
} 