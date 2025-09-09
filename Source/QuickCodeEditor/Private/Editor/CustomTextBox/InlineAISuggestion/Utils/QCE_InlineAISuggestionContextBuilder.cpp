// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/InlineAISuggestion/Utils/QCE_InlineAISuggestionContextBuilder.h"
#include "Editor/CustomTextBox/GenerateDefinition/QCE_GenerateDefinitionHelpers.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Editor/CustomTextBox/InlineAISuggestion/UI/QCE_InlineAISuggestionBox.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"
#include "Settings/UQCE_EditorSettings.h"
#include "Internationalization/Regex.h"


bool QCE_InlineAISuggestionContextBuilder::GetAIContext(const FString& VisibleCode,
																const FTextLocation& CursorLocation,
																const ETextBoxType& FileType,
																const FUserInputContext& UserContext,
																FString& OutContext)
{
	switch (FileType)
	{
	case ETextBoxType::Declaration: return GetDeclarationContext(VisibleCode, CursorLocation, UserContext, OutContext);
	case ETextBoxType::Implementation: return GetImplementationContext(VisibleCode, CursorLocation, UserContext, OutContext);
	default: return GetGeneralContext(VisibleCode, CursorLocation, UserContext, OutContext);	
	}
}

bool QCE_InlineAISuggestionContextBuilder::GetDeclarationContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	switch (UserContext.ContextType)
	{
	case EQCEDefaultContext::CurrentLineOrFunction:
		return GetCurrentLine(String, TextLocation, UserContext, OutContext);
	case EQCEDefaultContext::NLinesAboveCursor:
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	default:
		return GetCurrentLine(String, TextLocation, UserContext, OutContext);
	}
}

bool QCE_InlineAISuggestionContextBuilder::GetImplementationContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	switch (UserContext.ContextType)
	{
	case EQCEDefaultContext::CurrentFunction:
		return GetCurrentFunction(String, TextLocation, UserContext, OutContext);
	case EQCEDefaultContext::CurrentLineOrFunction:
		return GetFunctionBeforeCursor(String, TextLocation, UserContext, OutContext);
	case EQCEDefaultContext::NLinesAboveCursor:
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	default:
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
}

bool QCE_InlineAISuggestionContextBuilder::GetGeneralContext(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
}

bool QCE_InlineAISuggestionContextBuilder::GetCurrentFunction(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	// Convert cursor location to absolute position
	int32 CursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(String, TextLocation);
	if (CursorPosition == INDEX_NONE || CursorPosition < 0 || CursorPosition > String.Len())
	{
		OutContext = FString();
		return false;
	}

	// Define the regex pattern for function implementation
	FString RegexPattern = TEXT("(?:(?:inline|static|virtual|explicit|constexpr)\\s+)*(\\w+(?:\\s*(?:<[^>]*>)?\\s*::\\s*\\w+)*(?:\\s*<[^>]*>)?(?:\\s*\\*|\\s*&)*)\\s+(\\w+(?:\\s*::\\s*\\w+)+)\\s*\\(\\s*([^)]*)\\s*\\)\\s*(?:const|override|final|noexcept(?:\\([^)]*\\))?|\\w+)*\\s*\\{");
	
	// Search for function implementations using the regex pattern
	FRegexPattern Pattern(RegexPattern);
	FRegexMatcher Matcher(Pattern, String);
	
	int32 BestMatchStart = -1;
	int32 BestMatchOpenBrace = -1;
	int32 BestDistance = MAX_int32;
	
	// Find the function implementation closest to the cursor position
	while (Matcher.FindNext())
	{
		int32 MatchStart = Matcher.GetMatchBeginning();
		int32 MatchEnd = Matcher.GetMatchEnding();
		
		// Find the opening brace position (should be at the end of the match)
		int32 OpenBracePos = MatchEnd - 1;
		while (OpenBracePos >= MatchStart && String[OpenBracePos] != TEXT('{'))
		{
			OpenBracePos--;
		}
		
		if (OpenBracePos >= MatchStart && String[OpenBracePos] == TEXT('{'))
		{
			// Calculate distance from cursor to this function
			int32 Distance = FMath::Abs(CursorPosition - MatchStart);
			
			// Check if cursor is within this function or this is the closest one
			if ((CursorPosition >= MatchStart && CursorPosition <= MatchEnd) || Distance < BestDistance)
			{
				BestDistance = Distance;
				BestMatchStart = MatchStart;
				BestMatchOpenBrace = OpenBracePos;
			}
		}
	}
	
	// If no function found, use fallback method
	if (BestMatchStart == -1 || BestMatchOpenBrace == -1)
	{
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Find the matching closing brace using QCE_ParameterMatcher
	int32 CloseBracePos = -1;
	if (!QCE_ParameterMatcher::FindMatchingBracket(String, BestMatchOpenBrace, TEXT('{'), TEXT('}'), CloseBracePos))
	{
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Check if cursor is within the detected function bounds
	if (CursorPosition < BestMatchStart || CursorPosition > CloseBracePos)
	{
		// Cursor is not within the function, use fallback method
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Extract the function implementation (including the braces)
	int32 FunctionLength = CloseBracePos - BestMatchStart + 1;
	FString FunctionBody = String.Mid(BestMatchStart, FunctionLength);
	
	// Add cursor marker at the relative position within the function
	int32 RelativeCursorPos = CursorPosition - BestMatchStart;
	if (RelativeCursorPos >= 0 && RelativeCursorPos <= FunctionBody.Len())
	{
		FString LeftPart = FunctionBody.Left(RelativeCursorPos);
		FString RightPart = FunctionBody.Mid(RelativeCursorPos);
		OutContext = LeftPart + TEXT("<ins></ins>") + RightPart;
	}
	else
	{
		OutContext = FunctionBody + TEXT("<ins></ins>");
	}
	
	return true;
}

bool QCE_InlineAISuggestionContextBuilder::GetFunctionBeforeCursor(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	// Convert cursor location to absolute position
	int32 CursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(String, TextLocation);
	if (CursorPosition == INDEX_NONE || CursorPosition < 0 || CursorPosition > String.Len())
	{
		OutContext = FString();
		return false;
	}

	// Define the regex pattern for function implementation
	FString RegexPattern = TEXT("(?:(?:inline|static|virtual|explicit|constexpr)\\s+)*(\\w+(?:\\s*(?:<[^>]*>)?\\s*::\\s*\\w+)*(?:\\s*<[^>]*>)?(?:\\s*\\*|\\s*&)*)\\s+(\\w+(?:\\s*::\\s*\\w+)+)\\s*\\(\\s*([^)]*)\\s*\\)\\s*(?:const|override|final|noexcept(?:\\([^)]*\\))?|\\w+)*\\s*\\{");
	
	// Search for function implementations using the regex pattern
	FRegexPattern Pattern(RegexPattern);
	FRegexMatcher Matcher(Pattern, String);
	
	int32 BestMatchStart = -1;
	int32 BestMatchOpenBrace = -1;
	int32 BestDistance = MAX_int32;
	
	// Find the function implementation closest to the cursor position
	while (Matcher.FindNext())
	{
		int32 MatchStart = Matcher.GetMatchBeginning();
		int32 MatchEnd = Matcher.GetMatchEnding();
		
		// Find the opening brace position (should be at the end of the match)
		int32 OpenBracePos = MatchEnd - 1;
		while (OpenBracePos >= MatchStart && String[OpenBracePos] != TEXT('{'))
		{
			OpenBracePos--;
		}
		
		if (OpenBracePos >= MatchStart && String[OpenBracePos] == TEXT('{'))
		{
			// Calculate distance from cursor to this function
			int32 Distance = FMath::Abs(CursorPosition - MatchStart);
			
			// Check if cursor is within this function or this is the closest one
			if ((CursorPosition >= MatchStart && CursorPosition <= MatchEnd) || Distance < BestDistance)
			{
				BestDistance = Distance;
				BestMatchStart = MatchStart;
				BestMatchOpenBrace = OpenBracePos;
			}
		}
	}
	
	// If no function found, use fallback method
	if (BestMatchStart == -1 || BestMatchOpenBrace == -1)
	{
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Find the matching closing brace using QCE_ParameterMatcher
	int32 CloseBracePos = -1;
	if (!QCE_ParameterMatcher::FindMatchingBracket(String, BestMatchOpenBrace, TEXT('{'), TEXT('}'), CloseBracePos))
	{
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Check if cursor is within the detected function bounds
	if (CursorPosition < BestMatchStart || CursorPosition > CloseBracePos)
	{
		// Cursor is not within the function, use fallback method
		return GetLinesAboveCursor(String, TextLocation, UserContext, OutContext);
	}
	
	// Extract only the function from start to cursor position (not the entire function)
	int32 FunctionBeforeCursorLength = CursorPosition - BestMatchStart;
	FString FunctionBeforeCursor = String.Mid(BestMatchStart, FunctionBeforeCursorLength);
	
	// Add cursor marker at the end
	OutContext = FunctionBeforeCursor + TEXT("<ins></ins>");
	
	return true;
}

bool QCE_InlineAISuggestionContextBuilder::GetCurrentLine(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	// Convert cursor location to absolute position
	int32 CursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(String, TextLocation);
	if (CursorPosition == INDEX_NONE || CursorPosition < 0 || CursorPosition > String.Len())
	{
		OutContext = FString();
		return false;
	}
	
	// Find the start of the current line by walking backwards from cursor position
	int32 LineStartPosition = CursorPosition;
	while (LineStartPosition > 0 && String[LineStartPosition - 1] != TEXT('\n'))
	{
		LineStartPosition--;
	}
	
	// Extract the substring from line start to cursor position
	int32 ContextLength = CursorPosition - LineStartPosition;
	if (ContextLength < 0)
	{
		OutContext = FString();
		return false;
	}
	
	OutContext = String.Mid(LineStartPosition, ContextLength);
	OutContext += "<ins></ins>";
	return true;
}

bool QCE_InlineAISuggestionContextBuilder::GetLinesAboveCursor(const FString& String, const FTextLocation& TextLocation, const FUserInputContext& UserContext, FString& OutContext)
{
	int32 CursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(String, TextLocation);
	if (CursorPosition == INDEX_NONE || CursorPosition < 0 || CursorPosition > String.Len())
	{
		OutContext = FString();
		return false;
	}

	int32 MaxLines =  UserContext.NumberOfLines; 
	TArray<int32> LineBreaks;
	LineBreaks.Add(-1);
	
	for (int32 i = 0; i < String.Len(); i++)
	{
		if (String[i] == TEXT('\n'))
		{
			LineBreaks.Add(i);
		}
	}
	LineBreaks.Add(String.Len()); 

	// Find which line the cursor is on
	int32 CursorLineIndex = -1;
	for (int32 i = 0; i < LineBreaks.Num() - 1; i++)
	{
		if (CursorPosition > LineBreaks[i] && CursorPosition <= LineBreaks[i + 1])
		{
			CursorLineIndex = i;
			break;
		}
	}

	if (CursorLineIndex == -1)
	{
		OutContext = FString();
		return false;
	}

	// Collect non-empty/non-whitespace lines starting from cursor line going backwards
	TArray<int32> ValidLineIndices;
	
	for (int32 LineIndex = CursorLineIndex; LineIndex >= 0 && ValidLineIndices.Num() < MaxLines; LineIndex--)
	{
		// Extract the line content
		int32 LineStart = LineBreaks[LineIndex] + 1;
		int32 LineEnd = LineBreaks[LineIndex + 1];
		FString LineContent = String.Mid(LineStart, LineEnd - LineStart);
		
		// Remove the newline character if present
		if (LineContent.EndsWith(TEXT("\n")))
		{
			LineContent = LineContent.Left(LineContent.Len() - 1);
		}
		
		// Check if line is not empty and not just whitespace
		FString TrimmedLine = LineContent.TrimStartAndEnd();
		if (!TrimmedLine.IsEmpty())
		{
			ValidLineIndices.Insert(LineIndex, 0); // Insert at beginning to maintain order
		}
	}

	if (ValidLineIndices.IsEmpty())
	{
		OutContext = FString();
		return false;
	}

	// Build the context from the valid lines
	FString ContextBuilder;
	for (int32 i = 0; i < ValidLineIndices.Num(); i++)
	{
		int32 LineIndex = ValidLineIndices[i];
		int32 LineStart = LineBreaks[LineIndex] + 1;
		int32 LineEnd = LineBreaks[LineIndex + 1];
		FString LineContent = String.Mid(LineStart, LineEnd - LineStart);
		
		// Remove the newline character if present
		if (LineContent.EndsWith(TEXT("\n")))
		{
			LineContent = LineContent.Left(LineContent.Len() - 1);
		}
		
		// Add cursor marker if this is the cursor line
		if (LineIndex == CursorLineIndex)
		{
			int32 RelativeCursorPos = CursorPosition - LineStart;
			if (RelativeCursorPos >= 0 && RelativeCursorPos <= LineContent.Len())
			{
				FString LeftPart = LineContent.Left(RelativeCursorPos);
				FString RightPart = LineContent.Mid(RelativeCursorPos);
				LineContent = LeftPart + TEXT("<ins></ins>") + RightPart;
			}
			else
			{
				LineContent += TEXT("<ins></ins>");
			}
		}
		
		ContextBuilder += LineContent;
		if (i < ValidLineIndices.Num() - 1)
		{
			ContextBuilder += TEXT("\n");
		}
	}

	OutContext = ContextBuilder;
	return true;
}
