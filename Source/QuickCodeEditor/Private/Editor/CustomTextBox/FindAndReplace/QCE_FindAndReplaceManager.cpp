// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/FindAndReplace/QCE_FindAndReplaceManager.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "QuickCodeEditor.h"

bool QCE_FindAndReplaceManager::FindOccurrence(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, bool bMatchCase, bool bWholeWord, int32* OutNextOccurrenceLine)
{
    if (!TextBox)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("FindOccurrence: TextBox is null"));
        return false;
    }

    const FString FindString = FindTerm.ToString();
    if (FindString.IsEmpty())
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("FindOccurrence: Search term is empty"));
        return false;
    }

    const FString TextString = TextBox->GetText().ToString();
    if (TextString.IsEmpty())
    {
        return false;
    }

    FTextLocation CurrentCursorLocation = TextBox->GetLastCursorLocation();
    int32 StartSearchPos = QCE_CommonIOHelpers::ConvertTextLocationToPosition(TextString, CurrentCursorLocation);
    
    if (StartSearchPos == INDEX_NONE)
    {
        StartSearchPos = 0;
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("FindOccurrence: Invalid cursor position, starting from beginning"));
    }

    if (StartSearchPos < TextString.Len())
    {
        StartSearchPos++;
    }

    ESearchCase::Type SearchCase = bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
    int32 FindPos = INDEX_NONE;
    bool bFoundValidOccurrence = false;

    int32 SearchStartPos = StartSearchPos;
    while (SearchStartPos <= TextString.Len() && !bFoundValidOccurrence)
    {
        FindPos = TextString.Find(FindString, SearchCase, ESearchDir::FromStart, SearchStartPos);
        
        if (FindPos == INDEX_NONE)
        {
            break;
        }

        if (!bWholeWord || IsWholeWordMatch(TextString, FindPos, FindString.Len()))
        {
            bFoundValidOccurrence = true;
            break;
        }

        SearchStartPos = FindPos + 1;
    }

    if (!bFoundValidOccurrence && StartSearchPos > 0)
    {
        SearchStartPos = 0;
        while (SearchStartPos < StartSearchPos && !bFoundValidOccurrence)
        {
            FindPos = TextString.Find(FindString, SearchCase, ESearchDir::FromStart, SearchStartPos);
            
            if (FindPos == INDEX_NONE || FindPos >= StartSearchPos)
            {
                break;
            }

            if (!bWholeWord || IsWholeWordMatch(TextString, FindPos, FindString.Len()))
            {
                bFoundValidOccurrence = true;
                break;
            }

            SearchStartPos = FindPos + 1;
        }
    }

    if (!bFoundValidOccurrence)
    {
        return false;
    }

    int32 FindEndPos = FindPos + FindString.Len();

    TextBox->SelectSpecificOccurrence(FindString, FindPos, FindString.Len());
    int32 LineIndex = 0;
    int32 ColumnOffset = 0;
    ConvertAbsolutePositionToLocation(TextString, FindEndPos, LineIndex, ColumnOffset);

    if (OutNextOccurrenceLine)
    {
        *OutNextOccurrenceLine = LineIndex;
    }

    return true;
}

bool QCE_FindAndReplaceManager::ReplaceOccurrence(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, const FText& ReplaceTerm, bool bMatchCase, bool bWholeWord)
{
    if (!TextBox)
    {
        return false;
    }

    const FString TextString = TextBox->GetText().ToString();
    const FString FindString = FindTerm.ToString();
    const FString ReplaceString = ReplaceTerm.ToString();

    if (FindString.IsEmpty() || TextString.IsEmpty())
    {
        return false;
    }

    int32 NextOccurrenceLine = 0;
    if (!FindOccurrence(TextBox, FindTerm, bMatchCase, bWholeWord, &NextOccurrenceLine))
    {
        return false;
    }

    FTextLocation LastCursorLocation = TextBox->GetLastCursorLocation();
    int32 FindEndPos = QCE_CommonIOHelpers::ConvertTextLocationToPosition(TextString, LastCursorLocation);
    if (FindEndPos == INDEX_NONE)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to convert cursor location to absolute position for replacement"));
        return false;
    }

    int32 FindStartPos = FindEndPos - FindString.Len();
    if (FindStartPos < 0 || FindStartPos >= TextString.Len())
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Invalid find start position: %d"), FindStartPos);
        return false;
    }

    TSharedPtr<SMultiLineEditableText> EditableText = TextBox->GetEditableText();
    if (!EditableText.IsValid())
    {
        return false;
    }

    SMultiLineEditableText::FScopedEditableTextTransaction Transaction(EditableText);

    FString NewText = TextString.Left(FindStartPos) + ReplaceString + TextString.Mid(FindEndPos);
    TextBox->SetText(FText::FromString(NewText));

    int32 NewCursorPos = FindStartPos + ReplaceString.Len();

    int32 LineIndex = 0;
    int32 ColumnOffset = 0;
    ConvertAbsolutePositionToLocation(NewText, NewCursorPos, LineIndex, ColumnOffset);
    TextBox->SelectSpecificOccurrence(ReplaceString, FindStartPos, ReplaceString.Len());

    return true;
}

void QCE_FindAndReplaceManager::ReplaceOccurrences(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, const FText& ReplaceTerm, bool bMatchCase, bool bWholeWord)
{
    if (!TextBox)
    {
        return;
    }

    const FString FindString = FindTerm.ToString();
    const FString ReplaceString = ReplaceTerm.ToString();

    if (FindString.IsEmpty())
    {
        return;
    }

    FString TextString = TextBox->GetText().ToString();
    if (TextString.IsEmpty())
    {
        return;
    }

    // Create a scoped transaction for undo/redo support
    TSharedPtr<SMultiLineEditableText> EditableText = TextBox->GetEditableText();
    if (!EditableText.IsValid())
    {
        return;
    }

    SMultiLineEditableText::FScopedEditableTextTransaction Transaction(EditableText);

    // Use more efficient approach: find all occurrences first, then replace in reverse order
    // This avoids position offset issues when doing multiple replacements
    TArray<int32> OccurrencePositions;

    ESearchCase::Type SearchCase = bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
    int32 SearchStartPos = 0;

    while (true)
    {
        int32 FindPos = TextString.Find(FindString, SearchCase, ESearchDir::FromStart, SearchStartPos);
        if (FindPos == INDEX_NONE)
        {
            break;
        }

        // Check whole word boundaries if required
        bool bValidOccurrence = true;
        if (bWholeWord && !IsWholeWordMatch(TextString, FindPos, FindString.Len()))
        {
            bValidOccurrence = false;
        }

        if (bValidOccurrence)
        {
            OccurrencePositions.Add(FindPos);
        }

        SearchStartPos = FindPos + 1;
    }

    if (OccurrencePositions.Num() == 0)
    {
        return;
    }

    FString NewText = TextString;
    for (int32 i = OccurrencePositions.Num() - 1; i >= 0; i--)
    {
        int32 Pos = OccurrencePositions[i];
        NewText = NewText.Left(Pos) + ReplaceString + NewText.Mid(Pos + FindString.Len());
    }

    TextBox->SetText(FText::FromString(NewText));

    if (OccurrencePositions.Num() > 0)
    {
        int32 LastReplacementPos = OccurrencePositions[0]; 
        int32 LengthDifference = ReplaceString.Len() - FindString.Len();
        int32 AdjustedPos = LastReplacementPos;
        for (int32 i = 1; i < OccurrencePositions.Num(); i++)
        {
            if (OccurrencePositions[i] < LastReplacementPos)
            {
                AdjustedPos += LengthDifference;
            }
        }

        int32 LineIndex = 0;
        int32 ColumnOffset = 0;
        ConvertAbsolutePositionToLocation(NewText, AdjustedPos, LineIndex, ColumnOffset);
    }

    TextBox->SelectWordOccurrences(FString());

}

bool QCE_FindAndReplaceManager::IsWholeWordMatch(const FString& TextString, int32 FindPos, int32 FindLength)
{
    if (FindPos > 0)
    {
        TCHAR CharBefore = TextString[FindPos - 1];
        if (FChar::IsAlnum(CharBefore) || CharBefore == TEXT('_'))
        {
            return false;
        }
    }

    const int32 EndPos = FindPos + FindLength;
    if (EndPos < TextString.Len())
    {
        TCHAR CharAfter = TextString[EndPos];
        if (FChar::IsAlnum(CharAfter) || CharAfter == TEXT('_'))
        {
            return false;
        }
    }

    return true;
}

void QCE_FindAndReplaceManager::ConvertAbsolutePositionToLocation(const FString& TextString, int32 AbsolutePos, int32& OutLineIndex, int32& OutColumnOffset)
{
    OutLineIndex = 0;
    int32 LineStartPos = 0;

    for (int32 i = 0; i < AbsolutePos && i < TextString.Len(); i++)
    {
        if (TextString[i] == TEXT('\n'))
        {
            OutLineIndex++;
            LineStartPos = i + 1;
        }
    }

    OutColumnOffset = AbsolutePos - LineStartPos;
}