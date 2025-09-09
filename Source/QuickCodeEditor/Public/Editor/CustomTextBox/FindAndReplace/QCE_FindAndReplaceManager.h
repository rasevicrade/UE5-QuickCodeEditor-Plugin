// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SQCE_MultiLineEditableTextBox;

/**
 * QCE_FindAndReplaceManager provides comprehensive find and replace functionality for text boxes.
 * 
 * This utility class handles search operations with support for case sensitivity and whole word matching,
 * enabling precise text manipulation within the QuickCodeEditor's text editing interface.
 */
class QUICKCODEEDITOR_API QCE_FindAndReplaceManager
{
public:
    /** Finds the next occurrence of the specified search term in the text box. */
    static bool FindOccurrence(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, bool bMatchCase = true, bool bWholeWord = true, int32* OutNextOccurrenceLine = nullptr);

    /**  Replaces the next occurrence of the search term with the replacement text. */
    static bool ReplaceOccurrence(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, const FText& ReplaceTerm, bool bMatchCase = true, bool bWholeWord = true);

    /**
     * Replaces all occurrences of the search term with the replacement text
     */
    static void ReplaceOccurrences(SQCE_MultiLineEditableTextBox* TextBox, const FText& FindTerm, const FText& ReplaceTerm, bool bMatchCase, bool bWholeWord);

private:
    /**
     * Checks if the found text match represents a whole word boundary
     */
    static bool IsWholeWordMatch(const FString& TextString, int32 FindPos, int32 FindLength);

    /**
     * Converts an absolute character position to line and column coordinates
     */
    static void ConvertAbsolutePositionToLocation(const FString& TextString, int32 AbsolutePos, int32& OutLineIndex, int32& OutColumnOffset);
};