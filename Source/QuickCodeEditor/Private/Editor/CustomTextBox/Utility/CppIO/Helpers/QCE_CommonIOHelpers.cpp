// Copyright TechnicallyArtist 2025 All Rights Reserved.
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_CommonIOHelpers.h"

#include "QuickCodeEditor.h"
#include "Editor/CustomTextBox/GenerateDefinition/QCE_GenerateDefinitionHelpers.h"
#include "Editor/CustomTextBox/Utility/CppIO/Helpers/QCE_ParameterMatcher.h"

#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"

bool QCE_CommonIOHelpers::FilterPositionsByName(const FString& FileContent, const FString& SearchString, TArray<int32>& OutPositions)
{
    int32 SearchPos = 0;
    while (true)
    {
        int32 FoundPos = FileContent.Find(SearchString, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchPos);
        if (FoundPos == INDEX_NONE)
            break;

        // Check character before the match (if not at start)
        bool bValidStart = true;
        if (FoundPos > 0)
        {
            TCHAR PrevChar = FileContent[FoundPos - 1];
            bValidStart = FChar::IsWhitespace(PrevChar) || 
                         PrevChar == ':' ||  // For scope operator
                         PrevChar == '\n';
        }

        // Check character after the match (if not at end)
        bool bValidEnd = true;
        int32 EndPos = FoundPos + SearchString.Len();
        if (EndPos < FileContent.Len())
        {
            TCHAR NextChar = FileContent[EndPos];
            bValidEnd = FChar::IsWhitespace(NextChar) || 
                       NextChar == '(' ||  // For function calls
                       NextChar == '\n';
        }

        // Only add position if both boundaries are valid
        if (bValidStart && bValidEnd)
        {
            OutPositions.Add(FoundPos);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found valid function name match at position %d"), FoundPos);
        }

        SearchPos = FoundPos + 1;
    }

    return OutPositions.Num() != 0;
}

bool QCE_CommonIOHelpers::FilterCommentedPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, TArray<int32>& OutNonCommentMatches)
{
    for (int32 MatchPos : PossibleMatchPositions)
    {
        if (!IsPositionInComment(FileContent, MatchPos))
        {
            OutNonCommentMatches.Add(MatchPos);
        }
    }

    return OutNonCommentMatches.Num() != 0;
}

bool QCE_CommonIOHelpers::FilterNativeFunctionPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, TArray<int32>& OutUFunctionMatches)
{
    for (int32 MatchPos : PossibleMatchPositions)
    {
        FString UFunctionString;
        int32 MacroStartPos;
        if (FunctionHasUFunction(FileContent, MatchPos, UFunctionString, MacroStartPos))
        {
            OutUFunctionMatches.Add(MatchPos);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found UFUNCTION macro at position %d"), MatchPos);
        }
    }

    return OutUFunctionMatches.Num() != 0;
}

bool QCE_CommonIOHelpers::FilterScopedFunctionPositions(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const FString& ClassName, TArray<int32>& OutScopedMatches)
{
    // If ClassName is empty, all possible match positions are considered valid
    if (ClassName.IsEmpty())
    {
        OutScopedMatches = PossibleMatchPositions;
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Empty class name provided - all %d positions considered valid scoped matches"), PossibleMatchPositions.Num());
        return OutScopedMatches.Num() != 0;
    }

    for (int32 MatchPos : PossibleMatchPositions)
    {
        // Check if this match has the class scope before it
        int32 ScopeStart = MatchPos - ClassName.Len() - 2; // -2 for "::"
        if (ScopeStart >= 0)
        {
            FString ScopeCheck = FileContent.Mid(ScopeStart, ClassName.Len() + 2);
            if (ScopeCheck == ClassName + TEXT("::"))
            {
                // Verify the scope operator is not inside a string or comment
                if (!QCE_ParameterMatcher::IsPositionInStringOrComment(FileContent, ScopeStart + ClassName.Len()))
                {
                    OutScopedMatches.Add(MatchPos);
                    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found scoped match at position %d for class '%s'"), MatchPos, *ClassName);
                }
            }
        }
    }

    return OutScopedMatches.Num() != 0;
}

bool QCE_CommonIOHelpers::IsPositionInComment(const FString& FileContent, int32 Position)
{
    // Check for single-line comment
    int32 LineStart = Position;
    while (LineStart > 0 && FileContent[LineStart - 1] != '\n')
    {
        LineStart--;
    }

    FString LineUpToPosition = FileContent.Mid(LineStart, Position - LineStart);
    int32 SingleCommentPos = LineUpToPosition.Find(TEXT("//"));
    if (SingleCommentPos != INDEX_NONE)
    {
        return true; // Position is after // on the same line
    }

    // Check for multi-line comment
    int32 BlockCommentStart = FileContent.Find(TEXT("/*"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, Position);
    if (BlockCommentStart != INDEX_NONE)
    {
        int32 BlockCommentEnd = FileContent.Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, BlockCommentStart);
        if (BlockCommentEnd != INDEX_NONE && Position < BlockCommentEnd + 2)
        {
            return true; // Position is within /* */ block
        }
    }

    return false;
}

bool QCE_CommonIOHelpers::FunctionHasUFunction(const FString& FileContent, int32 FunctionNamePos, FString& OutUFunctionString, int32& OutMacroStartPos)
{
    // Find the start of the current line
    int32 LineStart = FunctionNamePos;
    while (LineStart > 0 && FileContent[LineStart - 1] != '\n')
    {
        LineStart--;
    }

    // Search backwards line by line for UFUNCTION macro with boundary detection
    int32 CurrentLineStart = LineStart;
    int32 SearchedLines = 0;
    const int32 MaxSearchLines = 20; // Limit search to prevent excessive backwards searching

    while (CurrentLineStart > 0 && SearchedLines < MaxSearchLines)
    {
        CurrentLineStart--;

        while (CurrentLineStart > 0 && FileContent[CurrentLineStart - 1] != '\n')
        {
            CurrentLineStart--;
        }

        // Get the previous line content
        int32 PrevLineEnd = CurrentLineStart;
        while (PrevLineEnd < FileContent.Len() && FileContent[PrevLineEnd] != '\n')
        {
            PrevLineEnd++;
        }

        FString LineContent = FileContent.Mid(CurrentLineStart, PrevLineEnd - CurrentLineStart);
        FString TrimmedLine = LineContent.TrimStartAndEnd();

        // Skip empty lines and pure comment lines
        if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("//")))
        {
            SearchedLines++;
            continue;
        }

        // Check for statement terminators that indicate function boundaries
        // If we encounter these before UFUNCTION, it means we've crossed into another function's scope
        if (TrimmedLine.EndsWith(TEXT(";")) || TrimmedLine.EndsWith(TEXT("}")))
        {
            // Found a statement terminator before UFUNCTION - this means the UFUNCTION
            // belongs to a previous function, not the one we're checking
            return false;
        }

        // Find the UFUNCTION macro position in this line
        int32 UFunctionPos = LineContent.Find(TEXT("UFUNCTION"));
        if (UFunctionPos == INDEX_NONE)
        {
            SearchedLines++;
            continue;
        }

        int32 UFunctionStartPos = CurrentLineStart + UFunctionPos;

        // Find the opening parenthesis after UFUNCTION
        int32 OpenParenPos = FileContent.Find(TEXT("("), ESearchCase::CaseSensitive, ESearchDir::FromStart, UFunctionStartPos);
        if (OpenParenPos == INDEX_NONE)
        {
            // UFUNCTION without parentheses
            OutUFunctionString = LineContent;
            OutMacroStartPos = CurrentLineStart;
            return true;
        }

        // Find the matching closing parenthesis
        int32 CloseParenPos = INDEX_NONE;
        if (QCE_ParameterMatcher::FindMatchingBracket(FileContent, OpenParenPos, TEXT('('), TEXT(')'), CloseParenPos, true))
        {
            OutUFunctionString = FileContent.Mid(CurrentLineStart, CloseParenPos - CurrentLineStart + 1);
            OutMacroStartPos = CurrentLineStart;
            return true;
        }
        else
        {
            // Fallback to single line if bracket matching fails
            OutUFunctionString = LineContent;
            OutMacroStartPos = CurrentLineStart;
            return true;
        }
    }

    return false;
}

bool QCE_CommonIOHelpers::FindFunctionImplementationHeaderStart(const FString& FileContent, int32 FunctionNamePos, int32& OutHeaderStartPos)
{
    // Start from function name position and search backwards for ::
    int32 CurrentPos = FunctionNamePos;
    while (CurrentPos > 1)  // Need at least 2 chars for ::
    {
        if (FileContent[CurrentPos - 1] == ':' && FileContent[CurrentPos - 2] == ':')
        {
            // Found ::, now go back until whitespace or newline to skip class name
            CurrentPos -= 2;
            while (CurrentPos > 0 && !FChar::IsWhitespace(FileContent[CurrentPos - 1]) && FileContent[CurrentPos - 1] != '\n')
            {
                CurrentPos--;
            }
            
            // Skip any whitespace or newlines
            while (CurrentPos > 0 && (FChar::IsWhitespace(FileContent[CurrentPos - 1]) || FileContent[CurrentPos - 1] == '\n'))
            {
                CurrentPos--;
            }
            
            // Now we're at the end of return type, go back until we find whitespace or newline
            int32 ReturnTypeEnd = CurrentPos;
            while (CurrentPos > 0 && !FChar::IsWhitespace(FileContent[CurrentPos - 1]) && FileContent[CurrentPos - 1] != '\n')
            {
                CurrentPos--;
            }
            
            OutHeaderStartPos = CurrentPos;
            return true;
        }
        CurrentPos--;
    }
    
    return false;  // Couldn't find ::
}

bool QCE_CommonIOHelpers::ValidateFunctionFlags(const UFunction* Function)
{
    if (!Function)
        return false;

    // Check for FUNC_BlueprintCallable and FUNC_Native flags
    const bool bHasBlueprintCallable = Function->HasAnyFunctionFlags(FUNC_BlueprintCallable);
    const bool bHasNative = Function->HasAnyFunctionFlags(FUNC_Native);

    return bHasBlueprintCallable || bHasNative;
}

bool QCE_CommonIOHelpers::ReadFileContent(const FString& FilePath, FString& OutContent)
{
    if (!FFileHelper::LoadFileToString(OutContent, *FilePath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    return true;
}

bool QCE_CommonIOHelpers::WriteFileContent(const FString& FilePath, const FString& Content)
{
    if (!FFileHelper::SaveStringToFile(Content, *FilePath))
    {
        UE_LOG(LogQuickCodeEditor, Error, TEXT("Failed to write file: %s"), *FilePath);
        return false;
    }
    return true;
}

TArray<TPair<FString, bool>> QCE_CommonIOHelpers::GetExpectedParameterSignature(const UFunction* Function)
{
    TArray<TPair<FString, bool>> ExpectedParameterTypes;
    for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
    {
        const FProperty* Property = *PropIt;
        if (Property->HasAnyPropertyFlags(CPF_Parm) && !Property->HasAnyPropertyFlags(CPF_ReturnParm))
        {
            FStringOutputDevice PropertyText;
            FString PropertyName = Property->GetName();
            ExportPropertyCppDeclaration(Property, PropertyText, EQCEExportedDeclaration::Parameter, nullptr, QCECPPF_BlueprintCppBackend, false, nullptr, nullptr, &PropertyName);
            bool bPassByRef = GetPropertyPassCPPArgsByRef(Property);
            ExpectedParameterTypes.Add(TPair<FString, bool>(PropertyText, bPassByRef));
        }
    }
    return ExpectedParameterTypes;
}

bool QCE_CommonIOHelpers::DoesParameterSignatureMatch(const FString& FileContent, int32 Position, const UFunction* Function)
{
    // Extract parameter string from this position
    FString ParameterString;
    if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, Position, ParameterString))
    {
        return false;
    }

    TArray<FString> FoundParams = QCE_ParameterMatcher::ToParameterArray(ParameterString);
    TArray<TPair<FString, bool>> ExpectedParams = GetExpectedParameterSignature(Function);

    // Check parameter count
    if (FoundParams.Num() != ExpectedParams.Num())
    {
        return false;
    }

    // Check parameter types
    for (int32 i = 0; i < ExpectedParams.Num(); ++i)
    {
        FString ExpectedType = QCE_ParameterMatcher::NormalizeParameter(ExpectedParams[i].Key);
        FString FoundType = QCE_ParameterMatcher::NormalizeParameter(FoundParams[i]);

        if (ExpectedType != FoundType)
        {
            return false;
        }
    }

    return true;
}

bool QCE_CommonIOHelpers::FilterPositionsByParameterSignature(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const UFunction* Function, TArray<int32>& OutSignatureMatches)
{
    for (int32 MatchPos : PossibleMatchPositions)
    {
        if (DoesParameterSignatureMatch(FileContent, MatchPos, Function))
        {
            OutSignatureMatches.Add(MatchPos);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter signature match at position %d for function '%s'"), MatchPos, *Function->GetName());
        }
    }

    return OutSignatureMatches.Num() != 0;
}

bool QCE_CommonIOHelpers::ParseFunctionDeclarationAtPosition(const FString& FileContent, int32 FunctionPosition, const FString& FunctionName, FFunctionDeclarationInfo& OutDeclarationInfo, const bool bRequiresUfunction)
{
    // Find the start of UFUNCTION macro above the declaration
    int32 LineStart = FunctionPosition;
    FString UFunctionLine;
    if (bRequiresUfunction && !FunctionHasUFunction(FileContent, FunctionPosition, UFunctionLine, LineStart))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("No UFUNCTION macro found for function '%s' at position %d"), *FunctionName, FunctionPosition);
        return false;
    }

    // Find both the opening brace and semicolon positions while respecting string/comment contexts
    int32 SemicolonPos = INDEX_NONE;
    int32 OpenBracePos = INDEX_NONE;
    if (!QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT(";"), FunctionPosition, ESearchDir::FromStart, SemicolonPos))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("No semicolon found for function '%s' at position %d"), *FunctionName, FunctionPosition);
        return false;
    }
    QCE_ParameterMatcher::FindCharacterRespectingContext(FileContent, TEXT("{"), FunctionPosition, ESearchDir::FromStart, OpenBracePos);

    // Check if we have an inline implementation (opening brace before semicolon)
    bool bHasInlineImplementation = OpenBracePos != INDEX_NONE && OpenBracePos < SemicolonPos;
    int32 DeclarationEndPos = SemicolonPos;

    if (bHasInlineImplementation)
    {
        // Find the matching closing brace
        int32 CloseBracePos = INDEX_NONE;
        if (!QCE_ParameterMatcher::FindMatchingBracket(FileContent, OpenBracePos, TEXT('{'), TEXT('}'), CloseBracePos, true))
        {
            UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not find matching closing brace for inline function '%s'"), *FunctionName);
            return false;
        }

        // Update the end position to include the implementation
        DeclarationEndPos = CloseBracePos;
    }

    FString ParameterString;
    if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, FunctionPosition, ParameterString))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not extract parameter string for function '%s'"), *FunctionName);
        return false;
    }

    FString Declaration = FileContent.Mid(LineStart, DeclarationEndPos - LineStart + 1);
    
    // Populate the output structure
    OutDeclarationInfo.FunctionDeclaration = Declaration; // For full declaration we want indentations
    OutDeclarationInfo.Parameters = QCE_ParameterMatcher::ToParameterArray(ParameterString);
    OutDeclarationInfo.bIsConst = HasConstModifier(OutDeclarationInfo.FunctionDeclaration.TrimStartAndEnd());
    OutDeclarationInfo.FunctionName = FunctionName;
    OutDeclarationInfo.ClassName = ExtractClassNameFromDeclarationFile(FileContent);
    OutDeclarationInfo.ReturnType = ExtractReturnType(FileContent, FunctionPosition);
   
    
    // Store the declaration positions
    OutDeclarationInfo.DeclarationStartPosition = LineStart;
    OutDeclarationInfo.DeclarationEndPosition = DeclarationEndPos + 1; // Include the semicolon or closing brace
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Successfully parsed function declaration for '%s' from position %d to %d"), *FunctionName, LineStart, DeclarationEndPos);
    
    return true;
}

FString QCE_CommonIOHelpers::ExtractClassNameFromDeclarationFile(const FString& DeclarationFileContent)
{
	if (DeclarationFileContent.IsEmpty())
	{
		return FString();
	}

	// Look for class declaration patterns: "class APIEXPORT ClassName" or "class ClassName"
	FString ClassKeyword = TEXT("class");
	TArray<FString> Lines;
	DeclarationFileContent.ParseIntoArrayLines(Lines);
	
	for (const FString& Line : Lines)
	{
		FString TrimmedLine = Line.TrimStartAndEnd();
		
		// Skip comments and empty lines
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("//")))
		{
			continue;
		}
		
		// Look for lines that start with "class"
		if (TrimmedLine.StartsWith(ClassKeyword))
		{
			// Parse the class declaration
			TArray<FString> Tokens;
			TrimmedLine.ParseIntoArrayWS(Tokens);
			
			if (Tokens.Num() >= 2)
			{
				// Handle patterns like:
				// "class ClassName"
				// "class APIEXPORT ClassName" 
				// "class ClassName : public BaseClass"
				
				for (int32 i = 1; i < Tokens.Num(); i++)
				{
					const FString& Token = Tokens[i];
					
					// Skip API export macros (usually all uppercase)
					if (Token.IsEmpty() || Token.Equals(TEXT(":")) || Token.Equals(TEXT("public")) || 
						Token.Equals(TEXT("private")) || Token.Equals(TEXT("protected")))
					{
						continue;
					}
					
					// Skip common API export patterns (all uppercase tokens)
					bool bIsApiExport = true;
					for (TCHAR Char : Token)
					{
						if (!FChar::IsUpper(Char) && Char != '_')
						{
							bIsApiExport = false;
							break;
						}
					}
					
					if (!bIsApiExport && FChar::IsAlpha(Token[0]))
					{
						// This looks like a class name
						UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found class name '%s' in declaration file"), *Token);
						return Token;
					}
				}
			}
		}
	}
	
	UE_LOG(LogQuickCodeEditor, Warning, TEXT("Could not find class declaration in file content"));
	return FString();
}

bool QCE_CommonIOHelpers::HasConstModifier(const FString& FunctionDeclaration)
{
    if (FunctionDeclaration.IsEmpty())
        return false;
    
    // For const functions, the const keyword appears after the closing parenthesis
    // e.g., "void MyFunction() const;"
    int32 LastParenPos = FunctionDeclaration.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
    if (LastParenPos == INDEX_NONE)
        return false;
    
    // Extract the part after the last closing parenthesis
    FString PostParenPart = FunctionDeclaration.Mid(LastParenPos + 1);
    
    // Check if "const" appears in this part (before semicolon or opening brace)
    // Use context-aware searching to avoid semicolons/braces in strings
    int32 SemicolonPos = INDEX_NONE;
    int32 BracePos = INDEX_NONE;
    QCE_ParameterMatcher::FindCharacterRespectingContext(PostParenPart, TEXT(";"), 0, ESearchDir::FromStart, SemicolonPos);
    QCE_ParameterMatcher::FindCharacterRespectingContext(PostParenPart, TEXT("{"), 0, ESearchDir::FromStart, BracePos);
    
    int32 EndPos = INDEX_NONE;
    if (SemicolonPos != INDEX_NONE && BracePos != INDEX_NONE)
        EndPos = FMath::Min(SemicolonPos, BracePos);
    else if (SemicolonPos != INDEX_NONE)
        EndPos = SemicolonPos;
    else if (BracePos != INDEX_NONE)
        EndPos = BracePos;
    
    if (EndPos != INDEX_NONE)
        PostParenPart = PostParenPart.Left(EndPos);
    
    return PostParenPart.Contains(TEXT("const"));
}

bool QCE_CommonIOHelpers::DoesImplementationSignatureMatchDeclaration(const FString& FileContent, int32 FunctionNamePos, const FFunctionDeclarationInfo& DeclarationInfo)
{
    // Extract implementation parameters from the current position
    FString ImplementationParameterList;
    if (!QCE_ParameterMatcher::GetParameterStringAtPosition(FileContent, FunctionNamePos, ImplementationParameterList))
        return false;
    
    TArray<FString> ImplementationParams = QCE_ParameterMatcher::ToParameterArray(ImplementationParameterList);
        
    // Check if parameter count matches
    if (DeclarationInfo.Parameters.Num() != ImplementationParams.Num())
    {
        return false;
    }
    
    // Use QCE_ParameterMatcher to compare parameters
    // TODO do a for loop like FilterPositionsByMatchingNodeParams
    // Check if parameter types match using individual comparison
    for (int32 i = 0; i < DeclarationInfo.Parameters.Num(); ++i)
    {
        if (!QCE_ParameterMatcher::DoParameterTypesMatch(ImplementationParams[i], DeclarationInfo.Parameters[i]))
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Parameter type mismatch at param %d: expected '%s', found '%s' for function '%s'"), 
                   i, *DeclarationInfo.Parameters[i], *ImplementationParams[i], *DeclarationInfo.FunctionName);
            return false;
        }
    }
    
    // Find the line containing this function to check modifiers
    int32 LineStart = FunctionNamePos;
    while (LineStart > 0 && FileContent[LineStart - 1] != '\n')
    {
        LineStart--;
    }
    
    int32 LineEnd = FunctionNamePos;
    while (LineEnd < FileContent.Len() && FileContent[LineEnd] != '{' && FileContent[LineEnd] != ';')
    {
        LineEnd++;
    }
    
    FString ImplementationLine = FileContent.Mid(LineStart, LineEnd - LineStart);
    
    // Check const-ness match
    bool bImplementationIsConst = HasConstModifier(ImplementationLine);
    if (bImplementationIsConst != DeclarationInfo.bIsConst)
    {
        return false;
    }
    
    return true;
}

bool QCE_CommonIOHelpers::FilterPositionsBySignatureMatch(const FString& FileContent, const TArray<int32>& PossibleMatchPositions, const FFunctionDeclarationInfo& DeclarationInfo, TArray<int32>& OutSignatureMatches)
{
    const FString FunctionName = DeclarationInfo.FunctionName;
    
    for (int32 MatchPos : PossibleMatchPositions)
    {
        if (DoesImplementationSignatureMatchDeclaration(FileContent, MatchPos, DeclarationInfo))
        {
            OutSignatureMatches.Add(MatchPos);
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Signature match found at position %d for function '%s'"), MatchPos, *FunctionName);
        }
        else
        {
            UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Signature mismatch at position %d for function '%s'"), MatchPos, *FunctionName);
        }
    }
    
    if (OutSignatureMatches.Num() == 0)
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("No signature matches found for function '%s' among %d positions"), *FunctionName, PossibleMatchPositions.Num());
    }
    
    return OutSignatureMatches.Num() != 0;
}

bool QCE_CommonIOHelpers::FindImplementationPositionInContent(const FString& FileContent, const FFunctionDeclarationInfo& DeclarationInfo, const FString& ClassName, int32& OutFunctionPosition)
{
    const FString FunctionName = DeclarationInfo.FunctionName;
    
    TArray<int32> PossibleMatchPositions;
    if (!FilterPositionsByName(FileContent, FunctionName, PossibleMatchPositions))
    {
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("Found %d name matches for function '%s' in implementation"), PossibleMatchPositions.Num(), *FunctionName);

    TArray<int32> NonCommentMatches;
    if (!FilterCommentedPositions(FileContent, PossibleMatchPositions, NonCommentMatches))
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("All %d name matches for function '%s' were in comments in implementation"), PossibleMatchPositions.Num(), *FunctionName);
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After comment filtering: %d matches for function '%s' in implementation"), NonCommentMatches.Num(), *FunctionName);

    // Filter matches that have the class scope (ClassName::FunctionName)
    TArray<int32> ScopedMatches;
    if (!FilterScopedFunctionPositions(FileContent, NonCommentMatches, ClassName, ScopedMatches))
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("No scoped matches found for function '%s' in class '%s', using non-comment matches as fallback"), *FunctionName, *ClassName);
        // If no scoped matches found, try using non-comment matches as fallback
        ScopedMatches = NonCommentMatches;
    }
    else
    {
        UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After scope filtering: %d matches for function '%s' in class '%s'"), ScopedMatches.Num(), *FunctionName, *ClassName);
    }

    // Check parameter signature match
    TArray<int32> SignatureMatches;
    if (!FilterPositionsBySignatureMatch(FileContent, ScopedMatches, DeclarationInfo, SignatureMatches))
    {
        return false;
    }
    
    UE_LOG(LogQuickCodeEditor, Verbose, TEXT("After signature filtering: %d matches for function '%s'"), SignatureMatches.Num(), *FunctionName);

    // If we found less or more than 1 match, our return wasn't successful
    if (SignatureMatches.Num() != 1)
    {
        UE_LOG(LogQuickCodeEditor, Warning, TEXT("Expected exactly 1 signature match for function '%s', but found %d matches"), *FunctionName, SignatureMatches.Num());
        return false;
    }

     OutFunctionPosition = SignatureMatches[0];
    return true;
}

FString QCE_CommonIOHelpers::ExtractReturnType(const FString& HeaderContent, int32 DeclarationCursorLocation)
{
	if (HeaderContent.IsEmpty() || DeclarationCursorLocation < 0 || DeclarationCursorLocation >= HeaderContent.Len())
	{
		return FString();
	}

	// Get the function name at cursor position
	FString FunctionName = GetWordAtPosition(HeaderContent, DeclarationCursorLocation);
	if (FunctionName.IsEmpty())
	{
		return FString();
	}

	// Find the start of the function name
	int32 FunctionNameStart = DeclarationCursorLocation;
	while (FunctionNameStart > 0 && IsWordCharacter(HeaderContent[FunctionNameStart - 1]))
	{
		FunctionNameStart--;
	}

	// Skip backwards over whitespace to find the end of return type
	int32 ReturnTypeEnd = FunctionNameStart - 1;
	while (ReturnTypeEnd >= 0 && FChar::IsWhitespace(HeaderContent[ReturnTypeEnd]))
	{
		ReturnTypeEnd--;
	}

	if (ReturnTypeEnd < 0)
	{
		return FString(); // No return type found (possibly constructor/destructor)
	}

	// Find the start of the return type by going backwards
	int32 ReturnTypeStart = ReturnTypeEnd;
	int32 ParenDepth = 0;
	int32 AngleBracketDepth = 0;
	bool bFoundReturnTypeStart = false;

	// Go backwards character by character
	while (ReturnTypeStart >= 0 && !bFoundReturnTypeStart)
	{
		TCHAR CurrentChar = HeaderContent[ReturnTypeStart];

		// Handle template parameters and function pointers
		if (CurrentChar == '>')
		{
			AngleBracketDepth++;
		}
		else if (CurrentChar == '<')
		{
			AngleBracketDepth--;
		}
		else if (CurrentChar == ')')
		{
			ParenDepth++;
		}
		else if (CurrentChar == '(')
		{
			ParenDepth--;
		}
		else if (ParenDepth == 0 && AngleBracketDepth == 0)
		{
			// We're not inside template parameters or function pointers
			if (CurrentChar == '\n' || CurrentChar == ';' || CurrentChar == '{' || CurrentChar == '}')
			{
				// Found end of previous statement or line
				ReturnTypeStart++;
				bFoundReturnTypeStart = true;
				break;
			}
			else if (ReturnTypeStart > 0)
			{
				// Check for UFUNCTION macro or other macros
				if (HeaderContent.Mid(FMath::Max(0, ReturnTypeStart - 8), 9).Contains(TEXT("UFUNCTION")))
				{
					// Skip back to find the end of UFUNCTION macro
					int32 MacroStart = ReturnTypeStart;
					while (MacroStart > 0 && HeaderContent[MacroStart] != ')')
					{
						MacroStart--;
					}
					if (MacroStart > 0)
					{
						ReturnTypeStart = MacroStart + 1;
						bFoundReturnTypeStart = true;
						break;
					}
				}
			}
		}

		ReturnTypeStart--;
	}

	// If we didn't find a clear start, use beginning of line
	if (!bFoundReturnTypeStart || ReturnTypeStart < 0)
	{
		ReturnTypeStart = FunctionNameStart;
		while (ReturnTypeStart > 0 && HeaderContent[ReturnTypeStart - 1] != '\n')
		{
			ReturnTypeStart--;
		}
	}

	// Extract the return type substring
	if (ReturnTypeStart <= ReturnTypeEnd)
	{
		FString ReturnType = HeaderContent.Mid(ReturnTypeStart, ReturnTypeEnd - ReturnTypeStart + 1);
		ReturnType = ReturnType.TrimStartAndEnd();

		// Remove any UFUNCTION macros or other unwanted prefixes
		static const TArray<FString> MacrosToRemove = {
			TEXT("UFUNCTION"),
			TEXT("UPROPERTY"),
			TEXT("UCLASS"),
			TEXT("USTRUCT")
		};

		for (const FString& Macro : MacrosToRemove)
		{
			int32 MacroIndex = ReturnType.Find(Macro);
			if (MacroIndex != INDEX_NONE)
			{
				// Find the end of the macro (closing parenthesis)
				int32 MacroEnd = MacroIndex;
				int32 Depth = 0;
				bool bInMacro = false;
				
				for (int32 i = MacroIndex; i < ReturnType.Len(); i++)
				{
					if (ReturnType[i] == '(')
					{
						Depth++;
						bInMacro = true;
					}
					else if (ReturnType[i] == ')')
					{
						Depth--;
						if (Depth == 0 && bInMacro)
						{
							MacroEnd = i + 1;
							break;
						}
					}
				}
				
				// Remove the macro
				ReturnType = ReturnType.Mid(MacroEnd).TrimStartAndEnd();
			}
		}

		// Check if this might be a constructor or destructor (no return type)
		if (ReturnType.IsEmpty() || ReturnType.Equals(FunctionName) || ReturnType.Equals(TEXT("~") + FunctionName))
		{
			return FString(); // Constructor or destructor
		}

		return ReturnType;
	}

	return FString();
}

FString QCE_CommonIOHelpers::GetWordAtPosition(const FString& FileContent, int32 Position)
{
	if (Position < 0 || Position >= FileContent.Len())
	{
		return FString();
	}

	// Find word boundaries
	int32 WordStart = Position;
	int32 WordEnd = Position;

	// Move back to find start of word
	while (WordStart > 0 && IsWordCharacter(FileContent[WordStart - 1]))
	{
		WordStart--;
	}

	// Move forward to find end of word
	while (WordEnd < FileContent.Len() && IsWordCharacter(FileContent[WordEnd]))
	{
		WordEnd++;
	}

	// Extract the word
	if (WordEnd > WordStart)
	{
		return FileContent.Mid(WordStart, WordEnd - WordStart);
	}

	return FString();
}

bool QCE_CommonIOHelpers::IsWordCharacter(TCHAR Char)
{
	return FChar::IsAlnum(Char) || Char == '_';
}

int32 QCE_CommonIOHelpers::ConvertTextLocationToPosition(const FString& FileContent, const FTextLocation& TextLocation)
{
	if (FileContent.IsEmpty())
	{
		return INDEX_NONE;
	}

	int32 CurrentLine = 0;
	int32 Position = 0;
	int32 TargetLine = TextLocation.GetLineIndex();
	int32 TargetOffset = TextLocation.GetOffset();

	// Navigate to the target line
	while (CurrentLine < TargetLine && Position < FileContent.Len())
	{
		if (FileContent[Position] == '\n')
		{
			CurrentLine++;
		}
		Position++;
	}

	// If we didn't reach the target line, return invalid
	if (CurrentLine != TargetLine)
	{
		return INDEX_NONE;
	}

	// Add the offset within the line, making sure we don't exceed the line length
	int32 LineStart = Position;
	int32 LineEnd = Position;
	while (LineEnd < FileContent.Len() && FileContent[LineEnd] != '\n')
	{
		LineEnd++;
	}

	int32 LineLength = LineEnd - LineStart;
	if (TargetOffset > LineLength)
	{
		return INDEX_NONE;
	}

	return LineStart + TargetOffset;
}

FTextLocation QCE_CommonIOHelpers::ConvertPositionToTextLocation(const FString& FileContent, int32 Position)
{
	if (FileContent.IsEmpty() || Position < 0 || Position > FileContent.Len())
	{
		return FTextLocation(0, 0);
	}

	int32 LineIndex = 0;
	int32 CurrentPos = 0;
	int32 LineStart = 0;

	// Find the line containing the position
	while (CurrentPos < Position && CurrentPos < FileContent.Len())
	{
		if (FileContent[CurrentPos] == '\n')
		{
			LineIndex++;
			LineStart = CurrentPos + 1;
		}
		CurrentPos++;
	}

	int32 Offset = Position - LineStart;
	return FTextLocation(LineIndex, Offset);
}

void QCE_CommonIOHelpers::ExportPropertyCppDeclaration(const FProperty* Property, FOutputDevice& Out, 
	EQCEExportedDeclaration::Type DeclarationType, const TCHAR* ArrayDimOverride, 
	uint32 AdditionalExportCPPFlags, bool bSkipParameterName, 
	const FString* ActualCppType, const FString* ActualExtendedType, 
	const FString* ActualParameterName)
{
	if (!Property)
	{
		return;
	}

	const bool bIsParameter = (DeclarationType == EQCEExportedDeclaration::Parameter) || (DeclarationType == EQCEExportedDeclaration::MacroParameter);
	const bool bIsInterfaceProp = CastField<const FInterfaceProperty>(Property) != nullptr;

	// export the property type text (e.g. FString; int32; TArray, etc.)
	FString ExtendedTypeText;
	const uint32 ExportCPPFlags = AdditionalExportCPPFlags | (bIsParameter ? QCECPPF_ArgumentOrReturnValue : 0);
	FString TypeText;
	if (ActualCppType)
	{
		TypeText = *ActualCppType;
	}
	else
	{
		TypeText = Property->GetCPPType(&ExtendedTypeText, ExportCPPFlags);
	}

	if (ActualExtendedType)
	{
		ExtendedTypeText = *ActualExtendedType;
	}

	const bool bCanHaveRef = 0 == (AdditionalExportCPPFlags & QCECPPF_NoRef);
	const bool bCanHaveConst = 0 == (AdditionalExportCPPFlags & QCECPPF_NoConst);
	if (!CastField<const FBoolProperty>(Property) && bCanHaveConst) // can't have const bitfields because then we cannot determine their offset and mask from the compiler
	{
		const FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property);

		// export 'const' for parameters
		const bool bIsConstParam   = bIsParameter && (Property->HasAnyPropertyFlags(CPF_ConstParm) || (bIsInterfaceProp && !Property->HasAllPropertyFlags(CPF_OutParm)));
		const bool bIsOnConstClass = ObjectProp && ObjectProp->PropertyClass && ObjectProp->PropertyClass->HasAnyClassFlags(CLASS_Const);
		const bool bShouldHaveRef = bCanHaveRef && Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm);

		const bool bConstAtTheBeginning = bIsOnConstClass || (bIsConstParam && !bShouldHaveRef);
		if (bConstAtTheBeginning)
		{
			TypeText = FString::Printf(TEXT("const %s"), *TypeText);
		}

		const UClass* const MyPotentialConstClass = (DeclarationType == EQCEExportedDeclaration::Member) ? Property->GetOwner<UClass>() : nullptr;
		const bool bFromConstClass = MyPotentialConstClass && MyPotentialConstClass->HasAnyClassFlags(CLASS_Const);
		const bool bConstAtTheEnd = bFromConstClass || (bIsConstParam && bShouldHaveRef);
		if (bConstAtTheEnd)
		{
			ExtendedTypeText += TEXT(" const");
		}
	}

	FString NameCpp;
	if (!bSkipParameterName)
	{
		ensure((0 == (AdditionalExportCPPFlags & QCECPPF_BlueprintCppBackend)) || ActualParameterName);
		NameCpp = ActualParameterName ? *ActualParameterName : Property->GetNameCPP();
	}
	if (DeclarationType == EQCEExportedDeclaration::MacroParameter)
	{
		NameCpp = FString(TEXT(", ")) + NameCpp;
	}

	TCHAR ArrayStr[MAX_SPRINTF] = {};
	const bool bExportStaticArray = 0 == (QCECPPF_NoStaticArray & AdditionalExportCPPFlags);
	if ((Property->ArrayDim != 1) && bExportStaticArray)
	{
		if (ArrayDimOverride)
		{
			FCString::Sprintf( ArrayStr, TEXT("[%s]"), ArrayDimOverride );
		}
		else
		{
			FCString::Sprintf( ArrayStr, TEXT("[%i]"), Property->ArrayDim );
		}
	}

	if(auto BoolProperty = CastField<const FBoolProperty>(Property) )
	{
		// if this is a member variable, export it as a bitfield
		if( Property->ArrayDim==1 && DeclarationType == EQCEExportedDeclaration::Member )
		{
			bool bCanUseBitfield = !BoolProperty->IsNativeBool();
			// export as a uint32 member....bad to hardcode, but this is a special case that won't be used anywhere else
			Out.Logf(TEXT("%s%s %s%s%s"), *TypeText, *ExtendedTypeText, *NameCpp, ArrayStr, bCanUseBitfield ? TEXT(":1") : TEXT(""));
		}

		//@todo we currently can't have out bools.. so this isn't really necessary, but eventually out bools may be supported, so leave here for now
		else if( bIsParameter && Property->HasAnyPropertyFlags(CPF_OutParm) )
		{
			// export as a reference
			Out.Logf(TEXT("%s%s%s %s%s"), *TypeText, *ExtendedTypeText
				, bCanHaveRef ? TEXT("&") : TEXT("")
				, *NameCpp, ArrayStr);
		}

		else
		{
			Out.Logf(TEXT("%s%s %s%s"), *TypeText, *ExtendedTypeText, *NameCpp, ArrayStr);
		}
	}
	else 
	{
		if ( bIsParameter )
		{
			if ( Property->ArrayDim > 1 )
			{
				// export as a pointer
				//Out.Logf( TEXT("%s%s* %s"), *TypeText, *ExtendedTypeText, *GetNameCPP() );
				// don't export as a pointer
				Out.Logf(TEXT("%s%s %s%s"), *TypeText, *ExtendedTypeText, *NameCpp, ArrayStr);
			}
			else
			{
				if ( GetPropertyPassCPPArgsByRef(Property) )
				{
					// export as a reference (const ref if it isn't an out parameter)
					Out.Logf(TEXT("%s%s%s%s %s"),
						(bCanHaveConst && !Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ConstParm)) ? TEXT("const ") : TEXT(""),
						*TypeText, *ExtendedTypeText,
						bCanHaveRef ? TEXT("&") : TEXT(""),
						*NameCpp);
				}
				else
				{
					// export as a pointer if this is an optional out parm, reference if it's just an out parm, standard otherwise...
					TCHAR ModifierString[2] = { TCHAR('\0'), TCHAR('\0') };
					if (bCanHaveRef && (Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm) || bIsInterfaceProp))
					{
						ModifierString[0] = TEXT('&');
					}
					Out.Logf(TEXT("%s%s%s %s%s"), *TypeText, *ExtendedTypeText, ModifierString, *NameCpp, ArrayStr);
				}
			}
		}
		else
		{
			Out.Logf(TEXT("%s%s %s%s"), *TypeText, *ExtendedTypeText, *NameCpp, ArrayStr);
		}
	}
}

bool QCE_CommonIOHelpers::GetPropertyPassCPPArgsByRef(const FProperty* Property)
{
	if (!Property)
	{
		return false;
	}

	// Container properties always pass by reference
	if (CastField<const FArrayProperty>(Property) ||
		CastField<const FMapProperty>(Property) ||
		CastField<const FSetProperty>(Property))
	{
		return true;
	}

	// String and text properties pass by reference (non-POD)
	if (CastField<const FStrProperty>(Property) ||
		CastField<const FTextProperty>(Property))
	{
		return true;
	}

	// Boolean properties pass by value (POD)
	if (CastField<const FBoolProperty>(Property))
	{
		return false;
	}

	// Numeric properties pass by value (POD)
	if (CastField<const FNumericProperty>(Property))
	{
		return false;
	}

	// Name properties pass by value (POD-like behavior in UE)
	if (CastField<const FNameProperty>(Property))
	{
		return false;
	}

	// Object and class properties pass by value (pointers)
	if (CastField<const FObjectProperty>(Property) ||
		CastField<const FClassProperty>(Property) ||
		CastField<const FSoftObjectProperty>(Property) ||
		CastField<const FSoftClassProperty>(Property))
	{
		return false;
	}

	// Struct properties - check if it's a known POD-like struct
	if (const FStructProperty* StructProp = CastField<const FStructProperty>(Property))
	{
		if (StructProp->Struct)
		{
			FName StructName = StructProp->Struct->GetFName();
			
			// Common UE POD-like structs that pass by value
			if (StructName == NAME_Vector ||
				StructName == NAME_Rotator ||
				StructName == NAME_Transform ||
				StructName == NAME_Color ||
				StructName == NAME_LinearColor ||
				StructName == TEXT("Vector2D") ||
				StructName == TEXT("IntPoint") ||
				StructName == TEXT("IntVector") ||
				StructName == TEXT("Quat"))
			{
				return false; // Pass by value
			}
		}
		
		// Other structs typically pass by reference
		return true;
	}

	// Enum properties pass by value
	if (CastField<const FEnumProperty>(Property))
	{
		return false;
	}

	// Delegate properties pass by reference
	if (CastField<const FDelegateProperty>(Property) ||
		CastField<const FMulticastDelegateProperty>(Property))
	{
		return true;
	}

	// Interface properties pass by reference
	if (CastField<const FInterfaceProperty>(Property))
	{
		return true;
	}

	// Default case: assume pass by value for unknown property types (conservative)
	return false;
}
