// Copyright TechnicallyArtist 2025 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/Text/SMultiLineEditableText.h"

class SQCE_MultiLineEditableTextBox;
class QCE_IndentationManager
{
public:
	// Indent selected lines by adding spaces
	static void IndentLine(SQCE_MultiLineEditableTextBox* TextBox);
	
	// Remove indentation from selected lines
	static void UnindentLine(SQCE_MultiLineEditableTextBox* TextBox);
	
	// Get the current line's indentation string
	static bool GetLineIndentation(const SQCE_MultiLineEditableTextBox* TextBox, FString& OutIndentation);
	
	// Process multi-line completion text with proper indentation
	static FString ProcessCompletionTextIndentation(const SQCE_MultiLineEditableTextBox* TextBox, const FString& CompletionText);
	
	// Determine indentation for new lines when Enter is pressed
	static FString GetEnterKeyIndentation(const SQCE_MultiLineEditableTextBox* TextBox);
	
	// Move cursor to first non-whitespace character on current line
	static void MoveCursorToFirstNonWhitespace(SQCE_MultiLineEditableTextBox* TextBox);
	
	// Handle smart backspace for indentation
	static bool HandleSmartBackspace(SQCE_MultiLineEditableTextBox* TextBox);

private:
	// Helper function to get single indent string based on settings (tab or N spaces)
	static FString GetSingleIndentString();
	
	// Helper function to get indent string for multiple levels
	static FString GetIndentString(int32 IndentLevels = 1);
};
