// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/GenerateDefinition/QCE_ImplementationLocationFinder.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "QuickCodeEditor.h"

int32 QCE_ImplementationLocationFinder::FindInsertionLocation(const FString& CppFileContent,
	const FFunctionDeclarationInfo& PreviousFunction, const FFunctionDeclarationInfo& NextFunction)
{
	if (CppFileContent.IsEmpty())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("QCE_ImplementationLocationFinder::FindInsertionLocation CppFileContent is empty"));
		return -1;
	}
	
	// Find implementation positions using FFunctionCppReader
	int32 PreviousFunctionPos = INDEX_NONE;
	int32 NextFunctionPos = INDEX_NONE;

	if (!PreviousFunction.FunctionName.IsEmpty())
	{
		if (!QCE_CommonIOHelpers::FindImplementationPositionInContent(CppFileContent, PreviousFunction, PreviousFunction.ClassName, PreviousFunctionPos))
		{
			PreviousFunctionPos = INDEX_NONE;
		}
	}

	if (!NextFunction.FunctionName.IsEmpty())
	{
		if (!QCE_CommonIOHelpers::FindImplementationPositionInContent(CppFileContent, NextFunction, NextFunction.ClassName, NextFunctionPos))
		{
			NextFunctionPos = INDEX_NONE;
		}
	}

	int32 InsertionPosition;

	// Determine insertion position based on available functions
	if (PreviousFunctionPos != INDEX_NONE && NextFunctionPos != INDEX_NONE)
	{
		// Both functions found - insert between them
		if (PreviousFunctionPos >= NextFunctionPos)
		{
			return -1;
		}
		
		int32 PreviousFunctionEnd = FindFunctionEnd(CppFileContent, PreviousFunctionPos);
		if (PreviousFunctionEnd == INDEX_NONE)
		{
			return -1;
		}
		
		// Insert after the previous function
		InsertionPosition = PreviousFunctionEnd;
		
		// Skip any existing whitespace
		while (InsertionPosition < CppFileContent.Len() && FChar::IsWhitespace(CppFileContent[InsertionPosition]) && CppFileContent[InsertionPosition] != '\n')
		{
			InsertionPosition++;
		}
		
		// Ensure we're at the beginning of a new line
		if (InsertionPosition < CppFileContent.Len() && CppFileContent[InsertionPosition] != '\n')
		{
			// Find the next newline
			while (InsertionPosition < CppFileContent.Len() && CppFileContent[InsertionPosition] != '\n')
			{
				InsertionPosition++;
			}
			if (InsertionPosition < CppFileContent.Len())
			{
				InsertionPosition++; // Move past the newline
			}
		}
		else if (InsertionPosition < CppFileContent.Len() && CppFileContent[InsertionPosition] == '\n')
		{
			InsertionPosition++; // Move past the newline
		}
		
		UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Insertion point found between '%s' and '%s' at position %d"), *PreviousFunction.FunctionName, *NextFunction.FunctionName, InsertionPosition);
	}
	else if (PreviousFunctionPos != INDEX_NONE)
	{
		// Only previous function found - insert after it
		int32 PreviousFunctionEnd = FindFunctionEnd(CppFileContent, PreviousFunctionPos);
		if (PreviousFunctionEnd == INDEX_NONE)
		{
			return -1;
		}
		
		InsertionPosition = PreviousFunctionEnd;
		
		// Move to next line for clean insertion
		while (InsertionPosition < CppFileContent.Len() && CppFileContent[InsertionPosition] != '\n')
		{
			InsertionPosition++;
		}
		if (InsertionPosition < CppFileContent.Len() && CppFileContent[InsertionPosition] == '\n')
		{
			InsertionPosition++;
		}
		
		UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Insertion point found after '%s' at position %d"), *PreviousFunction.FunctionName, InsertionPosition);
	}
	else if (NextFunctionPos != INDEX_NONE)
	{
		// Only next function found - insert before it
		// Find the start of the function header
		int32 HeaderStartPos = INDEX_NONE;
		if (QCE_CommonIOHelpers::FindFunctionImplementationHeaderStart(CppFileContent, NextFunctionPos, HeaderStartPos))
		{
			InsertionPosition = HeaderStartPos;
		}
		else
		{
			InsertionPosition = NextFunctionPos;
		}
		
		// Move to beginning of line
		while (InsertionPosition > 0 && CppFileContent[InsertionPosition - 1] != '\n')
		{
			InsertionPosition--;
		}
		
		UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Insertion point found before '%s' at position %d"), *NextFunction.FunctionName, InsertionPosition);
	}
	else
	{
		// Neither function found - insert at end of file
		InsertionPosition = CppFileContent.Len();
		
		// Ensure we're on a new line
		if (InsertionPosition > 0 && CppFileContent[InsertionPosition - 1] != '\n')
		{
			// We're at the end, but the file doesn't end with a newline
			// The insertion should add a newline before the new content
		}
		
		UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Insertion point set to end of file at position %d"), InsertionPosition);
	}
	
	// Convert to line/character position
	int32 LineNumber, CharacterPosition;
	if (!ConvertToLineCharPosition(CppFileContent, InsertionPosition, LineNumber, CharacterPosition))
	{
		return false;
	}
	
	return InsertionPosition;
}

bool QCE_ImplementationLocationFinder::ConvertToLineCharPosition(const FString& FileContent, int32 CharPosition, int32& OutLineNumber, int32& OutCharacterPosition)
{
	if (CharPosition < 0 || CharPosition > FileContent.Len())
	{
		return false;
	}
	
	OutLineNumber = 0;
	OutCharacterPosition = 0;
	
	for (int32 i = 0; i < CharPosition && i < FileContent.Len(); i++)
	{
		if (FileContent[i] == '\n')
		{
			OutLineNumber++;
			OutCharacterPosition = 0;
		}
		else
		{
			OutCharacterPosition++;
		}
	}
	
	return true;
}

int32 QCE_ImplementationLocationFinder::FindFunctionEnd(const FString& FileContent, int32 FunctionStartPos)
{
	// Find the opening brace
	int32 OpenBracePos = INDEX_NONE;
	if (!QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT("{"), FunctionStartPos, ESearchDir::FromStart, OpenBracePos))
	{
		return INDEX_NONE;
	}
	
	// Find the matching closing brace
	int32 CloseBracePos = INDEX_NONE;
	if (!QCE_ParameterMatcher::FindMatchingBracket(FileContent, OpenBracePos, '{', '}', CloseBracePos))
	{
		return INDEX_NONE;
	}
	
	// Return position after the closing brace
	return CloseBracePos + 1;
}
