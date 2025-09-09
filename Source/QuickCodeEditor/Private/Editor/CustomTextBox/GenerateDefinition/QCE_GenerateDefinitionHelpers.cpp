// Copyright TechnicallyArtist 2025 All Rights Reserved.

#include "Editor/CustomTextBox/GenerateDefinition/QCE_GenerateDefinitionHelpers.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBox.h"
#include "Editor/CustomTextBox/QCE_MultiLineEditableTextBoxWrapper.h"
#include "Editor/CustomTextBox/GenerateDefinition/QCE_ImplementationLocationFinder.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"
#include "Editor/MainEditorContainer.h"
#include "QuickCodeEditor.h"
#include "Internationalization/Regex.h"

bool QCE_GenerateDefinitionHelpers::HasDeclarationAtCursor(const TSharedPtr<SQCE_MultiLineEditableTextBox>& TextBox, FString& OutDeclarationString)
{
	if (!TextBox.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("TextBox is not valid"));
		return false;
	}

	const FString FileContent = TextBox->GetText().ToString();
	if (FileContent.IsEmpty())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("File content is empty"));
		return false;
	}

	const FTextLocation CursorLocation = TextBox->GetLastCursorLocation();
	
	// Convert FTextLocation to linear position in file content
	int32 CursorPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(FileContent, CursorLocation);
	if (CursorPosition == INDEX_NONE)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Invalid cursor position: Line %d, Offset %d"), CursorLocation.GetLineIndex(), CursorLocation.GetOffset());
		return false;
	}

	// Check if cursor is on a function name and extract the function
	return GetDeclarationStringAtCursor(FileContent, CursorPosition, OutDeclarationString);
}

bool QCE_GenerateDefinitionHelpers::TryGenerateAndInsertDefinition(UMainEditorContainer* EditorContainer, FTextLocation& OutInsertLocation)
{
	FString ImplementationText;
	if (!GetImplementationText(EditorContainer, ImplementationText))
		return false;

	FString HeaderContent = EditorContainer->GetDeclarationTextBoxWrapper()->GetTextBox()->GetText().ToString();
	FTextLocation DeclarationCursorLocation = EditorContainer->GetDeclarationTextBoxWrapper()->GetLastCursorLocation();
	FTextLocation InsertLocation;
	if (!GetInsertLocation(
		EditorContainer->IsLoadIsolated(),
		HeaderContent,
		DeclarationCursorLocation,
		ImplementationText,
		InsertLocation))
		return false;

	FString FunctionDefinition;
	FString FunctionName = QCE_CommonIOHelpers::GetWordAtPosition(HeaderContent, QCE_CommonIOHelpers::ConvertTextLocationToPosition(HeaderContent, DeclarationCursorLocation));
	if (FunctionName.IsEmpty())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not extract function name at cursor position"));
		return false;
	}
	if (!GenerateDefinition(
		FunctionName,
		HeaderContent,
		QCE_CommonIOHelpers::ConvertTextLocationToPosition(HeaderContent, DeclarationCursorLocation),
		FunctionDefinition))
	{
		return false;
	}

	// Insert the generated function definition at the calculated location
	TSharedPtr<SQCE_MultiLineEditableTextBox> ImplementationTextBox = EditorContainer->GetImplementationTextBoxWrapper()->GetTextBox();
	if (!InsertDefinition(FunctionDefinition, InsertLocation, ImplementationTextBox))
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to insert function definition"));
		return false;
	}
	
	// Output the insert location for UI navigation
	OutInsertLocation = InsertLocation;
	
	return true;
}

bool QCE_GenerateDefinitionHelpers::GenerateDefinition(const FString& FunctionName, const FString& HeaderContent, const int32 DeclarationCursorLocation, FString& OutFunctionDefinition)
{
	if (FunctionName.IsEmpty() || HeaderContent.IsEmpty() || DeclarationCursorLocation < 0)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Invalid input parameters for GenerateDefinition"));
		return false;
	}

	FFunctionDeclarationInfo DeclarationInfo;
	if (!QCE_CommonIOHelpers::ParseFunctionDeclarationAtPosition(HeaderContent, DeclarationCursorLocation, FunctionName, DeclarationInfo, false))
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to parse function declaration for '%s' at position %d"), *FunctionName, DeclarationCursorLocation);
		return false;
	}

	FString ClassName = DeclarationInfo.ClassName;
	if (ClassName.IsEmpty())
	{
		ClassName = QCE_CommonIOHelpers::ExtractClassNameFromDeclarationFile(HeaderContent);
	}

	OutFunctionDefinition += TEXT("\n");
	
	// Try to get return type from DeclarationInfo first, then fallback to manual extraction
	FString ReturnType = DeclarationInfo.ReturnType;
	if (ReturnType.IsEmpty())
	{
		ReturnType = QCE_CommonIOHelpers::ExtractReturnType(HeaderContent, DeclarationCursorLocation);
	}
	
	if (!ReturnType.IsEmpty())
	{
		ReturnType = ReturnType.Replace(TEXT("static "), TEXT(""));
		OutFunctionDefinition += ReturnType;
		OutFunctionDefinition += TEXT(" ");
	}

	if (!ClassName.IsEmpty())
	{
		OutFunctionDefinition += ClassName;
		OutFunctionDefinition += TEXT("::");
	}

	OutFunctionDefinition += DeclarationInfo.FunctionName;

	OutFunctionDefinition += TEXT("(");
	for (int32 i = 0; i < DeclarationInfo.Parameters.Num(); ++i)
	{
		OutFunctionDefinition += QCE_ParameterMatcher::NormalizeParameter(DeclarationInfo.Parameters[i], true, false);
		if (i < DeclarationInfo.Parameters.Num() - 1)
		{
			OutFunctionDefinition += TEXT(", ");
		}
	}
	OutFunctionDefinition += TEXT(")");

	if (DeclarationInfo.bIsConst)
	{
		OutFunctionDefinition += TEXT(" const");
	}

	OutFunctionDefinition += TEXT("\n{\n}\n");
	return true;
}

bool QCE_GenerateDefinitionHelpers::GetImplementationText(UMainEditorContainer* EditorContainer, FString& OutImplementationText)
{
	TSharedPtr<QCE_MultiLineEditableTextBoxWrapper> ImplementationTextBoxWrapper = EditorContainer->GetImplementationTextBoxWrapper();
	if (!ImplementationTextBoxWrapper.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Implementation text box wrapper is not valid"));
		return false;
	}

	TSharedPtr<SQCE_MultiLineEditableTextBox> ImplementationTextBox = ImplementationTextBoxWrapper->GetTextBox();
	if (!ImplementationTextBox.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Implementation text box is not valid"));
		return false;
	}

	OutImplementationText = ImplementationTextBox->GetText().ToString();
	return true;
}

bool QCE_GenerateDefinitionHelpers::GetInsertLocation(
	bool bIsIsolated,
	const FString& DeclarationContent,
	FTextLocation InDeclarationCursorLocation,
	const FString& ImplementationContent,
	FTextLocation& OutImplementationInsertLocation)
{
	if (ImplementationContent.IsEmpty())
	{
		OutImplementationInsertLocation = FTextLocation(0, 0);
		return true;
	}
	if (bIsIsolated)
	{
		return false;
	}

	// Otherwise, try to find the appropriate position based on surrounding functions
	FFunctionDeclarationInfo FunctionDeclarationBeforeCursor, FunctionDeclarationAfterCursor;
	bool bHasFunctionBefore = GetFunctionBeforePosition(DeclarationContent, InDeclarationCursorLocation, FunctionDeclarationBeforeCursor);
	bool bHasFunctionAfter = GetFunctionAfterPosition(DeclarationContent, InDeclarationCursorLocation, FunctionDeclarationAfterCursor);

	if (bHasFunctionBefore || bHasFunctionAfter)
	{
		int32 BeforeFunctionImplementationPosition = INDEX_NONE;
		int32 AfterFunctionImplementationPosition = INDEX_NONE;

		// Find positions of surrounding functions in implementation content
		if (bHasFunctionBefore)
		{
			QCE_CommonIOHelpers::FindImplementationPositionInContent(ImplementationContent, FunctionDeclarationBeforeCursor, TEXT(""), BeforeFunctionImplementationPosition);
		}
		if (bHasFunctionAfter)
		{
			QCE_CommonIOHelpers::FindImplementationPositionInContent(ImplementationContent, FunctionDeclarationAfterCursor, TEXT(""), AfterFunctionImplementationPosition);
		}

		// If we have a function after the cursor, insert above it (skipping over preceding comments)
		if (AfterFunctionImplementationPosition != INDEX_NONE)
		{
			// Find the proper insertion point by skipping over any preceding comment lines
			int32 InsertionPoint = FindInsertionPointSkippingComments(ImplementationContent, AfterFunctionImplementationPosition);
			
			OutImplementationInsertLocation = QCE_CommonIOHelpers::ConvertPositionToTextLocation(ImplementationContent, InsertionPoint);
			return true;
		}
		// Else if we have a function before the cursor, find its end and insert below it
		else if (BeforeFunctionImplementationPosition != INDEX_NONE)
		{
			// Find the opening brace of the function
			int32 BracePos = INDEX_NONE;
			if (QCE_ParameterMatcher::FindCharacterRespectingContext(ImplementationContent, TEXT("{"), BeforeFunctionImplementationPosition, ESearchDir::FromStart, BracePos))
			{
				// Find the matching closing brace
				int32 CloseBracePos = INDEX_NONE;
				if (QCE_ParameterMatcher::FindMatchingBracket(ImplementationContent, BracePos, TEXT('{'), TEXT('}'), CloseBracePos, true))
				{
					// Find the next line after the closing brace
					int32 InsertPos = CloseBracePos + 1;
					// Skip to next line if not already on one
					while (InsertPos < ImplementationContent.Len() && ImplementationContent[InsertPos] != '\n')
					{
						InsertPos++;
					}
					if (InsertPos < ImplementationContent.Len())
					{
						InsertPos++; // Skip the newline character
					}
					
					OutImplementationInsertLocation = QCE_CommonIOHelpers::ConvertPositionToTextLocation(ImplementationContent, InsertPos);
					return true;
				}
				else
				{
					UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not find matching closing brace for function at position %d"), BracePos);
				}
			}
			else
			{
				UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not find opening brace for function at position %d"), BeforeFunctionImplementationPosition);
			}
		}
	}

	OutImplementationInsertLocation = QCE_CommonIOHelpers::ConvertPositionToTextLocation(ImplementationContent, ImplementationContent.Len());
	return true;
}

bool QCE_GenerateDefinitionHelpers::GenerateDefinition(const FString& InDeclarationString, const FString& ClassName, FString& OutDefinition)
{
	// If there is a classname, create a scoped definition, otherwise return definition for given declaration
	return false;
}

bool QCE_GenerateDefinitionHelpers::InsertDefinition(const FString& FunctionDefinition, const FTextLocation& InsertLocation, const TSharedPtr<SQCE_MultiLineEditableTextBox>& ImplementationTextBox)
{
	if (!ImplementationTextBox.IsValid())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Implementation text box is not valid"));
		return false;
	}

	if (FunctionDefinition.IsEmpty())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Function definition is empty"));
		return false;
	}

	// Move cursor to the insert location
	ImplementationTextBox->GoTo(InsertLocation);

	// Insert the function definition at the cursor position
	ImplementationTextBox->InsertTextAtCursor(FText::FromString(FunctionDefinition));

	return true;
}

#pragma region Get declaration string (HasDeclarationAtCursor)

bool QCE_GenerateDefinitionHelpers::GetDeclarationStringAtCursor(const FString& FileContent, int32 CursorPosition, FString& OutFunctionDeclaration)
{
	// Find the word at cursor position to check if it's a function name
	FString WordAtCursor = QCE_CommonIOHelpers::GetWordAtPosition(FileContent, CursorPosition);
	if (WordAtCursor.IsEmpty() || !IsValidFunctionName(WordAtCursor))
	{
		return false;
	}


	// Find all positions where this function name appears
	TArray<int32> PossibleMatchPositions;
	if (!QCE_CommonIOHelpers::FilterPositionsByName(FileContent, WordAtCursor, PossibleMatchPositions))
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Function name '%s' not found in file content"), *WordAtCursor);
		return false;
	}

	// Filter to find the match closest to our cursor position
	int32 ClosestMatchPosition = FindClosestPosition(PossibleMatchPositions, CursorPosition);
	if (ClosestMatchPosition == INDEX_NONE)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("No close match found for function name '%s'"), *WordAtCursor);
		return false;
	}

	// Filter out commented positions
	TArray<int32> NonCommentMatches;
	if (!QCE_CommonIOHelpers::FilterCommentedPositions(FileContent, {ClosestMatchPosition}, NonCommentMatches) || NonCommentMatches.Num() == 0)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Function name '%s' at closest position is in a comment"), *WordAtCursor);
		return false;
	}

	// Check if this looks like a function (has UFUNCTION macro or followed by parentheses)
	if (!IsPositionLikelyFunction(FileContent, ClosestMatchPosition))
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Position %d does not appear to be a function for '%s'"), ClosestMatchPosition, *WordAtCursor);
		return false;
	}

	// Extract the complete function declaration
	return ExtractCompleteFunctionDeclaration(FileContent, ClosestMatchPosition, WordAtCursor, OutFunctionDeclaration);
}


bool QCE_GenerateDefinitionHelpers::IsValidFunctionName(const FString& Word)
{
	if (Word.IsEmpty())
	{
		return false;
	}

	// Function names must start with letter or underscore
	if (!FChar::IsAlpha(Word[0]) && Word[0] != '_')
	{
		return false;
	}

	// TODO Replace this with keywords in @FCppSyntaxTokenizer
	const TArray<FString> Keywords = {
		TEXT("if"), TEXT("else"), TEXT("for"), TEXT("while"), TEXT("do"), TEXT("switch"), TEXT("case"), TEXT("default"),
		TEXT("break"), TEXT("continue"), TEXT("return"), TEXT("goto"), TEXT("try"), TEXT("catch"), TEXT("throw"),
		TEXT("class"), TEXT("struct"), TEXT("enum"), TEXT("union"), TEXT("namespace"), TEXT("template"), TEXT("typedef"),
		TEXT("using"), TEXT("static"), TEXT("extern"), TEXT("const"), TEXT("volatile"), TEXT("mutable"), TEXT("inline"),
		TEXT("virtual"), TEXT("override"), TEXT("final"), TEXT("public"), TEXT("private"), TEXT("protected"),
		TEXT("int"), TEXT("float"), TEXT("double"), TEXT("char"), TEXT("bool"), TEXT("void"), TEXT("auto"),
		TEXT("int32"), TEXT("uint32"), TEXT("int64"), TEXT("uint64"), TEXT("FString"), TEXT("bool"), TEXT("TCHAR")
	};

	return !Keywords.Contains(Word);
}

int32 QCE_GenerateDefinitionHelpers::FindClosestPosition(const TArray<int32>& Positions, int32 TargetPosition)
{
	if (Positions.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 ClosestPosition = Positions[0];
	int32 ClosestDistance = FMath::Abs(ClosestPosition - TargetPosition);

	for (int32 Position : Positions)
	{
		int32 Distance = FMath::Abs(Position - TargetPosition);
		if (Distance < ClosestDistance)
		{
			ClosestPosition = Position;
			ClosestDistance = Distance;
		}
	}

	return ClosestPosition;
}

bool QCE_GenerateDefinitionHelpers::IsPositionLikelyFunction(const FString& FileContent, int32 Position)
{
	// Check if there's a UFUNCTION macro above this position
	FString UFunctionString;
	int32 MacroStartPos;
	if (QCE_CommonIOHelpers::FunctionHasUFunction(FileContent, Position, UFunctionString, MacroStartPos))
	{
		return true;
	}

	// Look for opening parenthesis after the function name
	int32 FunctionNameEnd = Position;
	const FString WordAtPos = QCE_CommonIOHelpers::GetWordAtPosition(FileContent, Position);
	if (!WordAtPos.IsEmpty())
	{
		FunctionNameEnd = Position + WordAtPos.Len();
	}

	// Skip whitespace after function name
	while (FunctionNameEnd < FileContent.Len() && FChar::IsWhitespace(FileContent[FunctionNameEnd]))
	{
		FunctionNameEnd++;
	}

	// Check if followed by opening parenthesis
	if (FunctionNameEnd < FileContent.Len() && FileContent[FunctionNameEnd] == '(')
	{
		return true;
	}

	return false;
}

bool QCE_GenerateDefinitionHelpers::ExtractCompleteFunctionDeclaration(const FString& FileContent, int32 FunctionNamePosition, const FString& FunctionName, FString& OutFunctionDeclaration)
{
	// Find declaration start by going backwards from function name to beginning of line
	int32 DeclarationStart = FunctionNamePosition;
	while (DeclarationStart > 0 && FileContent[DeclarationStart - 1] != '\n')
	{
		DeclarationStart--;
	}

	// Find the end of function declaration
	int32 SemicolonPos = INDEX_NONE;
	int32 OpenBracePos = INDEX_NONE;

	if (!QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT(";"), FunctionNamePosition, ESearchDir::FromStart, SemicolonPos))
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not find semicolon for function '%s'"), *FunctionName);
		return false;
	}
	
	QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT("{"), FunctionNamePosition, ESearchDir::FromStart, OpenBracePos);

	// Check if we have an inline implementation (opening brace before semicolon)
	bool bHasInlineImplementation = OpenBracePos != INDEX_NONE && OpenBracePos < SemicolonPos;

	if (bHasInlineImplementation)
		return false;
	

	OutFunctionDeclaration = FileContent.Mid(DeclarationStart, SemicolonPos - DeclarationStart + 1);
	OutFunctionDeclaration = OutFunctionDeclaration.TrimStartAndEnd();
	return true;
}

#pragma endregion

#pragma region Find insert position in implementation



bool QCE_GenerateDefinitionHelpers::GetFunctionBeforePosition(const FString& DeclarationContent, FTextLocation InDeclarationCursorLocation, FFunctionDeclarationInfo& OutFunctionInfo)
{
    // Input validation
    if (DeclarationContent.IsEmpty())
    {
       UE_LOG(LogQuickCodeEditor, Warning, TEXT("FileContent is empty"));
       return false;
    }

    // Convert FTextLocation to linear position
    int32 SearchPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(DeclarationContent, InDeclarationCursorLocation);
    if (SearchPosition == INDEX_NONE)
    {
       UE_LOG(LogQuickCodeEditor, Warning, TEXT("Invalid position: Line %d, Offset %d"), InDeclarationCursorLocation.GetLineIndex(), InDeclarationCursorLocation.GetOffset());
       return false;
    }

    // Create regex pattern to match function declarations: FunctionName(...);
    const FString FunctionPattern = TEXT("(?:UFUNCTION\\s*\\([^)]*\\)\\s*)?(?:(?:virtual|static|inline|explicit|const|mutable)\\s+)*(?:[A-Za-z_][\\w:]*(?:\\s*[&*])*\\s+)+([A-Za-z_]\\w*)\\s*\\(([^)]*)\\)\\s*(?:const\\s*)?(?:override\\s*)?(?:final\\s*)?(?:\\s*=\\s*(?:0|delete|default))?\\s*;");
    FRegexPattern FunctionRegex(FunctionPattern);
    FRegexMatcher FunctionMatcher(FunctionRegex, *DeclarationContent.Mid(0, SearchPosition));

    // Find all function declarations before the position
    FFunctionDeclarationInfo LastValidFunction;

    while (FunctionMatcher.FindNext())
    {
       int32 MatchStart = FunctionMatcher.GetMatchBeginning();
       int32 MatchEnd = FunctionMatcher.GetMatchEnding();
       
       // Only consider matches that are before our search position
       if (MatchEnd <= SearchPosition)
       {
          // Extract the function name (first capture group)
          FString FoundFunctionName = FunctionMatcher.GetCaptureGroup(1);
          
          // Filter out common C++ keywords that aren't function names
          if (IsValidFunctionName(FoundFunctionName))
          {
             // Use the robust helper method to parse the complete declaration
             FFunctionDeclarationInfo FunctionInfo;
             if (QCE_CommonIOHelpers::ParseFunctionDeclarationAtPosition(DeclarationContent, MatchStart, FoundFunctionName, FunctionInfo))
             {
                 // Keep track of the last (closest to search position) valid function
                 LastValidFunction = FunctionInfo;
             }
             else
             {
                 UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to parse function declaration for '%s' at position %d"), *FoundFunctionName, MatchStart);
             }
          }
       }
    }

    // Return the last valid function declaration found before the position
    if (!LastValidFunction.FunctionName.IsEmpty())
    {
       OutFunctionInfo = LastValidFunction;
       return true;
    }

    return false;
}

bool QCE_GenerateDefinitionHelpers::GetFunctionAfterPosition(const FString& DeclarationContent, FTextLocation Position, FFunctionDeclarationInfo& OutFunctionInfo)
{
	// Input validation
	if (DeclarationContent.IsEmpty())
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("FileContent is empty"));
		return false;
	}

	// Convert FTextLocation to linear position
	int32 SearchPosition = QCE_CommonIOHelpers::ConvertTextLocationToPosition(DeclarationContent, Position);
	if (SearchPosition == INDEX_NONE)
	{
		UE_LOG(LogQuickCodeEditor, Warning, TEXT("Invalid position: Line %d, Offset %d"), Position.GetLineIndex(), Position.GetOffset());
		return false;
	}

	// Create regex pattern to match function declarations: FunctionName(...);
	const FString FunctionPattern = TEXT("(?:UFUNCTION\\s*\\([^)]*\\)\\s*)?(?:(?:virtual|static|inline|explicit|const|mutable)\\s+)*(?:[A-Za-z_][\\w:]*(?:\\s*[&*])*\\s+)+([A-Za-z_]\\w*)\\s*\\(([^)]*)\\)\\s*(?:const\\s*)?(?:override\\s*)?(?:final\\s*)?(?:\\s*=\\s*(?:0|delete|default))?\\s*;");
	FRegexPattern FunctionRegex(FunctionPattern);
	FRegexMatcher FunctionMatcher(FunctionRegex, *DeclarationContent.Mid(SearchPosition));

	// Find the first function declaration after the position
	while (FunctionMatcher.FindNext())
	{
		int32 MatchStart = FunctionMatcher.GetMatchBeginning();
		int32 MatchEnd = FunctionMatcher.GetMatchEnding();
		
		// Extract the function name (first capture group)
		FString FoundFunctionName = FunctionMatcher.GetCaptureGroup(1);
		
		// Filter out common C++ keywords that aren't function names
		if (IsValidFunctionName(FoundFunctionName))
		{
			// Use the robust helper method to parse the complete declaration
			// Adjust position since we searched from SearchPosition offset
			int32 ActualFunctionPosition = MatchStart + SearchPosition;
			FFunctionDeclarationInfo FunctionInfo;
			if (QCE_CommonIOHelpers::ParseFunctionDeclarationAtPosition(DeclarationContent, ActualFunctionPosition, FoundFunctionName, FunctionInfo, false))
			{
				OutFunctionInfo = FunctionInfo;
				return true;
			}
			else
			{
				UE_LOG(LogQuickCodeEditor, Warning, TEXT("Failed to parse function declaration for '%s' at position %d"), *FoundFunctionName, ActualFunctionPosition);
			}
		}
	}

	return false;
}

#pragma endregion






int32 QCE_GenerateDefinitionHelpers::FindInsertionPointSkippingComments(const FString& ImplementationContent, int32 FunctionPosition)
{
	// Find the beginning of the line containing the function
	int32 LineStart = FunctionPosition;
	while (LineStart > 0 && ImplementationContent[LineStart - 1] != '\n')
	{
		LineStart--;
	}

	// Now scan backwards line by line to skip over comment lines
	int32 InsertionPoint = LineStart;
	
	while (InsertionPoint > 0)
	{
		// Move to the start of the previous line
		int32 PrevLineStart = InsertionPoint - 1; // Skip the newline
		while (PrevLineStart > 0 && ImplementationContent[PrevLineStart - 1] != '\n')
		{
			PrevLineStart--;
		}
		
		// Extract the previous line content
		int32 PrevLineEnd = InsertionPoint - 1; // Position of newline
		FString PrevLineContent = ImplementationContent.Mid(PrevLineStart, PrevLineEnd - PrevLineStart);
		
		// Trim whitespace from the start to check line content
		FString TrimmedLine = PrevLineContent.TrimStartAndEnd();
		
		// Check if this line is a comment line
		bool bIsCommentLine = false;
		if (TrimmedLine.StartsWith(TEXT("*")) || 
			TrimmedLine.StartsWith(TEXT("//")) || 
			TrimmedLine.StartsWith(TEXT("/*")) ||
			TrimmedLine.IsEmpty()) // Also skip empty lines between comments
		{
			bIsCommentLine = true;
		}
		
		if (bIsCommentLine)
		{
			// This line is a comment, continue scanning backwards
			InsertionPoint = PrevLineStart;
		}
		else
		{
			// Found a non-comment line, stop here
			break;
		}
	}
	
	return InsertionPoint;
}
